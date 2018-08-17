#include "stringmgr.h"
#include "../config.h"
#include "../date.h"
#include "../interop/interop.hpp"
#include "../objects/currency_object.h"
#include "../objects/objectmgr.h"
#include "../townmgr.h"
#include "argswrapper.hpp"
#include "string_ids.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>

using namespace openloco::interop;

namespace openloco::stringmgr
{
    const uint16_t NUM_USER_STRINGS = 2048;
    const uint8_t USER_STRING_SIZE = 32;
    const uint16_t USER_STRINGS_START = 0x8000;
    const uint16_t USER_STRINGS_END = USER_STRINGS_START + NUM_USER_STRINGS;

    const uint16_t NUM_TOWN_NAMES = 345;
    const uint16_t TOWN_NAMES_START = 0x9EE7;
    const uint16_t TOWN_NAMES_END = TOWN_NAMES_START + NUM_TOWN_NAMES;

    static loco_global<char * [0xFFFF], 0x005183FC> _strings;
    static loco_global<char[NUM_USER_STRINGS][USER_STRING_SIZE], 0x0095885C> _userStrings;

    static std::map<int32_t, string_id> day_to_string = {
        { 1, string_ids::day_1st },
        { 2, string_ids::day_2nd },
        { 3, string_ids::day_3rd },
        { 4, string_ids::day_4th },
        { 5, string_ids::day_5th },
        { 6, string_ids::day_6th },
        { 7, string_ids::day_7th },
        { 8, string_ids::day_8th },
        { 9, string_ids::day_9th },
        { 10, string_ids::day_10th },
        { 11, string_ids::day_11th },
        { 12, string_ids::day_12th },
        { 13, string_ids::day_13th },
        { 14, string_ids::day_14th },
        { 15, string_ids::day_15th },
        { 16, string_ids::day_16th },
        { 17, string_ids::day_17th },
        { 18, string_ids::day_18th },
        { 19, string_ids::day_19th },
        { 20, string_ids::day_20th },
        { 21, string_ids::day_21st },
        { 22, string_ids::day_22nd },
        { 23, string_ids::day_23rd },
        { 24, string_ids::day_24th },
        { 25, string_ids::day_25th },
        { 26, string_ids::day_26th },
        { 27, string_ids::day_27th },
        { 28, string_ids::day_28th },
        { 29, string_ids::day_29th },
        { 30, string_ids::day_30th },
        { 31, string_ids::day_31st },
    };

    static std::map<month_id, std::pair<string_id, string_id>> month_to_string = {
        { month_id::january, { string_ids::month_short_january, string_ids::month_long_january } },
        { month_id::february, { string_ids::month_short_february, string_ids::month_long_february } },
        { month_id::march, { string_ids::month_short_march, string_ids::month_long_march } },
        { month_id::april, { string_ids::month_short_april, string_ids::month_long_april } },
        { month_id::may, { string_ids::month_short_may, string_ids::month_long_may } },
        { month_id::june, { string_ids::month_short_june, string_ids::month_long_june } },
        { month_id::july, { string_ids::month_short_july, string_ids::month_long_july } },
        { month_id::august, { string_ids::month_short_august, string_ids::month_long_august } },
        { month_id::september, { string_ids::month_short_september, string_ids::month_long_september } },
        { month_id::october, { string_ids::month_short_october, string_ids::month_long_october } },
        { month_id::november, { string_ids::month_short_november, string_ids::month_long_november } },
        { month_id::december, { string_ids::month_short_december, string_ids::month_long_december } },
    };

    const char* get_string(string_id id)
    {
        char* str = _strings[id];
        return str;
    }

    static char* format_int32_grouped(int32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = (uint32_t)buffer;

        call(0x00495F35, regs);
        return (char*)regs.edi;
    }

    static char* format_int32_ungrouped(int32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = (uint32_t)buffer;

        call(0x495E2A, regs);
        return (char*)regs.edi;
    }

    static char* format_int48_grouped(uint64_t value, char* buffer, uint8_t separator)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edx = (uint32_t)(value / (1ULL << 32)); // regs.dx = (uint16_t)(value >> 32);
        regs.edi = (uint32_t)buffer;
        regs.ebx = (uint32_t)separator;

        call(0x496052, regs);
        return (char*)regs.edi;
    }

    static char* format_short_with_decimals(int16_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = (uint32_t)buffer;

        call(0x4963FC, regs);
        return (char*)regs.edi;
    }

    static char* format_int_with_decimals(int32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t)value;
        regs.edi = (uint32_t)buffer;

        call(0x4962F1, regs);
        return (char*)regs.edi;
    }

    // 0x00495D09
    static char* format_date_dmy_full(uint32_t totalDays, char* buffer)
    {
        auto date = calc_date(totalDays);

        string_id day_string = day_to_string[date.day];
        buffer = format_string(buffer, day_string, nullptr);

        *buffer = ' ';
        buffer++;

        string_id month_string = month_to_string[date.month].second;
        buffer = format_string(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = format_int32_ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495D77
    static char* format_date_my_full(uint32_t totalDays, char* buffer)
    {
        auto date = calc_date(totalDays);

        string_id month_string = month_to_string[date.month].second;
        buffer = format_string(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = format_int32_ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495DC7
    static char* format_date_my_abbrev(uint32_t totalDays, char* buffer)
    {
        auto date = calc_date(totalDays);

        string_id month_string = month_to_string[date.month].second;
        buffer = format_string(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = format_int32_ungrouped(date.year, buffer);

        return buffer;
    }

    // 0x00495DC7
    static char* format_raw_date_my_abbrev(uint32_t totalDays, char* buffer)
    {
        auto month = static_cast<month_id>(totalDays % 12);
        string_id month_string = month_to_string[month].first;
        buffer = format_string(buffer, month_string, nullptr);

        *buffer = ' ';
        buffer++;

        buffer = format_int32_ungrouped(totalDays / 12, buffer);

        return buffer;
    }

    static char* format_string_part(char* buffer, const char* sourceStr, void* args);

    static char* formatCurrency(int64_t value, char* buffer)
    {
        if (value < 0)
        {
            *buffer = '-';
            buffer++;
            value = -value;
        }

        currency_object* currency = objectmgr::get<currency_object>();

        int64_t localised_value = value * (1ULL << currency->factor);

        const char* prefix_symbol = get_string(currency->prefix_symbol);
        buffer = format_string_part(buffer, prefix_symbol, nullptr);

        buffer = format_int48_grouped(localised_value, buffer, currency->separator);

        const char* suffix_symbol = get_string(currency->suffix_symbol);
        buffer = format_string_part(buffer, suffix_symbol, nullptr);

        return buffer;
    }

    static char* format_string(char* buffer, string_id id, argswrapper& args);

    static char* format_string_part(char* buffer, const char* sourceStr, argswrapper& args)
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
                    case control_codes::int32_grouped:
                    {
                        int32_t value = args.pop32();
                        buffer = format_int32_grouped(value, buffer);
                        break;
                    }

                    case control_codes::int32_ungrouped:
                    {
                        int32_t value = args.pop32();
                        buffer = format_int32_ungrouped(value, buffer);
                        break;
                    }

                    case control_codes::int16_decimals:
                    {
                        int16_t value = args.pop16();
                        buffer = format_short_with_decimals(value, buffer);
                        break;
                    }

                    case control_codes::int32_decimals:
                    {
                        int32_t value = args.pop32();
                        buffer = format_int_with_decimals(value, buffer);
                        break;
                    }

                    case control_codes::int16_grouped:
                    {
                        int16_t value = args.pop16();
                        buffer = format_int32_grouped(value, buffer);
                        break;
                    }

                    case control_codes::uint16_ungrouped:
                    {
                        uint16_t value = args.pop16();
                        buffer = format_int32_ungrouped((int32_t)value, buffer);
                        break;
                    }

                    case control_codes::currency32:
                    {
                        int32_t value = args.pop32();
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case control_codes::currency48:
                    {
                        uint32_t value_low = (uint32_t)args.pop32();
                        int32_t value_high = (int16_t)args.pop16();
                        int64_t value = (value_high * (1ULL << 32)) | value_low;
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case control_codes::stringid_args:
                    {
                        string_id id = args.pop16();
                        buffer = format_string(buffer, id, args);
                        break;
                    }

                    case control_codes::stringid_str:
                    {
                        string_id id = *(string_id*)sourceStr;
                        sourceStr += 2;
                        buffer = format_string(buffer, id, args);
                        break;
                    }

                    case control_codes::string_ptr:
                    {
                        const char* str = (char*)args.pop32();
                        strcpy(buffer, str);
                        buffer += strlen(str);
                        break;
                    }

                    case control_codes::date:
                    {
                        char modifier = *sourceStr;
                        uint32_t totalDays = args.pop32();
                        sourceStr++;

                        switch (modifier)
                        {
                            case 0:
                                buffer = format_date_dmy_full(totalDays, buffer);
                                break;

                            case 4:
                                buffer = format_date_my_full(totalDays, buffer);
                                break;

                            case 5:
                                buffer = format_date_my_abbrev(totalDays, buffer);
                                break;

                            case 8:
                                buffer = format_raw_date_my_abbrev(totalDays, buffer);
                                break;

                            default:
                                throw std::out_of_range("format_string: unexpected modifier: " + std::to_string((uint8_t)modifier));
                        }

                        break;
                    }

                    case control_codes::velocity:
                    {
                        auto measurement_format = config::get().measurement_format;

                        uint32_t value = args.pop16();

                        const char* unit;
                        if (measurement_format == config::measurement_format::imperial)
                        {
                            // !!! TODO: Move to string id
                            unit = "mph";
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "kmh\xB9";
                            value = std::round(value * 1.609375);
                        }

                        buffer = format_int32_grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case control_codes::pop16:
                        args.skip16();
                        break;

                    case control_codes::push16:
                        args.push16();
                        break;

                    case control_codes::timeMS:
                        throw std::runtime_error("Unimplemented format string: 15");

                    case control_codes::timeHM:
                        throw std::runtime_error("Unimplemented format string: 16");

                    case control_codes::distance:
                    {
                        uint32_t value = args.pop16();
                        auto measurement_format = config::get().measurement_format;

                        const char* unit;
                        if (measurement_format == config::measurement_format::imperial)
                        {
                            // !!! TODO: Move to string id
                            unit = "ft";
                            value = std::round(value * 3.28125);
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "m";
                        }

                        buffer = format_int32_grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case control_codes::height:
                    {
                        int32_t value = (int16_t)args.pop16();

                        bool show_height_as_units = config::get().flags & config::flags::show_height_as_units;
                        uint8_t measurement_format = config::get().measurement_format;
                        const char* unit;

                        if (show_height_as_units)
                        {
                            // !!! TODO: move to string id
                            unit = " units";
                        }
                        else if (measurement_format == config::measurement_format::imperial)
                        {
                            // !!! TODO: Move to string id
                            unit = "ft";
                            value *= 16;
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "m";
                            value *= 5;
                        }

                        buffer = format_int32_grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case control_codes::power:
                    {
                        uint32_t value = args.pop16();
                        auto measurement_format = config::get().measurement_format;

                        const char* unit;
                        if (measurement_format == config::measurement_format::imperial)
                        {
                            // !!! TODO: Move to string id
                            unit = "hp";
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "kW";
                            value = std::round(value * 0.746);
                        }

                        buffer = format_int32_grouped(value, buffer);

                        strcpy(buffer, unit);
                        buffer += strlen(unit);

                        break;
                    }

                    case control_codes::inline_sprite_args:
                    {
                        *buffer = control_codes::inline_sprite_str;
                        uint32_t value = args.pop32();
                        uint32_t* sprite_ptr = (uint32_t*)(buffer + 1);
                        *sprite_ptr = value;
                        buffer += 5;

                        break;
                    }
                }
            }
        }
    }

    static char* format_string_part(char* buffer, const char* sourceStr, void* args)
    {
        auto wrapped = argswrapper(args);
        return format_string_part(buffer, sourceStr, wrapped);
    }

    // 0x004958C6
    static char* format_string(char* buffer, string_id id, argswrapper& args)
    {
        if (id < USER_STRINGS_START)
        {
            const char* sourceStr = get_string(id);
            if (sourceStr == nullptr)
            {
                throw std::runtime_error("Got a nullptr for string id " + std::to_string(id) + " -- cowardly refusing");
            }

            buffer = format_string_part(buffer, sourceStr, args);
            assert(*buffer == '\0');
            return buffer;
        }
        else if (id < USER_STRINGS_END)
        {
            id -= USER_STRINGS_START;
            args.skip16();
            const char* sourceStr = _userStrings[id];

            // !!! TODO: original code is prone to buffer overflow.
            buffer = strncpy(buffer, sourceStr, USER_STRING_SIZE);
            buffer += strlen(sourceStr);
            *buffer = '\0';

            return buffer;
        }
        else if (id < TOWN_NAMES_END)
        {
            id -= TOWN_NAMES_START;
            uint16_t town_id = args.pop16();
            auto town = townmgr::get(town_id);
            void* town_name = (void*)&town->name;
            return format_string(buffer, id, town_name);
        }
        else if (id == TOWN_NAMES_END)
        {
            uint16_t town_id = args.pop16();
            auto town = townmgr::get(town_id);
            return format_string(buffer, town->name, nullptr);
        }
        else
        {
            throw std::out_of_range("format_string: invalid string id: " + std::to_string((uint32_t)id));
        }
    }

    char* format_string(char* buffer, string_id id, void* args)
    {
        auto wrapped = argswrapper(args);
        return format_string(buffer, id, wrapped);
    }
}
