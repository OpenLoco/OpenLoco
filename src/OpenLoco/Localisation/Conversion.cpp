#include "Conversion.h"
#include "../Utility/Collection.hpp"
#include "Unicode.h"
#include <cstdlib>
#include <string>

namespace OpenLoco::Localisation
{
    struct EncodingConvertEntry
    {
        utf32_t unicode;
        uint8_t loco_code;
    };

    static const EncodingConvertEntry UnicodeToLocoTable[] = {
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
        { UnicodeChar::quote_open, LocoChar::quote_open },
        { UnicodeChar::quote_close, LocoChar::quote_close },
        { UnicodeChar::up, LocoChar::up },
        { UnicodeChar::small_up, LocoChar::small_up },
        { UnicodeChar::right, LocoChar::right },
        { UnicodeChar::down, LocoChar::down },
        { UnicodeChar::small_down, LocoChar::small_down },
        { UnicodeChar::air, LocoChar::air },
        { UnicodeChar::tick, LocoChar::tick },
        { UnicodeChar::cross, LocoChar::cross },
        { UnicodeChar::water, LocoChar::water },
        { UnicodeChar::road, LocoChar::road },
        { UnicodeChar::railway, LocoChar::railway },
    };

    static int32_t searchCompare(const void* pKey, const void* pEntry)
    {
        utf32_t key = *((utf32_t*)pKey);
        EncodingConvertEntry* entry = (EncodingConvertEntry*)pEntry;
        if (key < entry->unicode)
            return -1;
        if (key > entry->unicode)
            return 1;
        return 0;
    }

    utf32_t convertLocoToUnicode(uint8_t loco_code)
    {
        // We can't do a binary search here, as the table is sorted by Unicode point, not Loco's internal encoding.
        for (const auto& entry : UnicodeToLocoTable)
        {
            if (entry.loco_code == loco_code)
                return entry.unicode;
        }
        return loco_code;
    }

    uint8_t convertUnicodeToLoco(utf32_t unicode)
    {
        EncodingConvertEntry* entry = (EncodingConvertEntry*)std::bsearch(&unicode, UnicodeToLocoTable, Utility::length(UnicodeToLocoTable), sizeof(EncodingConvertEntry), searchCompare);
        if (entry != nullptr)
            return entry->loco_code;
        else if (unicode < 256)
            return unicode;
        else
            return '?';
    }

    std::string convertUnicodeToLoco(std::string unicode_string)
    {
        std::string out;
        uint8_t* input = (uint8_t*)unicode_string.c_str();
        while (utf32_t unicode_point = readCodePoint(&input))
        {
            out += convertUnicodeToLoco(unicode_point);
        }

        return out;
    }
}
