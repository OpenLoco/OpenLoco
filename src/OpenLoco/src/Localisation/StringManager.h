#pragma once

#include "Types.hpp"
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
        constexpr string_id null = 0xFFFF;
    }
}

namespace OpenLoco::StringManager
{
    const uint8_t kUserStringSize = 32;

    void reset();
    void setString(string_id id, std::string_view value);
    const char* swapString(string_id id, const char* src);
    const char* getString(string_id id);
    
    string_id userStringAllocate(char* str, uint8_t cl);
    const char* getUserString(string_id id);
    void emptyUserString(string_id stringId);
    bool isUserString(string_id id);
}
