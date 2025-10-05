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
    // 0x005183FC
    // 0x2000 lang strings, 0x10 temp obj strings, 0x45E loaded obj strings
    constexpr size_t kNumStringPointers = 0x246E; // 9326 strings

    // Pre-allocated buffers for string IDs that need mutable storage
    static constexpr size_t kStringBufferSize = 512;
    static std::array<char, kStringBufferSize> _buffer_337 = {};
    static std::array<char, kStringBufferSize> _buffer_338 = {};
    static std::array<char, kStringBufferSize> _buffer_1250 = {};
    static std::array<char, kStringBufferSize> _buffer_1506 = {}; // preferred_currency_buffer
    static std::array<char, kStringBufferSize> _buffer_1719 = {};
    static std::array<char, kStringBufferSize> _buffer_2039 = {};
    static std::array<char, kStringBufferSize> _buffer_2040 = {};

    // Initialize string pointer array with buffers for specific IDs
    static std::array<char*, kNumStringPointers> _strings = []() {
        std::array<char*, kNumStringPointers> strings = {};
        // Assign pre-allocated buffers to specific string IDs
        strings[337] = _buffer_337.data();
        strings[338] = _buffer_338.data();
        strings[1250] = _buffer_1250.data();
        strings[1506] = _buffer_1506.data();
        strings[1719] = _buffer_1719.data();
        strings[2039] = _buffer_2039.data();
        strings[2040] = _buffer_2040.data();
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
