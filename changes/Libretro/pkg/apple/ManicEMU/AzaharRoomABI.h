// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#if defined(_WIN32)
#define AZ_ROOM_EXPORT __declspec(dllexport)
#else
#define AZ_ROOM_EXPORT __attribute__((visibility("default")))
#endif

/* ABI 1. All strings are UTF-8, NUL terminated and copied before join returns.
 * Caller owns snapshot storage; no pointers or callbacks cross this boundary.
 * Calls must not overlap dlclose. Query functions do not initialize the core.
 * Join: 0 = accepted, NOT joined; 1 = invalid input; 2 = not ready; 3 = busy;
 * -1 = internal error. Only snapshot.state == 3 means joined.
 * IPv4 literals only. Nicknames: [A-Za-z0-9._ -]{4,20}. Password <=128 bytes.
 * Snapshot errors 1..13 map to RoomMember::Error + 1; 100 = internal,
 * 101 = handshake timeout. State 4 lasts until socket cleanup completes. */
enum { AZ_ROOM_OFFLINE = 0, AZ_ROOM_IDLE = 1, AZ_ROOM_JOINING = 2,
       AZ_ROOM_JOINED = 3, AZ_ROOM_LEAVING = 4 };
typedef struct az_room_snapshot {
    uint32_t state;
    uint32_t error;
    uint32_t protocol;
    uint32_t default_port;
    uint32_t member_count;
    char members[254][21];
} az_room_snapshot;
AZ_ROOM_EXPORT uint32_t retro_azahar_room_api_version(void);
AZ_ROOM_EXPORT int32_t retro_azahar_room_snapshot(az_room_snapshot* out, uint32_t size);
AZ_ROOM_EXPORT int32_t retro_azahar_room_join(const char* ip, uint32_t port,
                                           const char* nickname, const char* password);
AZ_ROOM_EXPORT int32_t retro_azahar_room_leave(void);
#ifdef __cplusplus
}
#endif
