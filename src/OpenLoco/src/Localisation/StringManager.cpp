#include "StringManager.h"
#include "ArgsWrapper.hpp"
#include "Config.h"
#include "Date.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Objects/CurrencyObject.h"
#include "Objects/ObjectManager.h"
#include "StringIds.h"
#include "World/TownManager.h"
#include <OpenLoco/Console/Console.h>
#include <OpenLoco/Interop/Interop.hpp>

#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>

using namespace OpenLoco::Interop;

namespace OpenLoco::StringManager
{
    const uint16_t kUserStringsStart = 0x8000;
    const uint16_t kUserStringsEnd = kUserStringsStart + Limits::kMaxUserStrings;

    const uint16_t kMaxTownNames = 345;
    const uint16_t kTownNamesStart = 0x9EE7;
    const uint16_t kTownNamesEnd = kTownNamesStart + kMaxTownNames;

    // 0x2000 lang strings, 0x10 temp obj strings, 0x45E loaded obj strings
    static loco_global<char* [0x246E], 0x005183FC> _strings;

    static auto& rawUserStrings() { return getGameState().userStrings; }

    // 0x0049650E
    void reset()
    {
        for (auto* str : rawUserStrings())
        {
            *str = '\0';
        }
    }

    const char* getString(string_id id)
    {
        char* str = _strings[id];
        return str;
    }

    void setString(string_id id, std::string_view value)
    {
        auto* dst = _strings[id];
        std::memcpy(dst, value.data(), value.size());
        dst[value.size()] = '\0';
    }

    const char* swapString(string_id id, const char* src)
    {
        auto* dst = _strings[id];
        _strings[id] = const_cast<char*>(src);
        return dst;
    }

    // 0x00496522
    string_id userStringAllocate(char* str /* edi */, uint8_t cl)
    {
        auto bestSlot = -1;
        for (auto i = 0u; i < Limits::kMaxUserStrings; ++i)
        {
            char* userStr = rawUserStrings()[i];
            if (*userStr == '\0')
            {
                bestSlot = i;
            }
            else if (cl > 0)
            {
                if (strcmp(str, userStr) == 0)
                {
                    GameCommands::setErrorText(StringIds::chosen_name_in_use);
                    return StringIds::empty;
                }
            }
        }

        if (bestSlot == -1)
        {
            GameCommands::setErrorText(StringIds::too_many_names_in_use);
            return StringIds::empty;
        }

        char* userStr = rawUserStrings()[bestSlot];
        strncpy(userStr, str, kUserStringSize);
        userStr[kUserStringSize - 1] = '\0';
        return bestSlot + kUserStringsStart;
    }

    const char* getUserString(string_id id)
    {
        return rawUserStrings()[id];
    }

    // 0x004965A6
    void emptyUserString(string_id stringId)
    {
        if (!isUserString(stringId))
        {
            return;
        }

        *rawUserStrings()[stringId - kUserStringsStart] = '\0';
    }

    bool isUserString(string_id stringId)
    {
        if (stringId < kUserStringsStart || stringId >= kUserStringsEnd)
        {
            return false;
        }
        return true;
    }

}
