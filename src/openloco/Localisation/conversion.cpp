#include "conversion.h"
#include "../utility/collection.hpp"
#include "unicode.h"
#include <cstdlib>
#include <string>

namespace openloco::localisation
{
    struct EncodingConvertEntry
    {
        utf32_t unicode;
        uint8_t loco_code;
    };

    static const EncodingConvertEntry UnicodeToLocoTable[] = {
        { unicode_char::a_ogonek_uc, loco_char::a_ogonek_uc },
        { unicode_char::a_ogonek, loco_char::a_ogonek },
        { unicode_char::c_acute_uc, loco_char::c_acute_uc },
        { unicode_char::c_acute, loco_char::c_acute },
        { unicode_char::e_ogonek_uc, loco_char::e_ogonek_uc },
        { unicode_char::e_ogonek, loco_char::e_ogonek },
        { unicode_char::l_stroke_uc, loco_char::l_stroke_uc },
        { unicode_char::l_stroke, loco_char::l_stroke },
        { unicode_char::n_acute_uc, loco_char::n_acute_uc },
        { unicode_char::n_acute, loco_char::n_acute },
        { unicode_char::s_acute_uc, loco_char::s_acute_uc },
        { unicode_char::s_acute, loco_char::s_acute },
        { unicode_char::z_acute_uc, loco_char::z_acute_uc },
        { unicode_char::z_acute, loco_char::z_acute },
        { unicode_char::z_dot_uc, loco_char::z_dot_uc },
        { unicode_char::z_dot, loco_char::z_dot },
        { unicode_char::quote_open, loco_char::quote_open },
        { unicode_char::quote_close, loco_char::quote_close },
        { unicode_char::up, loco_char::up },
        { unicode_char::small_up, loco_char::small_up },
        { unicode_char::right, loco_char::right },
        { unicode_char::down, loco_char::down },
        { unicode_char::small_down, loco_char::small_down },
        { unicode_char::air, loco_char::air },
        { unicode_char::tick, loco_char::tick },
        { unicode_char::cross, loco_char::cross },
        { unicode_char::water, loco_char::water },
        { unicode_char::road, loco_char::road },
        { unicode_char::railway, loco_char::railway },
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
        EncodingConvertEntry* entry = (EncodingConvertEntry*)std::bsearch(&unicode, UnicodeToLocoTable, utility::length(UnicodeToLocoTable), sizeof(EncodingConvertEntry), searchCompare);
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
