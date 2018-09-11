#pragma once

#include <cstdint>

namespace openloco::localisation
{
    uint32_t convertLocoToUnicode(uint8_t loco_char);
    uint8_t convertUnicodeToLoco(uint32_t unicode);

    namespace loco_polish
    {
        constexpr uint8_t a_ogonek_uc = 159; // 0x9F
        constexpr uint8_t c_acute_uc = 162;  // 0xA2
        constexpr uint8_t e_ogonek_uc = 166; // 0xA6
        constexpr uint8_t n_acute_uc = 198;  // 0xC6
        constexpr uint8_t l_stroke_uc = 167; // 0xA7
        constexpr uint8_t s_acute_uc = 208;  // 0xD0
        constexpr uint8_t z_dot_uc = 216;    // 0xD8
        constexpr uint8_t z_acute_uc = 215;  // 0xD7

        constexpr uint8_t a_ogonek = 221; // 0xDD
        constexpr uint8_t c_acute = 222;  // 0xDE
        constexpr uint8_t e_ogonek = 230; // 0xE6
        constexpr uint8_t n_acute = 240;  // 0xF0
        constexpr uint8_t l_stroke = 247; // 0xF7
        constexpr uint8_t s_acute = 248;  // 0xF8
        constexpr uint8_t z_dot = 253;    // 0xFD
        constexpr uint8_t z_acute = 254;  // 0xFE
    };

    namespace unicode_polish
    {
        constexpr uint32_t a_ogonek_uc = 260;
        constexpr uint32_t a_ogonek = 261;
        constexpr uint32_t c_acute_uc = 262;
        constexpr uint32_t c_acute = 263;
        constexpr uint32_t e_ogonek_uc = 280;
        constexpr uint32_t e_ogonek = 281;
        constexpr uint32_t n_acute_uc = 323;
        constexpr uint32_t n_acute = 324;
        constexpr uint32_t l_stroke_uc = 321;
        constexpr uint32_t l_stroke = 322;
        constexpr uint32_t s_acute_uc = 346;
        constexpr uint32_t s_acute = 347;
        constexpr uint32_t z_acute_uc = 377;
        constexpr uint32_t z_acute = 378;
        constexpr uint32_t z_dot_uc = 379;
        constexpr uint32_t z_dot = 380;
    };

    namespace loco_symbols
    {
        constexpr uint8_t up = 32 + 128;
        constexpr uint8_t down = 32 + 138;
        constexpr uint8_t tick = 32 + 140;
        constexpr uint8_t cross = 32 + 141;
        constexpr uint8_t right = 32 + 143;
        constexpr uint8_t railway = 32 + 145;
        constexpr uint8_t road = 32 + 150;
        constexpr uint8_t air = 32 + 151;
        constexpr uint8_t water = 32 + 152;
        constexpr uint8_t small_up = 32 + 156;
        constexpr uint8_t small_down = 32 + 157;
    };

    namespace unicode_symbols
    {
        constexpr uint32_t up = 0x25B2;
        constexpr uint32_t small_up = 0x25B4;
        constexpr uint32_t right = 0x25B6;
        constexpr uint32_t down = 0x25BC;
        constexpr uint32_t small_down = 0x25BE;
        constexpr uint32_t air = 0x2601;
        constexpr uint32_t tick = 0x2713;
        constexpr uint32_t cross = 0x274C;
        constexpr uint32_t road = 0xFE0F;
        constexpr uint32_t water = 0x1F30A;
        constexpr uint32_t railway = 0x1F6E4;
    };
}
