#include <cstdint>
#include <iostream>
#include <vector>
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <sys/mman.h>
#endif // _WIN32
#include "interop.hpp"

namespace openloco::interop
{
    static void * _hookTableAddress;
    static int32_t _hookTableOffset;
    static int32_t _maxHooks = 1000;
    constexpr auto HOOK_BYTE_COUNT = 140;

    static registers _hookRegisters;

    static void write_memory(uint32_t address, const void * data, size_t size);

    // This macro writes a little-endian 4-byte long value into *data
    // It is used to avoid type punning.
    #define write_address_strictalias(data, addr) \
        *(data + 0) = ((addr) & 0x000000ff) >> 0; \
        *(data + 1) = ((addr) & 0x0000ff00) >> 8; \
        *(data + 2) = ((addr) & 0x00ff0000) >> 16; \
        *(data + 3) = ((addr) & 0xff000000) >> 24;

    static void hookfunc(uintptr_t address, uintptr_t hookAddress, int32_t stacksize)
    {
        int32_t i = 0;
        uint8_t data[HOOK_BYTE_COUNT] = { 0 };

        uintptr_t registerAddress = (uintptr_t)&_hookRegisters;

        data[i++] = 0x89; // mov [_hookRegisters], eax
        data[i++] = (0b000 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress);
        i += 4;

        data[i++] = 0x89; // mov [_hookRegisters + 4], ebx
        data[i++] = (0b011 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 4);
        i += 4;

        data[i++] = 0x89; // mov [_hookRegisters + 8], ecx
        data[i++] = (0b001 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 8);
        i += 4;

        data[i++] = 0x89; // mov [_hookRegisters + 12], edx
        data[i++] = (0b010 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 12);
        i += 4;

        data[i++] = 0x89; // mov [_hookRegisters + 16], esi
        data[i++] = (0b110 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 16);
        i += 4;

        data[i++] = 0x89; // mov [_hookRegisters + 20], edi
        data[i++] = (0b111 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 20);
        i += 4;

        data[i++] = 0x89; // mov [_hookRegisters + 24], ebp
        data[i++] = (0b101 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 24);
        i += 4;

        // work out distance to nearest 0xC
        // (esp - numargs * 4) & 0xC
        // move to align - 4
        // save that amount

        // push the registers to be on the stack to access as arguments
        data[i++] = 0x68; // push _hookRegisters
        write_address_strictalias(&data[i], registerAddress);
        i += 4;

        data[i++] = 0xE8; // call

        write_address_strictalias(&data[i], hookAddress - address - i - 4);
        i += 4;

        data[i++] = 0x83; // add esp, 4
        data[i++] = 0xC4;
        data[i++] = 0x04;

        data[i++] = 0x25; // and eax,0xff
        data[i++] = 0xff;
        data[i++] = 0x00;
        data[i++] = 0x00;
        data[i++] = 0x00;
        data[i++] = 0xc1; // shl eax, 8
        data[i++] = 0xe0;
        data[i++] = 0x08;
        data[i++] = 0x9e; // sahf
        data[i++] = 0x9c; // pushf

        data[i++] = 0x8B; // mov eax, [_hookRegisters]
        data[i++] = (0b000 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress);
        i += 4;

        data[i++] = 0x8B; // mov ebx, [_hookRegisters + 4]
        data[i++] = (0b011 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 4);
        i += 4;

        data[i++] = 0x8B; // mov ecx, [_hookRegisters + 8]
        data[i++] = (0b001 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 8);
        i += 4;

        data[i++] = 0x8B; // mov edx, [_hookRegisters + 12]
        data[i++] = (0b010 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 12);
        i += 4;

        data[i++] = 0x8B; // mov esi, [_hookRegisters + 16]
        data[i++] = (0b110 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 16);
        i += 4;

        data[i++] = 0x8B; // mov edi, [_hookRegisters + 20]
        data[i++] = (0b111 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 20);
        i += 4;

        data[i++] = 0x8B; // mov ebp, [_hookRegisters + 24]
        data[i++] = (0b101 << 3) | 0b101;
        write_address_strictalias(&data[i], registerAddress + 24);
        i += 4;

        data[i++] = 0x9d; // popf

        data[i++] = 0xC3; // retn

    #ifdef _WIN32
        WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, data, i, 0);
    #else
                          // We own the pages with PROT_WRITE | PROT_EXEC, we can simply just memcpy the data
        memcpy((void *)address, data, i);
    #endif // _WIN32
    }

    void register_hook(uintptr_t address, hook_function function)
    {
        if (!_hookTableAddress) {
            size_t size = _maxHooks * HOOK_BYTE_COUNT;
    #ifdef _WIN32
            _hookTableAddress = VirtualAllocEx(GetCurrentProcess(), NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    #else
            _hookTableAddress = mmap(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (_hookTableAddress == MAP_FAILED)
            {
                perror("mmap");
                exit(1);
            }
    #endif // _WIN32
        }
        if (_hookTableOffset > _maxHooks) {
            return;
        }
        uint32_t hookaddress = (uint32_t)_hookTableAddress + (_hookTableOffset * HOOK_BYTE_COUNT);
        uint8_t data[9];
        int32_t i = 0;
        data[i++] = 0xE9; // jmp

        write_address_strictalias(&data[i], hookaddress - address - i - 4);
        i += 4;

        data[i++] = 0xC3; // retn

        write_memory(address, data, i);


        hookfunc(hookaddress, (uintptr_t)function, 0);
        _hookTableOffset++;
    }

    static void write_memory(uint32_t address, const void * data, size_t size)
    {
#ifdef _WIN32
        WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, data, i, 0);
#else
        // We own the pages with PROT_WRITE | PROT_EXEC, we can simply just memcpy the data
        int32_t err = mprotect((void *)0x401000, 0x4d7000 - 0x401000, PROT_READ | PROT_WRITE);
        if (err != 0)
        {
            perror("mprotect");
        }

        memcpy((void *)address, data,size);

        err = mprotect((void *)0x401000, 0x4d7000 - 0x401000, PROT_READ | PROT_EXEC);
        if (err != 0)
        {
            perror("mprotect");
        }
#endif // _WIN32
    }

    void write_ret(uint32_t address)
    {
        uint8_t opcode = 0xC3;
        write_memory(address, &opcode, sizeof(opcode));
    }

void
hook_stdcall(uint32_t address, void *fn)
{
    uint8_t data[5] = {0};
    data[0] = 0xE9; // JMP

    uintptr_t addr = reinterpret_cast<uintptr_t>(fn);

    write_address_strictalias(&data[1], addr-address - 5);

    write_memory(address, data, sizeof(data));
}

    void write_nop(uint32_t address, size_t count)
    {
        std::vector<uint8_t> buffer(count, 0x90);
        write_memory(address, buffer.data(), buffer.size());
    }
}
