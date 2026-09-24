// No game/CIA payloads, keys or emulator execution. Shared production validation.
#include "cia_update_policy.h"
#include <iostream>
#include <functional>
#include <cstddef>
using namespace ManicCIA;
static int checks;
static void Reject(std::function<void()> f,int code) {
    try {f(); throw std::runtime_error("accepted invalid input");}
    catch(const Failure& e) {if(e.status!=code) throw std::runtime_error("wrong status"); ++checks;}
}
int main() {
    static_assert(sizeof(manic_cia_result_v1)==32);
    static_assert(offsetof(manic_cia_result_v1,title_id)==8);
    static_assert(offsetof(manic_cia_result_v1,content_bytes)==24);
    auto sandbox=fs::temp_directory_path()/"manic-cia-policy-test";
    if(fs::exists(sandbox)) throw std::runtime_error("fresh test directory required");
    fs::create_directories(sandbox/"Documents");
    try {
        auto docs=sandbox/"Documents";
        auto expected=docs/"RoomExperiment-v1/3DS";
        if(Root(expected,docs)!=expected) throw std::runtime_error("valid isolated root rejected"); ++checks;
        Reject([&]{Root(docs/"3DS",docs);},MCIA_WRONG_ENVIRONMENT);
        Reject([&]{Root(sandbox/"Other/RoomExperiment-v1/3DS",docs);},MCIA_WRONG_ENVIRONMENT);
        Reject([&]{NoLinks(docs,"../outside");},MCIA_WRONG_ENVIRONMENT);
        {std::ifstream f(sandbox/"missing",std::ios::binary); Reject([&]{Read(f,0,0x2020);},MCIA_INVALID_FILE);}
        std::ofstream(sandbox/"empty").close();
        {std::ifstream f(sandbox/"empty",std::ios::binary); Reject([&]{Read(f,0,0x2020);},MCIA_INVALID_FILE);}
        Reject([]{Header({},0);},MCIA_INVALID_FILE);
        std::vector<uint8_t> h(0x2020);
        Reject([&]{Header(h,h.size());},MCIA_UNSUPPORTED);
        h[0]=0x20; h[1]=0x20;
        Reject([&]{Header(h,h.size());},MCIA_INVALID_FILE);
        h[12]=0x64;h[13]=1;h[16]=0xc4;h[17]=9;
        Reject([&]{Header(h,h.size());},MCIA_INVALID_FILE); // truncated sections
        Reject([&]{Header(h,MaxInput+1);},MCIA_INVALID_FILE);
        h[15]=0xff;
        Reject([&]{Header(h,0x10000000);},MCIA_INVALID_FILE); // bounded allocation
        Reject([]{SignedBody({0,0,0,0});},MCIA_INVALID_FILE);
#ifndef _WIN32
        fs::create_directory_symlink(sandbox,docs/"RoomExperiment-v1");
        Reject([&]{Root(expected,docs);},MCIA_WRONG_ENVIRONMENT);
#endif
        fs::remove_all(sandbox);
    } catch(...) {fs::remove_all(sandbox);throw;}
    std::cout << "PASS " << checks << " shared policy/ABI checks; no real CIA or emulator executed\n";
}
