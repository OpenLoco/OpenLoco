#include "Conversion.h"
#include "Unicode.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <string>

namespace OpenLoco::Localisation
{
    struct EncodingConvertEntry
    {
        utf32_t unicode;
        uint8_t locoCode;
    };

    static constexpr auto kUnicodeToLocoTable = std::to_array<EncodingConvertEntry>({
        { UnicodeChar::curly_bracket_open, LocoChar::replacement_character }, // These four are replaced with formatting control codes in Loco
        { UnicodeChar::vertical_line, LocoChar::replacement_character },
        { UnicodeChar::curly_bracket_close, LocoChar::replacement_character },
        { UnicodeChar::tilde, LocoChar::replacement_character },
        { UnicodeChar::a_ogonek_uc, LocoChar::a_ogonek_uc },
        { UnicodeChar::a_ogonek, LocoChar::a_ogonek },
        { UnicodeChar::c_acute_uc, LocoChar::c_acute_uc },
        { UnicodeChar::c_acute, LocoChar::c_acute },
        { UnicodeChar::e_ogonek_uc, LocoChar::e_ogonek_uc },
        { UnicodeChar::e_ogonek, LocoChar::e_ogonek },
        { UnicodeChar::l_stroke_uc, LocoChar::l_stroke_uc },
        { UnicodeChar::l_stroke, LocoChar::l_stroke },
        { UnicodeChar::n_acute_uc, LocoChar::n_acute_uc },
        { UnicodeChar::n_acute, LocoChar::n_acute },
        { UnicodeChar::s_acute_uc, LocoChar::s_acute_uc },
        { UnicodeChar::s_acute, LocoChar::s_acute },
        { UnicodeChar::z_acute_uc, LocoChar::z_acute_uc },
        { UnicodeChar::z_acute, LocoChar::z_acute },
        { UnicodeChar::z_dot_uc, LocoChar::z_dot_uc },
        { UnicodeChar::z_dot, LocoChar::z_dot },
        { UnicodeChar::single_quote_open, LocoChar::single_quote_open },
        { UnicodeChar::single_quote_close, LocoChar::single_quote_close },
        { UnicodeChar::quote_open, LocoChar::quote_open },
        { UnicodeChar::quote_close, LocoChar::quote_close },
        { UnicodeChar::bullet, LocoChar::bullet },
        { UnicodeChar::up, LocoChar::up },
        { UnicodeChar::small_up, LocoChar::small_up },
        { UnicodeChar::right, LocoChar::right },
        { UnicodeChar::down, LocoChar::down },
        { UnicodeChar::small_down, LocoChar::small_down },
        { UnicodeChar::left, LocoChar::left },
        { UnicodeChar::air, LocoChar::air },
        { UnicodeChar::tick, LocoChar::tick },
        { UnicodeChar::cross, LocoChar::cross },
        { UnicodeChar::water, LocoChar::water },
        { UnicodeChar::road, LocoChar::road },
        { UnicodeChar::railway, LocoChar::railway },
    });

    static constexpr auto kUnicodeTemporaryStrip = std::to_array<EncodingConvertEntry>({
        { 0x00C6, 'A' }, // Æ
        { 0x00D0, 'D' }, // Ð
        { 0x00D8, 'O' }, // Ø
        { 0x00DD, 'Y' }, // Ý
        { 0x00DE, 'T' }, // Þ
        { 0x00E6, 'a' }, // æ
        { 0x00F0, 'd' }, // ð
        { 0x00F8, 'o' }, // ø
        { 0x00FD, 'y' }, // ý
        { 0x00FE, 't' }, // þ
        { 0x00FF, 'y' }, // ÿ
        { 0x0100, 'A' }, // Ā
        { 0x0101, 'a' }, // ā
        { 0x0102, 'A' }, // Ă
        { 0x0103, 'a' }, // ă
        { 0x0108, 'C' }, // Ĉ
        { 0x0109, 'c' }, // ĉ
        { 0x010A, 'C' }, // Ċ
        { 0x010B, 'c' }, // ċ
        { 0x010C, 'C' }, // Č
        { 0x010D, 'c' }, // č
        { 0x010E, 'D' }, // Ď
        { 0x010F, 'd' }, // ď
        { 0x0110, 'D' }, // Đ
        { 0x0111, 'd' }, // đ
        { 0x0112, 'E' }, // Ē
        { 0x0113, 'e' }, // ē
        { 0x0114, 'E' }, // Ĕ
        { 0x0115, 'e' }, // ĕ
        { 0x0116, 'E' }, // Ė
        { 0x0117, 'e' }, // ė
        { 0x011A, 'E' }, // Ě
        { 0x011B, 'e' }, // ě
        { 0x011C, 'G' }, // Ĝ
        { 0x011D, 'g' }, // ĝ
        { 0x011E, 'G' }, // Ğ
        { 0x011F, 'g' }, // ğ
        { 0x0120, 'G' }, // Ġ
        { 0x0121, 'g' }, // ġ
        { 0x0122, 'G' }, // Ģ
        { 0x0123, 'g' }, // ģ
        { 0x0124, 'H' }, // Ĥ
        { 0x0125, 'h' }, // ĥ
        { 0x0126, 'H' }, // Ħ
        { 0x0127, 'h' }, // ħ
        { 0x0128, 'I' }, // Ĩ
        { 0x0129, 'i' }, // ĩ
        { 0x012A, 'I' }, // Ī
        { 0x012B, 'i' }, // ī
        { 0x012C, 'I' }, // Ĭ
        { 0x012D, 'i' }, // ĭ
        { 0x012E, 'I' }, // Į
        { 0x012F, 'i' }, // į
        { 0x0130, 'I' }, // İ
        { 0x0131, 'i' }, // ı
        { 0x0132, 'I' }, // Ĳ
        { 0x0133, 'i' }, // ĳ
        { 0x0134, 'J' }, // Ĵ
        { 0x0135, 'j' }, // ĵ
        { 0x0136, 'K' }, // Ķ
        { 0x0137, 'k' }, // ķ
        { 0x0138, 'k' }, // ĸ
        { 0x0139, 'L' }, // Ĺ
        { 0x013A, 'l' }, // ĺ
        { 0x013B, 'L' }, // Ļ
        { 0x013C, 'l' }, // ļ
        { 0x013D, 'L' }, // Ľ
        { 0x013E, 'l' }, // ľ
        { 0x013F, 'L' }, // Ŀ
        { 0x0140, 'l' }, // ŀ
        { 0x0145, 'N' }, // Ņ
        { 0x0146, 'n' }, // ņ
        { 0x0147, 'N' }, // Ň
        { 0x0148, 'n' }, // ň
        { 0x0149, 'n' }, // ŉ
        { 0x014A, 'n' }, // Ŋ
        { 0x014B, 'n' }, // ŋ
        { 0x014C, 'O' }, // Ō
        { 0x014D, 'o' }, // ō
        { 0x014E, 'O' }, // Ŏ
        { 0x014F, 'o' }, // ŏ
        { 0x0150, 'O' }, // Ő
        { 0x0151, 'o' }, // ő
        { 0x0152, 'O' }, // Œ
        { 0x0153, 'o' }, // œ
        { 0x0154, 'R' }, // Ŕ
        { 0x0155, 'r' }, // ŕ
        { 0x0156, 'R' }, // Ŗ
        { 0x0157, 'r' }, // ŗ
        { 0x0158, 'R' }, // Ř
        { 0x0159, 'r' }, // ř
        { 0x015C, 'S' }, // Ŝ
        { 0x015D, 's' }, // ŝ
        { 0x015E, 'S' }, // Ş
        { 0x015F, 's' }, // ş
        { 0x0160, 'S' }, // Š
        { 0x0161, 's' }, // š
        { 0x0162, 'T' }, // Ţ
        { 0x0163, 't' }, // ţ
        { 0x0164, 'T' }, // Ť
        { 0x0165, 't' }, // ť
        { 0x0166, 'T' }, // Ŧ
        { 0x0167, 't' }, // ŧ
        { 0x0168, 'U' }, // Ũ
        { 0x0169, 'u' }, // ũ
        { 0x016A, 'U' }, // Ū
        { 0x016B, 'u' }, // ū
        { 0x016C, 'U' }, // Ŭ
        { 0x016D, 'u' }, // ŭ
        { 0x016E, 'U' }, // Ů
        { 0x016F, 'u' }, // ů
        { 0x0170, 'U' }, // Ű
        { 0x0171, 'u' }, // ű
        { 0x0172, 'U' }, // Ų
        { 0x0173, 'u' }, // ų
        { 0x0174, 'W' }, // Ŵ
        { 0x0175, 'w' }, // ŵ
        { 0x0176, 'Y' }, // Ŷ
        { 0x0177, 'y' }, // ŷ
        { 0x0178, 'Y' }, // Ÿ
        { 0x017D, 'Z' }, // Ž
        { 0x017E, 'z' }, // ž
        { 0x017F, 's' }, // ſ
    });

    // Ensure that the table is sorted by Unicode point.
    static_assert(std::ranges::is_sorted(kUnicodeToLocoTable, {}, &EncodingConvertEntry::unicode));
    static_assert(std::ranges::is_sorted(kUnicodeTemporaryStrip, {}, &EncodingConvertEntry::unicode));

    utf32_t convertLocoToUnicode(uint8_t locoCode)
    {
        // Do not convert '?' character.
        if (locoCode == LocoChar::replacement_character)
        {
            return LocoChar::replacement_character;
        }

        // We can't do a binary search here, as the table is sorted by Unicode point, not Loco's internal encoding.
        for (const auto& entry : kUnicodeToLocoTable)
        {
            if (entry.locoCode == locoCode)
            {
                return entry.unicode;
            }
        }
        return locoCode;
    }

    uint8_t convertUnicodeToLoco(utf32_t unicode)
    {
        auto tableLookup = [unicode](auto&& table) {
            return std::ranges::lower_bound(
                table,
                unicode,
                {},
                &EncodingConvertEntry::unicode);
        };

        // Extended Latin characters that are supported by Locomotion as-is
        if (auto it = tableLookup(kUnicodeToLocoTable); it != kUnicodeToLocoTable.end() && it->unicode == unicode)
        {
            return it->locoCode;
        }

        // Remove diacritics from letters that don't have an associated glyph yet
        if (auto it = tableLookup(kUnicodeTemporaryStrip); it != kUnicodeTemporaryStrip.end() && it->unicode == unicode)
        {
            return it->locoCode;
        }

        // Basic Latin characters
        if (unicode < 256)
        {
            return static_cast<uint8_t>(unicode);
        }

        return LocoChar::replacement_character;
    }

    std::string convertUnicodeToLoco(const std::string& unicodeString)
    {
        std::string out;
        const uint8_t* input = (uint8_t*)unicodeString.c_str();
        while (utf32_t unicodePoint = readCodePoint(&input))
        {
            out += convertUnicodeToLoco(unicodePoint);
        }

        return out;
    }
}
