// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace LibRetro::RoomSession {
// Called by the emulation lifecycle, never from the network worker.
void Initialize() noexcept;
void GameReady(std::string console_id_hash,
               std::array<uint8_t, 6> mac = {255, 255, 255, 255, 255, 255}) noexcept;
void GameStopped() noexcept;
void Shutdown() noexcept;
bool Active() noexcept;
}
