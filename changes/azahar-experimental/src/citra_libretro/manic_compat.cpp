// SPDX-License-Identifier: GPL-2.0-or-later
#include "libretro.h"
#include "citra_libretro/manic_compat.h"
#ifndef MANIC_ROOM_EXPERIMENT
#error This adapter requires the isolated experimental build profile
#endif
namespace {
using Config = retro_azahar_keyboard_config_local;
using Callback = void (*)(const Config*);
Callback callback = nullptr;
void Request(const retro_keyboard_config* source) {
    if (!source || !callback) return;
    Config copy{};
    copy.button_config = source->button_config;
    copy.accept_mode = source->accept_mode;
    copy.multiline_mode = source->multiline_mode;
    copy.max_text_length = source->max_text_length;
    copy.max_digits = source->max_digits;
    copy.hint_text = source->hint_text;
    copy.button_text = source->button_text;
    copy.button_text_count = source->button_text_count;
    copy.prevent_digit = source->prevent_digit;
    copy.prevent_at = source->prevent_at;
    copy.prevent_percent = source->prevent_percent;
    copy.prevent_backslash = source->prevent_backslash;
    copy.prevent_profanity = source->prevent_profanity;
    copy.enable_callback = source->enable_callback;
    callback(&copy); // Borrowed pointers: frontend copies strings before returning.
}
}
static_assert(offsetof(Config,button_config) == offsetof(retro_keyboard_config,button_config));
static_assert(offsetof(Config,accept_mode) == offsetof(retro_keyboard_config,accept_mode));
static_assert(offsetof(Config,multiline_mode) == offsetof(retro_keyboard_config,multiline_mode));
static_assert(offsetof(Config,max_text_length) == offsetof(retro_keyboard_config,max_text_length));
static_assert(offsetof(Config,max_digits) == offsetof(retro_keyboard_config,max_digits));
static_assert(offsetof(Config,hint_text) == offsetof(retro_keyboard_config,hint_text));
static_assert(offsetof(Config,button_text) == offsetof(retro_keyboard_config,button_text));
static_assert(offsetof(Config,button_text_count) == offsetof(retro_keyboard_config,button_text_count));
static_assert(offsetof(Config,prevent_digit) == offsetof(retro_keyboard_config,prevent_digit));
static_assert(offsetof(Config,prevent_at) == offsetof(retro_keyboard_config,prevent_at));
static_assert(offsetof(Config,prevent_percent) == offsetof(retro_keyboard_config,prevent_percent));
static_assert(offsetof(Config,prevent_backslash) == offsetof(retro_keyboard_config,prevent_backslash));
static_assert(offsetof(Config,prevent_profanity) == offsetof(retro_keyboard_config,prevent_profanity));
static_assert(offsetof(Config,enable_callback) == offsetof(retro_keyboard_config,enable_callback));
static_assert(sizeof(Config)==sizeof(retro_keyboard_config));
// Like retro_run/load/unload, these four APIs require the emulation thread.
// Manic's CocoaTouch runloop and normalized bridge execute on the main thread.
extern "C" {
RETRO_API uint32_t retro_manic_experiment_api_version(void) { return 1; }
RETRO_API void retro_azahar_set_keyboard_callback(Callback cb) {
    callback = cb;
    retro_set_keyboard_callback(cb ? Request : nullptr);
}
RETRO_API void retro_azahar_keyboard_input(const char* text, int button) {
    if (callback && button >= 0 && button <= 2) retro_keyboard_input(text, button);
}
RETRO_API bool retro_azahar_load_amiibo(const char* path) {
    return path && retro_load_amiibo(path);
}
RETRO_API bool retro_azahar_is_searching_amiibo(void) { return retro_is_searching_amiibo(); }
}
