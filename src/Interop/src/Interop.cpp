#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include "Interop.hpp"
#include <OpenLoco/Diagnostics/Logging.h>

#pragma warning(disable : 4731) // frame pointer register 'ebp' modified by inline assembly code
#define PLATFORM_X86

#if defined(__GNUC__)
#ifdef __clang__
#define DISABLE_OPT __attribute__((noinline, optnone))
#else
#define DISABLE_OPT __attribute__((noinline, optimize("O0")))
#endif // __clang__
#else
#define DISABLE_OPT
#endif // defined(__GNUC__)

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Interop
{
    // This variable serves a purpose of identifying a crash if it has happened inside original code.
    // When switching to original code, stack frame pointer is modified and prevents breakpad from providing stack trace.
    volatile int32_t _originalAddress = 0;

#ifdef _ENABLE_CALL_BYVALUE_
    static int32_t DISABLE_OPT callByVal(int32_t address, int32_t _eax, int32_t _ebx, int32_t _ecx, int32_t _edx, int32_t _esi, int32_t _edi, int32_t _ebp)
    {
        int32_t result = 0;
        _originalAddress = address;
#if defined(PLATFORM_X86)
#ifdef _MSC_VER
        // clang-format off
        __asm {
            push ebp
            push address
            mov eax, _eax
            mov ebx, _ebx
            mov ecx, _ecx
            mov edx, _edx
            mov esi, _esi
            mov edi, _edi
            mov ebp, _ebp
            call[esp]
            lahf
            pop ebp
            pop ebp
            /* Load result with flags */
            mov result, eax
        }
// clang-format on
#else
        // clang-format off
        __asm__ volatile ("\
        \n\
        push %%ebx \n\
        push %%ebp \n\
        push %[address] \n\
        mov %[eax], %%eax \n\
        mov %[ebx], %%ebx \n\
        mov %[ecx], %%ecx \n\
        mov %[edx], %%edx \n\
        mov %[esi], %%esi \n\
        mov %[edi], %%edi \n\
        mov %[ebp], %%ebp \n\
        call *(%%esp) \n\
        lahf \n\
        add $4, %%esp \n\
        pop %%ebp \n\
        pop %%ebx \n\
        /* Load result with flags */ \n\
        mov %%eax, %[result] \n\
        " : [address] "+m" (address), [eax] "+m" (_eax), [ebx] "+m" (_ebx), [ecx] "+m" (_ecx), [edx] "+m" (_edx), [esi] "+m" (_esi), [edi] "+m" (_edi), [ebp] "+m" (_ebp), [result] "+m" (result)
            :
            : "eax", "ecx", "edx", "esi", "edi", "memory"
            );
// clang-format on
#endif
#endif // PLATFORM_X86
        _originalAddress = 0;
        // lahf only modifies ah, zero out the rest
        return result & 0xFF00;
    }
#endif

    static int32_t DISABLE_OPT callByRef(int32_t address, int32_t* _eax, int32_t* _ebx, int32_t* _ecx, int32_t* _edx, int32_t* _esi, int32_t* _edi, int32_t* _ebp)
    {
#ifdef _LOG_INTEROP_CALLS_
        Logging::info("0x{:x}", address);
        Logging::incrementIntend();
#endif
        int32_t result = 0;
        _originalAddress = address;
#if defined(PLATFORM_X86)
#ifdef _MSC_VER
        // clang-format off
        __asm {
            // Store C's base pointer
            push ebp
            push ebx
            // Store address to call
            push address

            // Set all registers to the input values
            mov eax, [_eax]
            mov eax, [eax]
            mov ebx, [_ebx]
            mov ebx, [ebx]
            mov ecx, [_ecx]
            mov ecx, [ecx]
            mov edx, [_edx]
            mov edx, [edx]
            mov esi, [_esi]
            mov esi, [esi]
            mov edi, [_edi]
            mov edi, [edi]
            mov ebp, [_ebp]
            mov ebp, [ebp]

            // Call function
            call[esp]

            // Store output eax
            push eax
            push ebp
            push ebx
            mov ebp, [esp + 20]
            mov ebx, [esp + 16]

            // Get resulting ecx, edx, esi, edi registers

            mov eax, [_edi]
            mov[eax], edi
            mov eax, [_esi]
            mov[eax], esi
            mov eax, [_edx]
            mov[eax], edx
            mov eax, [_ecx]
            mov[eax], ecx

            // Pop ebx reg into ecx
            pop ecx
            mov eax, [_ebx]
            mov[eax], ecx

            // Pop ebp reg into ecx
            pop ecx
            mov eax, [_ebp]
            mov[eax], ecx

            pop eax
            // Get resulting eax register
            mov ecx, [_eax]
            mov[ecx], eax

            // Save flags as return in eax
            lahf
            // Pop address
            pop ebp

            pop ebx
            pop ebp
            /* Load result with flags */
            mov result, eax
        }
// clang-format on
#else
        // clang-format off
        __asm__ volatile ("\
        \n\
        /* Store C's base pointer*/     \n\
        push %%ebp        \n\
        push %%ebx        \n\
        \n\
        /* Store %[address] to call*/   \n\
        push %[address]         \n\
        \n\
        /* Set all registers to the input values*/      \n\
        mov %[_eax], %%eax      \n\
        mov (%%eax), %%eax  \n\
        mov %[_ebx], %%ebx      \n\
        mov (%%ebx), %%ebx  \n\
        mov %[_ecx], %%ecx      \n\
        mov (%%ecx), %%ecx  \n\
        mov %[_edx], %%edx      \n\
        mov (%%edx), %%edx  \n\
        mov %[_esi], %%esi      \n\
        mov (%%esi), %%esi  \n\
        mov %[_edi], %%edi      \n\
        mov (%%edi), %%edi  \n\
        mov %[_ebp], %%ebp      \n\
        mov (%%ebp), %%ebp  \n\
        \n\
        /* Call function*/      \n\
        call *(%%esp)      \n\
        \n\
        /* Store output eax */ \n\
        push %%eax \n\
        push %%ebp \n\
        push %%ebx \n\
        mov 20(%%esp), %%ebp \n\
        mov 16(%%esp), %%ebx \n\
        /* Get resulting ecx, edx, esi, edi registers*/       \n\
        mov %[_edi], %%eax      \n\
        mov %%edi, (%%eax)  \n\
        mov %[_esi], %%eax      \n\
        mov %%esi, (%%eax)  \n\
        mov %[_edx], %%eax      \n\
        mov %%edx, (%%eax)  \n\
        mov %[_ecx], %%eax      \n\
        mov %%ecx, (%%eax)  \n\
        /* Pop ebx reg into ecx*/ \n\
        pop %%ecx\n\
        mov %[_ebx], %%eax \n\
        mov %%ecx, (%%eax) \n\
        \n\
        /* Pop ebp reg into ecx */\n\
        pop %%ecx \n\
        mov %[_ebp], %%eax \n\
        mov %%ecx, (%%eax) \n\
        \n\
        pop %%eax \n\
        /* Get resulting eax register*/ \n\
        mov %[_eax], %%ecx \n\
        mov %%eax, (%%ecx) \n\
        \n\
        /* Save flags as return in eax*/  \n\
        lahf \n\
        /* Pop address*/ \n\
        pop %%ebp \n\
        \n\
        pop %%ebx \n\
        pop %%ebp \n\
        /* Load result with flags */ \n\
        mov %%eax, %[result] \n\
        " : [address] "+m" (address), [_eax] "+m" (_eax), [_ebx] "+m" (_ebx), [_ecx] "+m" (_ecx), [_edx] "+m" (_edx), [_esi] "+m" (_esi), [_edi] "+m" (_edi), [_ebp] "+m" (_ebp), [result] "+m" (result)

            :
            : "eax", "ecx", "edx", "esi", "edi", "memory"
            );
// clang-format on
#endif
#endif // PLATFORM_X86
        _originalAddress = 0;

#ifdef _LOG_INTEROP_CALLS_
        Logging::decrementIntend();
#endif
        // lahf only modifies ah, zero out the rest
        return (result >> 8) & 0xFF;
    }

#ifdef _ENABLE_CALL_BYVALUE_
    static int32_t callByVal(int32_t address, const registers& registers)
    {
        return callByVal(
            address,
            registers.eax,
            registers.ebx,
            registers.ecx,
            registers.edx,
            registers.esi,
            registers.edi,
            registers.ebp);
    }
#endif

    int32_t call(int32_t address)
    {
        registers regs;
        return call(address, regs);
    }

    int32_t call(int32_t address, registers& registers)
    {
        return callByRef(
            address,
            &registers.eax,
            &registers.ebx,
            &registers.ecx,
            &registers.edx,
            &registers.esi,
            &registers.edi,
            &registers.ebp);
    }

    void readMemory(uint32_t address, void* data, size_t size)
    {
#ifdef _WIN32
        if (!ReadProcessMemory(GetCurrentProcess(), (LPVOID)address, data, size, nullptr))
        {
            const auto errCode = static_cast<uint32_t>(GetLastError());
            fprintf(stderr, "ReadProcessMemory failed! address = 0x%08x, size = %zu, GetLastError() = 0x%08x\n", address, size, errCode);
            throw std::runtime_error("ReadProcessMemory failed");
        }
#else
        // We own the pages with PROT_WRITE | PROT_EXEC, we can simply just memcpy the data
        std::memcpy(data, (void*)address, size);
#endif // _WIN32
    }

    void writeMemory(uint32_t address, const void* data, size_t size)
    {
#ifdef _WIN32
        if (!WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, data, size, nullptr))
        {
            const auto errCode = static_cast<uint32_t>(GetLastError());
            fprintf(stderr, "WriteProcessMemory failed! address = 0x%08x, size = %zu, GetLastError() = 0x%08x\n", address, size, errCode);
            throw std::runtime_error("WriteProcessMemory failed");
        }
#else
        // We own the pages with PROT_WRITE | PROT_EXEC, we can simply just memcpy the data
        std::memcpy((void*)address, data, size);
#endif // _WIN32
    }

    save_state::save_state(uintptr_t begin, uintptr_t end)
        : begin(begin)
        , end(end)
    {
        state.resize(end - begin);
        readMemory(begin, state.data(), state.size());
    }

    void save_state::reset()
    {
        Interop::writeMemory(begin, state.data(), state.size());
    }

    void save_state::logDiff(const save_state& lhs, const save_state& rhs)
    {
        // TODO should we allow different base addresses?
        //      if so then we need to do extra work for that.
        auto length = std::min(lhs.state.size(), rhs.state.size());
        for (size_t i = 0; i < length; i++)
        {
            auto left = lhs.state[i];
            auto right = rhs.state[i];
            if (left != right)
            {
                uint32_t addr = lhs.begin + i;
                std::printf("0x%06" PRIX32 ": %02" PRIX8 "  %02" PRIX8 "\n", addr, (uint8_t)left, (uint8_t)right);
            }
        }
    }

    bool operator==(const save_state& lhs, const save_state& rhs)
    {
        return std::equal(
            lhs.getState().begin(),
            lhs.getState().end(),
            rhs.getState().begin(),
            rhs.getState().end());
    }

    bool operator!=(const save_state& lhs, const save_state& rhs)
    {
        return !(lhs == rhs);
    }
}
