#ifndef __i386__
#include "emu.h"
#include "../openloco.h"
#include "i386.h"
#include <cstdio>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

#include "../console.h"
#include "interop.hpp"
#include <cassert>
#include <inttypes.h>
#include <map>
#include <mhash.h>

#ifdef __linux__
#include <sys/mman.h>
#endif

extern "C" {
#include <tinyalloc.h>
}

namespace openloco::interop
{
    struct InteropFunction
    {
        void (*function)();
        openloco::interop::hook_function hookFunction;
        openloco::interop::CallingConvention convention;
        size_t argumentCount;
        std::function<void()> simple;
    };

    std::vector<InteropFunction> _newHooks;

    std::map<uint32_t, InteropFunction> _hooks;
    std::map<uint32_t, InteropFunction> _libraryHooks;

    constexpr uint32_t executionEnd = 0xfffff000;

    void* hookMem;
    uintptr_t hookMemCur;

    const uint32_t hookMemStart = 0x10000000;
    const uint32_t hookMemSize = 0x1000;

    static const int softwareInterruptNumber = 0x80;

    uint32_t heapStart = 0x20000000;
    uint32_t heapSize = 1024 * 1024 * 128; // 16MiB
    void* heap;

    uintptr_t stackStart = 0xA0000000;
    uint32_t stackSize = 1 * 1024 * 1024;
    void* stack;

    static uint32_t _esp;

    void emu_init()
    {
        hookMem = malloc(0x1000);
        hookMemCur = reinterpret_cast<uintptr_t>(hookMem);

        heap = mmap((void*)heapStart, heapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (heap == MAP_FAILED)
            exit(EXIT_FAILURE);

        auto heapEnd = (void*)(heapStart + heapSize);
        auto success = ta_init(heap, heapEnd, 1024, 16, 8);
        if (!success)
            exit(EXIT_FAILURE);

        stack = mmap((void*)stackStart, stackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        printf("allocated stack space from 0x%" PRIXPTR " to 0x%" PRIXPTR, (uintptr_t)stack, (uintptr_t)stack + stackSize);

        _esp = stackStart + stackSize;
    }

    static void write_address_strictalias(uint8_t* data, uint32_t addr)
    {
        *(data + 0) = ((addr)&0x000000ff) >> 0;
        *(data + 1) = ((addr)&0x0000ff00) >> 8;
        *(data + 2) = ((addr)&0x00ff0000) >> 16;
        *(data + 3) = ((addr)&0xff000000) >> 24;
    }

    // Callback for tracing all kinds of memory errors
    static void UcErrorHook(uc_engine* uc, uc_mem_type type, uint64_t address, int size, int64_t value, void* user_data)
    {
        /*
FIXME: type is one of
    UC_MEM_READ_UNMAPPED = 19,    // Unmapped memory is read from
    UC_MEM_WRITE_UNMAPPED = 20,   // Unmapped memory is written to
    UC_MEM_FETCH_UNMAPPED = 21,   // Unmapped memory is fetched
    UC_MEM_WRITE_PROT = 22,  // Write to write protected, but mapped, memory
    UC_MEM_READ_PROT = 23,   // Read from read protected, but mapped, memory
    UC_MEM_FETCH_PROT = 24,  // Fetch from non-executable, but mapped, memory
*/
        printf("Unicorn-Engine error of type %d at 0x%" PRIx64 ", size = 0x%" PRIX32 "\n", type, address, size);
        uc_emu_stop(uc);

        //    ThreadContext ctx;
        //    TransferContext(&ctx, false);
        //    PrintContext(&ctx);

        int eip;
        uc_reg_read(uc, UC_X86_REG_EIP, &eip);
        printf("Emulation returned %X\n", eip);

        int esp;
        uc_reg_read(uc, UC_X86_REG_ESP, &esp);
        for (int i = 0; i < 100; i++)
        {
            //        printf("Stack [%d] = %X\n", i, *(uint32_t*)Memory(esp + i * 4));
        }

        assert(false);
    }

    static void hook_code(uc_engine* uc, uint64_t address, uint32_t size, void* user_data)
    {
        printf("Executing at 0x%" PRIx64 ", ilen = 0x%x\n", address, size);
    }

    void register_hook(uint32_t address, hook_function function)
    {
        auto newIndex = _newHooks.size();

        InteropFunction item = {};
        item.convention = CallingConvention::sawyer;
        item.hookFunction = function;
        _newHooks.push_back(item);

        uint8_t assembly[8] = {};

        // push imm32
        assembly[0] = 0x68;
        write_address_strictalias(&assembly[1], newIndex);

        // int imm8
        assembly[5] = 0xCD;
        assembly[6] = softwareInterruptNumber;

        // ret
        assembly[7] = 0xC3;

        write_memory(address, assembly, sizeof(assembly));
    }

    void write_ret(uint32_t address) {}
    void write_jmp(uint32_t address, void* fn) {}
    void write_nop(uint32_t address, size_t count) {}
    void hook_dump(uint32_t address, void* fn) {}

    void hookLibrary(uint32_t address, CallingConvention convention, size_t arguments, void (*fn)())
    {
        auto newIndex = _newHooks.size();

        InteropFunction item = {};
        item.convention = convention;
        item.argumentCount = arguments;
        item.function = fn;
        _newHooks.push_back(item);

        uint8_t assembly[8] = {};

        // push imm32
        assembly[0] = 0x68;
        write_address_strictalias(&assembly[1], newIndex);

        // int imm8
        assembly[5] = 0xCD;
        assembly[6] = softwareInterruptNumber;

        // ret
        assembly[7] = 0xC3;

        uint32_t data = hookMemStart + (hookMemCur - reinterpret_cast<uintptr_t>(hookMem));
        write_memory(address, &data, sizeof(data));

        memcpy((void*)hookMemCur, assembly, sizeof(assembly));
        hookMemCur += sizeof(assembly);
    }

    void hookLibrary(uint32_t address, std::function<void()> fn)
    {
        auto newIndex = _newHooks.size();

        InteropFunction item = {};
        item.simple = fn;
        _newHooks.push_back(item);

        uint8_t assembly[8] = {};

        // push imm32
        assembly[0] = 0x68;
        write_address_strictalias(&assembly[1], newIndex);

        // int imm8
        assembly[5] = 0xCD;
        assembly[6] = softwareInterruptNumber;

        // ret
        assembly[7] = 0xC3;

        uint32_t data = hookMemStart + (hookMemCur - reinterpret_cast<uintptr_t>(hookMem));
        write_memory(address, &data, sizeof(data));

        memcpy((void*)hookMemCur, assembly, sizeof(assembly));
        hookMemCur += sizeof(assembly);
    }

    void hookFunction(uint32_t address, CallingConvention convention, size_t arguments, void (*fn)())
    {
        auto newIndex = _newHooks.size();

        InteropFunction item = {};
        item.convention = convention;
        item.argumentCount = arguments;
        item.function = fn;
        _newHooks.push_back(item);

        uint8_t assembly[8] = {};

        // push imm32
        assembly[0] = 0x68;
        write_address_strictalias(&assembly[1], newIndex);

        // int imm8
        assembly[5] = 0xCD;
        assembly[6] = softwareInterruptNumber;

        // ret
        assembly[7] = 0xC3;

        write_memory(address, assembly, sizeof(assembly));
    }

    static uint32_t pop(uc_engine* uc)
    {
        uint32_t esp;
        uc_reg_read(uc, UC_X86_REG_ESP, &esp);

        uint32_t value;
        uc_mem_read(uc, esp, &value, 4);

        esp += 4;
        uc_reg_write(uc, UC_X86_REG_ESP, &esp);

        return value;
    }

    static uint32_t push(uc_engine* uc, uint32_t value)
    {
        uint32_t esp;
        uc_reg_read(uc, UC_X86_REG_ESP, &esp);

        esp -= 4;
        uc_reg_write(uc, UC_X86_REG_ESP, &esp);

        uc_mem_write(uc, esp, &value, 4);

        return value;
    }

    static void hook_interrupt(uc_engine* uc, uint32_t intno, void* user_data)
    {
        if (intno != softwareInterruptNumber)
            return;

        uint32_t index = pop(uc);
        console::log("Interupt 0x%X (%d)", intno, index);

        auto& hook = _newHooks[index];

        if (hook.convention == openloco::interop::CallingConvention::sawyer)
        {
            uint32_t backupEsp = _esp;
            uc_reg_read(uc, UC_X86_REG_ESP, &_esp);

            openloco::interop::registers regs = {};
            uc_reg_read(uc, UC_X86_REG_EAX, &regs.eax);
            uc_reg_read(uc, UC_X86_REG_EBX, &regs.ebx);
            uc_reg_read(uc, UC_X86_REG_ECX, &regs.ecx);
            uc_reg_read(uc, UC_X86_REG_EDX, &regs.edx);
            uc_reg_read(uc, UC_X86_REG_ESI, &regs.esi);
            uc_reg_read(uc, UC_X86_REG_EDI, &regs.edi);
            uc_reg_read(uc, UC_X86_REG_EBP, &regs.ebp);

            hook.hookFunction(regs);

            uc_reg_write(uc, UC_X86_REG_EAX, &regs.eax);
            uc_reg_write(uc, UC_X86_REG_EBX, &regs.ebx);
            uc_reg_write(uc, UC_X86_REG_ECX, &regs.ecx);
            uc_reg_write(uc, UC_X86_REG_EDX, &regs.edx);
            uc_reg_write(uc, UC_X86_REG_ESI, &regs.esi);
            uc_reg_write(uc, UC_X86_REG_EDI, &regs.edi);
            uc_reg_write(uc, UC_X86_REG_EBP, &regs.ebp);

            _esp = backupEsp;
            return;
        }

        if (hook.simple != nullptr)
        {
            hook.simple();
            return;
        }

        uint32_t retaddr = pop(uc);
        uint32_t retVal;
        switch (hook.argumentCount)
        {
            case 0:
                retVal = ((uint32_t(*)())hook.function)();
                break;

            case 1:
            {
                uint32_t arg1 = pop(uc);
                retVal = ((uint32_t(*)(uint32_t))hook.function)(arg1);
                break;
            }

            case 2:
            {
                uint32_t arg1 = pop(uc);
                uint32_t arg2 = pop(uc);
                retVal = ((uint32_t(*)(uint32_t, uint32_t))hook.function)(arg1, arg2);
                break;
            }

            case 3:
            {
                uint32_t arg1 = pop(uc);
                uint32_t arg2 = pop(uc);
                uint32_t arg3 = pop(uc);
                retVal = ((uint32_t(*)(uint32_t, uint32_t, uint32_t))hook.function)(arg1, arg2, arg3);
                break;
            }

            case 5:
            {
                uint32_t arg1 = pop(uc);
                uint32_t arg2 = pop(uc);
                uint32_t arg3 = pop(uc);
                uint32_t arg4 = pop(uc);
                uint32_t arg5 = pop(uc);
                retVal = ((uint32_t(*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))hook.function)(arg1, arg2, arg3, arg4, arg5);
                break;
            }

            case 7:
            {
                uint32_t arg1 = pop(uc);
                uint32_t arg2 = pop(uc);
                uint32_t arg3 = pop(uc);
                uint32_t arg4 = pop(uc);
                uint32_t arg5 = pop(uc);
                uint32_t arg6 = pop(uc);
                uint32_t arg7 = pop(uc);
                retVal = ((uint32_t(*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))hook.function)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
                break;
            }

            default:
                assert(0);
                break;
        }

        if (hook.convention == CallingConvention::cdecl)
        {
            for (size_t i = 0; i < hook.argumentCount; i++)
            {
                push(uc, 0xFFFFFFFF);
            }
        }

        uc_reg_write(uc, UC_X86_REG_EAX, &retVal);
        push(uc, retaddr);
    }

    namespace emu
    {
        static uc_engine* initialise()
        {
            uc_engine* uc = nullptr;
            uc_err err;

            err = uc_open(UC_ARCH_X86, UC_MODE_32, &uc);
            if (err)
            {
                throw std::runtime_error("Failed on uc_open() with error returned " + std::to_string(err) + ": " + std::string(uc_strerror(err)));
            }

            // Add hooks to catch errors
            uc_hook errorHooks[6];
            {
                // Hook for memory read on unmapped memory
                uc_hook_add(uc, &errorHooks[0], UC_HOOK_MEM_READ_UNMAPPED, (void*)UcErrorHook, nullptr, 1, 0);

                // Hook for invalid memory write events
                uc_hook_add(uc, &errorHooks[1], UC_HOOK_MEM_WRITE_UNMAPPED, (void*)UcErrorHook, nullptr, 1, 0);

                // Hook for invalid memory fetch for execution events
                uc_hook_add(uc, &errorHooks[2], UC_HOOK_MEM_FETCH_UNMAPPED, (void*)UcErrorHook, nullptr, 1, 0);

                // Hook for memory read on read-protected memory
                uc_hook_add(uc, &errorHooks[3], UC_HOOK_MEM_READ_PROT, (void*)UcErrorHook, nullptr, 1, 0);

                // Hook for memory write on write-protected memory
                uc_hook_add(uc, &errorHooks[4], UC_HOOK_MEM_WRITE_PROT, (void*)UcErrorHook, nullptr, 1, 0);

                // Hook for memory fetch on non-executable memory
                uc_hook_add(uc, &errorHooks[5], UC_HOOK_MEM_FETCH_PROT, (void*)UcErrorHook, nullptr, 1, 0);
            }

            uc_hook hook1;
            uc_hook_add(uc, &hook1, UC_HOOK_INTR, (void*)hook_interrupt, NULL, 0, -1);

            uc_mem_map_ptr(uc, 0x401000, 0x4d7000 - 0x401000, UC_PROT_READ | UC_PROT_EXEC, (void*)0x401000);
            uc_mem_map_ptr(uc, 0x4d7000, 0x1162000 - 0x4d7000, UC_PROT_READ | UC_PROT_WRITE, (void*)0x4d7000);
            uc_mem_map_ptr(uc, (uintptr_t)heap, heapSize, UC_PROT_READ | UC_PROT_WRITE, heap);

            uc_mem_map_ptr(uc, hookMemStart, hookMemSize, UC_PROT_READ | UC_PROT_EXEC, hookMem);

            uc_mem_map_ptr(uc, stackStart, stackSize, UC_PROT_READ | UC_PROT_WRITE, (void*)stack);

            uc_mem_map(uc, executionEnd, 0x1000, UC_PROT_ALL);

            return uc;
        }

        static int depth = 0;

        void call(uint32_t address, int32_t* _eax, int32_t* _ebx, int32_t* _ecx, int32_t* _edx, int32_t* _esi, int32_t* _edi, int32_t* _ebp)
        {
            auto uc = initialise();

            if (false)
            {
                uc_hook hook1;
                uc_hook_add(uc, &hook1, UC_HOOK_CODE, (void*)hook_code, NULL, 0, -1);
            }

            depth++;

            uint32_t espStart = _esp;
            uc_reg_write(uc, UC_X86_REG_ESP, &espStart);

            uc_reg_write(uc, UC_X86_REG_EAX, _eax);
            uc_reg_write(uc, UC_X86_REG_EBX, _ebx);
            uc_reg_write(uc, UC_X86_REG_ECX, _ecx);
            uc_reg_write(uc, UC_X86_REG_EDX, _edx);
            uc_reg_write(uc, UC_X86_REG_ESI, _esi);
            uc_reg_write(uc, UC_X86_REG_EDI, _edi);
            uc_reg_write(uc, UC_X86_REG_EBP, _ebp);

            // Push return address to stack
            push(uc, executionEnd);

            auto err = uc_emu_start(uc, address, executionEnd, 0, 0);

            if (err)
            {
                throw std::runtime_error("Failed on uc_emu_start() with error returned " + std::to_string(err) + ": " + std::string(uc_strerror(err)));
            }

            uint32_t espEnd;
            uc_reg_read(uc, UC_X86_REG_ESP, &espEnd);

            assert(espEnd == espStart);

            uc_reg_read(uc, UC_X86_REG_EAX, _eax);
            uc_reg_read(uc, UC_X86_REG_EBX, _ebx);
            uc_reg_read(uc, UC_X86_REG_ECX, _ecx);
            uc_reg_read(uc, UC_X86_REG_EDX, _edx);
            uc_reg_read(uc, UC_X86_REG_ESI, _esi);
            uc_reg_read(uc, UC_X86_REG_EDI, _edi);
            uc_reg_read(uc, UC_X86_REG_EBP, _ebp);

            uc_close(uc);

            depth--;
        }
    }
}

/*
extern "C" {
int has_hook(void* emu)
{
    uint32_t address =0;

    auto it = _hooks.find(address);
    if (it == _hooks.end())
    {
        return false;
    }

    printf("  hook @ 0x%X\n", address);

    uint32_t retaddr = pop_long(emu);
    uint32_t retVal;

    switch (it->second.argumentCount)
    {
        case 0:
            retVal = ((uint32_t(*)())it->second.function)();
            break;

        case 1:
        {
            uint32_t arg1 = pop_long(emu);
            retVal = ((uint32_t(*)(uint32_t))it->second.function)(arg1);
            break;
        }

        case 2:
        {
            uint32_t arg1 = pop_long(emu);
            uint32_t arg2 = pop_long(emu);
            retVal = ((uint32_t(*)(uint32_t, uint32_t))it->second.function)(arg1, arg2);
            break;
        }

        case 3:
        {
            uint32_t arg1 = pop_long(emu);
            uint32_t arg2 = pop_long(emu);
            uint32_t arg3 = pop_long(emu);
            retVal = ((uint32_t(*)(uint32_t, uint32_t, uint32_t))it->second.function)(arg1, arg2, arg3);
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

int has_lib_hook(x86emu_t* emu, uint32_t addr)
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
            retVal = ((uint32_t(*)())it->second.function)();
            break;

        case 1:
        {
            uint32_t arg1 = pop_long(emu);
            retVal = ((uint32_t(*)(uint32_t))it->second.function)(arg1);
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
            retVal = ((uint32_t(*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))it->second.function)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
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
}*/

namespace compat
{
    void* malloc(size_t size)
    {
        return ta_alloc(size);
    }

    void free(void* ptr)
    {
        ta_free(ptr);
    }

    void* realloc(void* ptr, size_t size)
    {
        ta_free(ptr);
        return ta_alloc(size);
    }
}
#endif
