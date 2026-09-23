// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
// Experimental Manic adapter contract 1; NOT retro_azahar_extension_version.
struct retro_azahar_keyboard_config_local {
    int button_config, accept_mode;
    bool multiline_mode;
    int max_text_length, max_digits;
    const char* hint_text;
    const char** button_text;
    int button_text_count;
    bool prevent_digit, prevent_at, prevent_percent, prevent_backslash;
    bool prevent_profanity, enable_callback;
};
#if defined(__cplusplus)
#define AZ_ASSERT static_assert
#else
#define AZ_ASSERT _Static_assert
#endif
AZ_ASSERT(sizeof(int)==4 && sizeof(bool)==1 && sizeof(void*)==8, "64-bit adapter ABI only");
AZ_ASSERT(sizeof(struct retro_azahar_keyboard_config_local)==56, "keyboard sizeof");
#define AZ_OFFSET(f,n) AZ_ASSERT(offsetof(struct retro_azahar_keyboard_config_local,f)==n, #f)
AZ_OFFSET(button_config,0); AZ_OFFSET(accept_mode,4); AZ_OFFSET(multiline_mode,8);
AZ_OFFSET(max_text_length,12); AZ_OFFSET(max_digits,16); AZ_OFFSET(hint_text,24);
AZ_OFFSET(button_text,32); AZ_OFFSET(button_text_count,40); AZ_OFFSET(prevent_digit,44);
AZ_OFFSET(prevent_at,45); AZ_OFFSET(prevent_percent,46); AZ_OFFSET(prevent_backslash,47);
AZ_OFFSET(prevent_profanity,48); AZ_OFFSET(enable_callback,49);
#undef AZ_OFFSET
#undef AZ_ASSERT
