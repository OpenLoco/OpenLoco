#pragma once
#include <cstdint>

namespace OpenLoco
{
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

    enum class GameSpeed : uint8_t
    {
        Normal = 0,
        FastForward = 1,
        ExtraFastForward = 2,
        MAX = ExtraFastForward,
    };

    void resetScreenAge();
    uint16_t getScreenAge();
    void setScreenAge(uint16_t newAge);
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
    GameSpeed getGameSpeed();
    void setGameSpeed(const GameSpeed speed);
}
