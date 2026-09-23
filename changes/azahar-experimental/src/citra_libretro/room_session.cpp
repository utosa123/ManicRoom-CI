// SPDX-License-Identifier: GPL-2.0-or-later
#include "citra_libretro/room_api.h"
#include "citra_libretro/room_session.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include "common/logging/log.h"
#include "network/network.h"

namespace {
using Member = Network::RoomMember;
using namespace std::chrono_literals;
// API/lifecycle operations are serialized separately from network callbacks.
std::mutex api_mutex, snapshot_mutex;
az_room_snapshot snapshot{};
std::shared_ptr<Member> member;
Member::CallbackHandle<Member::State> state_callback;
Member::CallbackHandle<Member::Error> error_callback;
Member::CallbackHandle<Network::RoomInformation> room_callback;
std::thread worker;
std::atomic<bool> cancel{false}, finished{true};
bool owns_network = false, ready = false;
std::string console_hash;
Network::MacAddress console_mac = Network::NoPreferredMac;

bool Bounded(const char* value, size_t limit, std::string& copy) {
    if (!value) return false;
    size_t length = 0;
    while (length <= limit && value[length]) ++length;
    if (length > limit) return false;
    copy.assign(value, length);
    return true;
}
bool IPv4(const std::string& ip) {
    size_t pos = 0;
    for (int part = 0; part < 4; ++part) {
        size_t begin = pos;
        unsigned value = 0;
        while (pos < ip.size() && ip[pos] >= '0' && ip[pos] <= '9') {
            value = value * 10 + (ip[pos++] - '0');
            if (pos - begin > 3 || value > 255) return false;
        }
        if (pos == begin || (pos - begin > 1 && ip[begin] == '0')) return false;
        if (part != 3 && (pos >= ip.size() || ip[pos++] != '.')) return false;
    }
    return pos == ip.size() && ip != "0.0.0.0" && ip != "255.255.255.255";
}
void ClearMembers() {
    snapshot.member_count = 0;
    std::memset(snapshot.members, 0, sizeof(snapshot.members));
}
void StopWorker() {
    cancel = true;
    if (worker.joinable()) worker.join();
}
void Run(std::string ip, uint16_t port, std::string nick, std::string password,
         std::string hash, Network::MacAddress mac) noexcept {
    try {
        member->Join(nick, hash, ip.c_str(), port, 0, mac,
                     password, "", [] { return cancel.load(); });
        // Covers a transport connection that never receives a room JoinSuccess.
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!cancel && member->IsConnected()) {
            if (member->GetState() == Member::State::Joining &&
                std::chrono::steady_clock::now() >= deadline) {
                std::lock_guard lock(snapshot_mutex);
                snapshot.error = 101;
                break;
            }
            std::this_thread::sleep_for(10ms);
        }
    } catch (...) {
        std::lock_guard lock(snapshot_mutex);
        snapshot.error = 100;
    }
    // A failed Join may have a client but no loop thread. Leave is idempotent.
    member->Leave(true);
    {
        std::lock_guard lock(snapshot_mutex);
        snapshot.state = AZ_ROOM_IDLE;
        ClearMembers();
    }
    finished = true;
}
}

namespace LibRetro::RoomSession {
void Initialize() noexcept {
    try {
        std::lock_guard api_lock(api_mutex);
        if (member) return;
        if (Network::GetRoomMember().expired()) {
            if (!Network::Init()) {
                std::lock_guard lock(snapshot_mutex);
                snapshot.error = 100;
                return;
            }
            owns_network = true;
        }
        member = Network::GetRoomMember().lock();
        state_callback = member->BindOnStateChanged([](const Member::State& state) {
            LOG_INFO(Network, "Room bridge member state={}", static_cast<unsigned>(state));
            std::lock_guard lock(snapshot_mutex);
            if (cancel) return;
            if (state == Member::State::Joined || state == Member::State::Moderator)
                snapshot.state = AZ_ROOM_JOINED;
            // Idle is exposed only after Run has cleaned up the transport.
        });
        error_callback = member->BindOnError([](const Member::Error& error) {
            LOG_INFO(Network, "Room bridge error={}", static_cast<unsigned>(error) + 1);
            std::lock_guard lock(snapshot_mutex);
            if (!cancel) snapshot.error = static_cast<uint32_t>(error) + 1;
        });
        room_callback = member->BindOnRoomInformationChanged([](const auto&) {
            // Invoked on MemberLoop after member_information is updated. Do not
            // call the unsynchronized getter from the UI/polling thread.
            std::lock_guard lock(snapshot_mutex);
            if (cancel) return;
            ClearMembers();
            for (const auto& peer : member->GetMemberInformation()) {
                if (snapshot.member_count == 254) break;
                auto& dest = snapshot.members[snapshot.member_count++];
                std::memcpy(dest, peer.nickname.data(), std::min(peer.nickname.size(), size_t{20}));
            }
        });
    } catch (...) {
        std::lock_guard lock(snapshot_mutex);
        snapshot.error = 100;
    }
}
void GameReady(std::string hash, std::array<uint8_t, 6> mac) noexcept {
    try {
        std::lock_guard api_lock(api_mutex);
        if (!member || !state_callback || !error_callback || !room_callback) return;
        StopWorker(); // A repeated GameReady cannot orphan an existing session.
        console_hash = std::move(hash);
        console_mac = mac;
        ready = true;
        std::lock_guard lock(snapshot_mutex);
        snapshot.state = AZ_ROOM_IDLE;
        snapshot.error = 0;
    } catch (...) {}
}
void GameStopped() noexcept {
    std::lock_guard api_lock(api_mutex);
    ready = false;
    StopWorker();
    console_hash.clear();
    std::lock_guard lock(snapshot_mutex);
    snapshot.state = AZ_ROOM_OFFLINE;
    ClearMembers();
}
void Shutdown() noexcept {
    std::lock_guard api_lock(api_mutex);
    ready = false;
    StopWorker();
    console_hash.clear();
    {
        std::lock_guard lock(snapshot_mutex);
        snapshot.state = AZ_ROOM_OFFLINE;
        ClearMembers();
    }
    if (member) {
        member->Unbind(state_callback);
        member->Unbind(error_callback);
        member->Unbind(room_callback);
        state_callback.reset(); error_callback.reset(); room_callback.reset();
        member.reset();
    }
    if (owns_network) Network::Shutdown();
    owns_network = false;
}
bool Active() noexcept { return !finished.load(); }
}

extern "C" uint32_t retro_azahar_room_api_version() { return 1; }
extern "C" int32_t retro_azahar_room_snapshot(az_room_snapshot* out, uint32_t size) {
    if (!out || size != sizeof(*out)) return 1;
    try {
        std::lock_guard lock(snapshot_mutex);
        *out = snapshot;
        out->protocol = Network::network_version;
        out->default_port = Network::DefaultRoomPort;
        return 0;
    } catch (...) { return -1; }
}
extern "C" int32_t retro_azahar_room_join(const char* address, uint32_t port,
                                        const char* nickname, const char* secret) {
    try {
        std::string ip, nick, password;
        if (!Bounded(address, 15, ip) || !IPv4(ip) || port < 1 || port > 65535 ||
            !Bounded(nickname, 20, nick) || nick.size() < 4 ||
            nick.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._ -")
                != std::string::npos || !Bounded(secret, 128, password)) return 1;
        std::lock_guard api_lock(api_mutex);
        if (!ready) return 2;
        if (!finished) return 3;
        if (worker.joinable()) worker.join();
        cancel = false;
        {
            std::lock_guard lock(snapshot_mutex);
            snapshot.state = AZ_ROOM_JOINING;
            snapshot.error = 0;
            ClearMembers();
        }
        finished = false;
        try { worker = std::thread(Run, ip, static_cast<uint16_t>(port), nick, password, console_hash, console_mac); }
        catch (...) {
            finished = true;
            std::lock_guard lock(snapshot_mutex);
            snapshot.state = AZ_ROOM_IDLE;
            snapshot.error = 100;
            return -1;
        }
        return 0;
    } catch (...) { return -1; }
}
extern "C" int32_t retro_azahar_room_leave() {
    try {
        std::lock_guard api_lock(api_mutex);
        cancel = true;
        std::lock_guard lock(snapshot_mutex);
        if (!finished) snapshot.state = AZ_ROOM_LEAVING;
        return 0;
    } catch (...) { return -1; }
}
