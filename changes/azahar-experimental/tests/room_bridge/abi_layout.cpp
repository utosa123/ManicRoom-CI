#include <cstdio>
#include "libretro.h"
#include "citra_libretro/manic_compat.h"
using Config = retro_azahar_keyboard_config_local;
static_assert(offsetof(Config,button_config)==offsetof(retro_keyboard_config,button_config));
static_assert(offsetof(Config,accept_mode)==offsetof(retro_keyboard_config,accept_mode));
static_assert(offsetof(Config,multiline_mode)==offsetof(retro_keyboard_config,multiline_mode));
static_assert(offsetof(Config,max_text_length)==offsetof(retro_keyboard_config,max_text_length));
static_assert(offsetof(Config,max_digits)==offsetof(retro_keyboard_config,max_digits));
static_assert(offsetof(Config,hint_text)==offsetof(retro_keyboard_config,hint_text));
static_assert(offsetof(Config,button_text)==offsetof(retro_keyboard_config,button_text));
static_assert(offsetof(Config,button_text_count)==offsetof(retro_keyboard_config,button_text_count));
static_assert(offsetof(Config,prevent_digit)==offsetof(retro_keyboard_config,prevent_digit));
static_assert(offsetof(Config,prevent_at)==offsetof(retro_keyboard_config,prevent_at));
static_assert(offsetof(Config,prevent_percent)==offsetof(retro_keyboard_config,prevent_percent));
static_assert(offsetof(Config,prevent_backslash)==offsetof(retro_keyboard_config,prevent_backslash));
static_assert(offsetof(Config,prevent_profanity)==offsetof(retro_keyboard_config,prevent_profanity));
static_assert(offsetof(Config,enable_callback)==offsetof(retro_keyboard_config,enable_callback));
static_assert(sizeof(Config)==sizeof(retro_keyboard_config));
static_assert(alignof(Config)==8 && alignof(retro_keyboard_config)==8);
int main() { std::puts("PASS keyboard ABI: sizeof=56 align=8; all 14 offsets match real public header"); }
