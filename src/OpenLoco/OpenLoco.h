#pragma once

#include "Utility/Prng.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace OpenLoco
{
    using string_id = uint16_t;
    namespace ScreenFlags
    {
        constexpr uint16_t title = 1 << 0;
        constexpr uint16_t editor = 1 << 1;
        constexpr uint16_t networked = 1 << 2;
        constexpr uint16_t networkHost = 1 << 3;
        constexpr uint16_t unknown_4 = 1 << 4;
        constexpr uint16_t unknown_5 = 1 << 5;
        constexpr uint16_t driverCheatEnabled = 1 << 6;
    }

    extern const char version[];

    std::string getVersionInfo();

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    uint16_t getScreenAge();
    uint16_t getScreenFlags();
    void setAllScreenFlags(uint16_t newScreenFlags);
    void setScreenFlag(uint16_t value);
    void clearScreenFlag(uint16_t value);
    bool isEditorMode();
    bool isTitleMode();
    bool isNetworked();
    bool isNetworkHost();
    bool isUnknown4Mode();
    bool isUnknown5Mode();
    bool isPaused();
    uint8_t getPauseFlags();
    void togglePause(bool value);
    uint32_t scenarioTicks();
    Utility::prng& gPrng();
    void initialiseViewports();

    void sub_431695(uint16_t var_F253A0);
    void main();
    void promptTickLoop(std::function<bool()> tickAction);
    void exitWithError(OpenLoco::string_id message, uint32_t errorCode);
}
