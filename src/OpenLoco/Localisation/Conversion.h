#pragma once

#include "Unicode.h"
#include <cstdint>
#include <string>

namespace OpenLoco::Localisation
{
    utf32_t convertLocoToUnicode(uint8_t loco_char);
    uint8_t convertUnicodeToLoco(utf32_t unicode);
    std::string convertUnicodeToLoco(const std::string& unicode_string);

    namespace LocoChar
    {
        // Polish characters (uppercase)
        constexpr uint8_t a_ogonek_uc = 159; // 0x9F
        constexpr uint8_t c_acute_uc = 162;  // 0xA2
        constexpr uint8_t e_ogonek_uc = 166; // 0xA6
        constexpr uint8_t n_acute_uc = 198;  // 0xC6
        constexpr uint8_t l_stroke_uc = 167; // 0xA7
        constexpr uint8_t s_acute_uc = 208;  // 0xD0
        constexpr uint8_t z_dot_uc = 216;    // 0xD8
        constexpr uint8_t z_acute_uc = 215;  // 0xD7

        // Quotation marks
        constexpr uint8_t quote_open = 180;
        constexpr uint8_t quote_close = 34;

        // Dingbats
        constexpr uint8_t up = 160;
        constexpr uint8_t down = 170;
        constexpr uint8_t tick = 172;
        constexpr uint8_t cross = 173;
        constexpr uint8_t right = 175;
        constexpr uint8_t small_up = 188;
        constexpr uint8_t small_down = 189;

        // Special symbols
        constexpr uint8_t railway = 177;
        constexpr uint8_t road = 182;
        constexpr uint8_t air = 183;
        constexpr uint8_t water = 184;

        // Polish characters (lowercase)
        constexpr uint8_t a_ogonek = 221; // 0xDD
        constexpr uint8_t c_acute = 222;  // 0xDE
        constexpr uint8_t e_ogonek = 230; // 0xE6
        constexpr uint8_t n_acute = 240;  // 0xF0
        constexpr uint8_t l_stroke = 247; // 0xF7
        constexpr uint8_t s_acute = 248;  // 0xF8
        constexpr uint8_t z_dot = 253;    // 0xFD
        constexpr uint8_t z_acute = 254;  // 0xFE
    };

    namespace UnicodeChar
    {
        // Polish characters
        constexpr utf32_t a_ogonek_uc = 260;
        constexpr utf32_t a_ogonek = 261;
        constexpr utf32_t c_acute_uc = 262;
        constexpr utf32_t c_acute = 263;
        constexpr utf32_t e_ogonek_uc = 280;
        constexpr utf32_t e_ogonek = 281;
        constexpr utf32_t n_acute_uc = 323;
        constexpr utf32_t n_acute = 324;
        constexpr utf32_t l_stroke_uc = 321;
        constexpr utf32_t l_stroke = 322;
        constexpr utf32_t s_acute_uc = 346;
        constexpr utf32_t s_acute = 347;
        constexpr utf32_t z_acute_uc = 377;
        constexpr utf32_t z_acute = 378;
        constexpr utf32_t z_dot_uc = 379;
        constexpr utf32_t z_dot = 380;

        // Quotation marks
        constexpr utf32_t quote_open = 0x201C;
        constexpr utf32_t quote_close = 0x201D;

        // Superscript
        constexpr utf32_t superscript_minus = 0x207B;

        // Dingbats
        constexpr utf32_t up = 0x25B2;
        constexpr utf32_t small_up = 0x25B4;
        constexpr utf32_t right = 0x25B6;
        constexpr utf32_t down = 0x25BC;
        constexpr utf32_t small_down = 0x25BE;
        constexpr utf32_t air = 0x2601;
        constexpr utf32_t tick = 0x2713;

        // Emoji
        constexpr utf32_t cross = 0x274C;
        constexpr utf32_t variation_selector = 0xFE0F;
        constexpr utf32_t water = 0x1F30A;
        constexpr utf32_t road = 0x1F6E3;
        constexpr utf32_t railway = 0x1F6E4;
    };
}
