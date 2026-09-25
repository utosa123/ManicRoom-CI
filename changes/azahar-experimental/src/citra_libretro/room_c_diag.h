// Temporary startup diagnostics. No emulator state changes or exception handling.
#pragma once
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <exception>
#include <string>
#if defined(IOS)
#include <os/log.h>
#endif
namespace RoomCDiag {
// Keep the storage suffix needed for diagnosis without logging the container UUID.
inline std::string StoragePathForLog(const std::string& path) {
    if (path.empty()) return "<empty>";
    const auto documents = path.find("/Documents/");
    if (documents != std::string::npos) return "[path]" + path.substr(documents);
    if (path.ends_with("/Documents")) return "[path]/Documents";
    return "[path outside Documents]";
}
inline void Log(const char* format, ...) {
#if defined(IOS)
    char text[2048]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args);
    os_log_with_type(OS_LOG_DEFAULT, OS_LOG_TYPE_DEFAULT, "[ROOM-C-DIAG] %{public}s", text);
#endif
}
// RETURN means control left normally, not that the game booted successfully.
class Span {
    const char* name; bool enabled; int exceptions; int result = -1;
public:
    Span(const char* n, bool e=true) : name(n), enabled(e), exceptions(std::uncaught_exceptions()) {
        if (enabled) Log("%s ENTER", name);
    }
    bool Result(bool value) { result=value ? 1 : 0; return value; }
    ~Span() {
        if (!enabled) return;
        if (std::uncaught_exceptions()>exceptions) Log("%s UNWIND (C++ exception)",name);
        else if (result>=0) Log("%s RETURN result=%d",name,result);
        else Log("%s RETURN",name);
    }
};
inline std::atomic<unsigned long long> frames{0};
inline bool Sample(unsigned long long count) { return count<=5 || count%300==0; }
}
