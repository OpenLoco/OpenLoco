#include "stringmgr.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::stringmgr
{
    static loco_global<char * [0xFFFF], 0x005183FC> _strings;

    const char* get_string(string_id id)
    {
        return _strings[id];
    }

    void format_string(char* buffer, string_id id, void* args)
    {
        registers regs;
        regs.eax = id;
        regs.edi = (uint32_t)buffer;
        regs.ecx = (uint32_t)args;
        call(0x004958C6, regs);
    }
}
