// Pure validation shared by the importer and host-side negative tests.
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include "cia_update_api.h"
namespace ManicCIA {
namespace fs = std::filesystem;
struct Failure { int32_t status; };
inline void Require(bool ok, int32_t status = MCIA_INVALID_FILE) {
    if (!ok) throw Failure{status};
}
inline uint64_t Integer(const uint8_t* p, size_t n, bool big = false) {
    uint64_t v = 0;
    for (size_t i = 0; i < n; ++i) v |= uint64_t(p[big ? n-1-i : i]) << (i*8);
    return v;
}
inline uint64_t Align(uint64_t n) { return (n + 63) & ~uint64_t(63); }
constexpr uint64_t MaxInput = uint64_t(8) << 30;
// AM directory APIs return trailing '/'. Strip it before taking parent_path;
// otherwise create_directories(parent_path) would create the commit destination.
inline fs::path Directory(const fs::path& path) {
    auto p=path.lexically_normal();
    while(p.has_relative_path() && p.filename().empty()) p=p.parent_path();
    return p;
}
inline void NoLinks(const fs::path& base, const fs::path& relative) {
    fs::path p = base;
    for (const auto& part : relative) {
        Require(part != ".." && !part.is_absolute(), MCIA_WRONG_ENVIRONMENT);
        p /= part;
        Require(!fs::is_symlink(fs::symlink_status(p)), MCIA_WRONG_ENVIRONMENT);
    }
}
inline fs::path Root(const fs::path& requested, const fs::path& documents) {
    auto expected = fs::weakly_canonical(documents) / "RoomExperiment-v1" / "3DS";
    NoLinks(fs::weakly_canonical(documents), "RoomExperiment-v1/3DS");
    Require(requested.is_absolute() && fs::weakly_canonical(requested) == expected,
            MCIA_WRONG_ENVIRONMENT);
    return expected;
}
inline std::vector<uint8_t> Read(std::ifstream& in, uint64_t offset, size_t size) {
    std::vector<uint8_t> b(size);
    in.clear(); in.seekg(static_cast<std::streamoff>(offset));
    in.read(reinterpret_cast<char*>(b.data()), static_cast<std::streamsize>(size));
    Require(in.good() || (in.eof() && in.gcount() == static_cast<std::streamsize>(size)));
    Require(in.gcount() == static_cast<std::streamsize>(size));
    return b;
}
struct Sections { uint64_t ticket, tmd, content, end; uint32_t ticket_size, tmd_size; };
inline Sections Header(const std::vector<uint8_t>& h, uint64_t file_size) {
    Require(h.size() == 0x2020 && file_size >= h.size() && file_size <= MaxInput);
    Require(Integer(h.data(),4) == 0x2020 && Integer(h.data()+4,2) == 0 &&
            Integer(h.data()+6,2) == 0, MCIA_UNSUPPORTED);
    uint64_t cert= Integer(h.data()+8,4), tik=Integer(h.data()+12,4),
             tmd=Integer(h.data()+16,4), meta=Integer(h.data()+20,4),
             content=Integer(h.data()+24,8);
    Require(cert <= 0x100000 && tik >= 0x164 && tik <= 0x100000 &&
            tmd >= 0x9c4 && tmd <= 0x100000 && meta <= 0x100000 && content <= MaxInput);
    Sections s{Align(0x2020+cert),0,0,0,static_cast<uint32_t>(tik),static_cast<uint32_t>(tmd)};
    s.tmd=Align(s.ticket+tik); s.content=Align(s.tmd+tmd); s.end=s.content+content;
    Require(s.end <= file_size && (meta == 0 || Align(s.end)+meta <= file_size));
    return s;
}
inline size_t SignedBody(const std::vector<uint8_t>& data) {
    Require(data.size() >= 4);
    switch(Integer(data.data(),4,true)) {
    case 0x10000: case 0x10003: return 0x240;
    case 0x10001: case 0x10004: return 0x140;
    case 0x10002: case 0x10005: return 0x40;
    default: throw Failure{MCIA_INVALID_FILE};
    }
}
} // namespace ManicCIA
