#include "StringManager.h"
#include "../Config.h"
#include "../Console.h"
#include "../Date.h"
#include "../GameCommands/GameCommands.h"
#include "../GameState.h"
#include "../Interop/Interop.hpp"
#include "../Objects/CurrencyObject.h"
#include "../Objects/ObjectManager.h"
#include "../TownManager.h"
#include "ArgsWrapper.hpp"
#include "StringIds.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>

using namespace OpenLoco::Interop;

namespace OpenLoco::StringManager
{
    const uint8_t kUserStringSize = 32;
    const uint16_t kUserStringsStart = 0x8000;
    const uint16_t kUserStringsEnd = kUserStringsStart + Limits::kMaxUserStrings;

    const uint16_t kMaxTownNames = 345;
    const uint16_t kTownNamesStart = 0x9EE7;
    const uint16_t kTownNamesEnd = kTownNamesStart + kMaxTownNames;

    static loco_global<char* [0xFFFF], 0x005183FC> _strings;

    static auto& rawUserStrings() { return getGameState().userStrings; }

    static std::map<int32_t, string_id> day_to_string = {
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

    static std::map<MonthId, std::pair<string_id, string_id>> month_to_string = {
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

    std::pair<string_id, string_id> monthToString(MonthId month)
    {
        return month_to_string[month];
    }

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

    static char* formatInt32Grouped(int32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = X86Pointer(buffer);

        call(0x00495F35, regs);
        return X86Pointer<char>(regs.edi);
    }

    static char* formatInt32Ungrouped(int32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = X86Pointer(buffer);

        call(0x495E2A, regs);
        return X86Pointer<char>(regs.edi);
    }

    static char* formatInt48Grouped(uint64_t value, char* buffer, uint8_t separator)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edx = (uint32_t)(value / (1ULL << 32)); // regs.dx = (uint16_t)(value >> 32);
        regs.edi = X86Pointer(buffer);
        regs.ebx = (uint32_t)separator;

        call(0x496052, regs);
        return X86Pointer<char>(regs.edi);
    }

    static char* formatShortWithDecimals(int16_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = X86Pointer(buffer);

        call(0x4963FC, regs);
        return X86Pointer<char>(regs.edi);
    }

    static char* formatIntWithDecimals(int32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = X86Pointer(buffer);

        call(0x4962F1, regs);
        return X86Pointer<char>(regs.edi);
    }

    // 0x00495D09
    static char* formatDateDMYFull(uint32_t totalDays, char* buffer)
    {
        auto date = calcDate(totalDays);

        string_id day_string = day_to_string[date.day];
        buffer = formatString(buffer, day_string, nullptr);

        *buffer = ' ';
        buffer++;

        string_id month_string = month_to_string[date.month].second;
        buffer = formatString(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495D77
    static char* formatDateMYFull(uint32_t totalDays, char* buffer)
    {
        auto date = calcDate(totalDays);

        string_id month_string = month_to_string[date.month].second;
        buffer = formatString(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495DC7
    static char* formatDateMYAbbrev(uint32_t totalDays, char* buffer)
    {
        auto date = calcDate(totalDays);

        string_id month_string = month_to_string[date.month].second;
        buffer = formatString(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = formatInt32Ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495DC7
    static char* formatRawDateMYAbbrev(uint32_t totalDays, char* buffer)
    {
        auto month = static_cast<MonthId>(totalDays % 12);
        string_id month_string = month_to_string[month].first;
        buffer = formatString(buffer, month_string, nullptr);

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

        int64_t localised_value = value * (1ULL << currency->factor);

        const char* prefix_symbol = getString(currency->prefix_symbol);
        buffer = formatStringPart(buffer, prefix_symbol, nullptr);

        buffer = formatInt48Grouped(localised_value, buffer, currency->separator);

        const char* suffix_symbol = getString(currency->suffix_symbol);
        buffer = formatStringPart(buffer, suffix_symbol, nullptr);

        return buffer;
    }

    static char* formatString(char* buffer, string_id id, ArgsWrapper& args);

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
                        buffer = formatShortWithDecimals(value, buffer);
                        break;
                    }

                    case ControlCodes::int32_decimals:
                    {
                        int32_t value = args.pop<int32_t>();
                        buffer = formatIntWithDecimals(value, buffer);
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
                        uint32_t value_low = args.pop<uint32_t>();
                        int32_t value_high = args.pop<int16_t>();
                        int64_t value = (value_high * (1ULL << 32)) | value_low;
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case ControlCodes::stringid_args:
                    {
                        string_id id = args.pop<string_id>();
                        buffer = formatString(buffer, id, args);
                        break;
                    }

                    case ControlCodes::stringid_str:
                    {
                        string_id id = *(string_id*)sourceStr;
                        sourceStr += 2;
                        buffer = formatString(buffer, id, args);
                        break;
                    }

                    case ControlCodes::string_ptr:
                    {
                        const char* str = args.pop<const char*>();
                        strcpy(buffer, str);
                        buffer += strlen(str);
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
                                throw std::out_of_range("formatString: unexpected modifier: " + std::to_string((uint8_t)modifier));
                        }

                        break;
                    }

                    case ControlCodes::velocity:
                    {
                        auto measurement_format = Config::get().measurementFormat;

                        int32_t value = args.pop<int16_t>();

                        const char* unit;
                        if (measurement_format == Config::MeasurementFormat::imperial)
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
                        throw std::runtime_error("Unimplemented format string: 15");

                    case ControlCodes::timeHM:
                        throw std::runtime_error("Unimplemented format string: 16");

                    case ControlCodes::distance:
                    {
                        uint32_t value = args.pop<uint16_t>();
                        auto measurement_format = Config::get().measurementFormat;

                        const char* unit;
                        if (measurement_format == Config::MeasurementFormat::imperial)
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

                        bool showHeightAsUnits = Config::get().flags & Config::Flags::showHeightAsUnits;
                        auto measurement_format = Config::get().measurementFormat;
                        const char* unit;

                        if (showHeightAsUnits)
                        {
                            unit = getString(StringIds::unit_units);
                        }
                        else if (measurement_format == Config::MeasurementFormat::imperial)
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
                        auto measurement_format = Config::get().measurementFormat;

                        const char* unit;
                        if (measurement_format == Config::MeasurementFormat::imperial)
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

                    case ControlCodes::inline_sprite_args:
                    {
                        *buffer = ControlCodes::inline_sprite_str;
                        uint32_t value = args.pop<uint32_t>();
                        uint32_t* sprite_ptr = (uint32_t*)(buffer + 1);
                        *sprite_ptr = value;
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
    static char* formatString(char* buffer, string_id id, ArgsWrapper& args)
    {
        if (id < kUserStringsStart)
        {
            const char* sourceStr = getString(id);
            if (sourceStr == nullptr)
            {
                sprintf(buffer, "(missing string id: %d)", id);
                Console::log("formatString: nullptr for string id: %d", id);
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
            const char* sourceStr = rawUserStrings()[id];

            // !!! TODO: original code is prone to buffer overflow.
            buffer = strncpy(buffer, sourceStr, kUserStringSize);
            buffer += strlen(sourceStr);
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
            Console::log("formatString: invalid string id: %d", id);
            buffer += strlen(buffer);
            return buffer;
        }
    }

    char* formatString(char* buffer, string_id id, const void* args)
    {
        auto wrapped = ArgsWrapper(args);
        return formatString(buffer, id, wrapped);
    }

    char* formatString(char* buffer, size_t bufferLen, string_id id, const void* args)
    {
        return formatString(buffer, id, args);
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

    // 0x004965A6
    void emptyUserString(string_id stringId)
    {
        if (stringId < kUserStringsStart || stringId >= kUserStringsEnd)
        {
            return;
        }

        *rawUserStrings()[stringId - kUserStringsStart] = '\0';
    }

    string_id isTownName(string_id stringId)
    {
        return stringId >= kTownNamesStart && stringId < kTownNamesEnd;
    }

    string_id toTownName(string_id stringId)
    {
        assert(stringId < kTownNamesStart && stringId + kTownNamesStart < kTownNamesEnd);
        return string_id(kTownNamesStart + stringId);
    }

    string_id fromTownName(string_id stringId)
    {
        assert(isTownName(stringId));
        return string_id(stringId - kTownNamesStart);
    }
}
