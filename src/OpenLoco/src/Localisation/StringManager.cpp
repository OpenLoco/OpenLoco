#include "StringManager.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "StringIds.h"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>

#include <cassert>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::StringManager
{
    // 0x2000 lang strings, 0x10 temp obj strings, 0x45E loaded obj strings
    constexpr size_t kNumStringPointers = 0x246E; // 9326 strings

    // Size for buffer strings that are used for temporary text storage
    static constexpr size_t kBufferStringSize = 512;

    static char _buffer_337[kBufferStringSize];
    static char _buffer_338[kBufferStringSize];
    static char _buffer_1250[kBufferStringSize];
    static char _preferred_currency_buffer[kBufferStringSize];
    static char _buffer_1719[kBufferStringSize];
    static char _buffer_2039[kBufferStringSize];
    static char _buffer_2040[kBufferStringSize];

    // 0x005183FC
    // Initialize string pointer array with buffers for specific IDs
    static std::array<char*, kNumStringPointers> _strings = []() {
        std::array<char*, kNumStringPointers> strings = {};

        // Assign pre-allocated buffers to specific string IDs
        strings[StringIds::buffer_337] = _buffer_337;
        strings[StringIds::buffer_338] = _buffer_338;
        strings[StringIds::buffer_1250] = _buffer_1250;
        strings[StringIds::preferred_currency_buffer] = _preferred_currency_buffer;
        strings[StringIds::buffer_1719] = _buffer_1719;
        strings[StringIds::buffer_2039] = _buffer_2039;
        strings[StringIds::buffer_2040] = _buffer_2040;

        return strings;
    }();

    static auto& rawUserStrings() { return getGameState().userStrings; }

    // 0x0049650E
    void reset()
    {
        for (auto* str : rawUserStrings())
        {
            *str = '\0';
        }
    }

    const char* getString(StringId id)
    {
        if (id >= _strings.size())
        {
            Diagnostics::Logging::error("Tried to access invalid string id: {}", id);
            return nullptr;
        }
        char* str = _strings[id];
        return str;
    }

    void setString(StringId id, std::string_view value)
    {
        auto* dst = _strings[id];
        std::memcpy(dst, value.data(), value.size());
        dst[value.size()] = '\0';
    }

    const char* swapString(StringId id, const char* src)
    {
        auto* dst = _strings[id];
        _strings[id] = const_cast<char*>(src);
        return dst;
    }

    // 0x00496522
    StringId userStringAllocate(char* str, bool mustBeUnique)
    {
        auto bestSlot = -1;
        for (auto i = 0u; i < Limits::kMaxUserStrings; ++i)
        {
            char* userStr = rawUserStrings()[i];
            if (*userStr == '\0')
            {
                bestSlot = i;
            }
            else if (mustBeUnique)
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

    const char* getUserString(StringId id)
    {
        return rawUserStrings()[id];
    }

    // 0x004965A6
    void emptyUserString(StringId stringId)
    {
        if (!isUserString(stringId))
        {
            return;
        }

        *rawUserStrings()[stringId - kUserStringsStart] = '\0';
    }

    bool isUserString(StringId stringId)
    {
        if (stringId < kUserStringsStart || stringId >= kUserStringsEnd)
        {
            return false;
        }
        return true;
    }

}
