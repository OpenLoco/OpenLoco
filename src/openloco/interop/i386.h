#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unicorn/unicorn.h>

constexpr int32_t DEFAULT_REG_VAL = 0xCCCCCCCC;

#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")

namespace openloco::interop
{
    void emu_init();

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

    void register_hook(uint32_t address, hook_function function);
    void write_ret(uint32_t address);
    void write_jmp(uint32_t address, void* fn);
    void write_nop(uint32_t address, size_t count);
    void hook_dump(uint32_t address, void* fn);
    void hook_lib(uint32_t address, void* fn);

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
