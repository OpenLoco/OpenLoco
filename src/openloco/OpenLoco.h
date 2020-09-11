#pragma once

#include "Utility/prng.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace openloco
{
    using string_id = uint16_t;
    namespace screen_flags
    {
        constexpr uint8_t title = 1 << 0;
        constexpr uint8_t editor = 1 << 1;
        constexpr uint8_t networked = 1 << 2;
        constexpr uint8_t trackUpgrade = 1 << 3;
        constexpr uint8_t unknown_4 = 1 << 4;
        constexpr uint8_t unknown_5 = 1 << 5;
        constexpr uint8_t unknown_6 = 1 << 6;
    }

    extern const char version[];

    std::string getVersionInfo();

    void* hInstance();
    const char* lpCmdLine();
    void lpCmdLine(const char* path);
    uint16_t getScreenAge();
    uint8_t getScreenFlags();
    bool isEditorMode();
    bool isTitleMode();
    bool isNetworked();
    bool isTrackUpgradeMode();
    bool isUnknown4Mode();
    bool isUnknown5Mode();
    bool isPaused();
    uint8_t getPauseFlags();
    void togglePause(bool value);
    uint32_t scenarioTicks();
    utility::prng& gPrng();
    void initialiseViewports();

    void sub_431695(uint16_t var_F253A0);
    void main();
    void promptTickLoop(std::function<bool()> tickAction);
    void exitWithError(openloco::string_id message, uint32_t errorCode);
}
