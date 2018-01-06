#pragma once

#include <cstdint>

#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")

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

#ifdef USE_MMAP
    #if defined(PLATFORM_64BIT)
        #define GOOD_PLACE_FOR_DATA_SEGMENT ((uintptr_t)0x200000000)
    #elif defined(PLATFORM_32BIT)
        #define GOOD_PLACE_FOR_DATA_SEGMENT ((uintptr_t)0x09000000)
    #else
        #error "Unknown platform"
    #endif
#else
    #define GOOD_PLACE_FOR_DATA_SEGMENT ((uintptr_t)0x8A4000)
#endif

#define LOCO_ADDRESS(address, type)     ((type*)(GOOD_PLACE_FOR_DATA_SEGMENT - 0x8A4000 + (address)))
#define LOCO_GLOBAL(address, type)      (*((type*)(GOOD_PLACE_FOR_DATA_SEGMENT - 0x8A4000 + (address))))

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
int32_t LOCO_CALLPROC_X(int32_t address, int32_t _eax, int32_t _ebx, int32_t _ecx, int32_t _edx, int32_t _esi, int32_t _edi, int32_t _ebp);
int32_t LOCO_CALLPROC_X(int32_t address, const registers &registers);
int32_t LOCO_CALLPROC_X(int32_t address);

/**
 * Returns the flags register
 *
 * Flags register is as follows:
 * 0bSZ0A_0P0C_0000_00000
 * S = Signed flag
 * Z = Zero flag
 * C = Carry flag
 * A = Adjust flag
 * P = Parity flag
 * All other bits are undefined.
 */
int32_t LOCO_CALLFUNC_X(int32_t address, int32_t *_eax, int32_t *_ebx, int32_t *_ecx, int32_t *_edx, int32_t *_esi, int32_t *_edi, int32_t *_ebp);
int32_t LOCO_CALLFUNC_X(int32_t address, registers &registers);

template<typename T, uint32_t TAddress>
struct loco_global
{
    void operator=(T rhs)
    {
        LOCO_GLOBAL(TAddress, T) = rhs;
    }

    operator T()
    {
        return LOCO_GLOBAL(TAddress, T);
    }
};
