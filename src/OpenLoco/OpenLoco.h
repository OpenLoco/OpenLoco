#pragma once

#include "Utility/Prng.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace OpenLoco
{
    using string_id = uint16_t;

    namespace Engine
    {
        constexpr uint32_t MaxTimeDeltaMs = 500;
        constexpr uint32_t UpdateRateHz = 40;
        constexpr uint32_t UpdateRateInMs = 1000 / UpdateRateHz;
        constexpr uint32_t MaxUpdates = 3;
    }

    namespace ScreenFlags
    {
        constexpr uint16_t title = 1 << 0;
        constexpr uint16_t editor = 1 << 1;
        constexpr uint16_t networked = 1 << 2;
        constexpr uint16_t networkHost = 1 << 3;
        constexpr uint16_t progressBarActive = 1 << 4;
        constexpr uint16_t initialised = 1 << 5;
        constexpr uint16_t driverCheatEnabled = 1 << 6;
        constexpr uint16_t sandboxMode = 1 << 7;          // new in OpenLoco
        constexpr uint16_t pauseOverrideEnabled = 1 << 8; // new in OpenLoco
    }

#pragma pack(push, 1)
    struct loco_ptr
    {
        uintptr_t _ptr;
        loco_ptr(const void* x)
        {
            _ptr = reinterpret_cast<uintptr_t>(x);
        }
        loco_ptr(int32_t x)
        {
            _ptr = x;
        }
        operator int32_t() const
        {
            assert(_ptr < UINT32_MAX);
            return (int32_t)_ptr;
        }
        operator uint32_t() const
        {
            assert(_ptr < UINT32_MAX);
            return (uint32_t)_ptr;
        }
#ifndef __i386__
        operator uintptr_t() const
        {
            return _ptr;
        }
#endif
    };
#pragma pack(pop)

    template<typename T>
    class loco_ptr4
    {
    public:
        uint32_t ptr;
        loco_ptr4(uint32_t ptr)
            : ptr(ptr)
        {
        }
        loco_ptr4(T* ptr = nullptr)
        {
            assert((uintptr_t)ptr < UINT32_MAX);
            this->ptr = static_cast<uint32_t>((uintptr_t)ptr);
        }

        T* get()
        {
            return (T*)(uintptr_t)ptr;
        }

        T* get() const
        {
            return (T*)(uintptr_t)ptr;
        }

        T* operator*() const
        {
            return this->get();
        }
    };

    template<typename T>
    class loco_ptr2
    {
    public:
        uint32_t ptr;
        loco_ptr2(uint32_t ptr)
            : ptr(ptr)
        {
        }
        loco_ptr2(T* ptr = nullptr)
        {
            assert((uintptr_t)ptr < UINT32_MAX);
            this->ptr = static_cast<uint32_t>((uintptr_t)ptr);
        }

        T* get()
        {
            return (T*)(uintptr_t)ptr;
        }

        T* get() const
        {
            return (T*)(uintptr_t)ptr;
        }

        T* operator*() const
        {
            return this->get();
        }

        // Not allowed on void. No idea how enable_if is supposed to work.
        T& operator[](int index)
        {
            return get()[index];
        }
    };

    static_assert(sizeof(loco_ptr2<int>) == 4);

    extern const char version[];

    std::string getVersionInfo();

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    void resetScreenAge();
    uint16_t getScreenAge();
    uint16_t getScreenFlags();
    void setAllScreenFlags(uint16_t newScreenFlags);
    void setScreenFlag(uint16_t value);
    void clearScreenFlag(uint16_t value);
    bool isEditorMode();
    bool isTitleMode();
    bool isNetworked();
    bool isNetworkHost();
    bool isProgressBarActive();
    bool isInitialised();
    bool isDriverCheatEnabled();
    bool isSandboxMode();
    bool isPauseOverrideEnabled();
    bool isPaused();
    uint8_t getPauseFlags();
    void setPauseFlag(uint8_t value);
    void unsetPauseFlag(uint8_t value);
    uint8_t getGameSpeed();
    void setGameSpeed(uint8_t speed);
    uint32_t scenarioTicks();
    Utility::prng& gPrng();
    void initialiseViewports();

    void sub_431695(uint16_t var_F253A0);
    void main();
    void promptTickLoop(std::function<bool()> tickAction);
    [[noreturn]] void exitCleanly();
    void exitWithError(OpenLoco::string_id message, uint32_t errorCode);

    void sub_444387();
}

#ifndef __i386__
namespace compat
{
    void* malloc(size_t size);
    void free(void* ptr);
    void* realloc(void* ptr, size_t size);
}
#endif
