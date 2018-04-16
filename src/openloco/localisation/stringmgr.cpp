#include "argswrapper.hpp"
#include "stringmgr.h"
#include "../config.h"
#include "../console.h"
#include "../interop/interop.hpp"
#include "../objects/objectmgr.h"
#include "../townmgr.h"

#include <cassert>
#include <cstring>
#include <stdexcept>

using namespace openloco::interop;

namespace openloco::stringmgr
{
    const uint16_t NUM_USER_STRINGS   = 2048;
    const uint8_t  USER_STRING_SIZE   = 32;
    const uint16_t USER_STRINGS_START = 0x8000;
    const uint16_t USER_STRINGS_END   = USER_STRINGS_START + NUM_USER_STRINGS;

    const uint16_t NUM_TOWN_NAMES     = 345;
    const uint16_t TOWN_NAMES_START   = 0x9EE7;
    const uint16_t TOWN_NAMES_END     = TOWN_NAMES_START + NUM_TOWN_NAMES;

    static loco_global<char * [0xFFFF], 0x005183FC> _strings;
    static loco_global<char [NUM_USER_STRINGS][USER_STRING_SIZE], 0x0095885C> _userStrings;

    const char* get_string(string_id id)
    {
        console::log("Fetching string %d", id);
        char* str = _strings[id];
        console::log("Found at %p", str);
        console::log("Reads: '%s'", str);

        return str;
    }

    // TODO: decltype(value)
    static char* format_comma(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = value;
        regs.edi = (uint32_t) buffer;

        call(0x00495F35, regs);
        return (char*) regs.edi;
    }

    // TODO: decltype(value)
    static char* format_int(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = value;
        regs.edi = (uint32_t) buffer;

        call(0x495E2A, regs);
        return (char*) regs.edi;
    }

    // TODO: decltype(value)
    static char* formatNumeric_2(uint16_t value, char* buffer, char decimal_separator)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;
        regs.ebx = (uint32_t) decimal_separator;

        call(0x496052, regs);
        return (char*) regs.edi;
    }

    // TODO: decltype(value)
    static char* formatNumeric_4(uint16_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x4963FC, regs);
        return (char*) regs.edi;
    }

    // TODO: decltype(value)
    static char* format_comma2dp32(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = value;
        regs.edi = (uint32_t) buffer;

        call(0x4962F1, regs);
        return (char*) regs.edi;
    }

    static char* formatDayMonthYearFull(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x495D09, regs);
        return (char*) regs.edi;
    }

    static char* formatMonthYearFull(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x495D77, regs);
        return (char*) regs.edi;
    }

    static char* formatMonthYearAbbrev_0(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x495DC7, regs);
        return (char*) regs.edi;
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

        currency_object* currency = objectmgr::get<currency_object>(0);

        int64_t localised_value = value << currency->factor;

        const char* prefix_symbol = get_string(currency->prefix_symbol);
        buffer = format_string_part(buffer, prefix_symbol, nullptr);

        buffer = formatNumeric_2(localised_value, buffer, currency->decimal_separator);

        const char* suffix_symbol = get_string(currency->suffix_symbol);
        buffer = format_string_part(buffer, suffix_symbol, nullptr);

        return buffer;
    }

    static char* format_string(char* buffer, string_id id, argswrapper &args);

    static char* format_string_part(char* buffer, const char* sourceStr, argswrapper &args)
    {
        while (true)
        {
            uint8_t ch = *sourceStr;
            sourceStr++;
            if (ch == 0)
            {
                *buffer = '\0';
                return buffer;
            }
            else if (ch <= 4)
            {
                *buffer = ch;
                buffer++;

                ch = *sourceStr;
                sourceStr++;

                *buffer = ch;
                buffer++;
            }
            else if (ch <= 16)
            {
                *buffer = ch;
                buffer++;
            }
            else if (ch <= 0x1F)
            {
                if (ch > 22)
                {
                    *buffer = ch;
                    buffer++;

                    ch = *sourceStr;
                    sourceStr++;

                    *buffer = ch;
                    buffer++;

                    ch = *sourceStr;
                    sourceStr++;

                    *buffer = ch;
                    buffer++;
                }

                ch = *sourceStr;
                sourceStr++;

                *buffer = ch;
                buffer++;

                ch = *sourceStr;
                sourceStr++;

                *buffer = ch;
                buffer++;
            }
            else if (ch < 0x7B || ch >= 0x90)
            {
                *buffer = ch;
                buffer++;
            }
            else
            {
                switch (ch)
                {
                    case 123 + 0:
                    {
                        uint32_t value = args.pop32();
                        buffer = format_comma(value, buffer);
                        break;
                    }

                    case 123 + 1:
                    {
                        uint32_t value = args.pop32();
                        buffer = format_int(value, buffer);
                        break;
                    }

                    case 123 + 2:
                    {
                        uint16_t value = args.pop16();
                        buffer = formatNumeric_4(value, buffer);
                        break;
                    }

                    case 123 + 3:
                    {
                        uint32_t value = args.pop32();
                        buffer = format_comma2dp32(value, buffer);
                        break;
                    }

                    case 123 + 4:
                    {
                        uint16_t value = args.pop16();
                        buffer = format_comma(value, buffer);
                        break;
                    }

                    case 123 + 5:
                    {
                        uint16_t value = args.pop16();
                        buffer = format_int((uint32_t) value, buffer);
                        break;
                    }

                    case 123 + 6:
                    {
                        int64_t value = args.pop32();
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case 123 + 7:
                    {
                        int32_t value_low = args.pop32();
                        int16_t value_high = args.pop16();
                        int64_t value = ((int64_t) value_high << 32) + value_low;
                        buffer = formatCurrency(value, buffer);
                        break;
                    }

                    case 123 + 8:
                    {
                        string_id id = args.pop16();
                        const char* sourceStr_ = sourceStr;
                        buffer = format_string(buffer, id, args);
                        sourceStr = sourceStr_;
                        break;
                    }

                    case 123 + 9:
                    {
                        string_id id = *(string_id*) sourceStr;
                        sourceStr += 2;
                        const char* sourceStr_ = sourceStr;
                        buffer = format_string(buffer, id, args);
                        sourceStr = sourceStr_;
                        break;
                    }

                    case 123 + 10:
                    {
                        const char* sourceStr_ = sourceStr;
                        sourceStr = (char*) args.pop32();

                        do
                        {
                            *buffer = *sourceStr;
                            buffer++;
                            sourceStr++;
                        }
                        while (*sourceStr != '\0');

                        buffer--;
                        sourceStr = sourceStr_;
                        break;
                    }

                    case 123 + 11:
                    {
                        char modifier = *sourceStr;
                        uint32_t value = args.pop32();
                        sourceStr++;

                        switch (modifier)
                        {
                            case 0:
                                buffer = formatDayMonthYearFull(value, buffer);
                                break;

                            case 4:
                                buffer = formatMonthYearFull(value, buffer);
                                break;

                            case 8:
                                buffer = formatMonthYearAbbrev_0(value, buffer);
                                break;

                            default:
                               throw std::out_of_range("format_string: unexpected modifier: " + std::to_string((uint8_t) modifier));
                        }

                        break;
                    }

                    case 123 + 12:
                    {
                        // velocity
                        auto measurement_format = config::get().measurement_format;

                        uint32_t value = args.pop16();

                        const char* unit;
                        if (measurement_format == config::measurement_formats::FORMAT_IMPERIAL)
                        {
                            // !!! TODO: Move to string id
                            unit = "mph";
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "kmh";
                            value = (value * 1648) >> 10;
                        }

                        buffer = format_comma(value, buffer);

                        do
                        {
                            *buffer = *unit;
                            buffer++;
                            unit++;
                        }
                        while (*unit != '\0');

                        buffer--;

                        break;
                    }

                    case 123 + 13:
                        // pop16
                        args.pop16();
                        break;

                    case 123 + 14:
                        // push16
                        args.push16();
                        break;

                    case 123 + 15:
                        // timeMS
                        console::error("Unimplemented format string: 15");
                        args.pop16();
                        // !!! TODO: implement timeMS
                        break;

                    case 123 + 16:
                        // timeHM
                        console::error("Unimplemented format string: 16");
                        args.pop16();
                        // !!! TODO: implement timeHM
                        break;

                    case 123 + 17:
                    {
                        // distance
                        uint32_t value = args.pop16();
                        auto measurement_format = config::get().measurement_format;

                        const char* unit;
                        if (measurement_format == config::measurement_formats::FORMAT_IMPERIAL)
                        {
                            // !!! TODO: Move to string id
                            unit = "ft";
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "m";
                            value = (value * 840) >> 8;
                        }

                        buffer = format_comma(value, buffer);

                        do
                        {
                            *buffer = *unit;
                            buffer++;
                            unit++;
                        }
                        while (*unit != '\0');

                        buffer--;

                        break;
                    }

                    case 123 + 18:
                    {
                        // height
                        uint32_t value = args.pop16();

                        bool show_height_as_units = config::get().flags & config::flags::SHOW_HEIGHT_AS_UNITS;
                        uint8_t measurement_format = config::get().measurement_format;
                        const char* unit;

                        if (show_height_as_units)
                        {
                            // !!! TODO: move to string id
                            unit = " units";
                        }
                        else if (measurement_format == config::measurement_formats::FORMAT_IMPERIAL)
                        {
                            // !!! TODO: Move to string id
                            unit = "ft";
                            value *= 10;
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "m";
                            value *= 5;
                        }

                        buffer = format_comma(value, buffer);

                        do
                        {
                            *buffer = *unit;
                            buffer++;
                            unit++;
                        }
                        while (*unit != '\0');

                        buffer--;

                        break;
                    }

                    case 123 + 19:
                    {
                        // power
                        uint32_t value = args.pop16();
                        auto measurement_format = config::get().measurement_format;

                        const char* unit;
                        if (measurement_format == config::measurement_formats::FORMAT_IMPERIAL)
                        {
                            // !!! TODO: Move to string id
                            unit = "hp";
                        }
                        else
                        {
                            // !!! TODO: Move to string id
                            unit = "kW";
                            value = (value * 764) >> 10;
                        }

                        buffer = format_comma(value, buffer);

                        do
                        {
                            *buffer = *unit;
                            buffer++;
                            unit++;
                        }
                        while (*unit != '\0');

                        buffer--;

                        break;
                    }

                    case 123 + 20:
                    {
                        // sprite
                        *buffer = 23;

                        uint32_t value = args.pop32();
                        uint32_t* sprite_ptr = (uint32_t*) (buffer + 1);
                        *sprite_ptr = value;
                        buffer += 5;

                        break;
                    }
                }
            }
        }

        return buffer;
    }

    static char* format_string_part(char* buffer, const char* sourceStr, void* args)
    {
        auto wrapped = argswrapper(args);
        return format_string_part(buffer, sourceStr, wrapped);
    }

    // 0x004958C6
    static char* format_string(char* buffer, string_id id, argswrapper &args)
    {
        if (id < USER_STRINGS_START)
        {
            const char* sourceStr = get_string(id);
            if (sourceStr == nullptr)
            {
                console::error("Got a nullptr for string id %d -- cowardly refusing", id);
                return buffer;
            }

            buffer = format_string_part(buffer, sourceStr, args);
            assert(*buffer == '\0');
            return buffer;
        }
        else if (id < USER_STRINGS_END)
        {
            id -= USER_STRINGS_START;
            args.pop16();
            const char* sourceStr = _userStrings[id];

            // !!! TODO: original code is prone to buffer overflow.
            buffer = strncpy(buffer, sourceStr, USER_STRING_SIZE);
            buffer[USER_STRING_SIZE - 1] = '\0';
            buffer += strlen(sourceStr);

            return buffer;
        }
        else if (id < TOWN_NAMES_END)
        {
            id -= TOWN_NAMES_START;
            uint16_t town_id = args.pop16();
            auto town = townmgr::get(town_id);
            void* town_name = (void*) &town->name;
            return format_string(buffer, id, town_name);
        }
        else if (id == TOWN_NAMES_END)
        {
            auto temp = args;
            uint16_t town_id = args.pop16();
            auto town = townmgr::get(town_id);
            buffer = format_string(buffer, town->name, nullptr);
            args = temp;
            return buffer;
        }
        else
        {
            // throw std::out_of_range("format_string: invalid string id: " + std::to_string((uint32_t) id));
            console::error("Invalid string id: %d", (uint32_t) id);
            return buffer;
        }
    }

    char* format_string(char* buffer, string_id id, void* args)
    {
        auto wrapped = argswrapper(args);
        return format_string(buffer, id, wrapped);
    }
}
