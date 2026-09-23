// SPDX-License-Identifier: GPL-2.0-or-later
#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include "citra_libretro/room_api.h"
#include "citra_libretro/room_session.h"
#include "network/network.h"
#include "network/verify_user.h"
#include "enet/enet.h"
using namespace std::chrono_literals;
namespace Session = LibRetro::RoomSession;
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
az_room_snapshot Read() {
    az_room_snapshot value{};
    Check(retro_azahar_room_snapshot(&value, sizeof(value)) == 0, "snapshot");
    return value;
}
template<class F> void Wait(F predicate, int ms = 3000) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (!predicate()) {
        Check(std::chrono::steady_clock::now() < end, "wait timeout");
        std::this_thread::sleep_for(10ms);
    }
}
void Idle() { Wait([] { return !Session::Active(); }); }
// Controlled ENet endpoint, not a PC emulator. Exercises rejection and a peer
// that accepts transport but never sends an application-level join response.
class ProtocolFixture {
public:
    ENetHost* host = nullptr;
    unsigned port = 35100;
    std::atomic<bool> stop{false};
    std::thread loop;
    explicit ProtocolFixture(bool reject) {
        ENetAddress address{};
        enet_address_set_host_ip(&address, "127.0.0.1");
        for (; port < 35200; ++port) {
            address.port = static_cast<enet_uint16>(port);
            host = enet_host_create(&address, 1, 1, 0, 0);
            if (host) break;
        }
        Check(host != nullptr, "fixture bind");
        loop = std::thread([this, reject] {
            while (!stop) {
                ENetEvent event{};
                if (enet_host_service(host, &event, 10) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
                    if (reject) {
                        const unsigned char wrong_version[] = {Network::IdVersionMismatch, 0, 0, 0, 99};
                        enet_peer_send(event.peer, 0, enet_packet_create(wrong_version, sizeof(wrong_version), ENET_PACKET_FLAG_RELIABLE));
                    }
                    enet_packet_destroy(event.packet);
                }
            }
        });
    }
    ~ProtocolFixture() { stop = true; loop.join(); enet_host_destroy(host); }
};
int main() {
    try {
        Check(retro_azahar_room_api_version() == 1, "ABI");
        Check(Read().state == AZ_ROOM_OFFLINE, "offline before init");
        Check(retro_azahar_room_snapshot(nullptr, 0) == 1, "null buffer");
        Check(retro_azahar_room_join("127.0.0.1", 24872, "test", "") == 2, "not ready");
        Session::Initialize();
        auto same = Network::GetRoomMember().lock();
        Session::Initialize();
        Check(same == Network::GetRoomMember().lock(), "must not initialize twice");
        const Network::MacAddress console_mac{0, 31, 50, 1, 2, 3};
        Session::GameReady("test-only-console-identity", console_mac);
        for (const auto* ip : {"", "1.2.3", "256.1.2.3", "01.2.3.4", "0.0.0.0", "::1"})
            Check(retro_azahar_room_join(ip, 24872, "test", "") == 1, "bad IP");
        Check(retro_azahar_room_join("127.0.0.1", 0, "test", "") == 1, "bad port");
        Check(retro_azahar_room_join("127.0.0.1", 65536, "test", "") == 1, "overflow port");
        Check(retro_azahar_room_join("127.0.0.1", 24872, "abc", "") == 1, "short nick");
        Check(retro_azahar_room_join("127.0.0.1", 24872, "test", std::string(129, 'x').c_str()) == 1, "long secret");
        std::puts("PASS ABI, lifecycle, input validation");

        auto room = Network::GetRoom().lock();
        // Select a free loopback port without binding any LAN interface.
        unsigned port = 35000;
        while (port < 35100 && !room->Create("Test Room", "", "127.0.0.1", port, "secret", 4,
                 "", "", 0, std::make_unique<Network::VerifyUser::NullBackend>())) ++port;
        Check(port < 35100, "room create");
        Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "wrong") == 0, "accepted");
        Idle();
        Check(Read().state == AZ_ROOM_IDLE && Read().error == 8, "wrong password");
        std::puts("PASS real room password rejection");

        Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "secret") == 0, "join retry");
        Wait([] { return Read().state == AZ_ROOM_JOINED; });
        Check(same->GetState() == Network::RoomMember::State::Joined, "same NWM member");
        Check(same->GetMacAddress() == console_mac, "preserve CFG MAC used before join");
        Check(Read().member_count == 1, "participant list");
        Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "secret") == 3, "busy");
        Network::RoomMember peer;
        peer.Join("DesktopTest", "second-identity", "127.0.0.1", port, 0, Network::NoPreferredMac, "secret");
        Wait([&] { return peer.GetState() == Network::RoomMember::State::Joined; });
        Wait([] { return Read().member_count == 2; });
        std::atomic<bool> got_packet{false};
        auto callback = same->BindOnWifiPacketReceived([&](const auto& p) {
            got_packet = p.data == std::vector<u8>{1,2,3,4} && p.channel == 11;
        });
        Network::WifiPacket wifi{};
        wifi.type = Network::WifiPacket::PacketType::Data;
        wifi.channel = 11;
        wifi.data = {1,2,3,4};
        wifi.transmitter_address = peer.GetMacAddress();
        wifi.destination_address = Network::BroadcastMac;
        peer.SendWifiPacket(wifi);
        Wait([&] { return got_packet.load(); });
        same->Unbind(callback);
        peer.Leave(true);
        std::puts("PASS actual join, membership, same instance, WiFi packet transport (not game UDS)");

        retro_azahar_room_leave(); Idle();
        Check(Read().state == AZ_ROOM_IDLE, "leave");
        Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "secret") == 0, "rejoin");
        Wait([] { return Read().state == AZ_ROOM_JOINED; });
        room->Destroy();
        Idle();
        Check(Read().error == 1, "host lost");
        std::puts("PASS leave, rejoin, host close");

        auto begin = std::chrono::steady_clock::now();
        Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "") == 0, "cancel join");
        std::this_thread::sleep_for(40ms);
        retro_azahar_room_leave(); Idle();
        Check(std::chrono::steady_clock::now() - begin < 1s, "prompt cancellation");
        Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "") == 0, "timeout join");
        Wait([] { return !Session::Active(); }, 7000);
        Check(Read().error == 9, "timeout error");
        std::puts("PASS cancel under 1s, reconnect after cancel, real timeout");
        {
            ProtocolFixture fixture(true);
            Check(retro_azahar_room_join("127.0.0.1", fixture.port, "iPhoneTest", "") == 0, "version join");
            Idle();
            Check(Read().error == 7, "wrong version surfaced");
        }
        {
            ProtocolFixture fixture(false);
            Check(retro_azahar_room_join("127.0.0.1", fixture.port, "iPhoneTest", "") == 0, "silent room join");
            Wait([] { return !Session::Active(); }, 13000);
            Check(Read().error == 101, "application handshake timeout");
        }
        std::puts("PASS controlled ENet fixtures: wrong-version error and missing JoinSuccess timeout");
        for (int i = 0; i < 10; ++i) {
            Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "") == 0, "cycle");
            Session::GameStopped();
            Check(Read().state == AZ_ROOM_OFFLINE, "stopped");
            Session::GameReady("test-only-console-identity");
        }
        Session::Shutdown(); same.reset(); room.reset();
        Session::Initialize(); Session::GameReady("test-only-console-identity");
        Check(Read().state == AZ_ROOM_IDLE, "reload lifecycle");
        Session::Shutdown();
        std::atomic<bool> reading{true};
        std::thread reader([&] { while (reading) { auto snapshot = Read(); (void)snapshot; } });
        for (int i = 0; i < 100; ++i) {
            Session::Initialize(); Session::GameReady("concurrent-test");
            Check(retro_azahar_room_join("127.0.0.1", port, "iPhoneTest", "") == 0, "concurrent join");
            retro_azahar_room_leave(); retro_azahar_room_leave();
            Session::GameReady("replacement-game");
            Check(!Session::Active(), "GameReady drains old worker");
            Session::Shutdown();
        }
        reading = false; reader.join();
        std::puts("PASS 100 concurrent snapshot/cancel/replace/shutdown/reinit cycles");
        std::puts("PASS repeated stop/unload/init, all tests passed");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        Session::Shutdown();
        return 1;
    }
}
