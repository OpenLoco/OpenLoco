#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unicorn/unicorn.h>

//#if defined(__i386__)
#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")
//#else
//#define assert_struct_size(x, y)
//#endif

constexpr int32_t DEFAULT_REG_VAL = 0xCCCCCCCC;

namespace OpenLoco::Interop
{
#pragma pack(push, 1)
    /**
    * x86 register structure, only used for easy interop to Locomotion code.
    */
    struct registers
    {
        union
        {
            int32_t eax{ DEFAULT_REG_VAL };
            int16_t ax;
            struct
            {
                int8_t al;
                int8_t ah;
            };
        };
        union
        {
            int32_t ebx{ DEFAULT_REG_VAL };
            int16_t bx;
            struct
            {
                int8_t bl;
                int8_t bh;
            };
        };
        union
        {
            int32_t ecx{ DEFAULT_REG_VAL };
            int16_t cx;
            struct
            {
                int8_t cl;
                int8_t ch;
            };
        };
        union
        {
            int32_t edx{ DEFAULT_REG_VAL };
            int16_t dx;
            struct
            {
                int8_t dl;
                int8_t dh;
            };
        };
        union
        {
            int32_t esi{ DEFAULT_REG_VAL };
            int16_t si;
        };
        union
        {
            int32_t edi{ DEFAULT_REG_VAL };
            int16_t di;
        };
        union
        {
            int32_t ebp{ DEFAULT_REG_VAL };
            int16_t bp;
        };
    };
    assert_struct_size(registers, 7 * 4);
#pragma pack(pop)

    using hook_function = uint8_t (*)(registers& regs);

    void registerHook(uintptr_t address, hook_function function);
    void writeRet(uint32_t address);
    void writeJmp(uint32_t address, void* fn);
    void writeNop(uint32_t address, size_t count);
    void hookDump(uint32_t address, void* fn);
    void hookLib(uint32_t address, void* fn);

    void emu_init();
    void registerHooks();
    void loadSections();

    enum CallingConvention
    {
        cdecl,
        stdcall,
        sawyer,
    };

    void hookFunction(uint32_t address, CallingConvention convention, size_t arguments, void (*)());
    void hookLibrary(uint32_t address, CallingConvention convention, size_t arguments, void (*)());
    void hookLibrary(uint32_t address, std::function<void()>);
}
