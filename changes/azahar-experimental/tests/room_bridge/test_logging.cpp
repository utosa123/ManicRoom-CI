// SPDX-License-Identifier: GPL-2.0-or-later
// Test-only synchronous log sink. Network/Room/RoomMember/ENet are production code.
#include <cstdio>
#include "common/logging/log.h"
#include "common/logging/backend.h"
namespace Common::Log {
void Stop() {}
void FmtLogMessageImpl(Class, Level, const char*, unsigned int, const char*,
                       fmt::string_view format, const fmt::format_args& args) {
    std::fprintf(stderr, "%s\n", fmt::vformat(format, args).c_str());
}
}
