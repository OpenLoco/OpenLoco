#include "Formatting.h"
#include "ArgsWrapper.hpp"
#include "Config.h"
#include "Date.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Logging.h"
#include "Objects/CurrencyObject.h"
#include "Objects/ObjectManager.h"
#include "StringIds.h"
#include "StringManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Interop/Interop.hpp>

#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::StringManager
{
    static const std::map<int32_t, StringId> kDayToString = {
        { 1, StringIds::day_1st },
        { 2, StringIds::day_2nd },
        { 3, StringIds::day_3rd },
        { 4, StringIds::day_4th },
        { 5, StringIds::day_5th },
        { 6, StringIds::day_6th },
        { 7, StringIds::day_7th },
        { 8, StringIds::day_8th },
        { 9, StringIds::day_9th },
        { 10, StringIds::day_10th },
        { 11, StringIds::day_11th },
        { 12, StringIds::day_12th },
        { 13, StringIds::day_13th },
        { 14, StringIds::day_14th },
        { 15, StringIds::day_15th },
        { 16, StringIds::day_16th },
        { 17, StringIds::day_17th },
        { 18, StringIds::day_18th },
        { 19, StringIds::day_19th },
        { 20, StringIds::day_20th },
        { 21, StringIds::day_21st },
        { 22, StringIds::day_22nd },
        { 23, StringIds::day_23rd },
        { 24, StringIds::day_24th },
        { 25, StringIds::day_25th },
        { 26, StringIds::day_26th },
        { 27, StringIds::day_27th },
        { 28, StringIds::day_28th },
        { 29, StringIds::day_29th },
        { 30, StringIds::day_30th },
        { 31, StringIds::day_31st },
    };

    static const std::map<MonthId, std::pair<StringId, StringId>> kMonthToStringMap = {
        { MonthId::january, { StringIds::month_short_january, StringIds::month_long_january } },
        { MonthId::february, { StringIds::month_short_february, StringIds::month_long_february } },
        { MonthId::march, { StringIds::month_short_march, StringIds::month_long_march } },
        { MonthId::april, { StringIds::month_short_april, StringIds::month_long_april } },
        { MonthId::may, { StringIds::month_short_may, StringIds::month_long_may } },
        { MonthId::june, { StringIds::month_short_june, StringIds::month_long_june } },
        { MonthId::july, { StringIds::month_short_july, StringIds::month_long_july } },
        { MonthId::august, { StringIds::month_short_august, StringIds::month_long_august } },
        { MonthId::september, { StringIds::month_short_september, StringIds::month_long_september } },
        { MonthId::october, { StringIds::month_short_october, StringIds::month_long_october } },
        { MonthId::november, { StringIds::month_short_november, StringIds::month_long_november } },
        { MonthId::december, { StringIds::month_short_december, StringIds::month_long_december } },
    };

    std::pair<StringId, StringId> monthToString(MonthId month)
    {
        return kMonthToStringMap.find(month)->second;
    }

    template<typename T>
    static char* formatNumberToString(T value, char* buffer, uint8_t numDecimals, bool useGroupedNumbers)
    {
        if (value < 0)
        {
            value = -value;
            *buffer++ = '-';
        }

        // Build up the formatted number in reverse
        std::string number{};
        bool passedDecimals = numDecimals == 0;
        auto groupLength = 0;
        while (value > 0)
        {
            if (!passedDecimals && groupLength == numDecimals)
            {
                number += '.';
                passedDecimals = true;
                groupLength = 0;
            }

            if (useGroupedNumbers && groupLength == 3)
            {
                number += ',';
                groupLength = 0;
            }

            number += '0' + (value % 10);
            value /= 10;
            groupLength++;
        }

        // Reverse the number buffer
        std::reverse(number.begin(), number.end());

        // Copy number buffer to dest buffer
        std::strncpy(buffer, number.c_str(), number.size());

        return buffer + number.size();
    }

    // 0x00495F35
    static char* formatInt32Grouped(int32_t value, char* buffer)
    {
        return formatNumberToString(value, buffer, 0, true);
    }

    // 0x00495E2A
    static char* formatInt32Ungrouped(int32_t value, char* buffer)
    {
        return formatNumberToString(value, buffer, 0, false);
    }

    // 0x00496052
    static char* formatInt48Grouped(int64_t value, char* buffer, uint8_t separator)
    {
        value *= static_cast<uint64_t>(std::pow(10, separator));
        return formatNumberToString(value, buffer, 0, true);
    }

    // 0x004963FC
    static char* formatShortWithSingleDecimal(int16_t value, char* buffer)
    {
        return formatNumberToString(value, buffer, 1, true);
    }

    // 0x004962F1
    static char* formatIntWithTwoDecimals(int32_t value, char* buffer)
    {
        return formatNumberToString(value, buffer, 2, true);
    }

    // 0x00495D09
    static char* formatDateDMYFull(uint32_t totalDays, char* buffer)
    {
        auto date = calcDate(totalDays);

        StringId dayString = kDayToString.find(date.day)->second;
        buffer = formatString(buffer, dayString, nullptr);

        *buffer = ' ';
        buffer++;

        StringId monthString = monthToString(date.month).second;
        buffer = formatString(buffer, monthString, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495D77
    static char* formatDateMYFull(uint32_t totalDays, char* buffer)
    {
        auto date = calcDate(totalDays);

        StringId monthString = monthToString(date.month).second;
        buffer = formatString(buffer, monthString, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495DC7
    static char* formatDateMYAbbrev(uint32_t totalDays, char* buffer)
    {
        auto date = calcDate(totalDays);

        StringId monthString = monthToString(date.month).second;
        buffer = formatString(buffer, monthString, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495DC7
    static char* formatRawDateMYAbbrev(uint32_t totalDays, char* buffer)
    {
        auto month = static_cast<MonthId>(totalDays % 12);
        StringId monthString = monthToString(month).first;
        buffer = formatString(buffer, monthString, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(totalDays / 12, buffer);

        return buffer;
    }

    static char* formatStringPart(char* buffer, const char* sourceStr, void* args);

    static char* formatCurrency(int64_t value, char* buffer)
    {
        if (value < 0)
        {
            *buffer = '-';
            buffer++;
            value = -value;
        }

        CurrencyObject* currency = ObjectManager::get<CurrencyObject>();

        int64_t localisedValue = value * (1ULL << currency->factor);

        const char* prefixSymbol = getString(currency->prefixSymbol);
        buffer = formatStringPart(buffer, prefixSymbol, nullptr);

        buffer = formatInt48Grouped(localisedValue, buffer, currency->separator);

        const char* suffixSymbol = getString(currency->suffixSymbol);
        buffer = formatStringPart(buffer, suffixSymbol, nullptr);

        return buffer;
    }

    static char* formatString(char* buffer, StringId id, ArgsWrapper& args);

    constexpr uint32_t hpTokW(uint32_t hp)
    {
        // vanilla conversion ratio is 764 / 1024, or 0.74609375
        return hp * 0.74609375;
    }
    static_assert(0 == hpTokW(0));
    static_assert(0 == hpTokW(1));
    static_assert(1 == hpTokW(2));
    static_assert(920 == hpTokW(1234));
    static_assert(48895 == hpTokW(65535));

    // Loco string argument safe strlen
    size_t locoStrlen(const char* buffer)
    {
        if (buffer == nullptr)
        {
            return 0;
        }
        auto* ptr = buffer;
        while (*ptr != '\0')
        {
            const auto ch = *ptr++;
            if (ch >= ControlCodes::oneArgBegin && ch < ControlCodes::oneArgEnd)
            {
                ptr++;
            }
            else if (ch >= ControlCodes::twoArgBegin && ch < ControlCodes::twoArgEnd)
            {
                ptr += 2;
            }
            else if (ch >= ControlCodes::fourArgBegin && ch < ControlCodes::fourArgEnd)
            {
                ptr += 4;
            }
        }
        return ptr - buffer;
    }

    // Loco string argument safe strlen_s
    size_t locoStrlenS(const char* buffer, std::size_t size)
    {
        if (buffer == nullptr || size == 0)
        {
            return 0;
        }
        auto* ptr = buffer;
        std::size_t i = 0;
        while (*ptr != '\0' && i < size)
        {
            i++;
            const auto ch = *ptr++;
            if (ch >= ControlCodes::oneArgBegin && ch < ControlCodes::oneArgEnd)
            {
                i += 1;
                if (i > size)
                {
                    return i - 2; // Ignore the Control Code and Arg
                }
                ptr++;
            }
            else if (ch >= ControlCodes::twoArgBegin && ch < ControlCodes::twoArgEnd)
            {
                i += 2;
                if (i > size)
                {
                    return i - 3; // Ignore the Control Code and Arg
                }
                ptr += 2;
            }
            else if (ch >= ControlCodes::fourArgBegin && ch < ControlCodes::fourArgEnd)
            {
                i += 4;
                if (i > size)
                {
                    return i - 5; // Ignore the Control Code and Arg
                }
                ptr += 4;
            }
        }
        return i;
    }

    char* locoStrcpy(char* dest, const char* src)
    {
        if (src == nullptr)
        {
            return dest;
        }

        std::copy(src, src + locoStrlen(src) + 1, dest);

        return dest;
    }

    char* locoStrcpyS(char* dest, std::size_t destSize, const char* src, std::size_t srcSize)
    {
        if (src == nullptr)
        {
            return dest;
        }

        auto size = locoStrlenS(src, std::min(destSize, srcSize + 1));

        std::copy(src, src + size, dest);
        dest[size] = '\0';

        return dest;
    }

    static char* formatStringPart(char* buffer, const char* sourceStr, ArgsWrapper& args)
    {
        while (true)
        {
            uint8_t ch = *sourceStr;

            if (ch == 0)
            {
                *buffer = '\0';
                return buffer;
            }
            else if (ch <= 4)
            {
                std::memcpy(buffer, sourceStr, 2);
                buffer += 2;
                sourceStr += 2;
            }
            else if (ch <= 16)
            {
                std::memcpy(buffer, sourceStr, 1);
                buffer += 1;
                sourceStr += 1;
            }
            else if (ch <= 22)
            {
                std::memcpy(buffer, sourceStr, 3);
                buffer += 3;
                sourceStr += 3;
            }
            else if (ch <= 0x1F)
            {
                std::memcpy(buffer, sourceStr, 5);
                buffer += 5;
                sourceStr += 5;
            }
            else if (ch < 0x7B || ch >= 0x90)
            {
                std::memcpy(buffer, sourceStr, 1);
                buffer += 1;
                sourceStr += 1;
            }
            else
            {
                sourceStr++;

                switch (ch)
                {
                    case ControlCodes::int32_grouped:
                    {
                        int32_t value = args.pop<int32_t>();
                        buffer = formatInt32Grouped(value, buffer);
                        break;
                    }

                    case ControlCodes::int32_ungrouped:
                    {
                        int32_t value = args.pop<int32_t>();
                        buffer = formatInt32Ungrouped(value, buffer);
                        break;
                    }

                    case ControlCodes::int16_decimals:
                    {
                        int16_t value = args.pop<int16_t>();
                        buffer = formatShortWithSingleDecimal(value, buffer);
                        break;
                    }

                    case ControlCodes::int32_decimals:
                    {
                        int32_t value = args.pop<int32_t>();
                        buffer = formatIntWithTwoDecimals(value, buffer);
                        break;
                    }

                    case ControlCodes::int16_grouped:
                    {
                        int16_t value = args.pop<int16_t>();
                        buffer = formatInt32Grouped(value, buffer);
                        break;
                    }

                    case ControlCodes::uint16_ungrouped:
                    {
                        int32_t value = args.pop<uint16_t>();
                        buffer = formatInt32Ungrouped(value, buffer);
                        break;
                    }

                    case ControlCodes::currency32:
                    {
                        int32_t value = args.pop<uint32_t>();
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case ControlCodes::currency48:
                    {
                        uint32_t valueLow = args.pop<uint32_t>();
                        int32_t valueHigh = args.pop<int16_t>();
                        int64_t value = (valueHigh * (1ULL << 32)) | valueLow;
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case ControlCodes::stringidArgs:
                    {
                        StringId id = args.pop<StringId>();
                        buffer = formatString(buffer, id, args);
                        break;
                    }

                    case ControlCodes::stringidStr:
                    {
                        StringId id = *(StringId*)sourceStr;
                        sourceStr += 2;
                        buffer = formatString(buffer, id, args);
                        break;
                    }

                    case ControlCodes::string_ptr:
                    {
                        const char* str = args.pop<const char*>();
                        locoStrcpy(buffer, str);
                        buffer += locoStrlen(str);
                        break;
                    }

                    case ControlCodes::date:
                    {
                        char modifier = *sourceStr;
                        uint32_t totalDays = args.pop<uint32_t>();
                        sourceStr++;

                        switch (modifier)
                        {
                            case DateModifier::dmy_full:
                                buffer = formatDateDMYFull(totalDays, buffer);
                                break;

                            case DateModifier::my_full:
                                buffer = formatDateMYFull(totalDays, buffer);
                                break;

                            case DateModifier::my_abbr:
                                buffer = formatDateMYAbbrev(totalDays, buffer);
                                break;

                            case DateModifier::raw_my_abbr:
                                buffer = formatRawDateMYAbbrev(totalDays, buffer);
                                break;

                            default:
                                throw Exception::OutOfRange("formatString: unexpected modifier: " + std::to_string((uint8_t)modifier));
                        }

                        break;
                    }

                    case ControlCodes::velocity:
                    {
                        auto measurementFormat = Config::get().old.measurementFormat;

                        int32_t value = args.pop<int16_t>();

                        const char* unit;
                        if (measurementFormat == Config::MeasurementFormat::imperial)
                        {
                            unit = getString(StringIds::unit_mph);
                        }
                        else
                        {
                            unit = getString(StringIds::unit_kmh);
                            value = std::round(value * 1.609375);
                        }

                        buffer = formatInt32Grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case ControlCodes::pop16:
                        args.skip<uint16_t>();
                        break;

                    case ControlCodes::push16:
                        args.push<uint16_t>();
                        break;

                    case ControlCodes::timeMS:
                        throw Exception::RuntimeError("Unimplemented format string: 15");

                    case ControlCodes::timeHM:
                        throw Exception::RuntimeError("Unimplemented format string: 16");

                    case ControlCodes::distance:
                    {
                        uint32_t value = args.pop<uint16_t>();
                        auto measurementFormat = Config::get().old.measurementFormat;

                        const char* unit;
                        if (measurementFormat == Config::MeasurementFormat::imperial)
                        {
                            unit = getString(StringIds::unit_ft);
                            value = std::round(value * 3.28125);
                        }
                        else
                        {
                            unit = getString(StringIds::unit_m);
                        }

                        buffer = formatInt32Grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case ControlCodes::height:
                    {
                        int32_t value = args.pop<int16_t>();

                        bool showHeightAsUnits = (Config::get().hasFlags(Config::Flags::showHeightAsUnits));
                        auto measurementFormat = Config::get().old.measurementFormat;
                        const char* unit;

                        if (showHeightAsUnits)
                        {
                            unit = getString(StringIds::unit_units);
                        }
                        else if (measurementFormat == Config::MeasurementFormat::imperial)
                        {
                            unit = getString(StringIds::unit_ft);
                            value *= 16;
                        }
                        else
                        {
                            unit = getString(StringIds::unit_m);
                            value *= 5;
                        }

                        buffer = formatInt32Grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case ControlCodes::power:
                    {
                        uint32_t value = args.pop<uint16_t>();
                        auto measurementFormat = Config::get().old.measurementFormat;

                        const char* unit;
                        if (measurementFormat == Config::MeasurementFormat::imperial)
                        {
                            unit = getString(StringIds::unit_hp);
                        }
                        else
                        {
                            unit = getString(StringIds::unit_kW);
                            value = hpTokW(value);
                        }

                        buffer = formatInt32Grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case ControlCodes::inlineSpriteArgs:
                    {
                        *buffer = ControlCodes::inlineSpriteStr;
                        uint32_t value = args.pop<uint32_t>();
                        uint32_t* spritePtr = (uint32_t*)(buffer + 1);
                        *spritePtr = value;
                        buffer += 5;

                        break;
                    }
                }
            }
        }
    }

    static char* formatStringPart(char* buffer, const char* sourceStr, void* args)
    {
        auto wrapped = ArgsWrapper(args);
        return formatStringPart(buffer, sourceStr, wrapped);
    }

    // 0x004958C6
    static char* formatString(char* buffer, StringId id, ArgsWrapper& args)
    {
        if (id < kUserStringsStart)
        {
            const char* sourceStr = getString(id);
            if (sourceStr == nullptr)
            {
                sprintf(buffer, "(missing string id: %d)", id);
                Logging::warn("formatString: nullptr for string id: {}", id);
                buffer += strlen(buffer);
                return buffer;
            }

            buffer = formatStringPart(buffer, sourceStr, args);
            assert(*buffer == '\0');
            return buffer;
        }
        else if (id < kUserStringsEnd)
        {
            id -= kUserStringsStart;
            args.skip<uint16_t>();
            const char* sourceStr = getUserString(id);

            // !!! TODO: original code is prone to buffer overflow.
            buffer = strncpy(buffer, sourceStr, kUserStringSize);
            buffer += locoStrlen(sourceStr);
            *buffer = '\0';

            return buffer;
        }
        else if (id < kTownNamesEnd)
        {
            id -= kTownNamesStart;
            const auto townId = TownId(args.pop<uint16_t>());
            auto town = TownManager::get(townId);
            void* town_name = (void*)&town->name;
            return formatString(buffer, id, town_name);
        }
        else if (id == kTownNamesEnd)
        {
            const auto townId = TownId(args.pop<uint16_t>());
            auto town = TownManager::get(townId);
            return formatString(buffer, town->name, nullptr);
        }
        else
        {
            sprintf(buffer, "(invalid string id: %d)", id);
            Logging::warn("formatString: invalid string id: {}", id);
            buffer += strlen(buffer);
            return buffer;
        }
    }

    char* formatString(char* buffer, StringId id, const void* args)
    {
        auto wrapped = ArgsWrapper(args);
        return formatString(buffer, id, wrapped);
    }

    char* formatString(char* buffer, [[maybe_unused]] size_t bufferLen, StringId id, const void* args)
    {
        return formatString(buffer, id, args);
    }

    StringId isTownName(StringId stringId)
    {
        return stringId >= kTownNamesStart && stringId < kTownNamesEnd;
    }

    StringId toTownName(StringId stringId)
    {
        assert(stringId < kTownNamesStart && stringId + kTownNamesStart < kTownNamesEnd);
        return StringId(kTownNamesStart + stringId);
    }

    StringId fromTownName(StringId stringId)
    {
        assert(isTownName(stringId));
        return StringId(stringId - kTownNamesStart);
    }

    int32_t internalLengthToComma1DP(const int32_t length)
    {
        return length * 100 / 4 / 32;
    }
}
