#include "emu.h"
#include "i386.h"
#include <cstdio>
#include <x86emu.h>

#include <map>
#include <prim_ops.h>
#include <mem.h>
#include "interop.hpp"

x86emu_t *emu;

struct InteropFunction
{
    void (*function)();
    openloco::interop::hook_function hookFunction;
    openloco::interop::CallingConvention convention;
    size_t argumentCount;
};

std::map<uint32_t, InteropFunction> _hooks;
std::map<uint32_t, InteropFunction> _libraryHooks;

[[maybe_unused]] void flush_log(x86emu_t *emu, char *buf, unsigned size)
{
    if (!buf || !size)
        return;

    fwrite(buf, size, 1, stdout);
}

x86emu_memio_handler_t _parent;

uint32_t emu_me( x86emu_s *emu, u32 address, u32 *val, unsigned type)
{
    if(address < 0x1000000) {
        return _parent(emu, address, val, type);
    }

    if(address >= 0x401000 && address < 0x401000 +  4096 * 214) {
        return _parent(emu, address, val, type);
    }

    if(address >= 0x4d7000 && address < 0x4d7000 +  4096 * (78 + 3133)) {
        return _parent(emu, address, val, type);
    }

    u32 value =0xFFFFFFFF;

    switch (type) {
        case X86EMU_MEMIO_R | X86EMU_MEMIO_8:
            value= *((uint8_t*)(uintptr_t) address);
            *val = value;
            break;

        case X86EMU_MEMIO_W | X86EMU_MEMIO_32:
             value = *val;
            *((uint32_t*)(uintptr_t) address) = *val;
            break;

        default:
            printf("%x %08X %08X\n", type,address, *val);
    }


    return 0;
}

void emu_init()
{
    emu = x86emu_new(X86EMU_PERM_R | X86EMU_PERM_W | X86EMU_PERM_X, 0);
//    x86emu_set_log(emu, 10000, flush_log);
    emu->log.trace = X86EMU_TRACE_DEFAULT | X86EMU_TRACE_ACC;

    // text
    auto textSize = 4096 * 214;
    auto textBase = 0x401000;
    for (uintptr_t sizeDone = 0; sizeDone < textSize; sizeDone += X86EMU_PAGE_SIZE)
    {
        x86emu_set_page(emu, textBase + sizeDone, (void *) (textBase + sizeDone));
    }
    x86emu_set_perm(emu, textBase, textBase + textSize, X86EMU_PERM_RWX | X86EMU_PERM_VALID);

    // data
    auto dataSize = 4096 * (78 + 3133);
    auto dataBase = 0x4d7000;
    for (uintptr_t sizeDone = 0; sizeDone < dataSize; sizeDone += X86EMU_PAGE_SIZE)
    {
        x86emu_set_page(emu, dataBase + sizeDone, (void *) (dataBase + sizeDone));
    }
    x86emu_set_perm(emu, dataBase, dataBase + dataSize, X86EMU_PERM_RW | X86EMU_PERM_VALID);

    _parent =x86emu_set_memio_handler(emu, emu_me);
}

namespace openloco::interop
{
    void register_hook(uint32_t address, hook_function function)
    {
        InteropFunction item = {};
        item.convention = CallingConvention::sawyer;
        item.hookFunction = function;
        _hooks.emplace(address, item);
    }
    void write_ret(uint32_t address) {}
    void write_jmp(uint32_t address, void *fn) {}
    void write_nop(uint32_t address, size_t count) {}
    void hook_dump(uint32_t address, void *fn) {}
    void hook_lib(uint32_t address, void *fn) {}

    void hookFunction(uint32_t address, CallingConvention convention, size_t arguments, void (*fn)())
    {
        InteropFunction item = {};
        item.convention = convention;
        item.argumentCount = arguments;
        item.function = fn;
        _hooks.emplace(address, item);
    }
    void hookLibrary(uint32_t address, CallingConvention convention, size_t arguments, void (*fn)())
    {
        auto it = _libraryHooks.find(address);
        if (it != _libraryHooks.end())
        {
            _libraryHooks.erase (it);
        }

        InteropFunction item = {};
        item.convention = convention;
        item.argumentCount = arguments;
        item.function = fn;
        _libraryHooks.emplace(address, item);
    }
}

extern "C" {
int has_hook(x86emu_t *emu)
{
    uint32_t address = emu->x86.R_EIP;

    auto it = _hooks.find(address);
    if (it == _hooks.end())
    {
        return false;
    }

    printf("  hook @ 0x%X\n", address);

    if (it->second.convention == openloco::interop::CallingConvention::sawyer)
    {
        uint32_t retaddr = pop_long(emu);
        openloco::interop::registers regs = {};
        regs.eax = emu->x86.R_EAX;
        regs.ebx = emu->x86.R_EBX;
        regs.ecx = emu->x86.R_ECX;
        regs.edx = emu->x86.R_EDX;
        regs.esi = emu->x86.R_ESI;
        regs.edi = emu->x86.R_EDI;
        regs.ebp = emu->x86.R_EBP;

        it->second.hookFunction(regs);

        emu->x86.R_EAX = regs.eax;
        emu->x86.R_EBX = regs.ebx;
        emu->x86.R_ECX = regs.ecx;
        emu->x86.R_EDX = regs.edx;
        emu->x86.R_ESI = regs.esi;
        emu->x86.R_EDI = regs.edi;
        emu->x86.R_EBP = regs.ebp;

        emu->x86.R_EIP = retaddr;

        return 1;
    }

    uint32_t retaddr = pop_long(emu);
    uint32_t retVal;

    switch (it->second.argumentCount)
    {
        case 0:
            retVal = ((uint32_t (*)()) it->second.function)();
            break;

        case 1:
        {
            uint32_t arg1 = pop_long(emu);
            retVal = ((uint32_t (*)(uint32_t)) it->second.function)(arg1);
            break;
        }

        case 2:
        {
            uint32_t arg1 = pop_long(emu);
            uint32_t arg2 = pop_long(emu);
            retVal = ((uint32_t (*)(uint32_t,uint32_t)) it->second.function)(arg1, arg2);
            break;
        }

        case 3:
        {
            uint32_t arg1 = pop_long(emu);
            uint32_t arg2 = pop_long(emu);
            uint32_t arg3 = pop_long(emu);
            retVal = ((uint32_t (*)(uint32_t,uint32_t,uint32_t)) it->second.function)(arg1, arg2, arg3);
            break;
        }

        default:
            assert(0);
            break;
    }

    if (it->second.convention == openloco::interop::CallingConvention::cdecl)
    {
        for (int i = 0; i < it->second.argumentCount; i++)
        {
            push_long(emu, 0xFFFFFF);
        }
    }

    emu->x86.R_EAX = retVal;
    emu->x86.R_EIP = retaddr;

    return 1;
}

int has_lib_hook(x86emu_t *emu, uint32_t addr)
{
    auto it = _libraryHooks.find(addr);
    if (it == _libraryHooks.end())
    {
        return false;
    }

    printf("  hook @ 0x%X\n", addr);


    uint32_t retVal;

    switch (it->second.argumentCount)
    {
        case 0:
            retVal = ((uint32_t (*)()) it->second.function)();
            break;

        case 1:
        {
            uint32_t arg1 = pop_long(emu);
            retVal = ((uint32_t (*)(uint32_t)) it->second.function)(arg1);
            break;
        }

        case 7:
        {
            uint32_t arg1 = pop_long(emu);
            uint32_t arg2 = pop_long(emu);
            uint32_t arg3 = pop_long(emu);
            uint32_t arg4 = pop_long(emu);
            uint32_t arg5 = pop_long(emu);
            uint32_t arg6 = pop_long(emu);
            uint32_t arg7 = pop_long(emu);
            retVal = ((uint32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)) it->second.function)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            break;
        }

        default:
            assert(0);
            break;
    }

    if (it->second.convention == openloco::interop::CallingConvention::cdecl)
    {
        for (int i = 0; i < it->second.argumentCount; i++)
        {
            push_long(emu, 0xFFFFFF);
        }
    }


    emu->x86.R_EAX = retVal;
    return 1;
}
}