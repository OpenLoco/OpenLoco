#pragma once

#include <cstdio>
#include <cstdint>

#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")

namespace openloco::interop
{
    /**
    * x86 register structure, only used for easy interop to Locomotion code.
    */
#pragma pack(push, 1)
    struct registers
    {
        union
        {
            int32_t eax;
            int16_t ax;
            struct
            {
                int8_t al;
                int8_t ah;
            };
        };
        union
        {
            int32_t ebx;
            int16_t bx;
            struct
            {
                int8_t bl;
                int8_t bh;
            };
        };
        union
        {
            int32_t ecx;
            int16_t cx;
            struct
            {
                int8_t cl;
                int8_t ch;
            };
        };
        union
        {
            int32_t edx;
            int16_t dx;
            struct
            {
                int8_t dl;
                int8_t dh;
            };
        };
        union
        {
            int32_t esi;
            int16_t si;
        };
        union
        {
            int32_t edi;
            int16_t di;
        };
        union
        {
            int32_t ebp;
            int16_t bp;
        };

        registers();
    };
    assert_struct_size(registers, 7 * 4);
#pragma pack(pop)

    uintptr_t remap_address(uintptr_t locoAddress);

    template<uint32_t TAddress, typename T>
    T& addr()
    {
        return *((T *)remap_address(TAddress));
    }

    /**
    * Returns the flags register
    *
    * Flags register is as follows:
    * 0bSZ0A_0P0C_0000_0000
    * S = Signed flag
    * Z = Zero flag
    * C = Carry flag
    * A = Adjust flag
    * P = Parity flag
    * All other bits are undefined.
    */
    int32_t call(int32_t address);
    int32_t call(int32_t address, registers &registers);

    template<typename T, uint32_t TAddress>
    struct loco_global
    {
        void operator=(T rhs)
        {
            addr<TAddress, T>() = rhs;
        }

        operator T&()
        {
            return addr<TAddress, T>();
        }

        T& operator *()
        {
            return addr<TAddress, T>();
        }

        T* operator ->()
        {
            return &(addr<TAddress, T>());
        }
    };

    template<typename T, size_t TSize, uint32_t TAddress>
    struct loco_global_array
    {
        operator T*()
        {
            return get();
        }

        T* get()
        {
            return &(addr<TAddress, T>());
        }

        size_t size()
        {
            return TSize;
        }
    };

    enum
    {
        X86_FLAG_CARRY = 1 << 0,
        X86_FLAG_PARITY = 1 << 2,
        X86_FLAG_ADJUST = 1 << 4,
        X86_FLAG_ZERO = 1 << 6,
        X86_FLAG_SIGN = 1 << 7,
    };

    using hook_function = uint8_t(*)(registers &regs);

    void register_hook(uintptr_t address, hook_function function);
    void write_ret(uint32_t address);
    void write_nop(uint32_t address, size_t count);

    void register_hooks();
    void hook_stdcall(uint32_t address, void *fn);
}
