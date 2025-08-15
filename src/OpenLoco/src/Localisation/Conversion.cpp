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

    // Ensure that the table is sorted by Unicode point.
    static_assert(std::ranges::is_sorted(kUnicodeToLocoTable, {}, &EncodingConvertEntry::unicode));

    utf32_t convertLocoToUnicode(uint8_t locoCode)
    {
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
        const auto it = std::ranges::lower_bound(
            kUnicodeToLocoTable,
            unicode,
            {},
            &EncodingConvertEntry::unicode);

        if (it != kUnicodeToLocoTable.end() && it->unicode == unicode)
        {
            return it->locoCode;
        }

        if (unicode < 256)
        {
            return static_cast<uint8_t>(unicode);
        }

        return '?';
    }

    std::string convertUnicodeToLoco(const std::string& unicodeString)
    {
        std::string out;
        uint8_t* input = (uint8_t*)unicodeString.c_str();
        while (utf32_t unicodePoint = readCodePoint(&input))
        {
            out += convertUnicodeToLoco(unicodePoint);
        }

        return out;
    }
}
