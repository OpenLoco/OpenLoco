#include "stringmgr.h"
#include "../config.h"
#include "../interop/interop.hpp"
#include "../townmgr.h"

#include <cstring>
#include <cstdio>
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
        printf("Fetching string %d\n", id);
        char* str = _strings[id];
        printf("Found at %p\n", str);

        return str;
    }

    // TODO: decltype(value)
    static char* format_comma(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = value;
        regs.edi = (uint32_t) buffer;

        call(0x4959A1, regs);
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

    static char* format_comma(uint16_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x495F35, regs);
        return (char*) regs.edi;
    }

    static char* formatDayMonthYearFull(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x4FF67C, regs);
        return (char*) regs.edi;
    }

    static char* formatMonthYearFull(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x4FF68C, regs);
        return (char*) regs.edi;
    }

    static char* formatMonthYearAbbrev_0(uint32_t value, char* buffer)
    {
        registers regs;
        regs.eax = (uint32_t) value;
        regs.edi = (uint32_t) buffer;

        call(0x4FF69C, regs);
        return (char*) regs.edi;
    }

    static char* format_string_part(char* buffer, char* sourceStr, void* args)
    {
        while (uint8_t ch = *sourceStr++)
        {
            if (ch <= 0x1F)
            {
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
                else if (ch <= 22)
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
                else
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

                    ch = *sourceStr;
                    sourceStr++;

                    *buffer = ch;
                    buffer++;

                    ch = *sourceStr;
                    sourceStr++;

                    *buffer = ch;
                    buffer++;
                }
            }
            else if (ch < '}') // 0x7B
            {
                *buffer = ch;
                buffer++;
            }
            else if (ch < 0x90)
            {
                switch (ch)
                {
                    case 123 + 0:
                    {
                        uint32_t value = *(uint32_t*) args;
                        args = (uint32_t*) args + 1;
                        buffer = format_comma(value, buffer);
                        break;
                    }

                    case 123 + 1:
                    {
                        uint32_t value = *(uint32_t*) args;
                        args = (uint32_t*) args + 1;
                        buffer = format_int(value, buffer);
                        break;
                    }

                    case 123 + 2:
                    {
                        uint16_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;
                        buffer = formatNumeric_4(value, buffer);
                        break;
                    }

                    case 123 + 3:
                    {
                        uint32_t value = *(uint32_t*) args;
                        args = (uint32_t*) args + 1;
                        buffer = format_comma2dp32(value, buffer);
                        break;
                    }

                    case 123 + 4:
                    {
                        uint16_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;
                        buffer = format_comma(value, buffer);
                        break;
                    }

                    case 123 + 5:
                    {
                        uint16_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;
                        buffer = format_int((uint32_t) value, buffer);
                        break;
                    }

                    case 123 + 6:
                    {
                        args = (uint32_t*) args + 1;
                        // !!! TODO: implement and call sub_495B66
                        break;
                    }

                    case 123 + 7:
                    {
                        args = (uint16_t*) args + 3;
                        // !!! TODO: implement and call sub_495B5B
                        break;
                    }

                    case 123 + 8:
                    {
                        uint16_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;
                        char* sourceStr_ = sourceStr;
                        buffer = format_string(buffer, value, args);
                        sourceStr = sourceStr_;
                        break;
                    }

                    case 123 + 9:
                    {
                        string_id id = *(uint16_t*) sourceStr;
                        sourceStr += 2;
                        char* sourceStr_ = sourceStr;
                        buffer = format_string(buffer, id, args);
                        sourceStr = sourceStr_;
                        break;
                    }

                    case 123 + 10:
                    {
                        char* sourceStr_ = sourceStr;
                        sourceStr = (char*) args;
                        args = (uint32_t*) args + 1;

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
/*
                    case 123 + 11:
                    {
                        char modifier = *sourceStr;
                        uint32_t value = *(uint32_t*) args;
                        sourceStr++;
                        args = (uint32_t*) args + 1;

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
*/
                    case 123 + 12:
                    {
                        // velocity
                        auto measurement_format = config::get().measurement_format;

                        uint32_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;

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
                        args = (uint16_t*) args + 1;
                        break;

                    case 123 + 14:
                        // push16
                        args = (uint16_t*) args - 1;
                        break;

                    case 123 + 15:
                        // timeMS
                        args = (uint16_t*) args + 1;
                        // !!! TODO: implement timeMS
                        break;

                    case 123 + 16:
                        // timeHM
                        args = (uint16_t*) args + 1;
                        // !!! TODO: implement timeHM
                        break;

                    case 123 + 17:
                    {
                        // distance
                        uint32_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;
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
                        uint32_t value = *(uint16_t*) args;
                        args = (uint16_t*) args + 1;

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
                        uint32_t value = *(uint16_t*) args;
                        args = (uint32_t*) args + 1;
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
                        uint32_t value = *(uint32_t*) args;
                        args = (uint32_t*) args + 1;
                        *(buffer + 1) = value;
                        buffer += 5;
                        // !!! TODO: implement sprite
                        break;
                    }
                }
            }
            else
            {
                *buffer = ch;
                buffer++;
            }
        }

        return buffer;
    }

    // 0x004958C6
    char* format_string(char* buffer, string_id id, void* args)
    {
        /*
        registers regs;
        regs.eax = id;
        regs.edi = (uint32_t)buffer;
        regs.ecx = (uint32_t)args;
        */

        if (id >= USER_STRINGS_START)
        {
            if (id < USER_STRINGS_END)
            {
                // sub     id, 8000h
                id -= USER_STRINGS_START;

                // add     args, 2
                args = (uint8_t*) args + 2;

                // imul    id, 20h
                // id *= USER_STRING_SIZE;

                // add     id, offset _userStrings
                char* sourceStr = _userStrings[id];

                // loc_4958EF:
                // mov     dl, [id]
                // mov     [buffer], dl
                // inc     id
                // inc     buffer
                // or      dl, dl
                // jnz     short loc_4958EF
                // dec     buffer

                // !!! TODO: original code is prone to buffer overflow.
                buffer = strncpy(buffer, sourceStr, USER_STRING_SIZE);
                buffer[USER_STRING_SIZE - 1] = '\0';
                buffer += strlen(sourceStr);

                return buffer;
            }
            else if (id < TOWN_NAMES_END)
            {
                // add     id, 0FFFF6119h
                id -= TOWN_NAMES_START;

                // movzx   args, word ptr [args]
                uint16_t town_id = *(uint16_t*) args;
                args = (uint8_t*) args + 2;

                // imul    args, 270h
                auto town = townmgr::get(town_id);

                // lea     args, towns[args]
                void* town_name = (void*) &town->name;

                // call    format_string
                return format_string(buffer, id, town_name);
            }
            else if (id == TOWN_NAMES_END)
            {
                auto temp = args;

                // movzx   ecx, word ptr [ecx]
                uint16_t town_id = *(uint16_t*) args;
                args = (uint8_t*) args + 2;

                // movzx   id, word ptr towns[args]
                auto town = townmgr::get(town_id);

                // call    format_string
                buffer = format_string(buffer, town->name, nullptr);

                // pop     args
                args = temp;

                return buffer;
            }
            else
            {
                // throw std::out_of_range("format_string: invalid string id: " + std::to_string((uint32_t) id));
                printf("Invalid string id: %d\n", (uint32_t) id);
                return buffer;
            }
        }
        else
        {
            /*
            registers regs;
            regs.eax = id;
            regs.edi = (uint32_t)buffer;
            regs.ecx = (uint32_t)args;
            */

            char* sourceStr = (char*) get_string(id);
            if (sourceStr == nullptr || sourceStr == (char*) 0x50)
            {
                printf("Got a nullptr for string id %d -- cowardly refusing\n", id);
                return buffer;
            }

            return format_string_part(buffer, sourceStr, args);
        }
    }
}
