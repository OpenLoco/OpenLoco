#include "stringmgr.h"
#include "../interop/interop.hpp"
#include "../townmgr.h"

#include <cstring>
#include <stdexcept>

using namespace openloco::interop;

namespace openloco::stringmgr
{
    const uint16_t NUM_USER_STRINGS = 2048;
    const uint8_t USER_STRING_SIZE = 32;

    static loco_global<char * [0xFFFF], 0x005183FC> _strings;
    static loco_global<char [NUM_USER_STRINGS][USER_STRING_SIZE], 0x0095885C> _userStrings;

    const char* get_string(string_id id)
    {
        return _strings[id];
    }

    // 0x004958C6
    void format_string(char* buffer, string_id id, void* args)
    {
        registers regs;
        regs.eax = id;
        regs.edi = (uint32_t)buffer;
        regs.ecx = (uint32_t)args;

        if (id >= 0x8000)
        {
            if (id < 0x8800)
            {
                // sub     id, 8000h
                id -= 0x8000;

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

                // !!! TODO: original code is prone to buffer overflow.
                buffer = strncpy(buffer, sourceStr, USER_STRING_SIZE);
                buffer[USER_STRING_SIZE - 1] = '\0';

                // dec     buffer
                // TODO: ???

                return;
            }
            else if (id < 41024) // 0xA040
            {
                // add     id, 0FFFF6119h
                id -= 40679;

                // push    args
                auto temp = args;

                // movzx   args, word ptr [args]
                uint16_t town_id = *(uint16_t*) args;
                args = (uint8_t*) args + 2;

                // imul    args, 270h
                auto town = townmgr::get(town_id);

                // lea     args, towns[args]
                // !!! FIXME: string_id?
                args = (void*) &town->name;

                // call    format_string
                format_string(buffer, id, args);
                // TODO: is `args` we pushed earlier used in call?

                // pop     args
                args = temp;

                return;
            }
            else if (id == 41024)
            {
                auto temp = args;

                // movzx   ecx, word ptr [ecx]
                uint16_t town_id = *(uint16_t*) args;
                args = (uint8_t*) args + 2;

                // movzx   id, word ptr towns[args]
                auto town = townmgr::get(town_id);

                // call    format_string
                format_string(buffer, town->name, nullptr);

                // pop     args
                args = temp;

                return;
            }
            else
            {
                throw std::out_of_range("format_string: invalid string id: " + id);
            }
        }
        else
        {
            registers regs;
            regs.eax = id;
            regs.edi = (uint32_t)buffer;
            regs.ecx = (uint32_t)args;

            call(0x00495935, regs);
        }

    }
}
