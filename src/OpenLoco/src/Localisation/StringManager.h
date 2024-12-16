#pragma once

#include "Types.hpp"
#include <Engine/Limits.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#ifdef small
#error "small is defined, likely by windows.h"
#endif

namespace OpenLoco
{
    namespace StringIds
    {
        constexpr StringId null = 0xFFFF;
    }
}

namespace OpenLoco::StringManager
{
    constexpr uint8_t kUserStringSize = 32;
    constexpr uint16_t kUserStringsStart = 0x8000;
    constexpr uint16_t kUserStringsEnd = kUserStringsStart + Limits::kMaxUserStrings;

    constexpr uint16_t kMaxTownNames = 345;
    constexpr uint16_t kTownNamesStart = 0x9EE7;
    constexpr uint16_t kTownNamesEnd = kTownNamesStart + kMaxTownNames;

    void reset();
    void setString(StringId id, std::string_view value);
    const char* swapString(StringId id, const char* src);
    const char* getString(StringId id);

    StringId userStringAllocate(char* str, bool mustBeUnique);
    const char* getUserString(StringId id);
    void emptyUserString(StringId stringId);
    bool isUserString(StringId id);
}
