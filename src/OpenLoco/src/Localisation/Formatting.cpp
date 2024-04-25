#include "Formatting.h"
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
#include <fmt/core.h>
#include <map>
#include <stdexcept>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::StringManager
{
    struct StringBuffer
    {
        using value_type = char;

        char* buffer;
        size_t offset;
        size_t maxLen;

    public:
        StringBuffer(value_type* buffer, size_t maxLen)
            : buffer(buffer)
            , offset(0)
            , maxLen(maxLen)
        {
        }

        void appendData(const void* data, size_t size)
        {
            if (offset + size >= maxLen)
            {
                throw std::overflow_error("String buffer overflow");
            }

            std::memcpy(buffer + offset, data, size);
            offset += size;
        }

        void append(value_type chr)
        {
            if (offset >= maxLen)
            {
                throw std::overflow_error("String buffer overflow");
            }

            buffer[offset] = chr;
            offset++;
        }

        void append(const char* input)
        {
            return append(input, 0xFFFFFFFFU);
        }

        void append(const char* input, size_t inputLen)
        {
            for (size_t i = 0; i < inputLen;)
            {
                auto ch = input[i];
                if (ch == '\0')
                {
                    break;
                }

                if (ch >= ControlCodes::oneArgBegin && ch < ControlCodes::oneArgEnd)
                {
                    append(ch);
                    input++;
                }
                else if (ch >= ControlCodes::twoArgBegin && ch < ControlCodes::twoArgEnd)
                {
                    if (offset + 2 >= maxLen)
                    {
                        throw std::overflow_error("String buffer overflow");
                    }
                    if (i + 2 > inputLen)
                    {
                        throw std::overflow_error("String buffer overflow");
                    }
                    std::memcpy(buffer + offset, input, 2);
                    offset += 2;
                    i += 2;
                }
                else if (ch >= ControlCodes::fourArgBegin && ch < ControlCodes::fourArgEnd)
                {
                    if (offset + 4 >= maxLen)
                    {
                        throw std::overflow_error("String buffer overflow");
                    }
                    if (i + 4 > inputLen)
                    {
                        throw std::overflow_error("String buffer overflow");
                    }
                    std::memcpy(buffer + offset, input, 4);
                    offset += 4;
                    i += 4;
                }
                else
                {
                    append(ch);
                    i++;
                }
            }
        }

        // std::back_inserter support.
        void push_back(value_type chr)
        {
            append(chr);
        }

        template<typename TLocale, typename... TArgs>
        void format(TLocale&& loc, fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            fmt::format_to(std::back_inserter(*this), loc, fmt, std::forward<TArgs>(args)...);
        }

        char* current() const
        {
            return buffer + offset;
        }

        void nullTerminate()
        {
            if (offset < maxLen)
                buffer[offset] = '\0';
            else
                buffer[maxLen - 1] = '\0';
        }

        void grow(size_t numChars)
        {
            if (offset + numChars >= maxLen)
            {
                throw std::overflow_error("String buffer overflow");
            }
            offset += numChars;
        }
    };

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

    static void formatString(StringBuffer& buffer, StringId id);

    // 0x00495F35
    static void formatInt32Grouped(int32_t value, StringBuffer& buffer)
    {
        fmt::format_to(std::back_inserter(buffer), std::locale(), "{:L}", value);
    }

    // 0x00495E2A
    static void formatInt32Ungrouped(int32_t value, StringBuffer& buffer)
    {
        fmt::format_to(std::back_inserter(buffer), "{}", value);
    }

    // 0x00496052
    static void formatInt48Grouped(uint64_t value, StringBuffer& buffer, uint8_t separator)
    {
        fmt::format_to(std::back_inserter(buffer), std::locale(), "{:L}", value * static_cast<uint64_t>(std::pow(10, separator)));
    }

    // 0x004963FC
    static void formatShortWithOneDecimal(int16_t value, StringBuffer& buffer)
    {
        fmt::format_to(std::back_inserter(buffer), std::locale(), "{:L}", value / 10);
        fmt::format_to(std::back_inserter(buffer), ".{}", value % 10);
    }

    // 0x004962F1
    static void formatIntWithTwoDecimals(int32_t value, StringBuffer& buffer)
    {
        fmt::format_to(std::back_inserter(buffer), std::locale(), "{:L}", value / 100);
        fmt::format_to(std::back_inserter(buffer), ".{}", value % 100);
    }

    // 0x00495D09
    static void formatDateDMYFull(uint32_t totalDays, StringBuffer& buffer)
    {
        auto date = calcDate(totalDays);

        StringId dayString = kDayToString.find(date.day)->second;
        formatString(buffer, dayString);

        buffer.append(' ');

        StringId monthString = monthToString(date.month).second;
        formatString(buffer, monthString);

        buffer.append(' ');

        formatInt32Ungrouped(date.year, buffer);
    }

    // 0x00495D77
    static void formatDateMYFull(uint32_t totalDays, StringBuffer& buffer)
    {
        auto date = calcDate(totalDays);

        StringId monthString = monthToString(date.month).second;
        formatString(buffer, monthString);

        buffer.append(' ');

        formatInt32Ungrouped(date.year, buffer);
    }

    // 0x00495DC7
    static void formatDateMYAbbrev(uint32_t totalDays, StringBuffer& buffer)
    {
        auto date = calcDate(totalDays);

        StringId monthString = monthToString(date.month).second;
        formatString(buffer, monthString);

        buffer.append(' ');

        formatInt32Ungrouped(date.year, buffer);
    }

    // 0x00495DC7
    static void formatRawDateMYAbbrev(uint32_t totalDays, StringBuffer& buffer)
    {
        auto month = static_cast<MonthId>(totalDays % 12);
        StringId monthString = monthToString(month).first;
        formatString(buffer, monthString);

        buffer.append(' ');

        formatInt32Ungrouped(totalDays / 12, buffer);
    }

    static void formatStringPart(StringBuffer& buffer, const char* sourceStr);

    static void formatCurrency(int64_t value, StringBuffer& buffer)
    {
        if (value < 0)
        {
            buffer.append('-');
            value = -value;
        }

        CurrencyObject* currency = ObjectManager::get<CurrencyObject>();

        int64_t localisedValue = value * (1ULL << currency->factor);

        const char* prefixSymbol = getString(currency->prefixSymbol);
        formatStringPart(buffer, prefixSymbol);

        formatInt48Grouped(localisedValue, buffer, currency->separator);

        const char* suffixSymbol = getString(currency->suffixSymbol);
        formatStringPart(buffer, suffixSymbol);
    }

    static void formatStringImpl(StringBuffer& buffer, StringId id, FormatArgumentsView& args);

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

    static void formatStringPart(StringBuffer& buffer, const char* sourceStr, FormatArgumentsView& args)
    {
        while (true)
        {
            uint8_t ch = *sourceStr;

            if (ch == 0)
            {
                return;
            }
            else if (ch <= 4)
            {
                buffer.appendData(sourceStr, 2);
                sourceStr += 2;
            }
            else if (ch <= 16)
            {
                buffer.appendData(sourceStr, 1);
                sourceStr += 1;
            }
            else if (ch <= 22)
            {
                buffer.appendData(sourceStr, 3);
                sourceStr += 3;
            }
            else if (ch <= 0x1F)
            {
                buffer.appendData(sourceStr, 5);
                sourceStr += 5;
            }
            else if (ch < 0x7B || ch >= 0x90)
            {
                buffer.appendData(sourceStr, 1);
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
                        formatInt32Grouped(value, buffer);
                        break;
                    }

                    case ControlCodes::int32_ungrouped:
                    {
                        int32_t value = args.pop<int32_t>();
                        formatInt32Ungrouped(value, buffer);
                        break;
                    }

                    case ControlCodes::int16_decimals:
                    {
                        int16_t value = args.pop<int16_t>();
                        formatShortWithOneDecimal(value, buffer);
                        break;
                    }

                    case ControlCodes::int32_decimals:
                    {
                        int32_t value = args.pop<int32_t>();
                        formatIntWithTwoDecimals(value, buffer);
                        break;
                    }

                    case ControlCodes::int16_grouped:
                    {
                        int16_t value = args.pop<int16_t>();
                        formatInt32Grouped(value, buffer);
                        break;
                    }

                    case ControlCodes::uint16_ungrouped:
                    {
                        int32_t value = args.pop<uint16_t>();
                        formatInt32Ungrouped(value, buffer);
                        break;
                    }

                    case ControlCodes::currency32:
                    {
                        int32_t value = args.pop<uint32_t>();
                        formatCurrency(value, buffer);
                        break;
                    }

                    case ControlCodes::currency48:
                    {
                        uint32_t valueLow = args.pop<uint32_t>();
                        int32_t valueHigh = args.pop<int16_t>();
                        int64_t value = (valueHigh * (1ULL << 32)) | valueLow;
                        formatCurrency(value, buffer);
                        break;
                    }

                    case ControlCodes::stringidArgs:
                    {
                        StringId id = args.pop<StringId>();
                        formatStringImpl(buffer, id, args);
                        break;
                    }

                    case ControlCodes::stringidStr:
                    {
                        StringId id = *(StringId*)sourceStr;
                        sourceStr += 2;
                        formatStringImpl(buffer, id, args);
                        break;
                    }

                    case ControlCodes::string_ptr:
                    {
                        const char* str = args.pop<const char*>();
                        buffer.append(str);
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
                                formatDateDMYFull(totalDays, buffer);
                                break;

                            case DateModifier::my_full:
                                formatDateMYFull(totalDays, buffer);
                                break;

                            case DateModifier::my_abbr:
                                formatDateMYAbbrev(totalDays, buffer);
                                break;

                            case DateModifier::raw_my_abbr:
                                formatRawDateMYAbbrev(totalDays, buffer);
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

                        formatInt32Grouped(value, buffer);
                        buffer.append(unit);

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

                        formatInt32Grouped(value, buffer);
                        buffer.append(unit);

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

                        formatInt32Grouped(value, buffer);
                        buffer.append(unit);

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

                        formatInt32Grouped(value, buffer);
                        buffer.append(unit);

                        break;
                    }

                    case ControlCodes::inlineSpriteArgs:
                    {
                        uint32_t value = args.pop<uint32_t>();

                        buffer.append(static_cast<char>(ControlCodes::inlineSpriteStr));
                        buffer.appendData(&value, sizeof(value));

                        break;
                    }
                }
            }
        }
    }

    static void formatStringPart(StringBuffer& buffer, const char* sourceStr)
    {
        auto wrapped = FormatArgumentsView();
        formatStringPart(buffer, sourceStr, wrapped);
    }

    // 0x004958C6
    static void formatStringImpl(StringBuffer& buffer, StringId id, FormatArgumentsView& args)
    {
        if (id < kUserStringsStart)
        {
            const char* sourceStr = getString(id);
            if (sourceStr == nullptr)
            {
                buffer.format(std::locale(), "(missing string id: {})", id);
                Logging::warn("formatString: nullptr for string id: {}", id);
                return;
            }

            formatStringPart(buffer, sourceStr, args);
        }
        else if (id < kUserStringsEnd)
        {
            id -= kUserStringsStart;
            args.skip<uint16_t>();
            const char* sourceStr = getUserString(id);

            buffer.append(sourceStr, kUserStringSize);
        }
        else if (id < kTownNamesEnd)
        {
            id -= kTownNamesStart;

            const auto townId = TownId(args.pop<uint16_t>());
            auto town = TownManager::get(townId);

            // TODO: Clean this up once we have only FormatArguments.
            FormatArgumentsBuffer buf;
            auto fmt = FormatArguments(buf);
            fmt.push(town->name);

            auto fmtView = FormatArgumentsView(fmt);
            formatStringImpl(buffer, id, fmtView);
        }
        else if (id == kTownNamesEnd)
        {
            const auto townId = TownId(args.pop<uint16_t>());
            auto town = TownManager::get(townId);
            formatString(buffer, town->name);
        }
        else
        {
            buffer.format(std::locale(), "(invalid string id: {})", id);
            Logging::warn("formatString: invalid string id: {}", id);
        }
    }

    static void formatString(StringBuffer& buffer, StringId id)
    {
        auto args = FormatArgumentsView{};
        formatStringImpl(buffer, id, args);
    }

    // TODO: Remove unsafe variant.
    char* formatString(char* buffer, StringId id)
    {
        return formatString(buffer, 0xFFFFFFFFU, id);
    }

    char* formatString(char* buffer, size_t bufferLen, StringId id)
    {
        auto wrapped = FormatArgumentsView{};
        auto buf = StringBuffer(buffer, bufferLen);

        formatStringImpl(buf, id, wrapped);

        buf.nullTerminate();
        return buf.current();
    }

    // TODO: Remove this unsafe variant.
    char* formatString(char* buffer, StringId id, const void* args)
    {
        // Just to avoid code duplication we pass an arbitrary size.
        return formatString(buffer, 0xFFFFFFFFU, id, args);
    }

    char* formatString(char* buffer, size_t bufferLen, StringId id, const void* args)
    {
        auto wrapped = FormatArgumentsView(args);
        auto buf = StringBuffer(buffer, bufferLen);

        formatStringImpl(buf, id, wrapped);

        buf.nullTerminate();
        return buf.current();
    }

    char* formatString(char* buffer, StringId id, FormatArgumentsView args)
    {
        return formatString(buffer, 0xFFFFFFFFU, id, args);
    }

    char* formatString(char* buffer, size_t bufferLen, StringId id, FormatArgumentsView args)
    {
        auto buf = StringBuffer(buffer, bufferLen);

        formatStringImpl(buf, id, args);

        buf.nullTerminate();
        return buf.current();
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
