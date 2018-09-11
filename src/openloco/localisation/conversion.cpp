#include "conversion.h"
#include <cstdlib>

namespace openloco::localisation
{
    struct EncodingConvertEntry
    {
        uint32_t unicode;
        uint8_t loco_code;
    };

    static constexpr size_t numLocoEncodingExceptions = 27;
    static const EncodingConvertEntry UnicodeToLocoTable[numLocoEncodingExceptions] = {
        { unicode_polish::a_ogonek_uc, loco_polish::a_ogonek_uc },
        { unicode_polish::a_ogonek, loco_polish::a_ogonek },
        { unicode_polish::c_acute_uc, loco_polish::c_acute_uc },
        { unicode_polish::c_acute, loco_polish::c_acute },
        { unicode_polish::e_ogonek_uc, loco_polish::e_ogonek_uc },
        { unicode_polish::e_ogonek, loco_polish::e_ogonek },
        { unicode_polish::l_stroke_uc, loco_polish::l_stroke_uc },
        { unicode_polish::l_stroke, loco_polish::l_stroke },
        { unicode_polish::n_acute_uc, loco_polish::n_acute_uc },
        { unicode_polish::n_acute, loco_polish::n_acute },
        { unicode_polish::s_acute_uc, loco_polish::s_acute_uc },
        { unicode_polish::s_acute, loco_polish::s_acute },
        { unicode_polish::z_acute_uc, loco_polish::z_acute_uc },
        { unicode_polish::z_acute, loco_polish::z_acute },
        { unicode_polish::z_dot_uc, loco_polish::z_dot_uc },
        { unicode_polish::z_dot, loco_polish::z_dot },
        { unicode_symbols::up, loco_symbols::up },
        { unicode_symbols::small_up, loco_symbols::small_up },
        { unicode_symbols::right, loco_symbols::right },
        { unicode_symbols::down, loco_symbols::down },
        { unicode_symbols::small_down, loco_symbols::small_down },
        { unicode_symbols::air, loco_symbols::air },
        { unicode_symbols::tick, loco_symbols::tick },
        { unicode_symbols::cross, loco_symbols::cross },
        { unicode_symbols::road, loco_symbols::road },
        { unicode_symbols::water, loco_symbols::water },
        { unicode_symbols::railway, loco_symbols::railway },
    };

    static int32_t searchCompare(const void* pKey, const void* pEntry)
    {
        uint32_t key = *((uint32_t*)pKey);
        EncodingConvertEntry* entry = (EncodingConvertEntry*)pEntry;
        if (key < entry->unicode)
            return -1;
        if (key > entry->unicode)
            return 1;
        return 0;
    }

    uint32_t convertLocoToUnicode(uint8_t loco_code)
    {
        // We can't do a binary search here, as the table is sorted by Unicode point, not Loco's internal encoding.
        for (const auto& entry : UnicodeToLocoTable)
        {
            if (entry.loco_code == loco_code)
                return entry.unicode;
        }
        return loco_code;
    }

    uint8_t convertUnicodeToLoco(uint32_t unicode)
    {
        EncodingConvertEntry* entry = (EncodingConvertEntry*)std::bsearch(&unicode, UnicodeToLocoTable, numLocoEncodingExceptions, sizeof(EncodingConvertEntry), searchCompare);
        if (entry != nullptr)
            return entry->loco_code;
        else if (unicode < 256)
            return unicode;
        else
            return '?';
    }
}
