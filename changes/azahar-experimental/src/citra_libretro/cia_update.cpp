// Experimental offline update import. No Room or legacy extension-version contract.
#include "cia_update_policy.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <set>
#include <type_traits>
#include <cryptopp/sha.h>
#include <libretro.h>
#include "common/file_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/file_sys/cia_container.h"
#include "core/hle/service/am/am.h"
#include "core/loader/loader.h"
#include <unistd.h>
#include <stdio.h>

bool ManicCIAHasFrontend();
namespace ManicCIA {
static std::mutex mutex;
struct Cancel {
    manic_cia_cancel_v1 callback; void* context;
    void Check() const { if (callback && callback(context)) throw Failure{MCIA_CANCELLED}; }
};
struct Temporary {
    fs::path path;
    ~Temporary() { std::error_code ec; fs::remove_all(path,ec); }
};
struct Paths {
    std::string root;
    bool compression;
    explicit Paths(const fs::path& p) : root(p.generic_string()+"/"),
        compression(Settings::values.compress_cia_installs.GetValue()) {}
    ~Paths() {
        try { FileUtil::SetUserPath(root); Settings::values.compress_cia_installs = compression; }
        catch (...) {} // No exception crosses the C ABI, including during unwinding.
    }
};
static std::array<uint8_t,32> Hash(std::ifstream& in, uint64_t offset, uint64_t size, Cancel cancel) {
    CryptoPP::SHA256 hash;
    while(size) {
        cancel.Check();
        auto n=static_cast<size_t>(std::min<uint64_t>(size,65536));
        auto b=Read(in,offset,n); hash.Update(b.data(),b.size()); offset+=n; size-=n;
    }
    std::array<uint8_t,32> out{}; hash.Final(out.data()); return out;
}
static int32_t Install(const char* root_arg, const char* path_arg, manic_cia_result_v1& out, Cancel cancel) {
    std::unique_lock lock(mutex,std::try_to_lock);
    Require(lock.owns_lock() && !ManicCIAHasFrontend() && !Core::System::GetInstance().IsPoweredOn(), MCIA_BUSY);
    Require(root_arg && path_arg, MCIA_INVALID_FILE);
    const char* home=std::getenv("HOME");
    Require(home && *home, MCIA_WRONG_ENVIRONMENT);
    const auto root=Root(fs::path(root_arg),fs::path(home)/"Documents");
    auto input=fs::path(path_arg);
    auto extension=input.extension().string();
    std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return std::tolower(c);});
    Require(extension==".cia" && fs::is_regular_file(input) && !fs::is_symlink(input));
    auto size=fs::file_size(input);
    Require(size>=0x2020 && size<=MaxInput);
    // Bound untrusted allocations before calling the upstream container parser.
    std::ifstream original(input,std::ios::binary);
    Header(Read(original,0,0x2020),size);
    cancel.Check();
    fs::create_directories(root);
    Require(fs::space(root).available > size*2 + (uint64_t(64)<<20),MCIA_NO_SPACE);
    std::string pattern=(root/".cia-import-XXXXXX").string();
    std::vector<char> tmp(pattern.begin(),pattern.end()); tmp.push_back(0);
    Require(mkdtemp(tmp.data()) != nullptr, MCIA_INSTALL_FAILED);
    Temporary staging{fs::path(tmp.data())};
    const auto source=staging.path/"input.cia"; // No personal filename in core logs.
    {
        std::ofstream copy(source,std::ios::binary|std::ios::trunc);
        for(uint64_t off=0;off<size;) {
            cancel.Check(); auto n=static_cast<size_t>(std::min<uint64_t>(65536,size-off));
            auto b=Read(original,off,n); copy.write(reinterpret_cast<char*>(b.data()),n);
            Require(copy.good(),MCIA_INSTALL_FAILED); off+=n;
        }
        copy.close(); Require(copy.good(),MCIA_INSTALL_FAILED);
    }
    std::ifstream in(source,std::ios::binary);
    auto header=Read(in,0,0x2020); auto sections=Header(header,size);
    auto tmd_bytes=Read(in,sections.tmd,sections.tmd_size);
    auto body=SignedBody(tmd_bytes);
    Require(tmd_bytes.size()>=body+0x9c4);
    auto count=Integer(tmd_bytes.data()+body+0x9e,2,true);
    Require(count>0 && count<=256 && tmd_bytes.size()>=body+0x9c4+count*0x30,MCIA_UNSUPPORTED);
    auto tid=Integer(tmd_bytes.data()+body+0x4c,8,true);
    Require((tid>>32)==0x0004000e,MCIA_UNSUPPORTED); // Generic CTR update category.
    auto ticket=Read(in,sections.ticket,sections.ticket_size);
    auto ticket_body=SignedBody(ticket);
    Require(ticket.size()>=ticket_body+0x164+8);
    Require(Integer(ticket.data()+ticket_body+0x9c,8,true)==tid);
    // Ticket parser's content-index reader assumes a full main header.
    auto index_size=Integer(ticket.data()+ticket_body+0x164+4,4,true);
    Require(index_size>=0x14 && ticket_body+0x164+index_size<=ticket.size());
    std::set<uint32_t> ids;
    std::vector<std::array<uint8_t,32>> hashes;
    uint64_t offset=sections.content;
    for(size_t i=0;i<count;++i) {
        const auto* c=tmd_bytes.data()+body+0x9c4+i*0x30;
        auto bytes=Integer(c+8,8,true);
        Require(Integer(c+4,2,true)==i && (header[0x20+i/8]&(0x80>>(i%8))),MCIA_UNSUPPORTED);
        Require((Integer(c+6,2,true)&(1|0x4000|0x8000))==0,MCIA_UNSUPPORTED);
        Require(ids.insert(static_cast<uint32_t>(Integer(c,4,true))).second);
        Require(bytes>=0x200 && bytes<=sections.end-offset);
        auto hash=Hash(in,offset,bytes,cancel);
        Require(std::equal(hash.begin(),hash.end(),c+16)); hashes.push_back(hash); offset+=bytes;
    }
    Require(offset==sections.end);
    // Upstream validation checks NCCH headers and refuses encrypted contents.
    bool compressed=false;
    Require(Service::AM::CheckCIAToInstall(source.string(),compressed,true)==Service::AM::InstallStatus::Success && !compressed,MCIA_UNSUPPORTED);
    FileUtil::IOFile file(source.string(),"rb"); FileSys::CIAContainer container;
    Require(container.Load(&file)==Loader::ResultStatus::Success);
    const auto& tmd=container.GetTitleMetadata();
    Require(tmd.GetTitleID()==tid && container.GetTicket().GetTitleID()==tid);
    out.title_id=tid; out.title_version=tmd.GetTitleVersion();
    out.content_count=static_cast<uint32_t>(count); out.content_bytes=sections.end-sections.content;
    Paths restore(root);
    FileUtil::SetUserPath(root.generic_string()+"/");
    Require(FileUtil::GetUserPath(FileUtil::UserPath::UserDir)==root.generic_string()+"/",MCIA_WRONG_ENVIRONMENT);
    auto media=Service::AM::GetTitleMediaType(tid);
    Require(media==Service::FS::MediaType::SDMC,MCIA_UNSUPPORTED);
    fs::path destination=Service::AM::GetTitlePath(media,tid);
    auto relative=destination.lexically_relative(root);
    NoLinks(root,relative);
    Require(!fs::exists(destination),MCIA_ALREADY_INSTALLED); // Never replace a prior install/save.
    const auto staged_root=staging.path/"user";
    fs::create_directory(staged_root);
    FileUtil::SetUserPath(staged_root.generic_string()+"/");
    Require(FileUtil::GetUserPath(FileUtil::UserPath::UserDir)==staged_root.generic_string()+"/",MCIA_WRONG_ENVIRONMENT);
    Settings::values.compress_cia_installs=false;
    cancel.Check();
    {
        // Same real AM installer used by InstallCIA, but bounded reads avoid that
        // wrapper's zero-byte-read loop and we inspect every write result.
        Service::AM::CIAFile installer(Core::System::GetInstance(),media);
        for(uint64_t off=0;off<size;) {
            cancel.Check(); auto n=static_cast<size_t>(std::min<uint64_t>(65536,size-off));
            auto b=Read(in,off,n);
            auto written=installer.Write(off,n,true,false,b.data());
            Require(written.Succeeded() && written.Unwrap()==n,MCIA_INSTALL_FAILED);
            off+=n;
        }
        installer.Close();
        for(const auto& entry:installer.GetInstallResults())
            Require(entry.result.IsSuccess(),MCIA_INSTALL_FAILED);
    }
    // The upstream helper does not propagate every short write/Close failure.
    // Re-read each installed content and TMD before exposing any result to the game.
    for(size_t i=0;i<count;++i) {
        fs::path p=Service::AM::GetTitleContentPath(media,tid,i);
        Require(fs::is_regular_file(p) && fs::file_size(p)==tmd.GetContentSizeByIndex(i),MCIA_INSTALL_FAILED);
        std::ifstream installed(p,std::ios::binary);
        Require(Hash(installed,0,fs::file_size(p),cancel)==hashes[i],MCIA_INSTALL_FAILED);
    }
    FileSys::TitleMetadata installed_tmd;
    Require(installed_tmd.Load(Service::AM::GetTitleMetadataPath(media,tid))==Loader::ResultStatus::Success &&
        installed_tmd.GetTitleID()==tid && installed_tmd.GetTitleVersion()==tmd.GetTitleVersion() &&
        installed_tmd.GetContentCount()==count,MCIA_INSTALL_FAILED);
    fs::path staged_title=Service::AM::GetTitlePath(media,tid);
    // Decrypted update loading uses TMD + NCCH, not the ticket. Keep the upstream
    // ticket confined to staging and discard it; no NAND/ticket database mutation.
    cancel.Check();
    NoLinks(root,relative);
    Require(!fs::exists(destination),MCIA_ALREADY_INSTALLED);
    fs::create_directories(destination.parent_path());
    FileUtil::SetUserPath(root.generic_string()+"/");
    Require(FileUtil::GetUserPath(FileUtil::UserPath::UserDir)==root.generic_string()+"/",MCIA_WRONG_ENVIRONMENT);
    // iOS exclusive same-volume rename cannot replace even a concurrently created directory.
    Require(renamex_np(staged_title.c_str(),destination.c_str(),RENAME_EXCL)==0,MCIA_INSTALL_FAILED);
    return MCIA_SUCCESS;
}
} // namespace ManicCIA
extern "C" RETRO_API int32_t retro_manic_install_update_cia_v1(const char* root,const char* path,
    manic_cia_result_v1* result,uint32_t result_size,manic_cia_cancel_v1 cancel,void* context) {
    if (!result || result_size!=sizeof(*result)) return MCIA_INVALID_FILE;
    *result={sizeof(*result),MCIA_INSTALL_FAILED,0,0,0,0};
    try { result->status=ManicCIA::Install(root,path,*result,{cancel,context}); }
    catch(const ManicCIA::Failure& e) { result->status=e.status; }
    catch(...) { result->status=MCIA_INSTALL_FAILED; }
    return result->status;
}
static_assert(sizeof(manic_cia_result_v1)==32);
static_assert(offsetof(manic_cia_result_v1,title_id)==8);
static_assert(std::is_same_v<decltype(&retro_manic_install_update_cia_v1),manic_install_update_cia_v1>);
