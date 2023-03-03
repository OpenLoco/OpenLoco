#pragma once

#include "StringManager.h"
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
    namespace Font
    {
        constexpr int16_t m1 = -1;
        constexpr int16_t m2 = -2;

        constexpr int16_t medium_normal = 0;
        constexpr int16_t medium_bold = 224;
        constexpr int16_t small = 448;
        constexpr int16_t large = 672;
    }

    namespace ControlCodes
    {
        // Arguments (1 byte): uint8_t
        constexpr uint8_t moveX = 1;
        constexpr uint8_t adjustPalette = 2; // Not used
        // 3-4 Not used
        constexpr uint8_t oneArgBegin = moveX;
        constexpr uint8_t oneArgEnd = 4 + 1;

        // Arguments: none
        constexpr uint8_t newline = 5;
        constexpr uint8_t newlineSmaller = 6;

        namespace Font
        {
            constexpr uint8_t small = 7;
            constexpr uint8_t large = 8;
            constexpr uint8_t bold = 9;
            constexpr uint8_t regular = 10;
            constexpr uint8_t outline = 11;
            constexpr uint8_t outlineOff = 12;
        }

        constexpr uint8_t windowColour1 = 13;
        constexpr uint8_t windowColour2 = 14;
        constexpr uint8_t windowColour3 = 15;
        constexpr uint8_t windowColour4 = 16; // Not used
        constexpr uint8_t noArgBegin = newline;
        constexpr uint8_t noArgEnd = windowColour4 + 1;

        // Arguments (2 bytes): int8_t, int8_t
        constexpr uint8_t newlineXY = 17;
        // 18-22 Not used
        constexpr uint8_t twoArgBegin = newlineXY;
        constexpr uint8_t twoArgEnd = 22 + 1;

        // Arguments (4 bytes): uint32_t
        constexpr uint8_t inlineSpriteStr = 23;
        // 24-31 Not used
        constexpr uint8_t fourArgBegin = inlineSpriteStr;
        constexpr uint8_t fourArgEnd = 31 + 1;

        // Arguments in Args buffer
        // Note:
        // Pre formatString:
        //     ControlCodes valid args in args buffer.
        // Post formatString:
        //     ControlCodes are invalid
        //     inlineSpriteArgs replaced with inlineSpriteStr, arg is in string
        constexpr uint8_t int32_grouped = 123 + 0;
        constexpr uint8_t int32_ungrouped = 123 + 1;
        constexpr uint8_t int16_decimals = 123 + 2;
        constexpr uint8_t int32_decimals = 123 + 3;
        constexpr uint8_t int16_grouped = 123 + 4;
        constexpr uint8_t uint16_ungrouped = 123 + 5;
        constexpr uint8_t currency32 = 123 + 6;
        constexpr uint8_t currency48 = 123 + 7;
        constexpr uint8_t stringidArgs = 123 + 8;
        constexpr uint8_t stringidStr = 123 + 9;
        constexpr uint8_t string_ptr = 123 + 10;
        constexpr uint8_t date = 123 + 11;
        constexpr uint8_t velocity = 123 + 12;
        constexpr uint8_t pop16 = 123 + 13;
        constexpr uint8_t push16 = 123 + 14;
        constexpr uint8_t timeMS = 123 + 15;
        constexpr uint8_t timeHM = 123 + 16;
        constexpr uint8_t distance = 123 + 17;
        constexpr uint8_t height = 123 + 18;
        constexpr uint8_t power = 123 + 19;
        constexpr uint8_t inlineSpriteArgs = 123 + 20;

        namespace Colour
        {
            // Arguments: none
            constexpr uint8_t black = 144;
            constexpr uint8_t grey = 145;
            constexpr uint8_t white = 146;
            constexpr uint8_t red = 147;
            constexpr uint8_t green = 148;
            constexpr uint8_t yellow = 149;
            constexpr uint8_t topaz = 150;
            constexpr uint8_t celadon = 151;
            constexpr uint8_t babyBlue = 152;
            constexpr uint8_t paleLavender = 153;
            constexpr uint8_t paleGold = 154;
            constexpr uint8_t lightPink = 155;
            constexpr uint8_t pearlAqua = 156;
            constexpr uint8_t paleSilver = 157;
        }
    }

    namespace DateModifier
    {
        constexpr uint8_t dmy_full = 0;
        constexpr uint8_t my_full = 4;
        constexpr uint8_t my_abbr = 5;
        constexpr uint8_t raw_my_abbr = 8;
    }
}

namespace OpenLoco
{
    enum class MonthId : uint8_t;
}

namespace OpenLoco::StringManager
{
    char* formatString(char* buffer, string_id id, const void* args = nullptr);
    char* formatString(char* buffer, size_t bufferLen, string_id id, const void* args = nullptr);

    string_id isTownName(string_id stringId);
    string_id toTownName(string_id stringId);
    string_id fromTownName(string_id stringId);

    std::pair<string_id, string_id> monthToString(MonthId month);

    int32_t internalLengthToComma1DP(const int32_t length);
    size_t locoStrlen(const char* buffer);
    size_t locoStrlenS(const char* buffer, std::size_t size);
    char* locoStrcpy(char* dest, const char* src);
    char* locoStrcpyS(char* dest, std::size_t destSize, const char* src, std::size_t srcSize);
}
