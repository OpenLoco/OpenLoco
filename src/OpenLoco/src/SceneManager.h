#pragma once
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstdint>

namespace OpenLoco
{
    enum class ScreenFlags : uint16_t
    {
        none = 0U,
        title = 1U << 0,
        editor = 1U << 1,
        networked = 1U << 2,
        networkHost = 1U << 3,
        progressBarActive = 1U << 4,
        initialised = 1U << 5,
        driverCheatEnabled = 1U << 6,
        sandboxMode = 1U << 7,          // new in OpenLoco
        pauseOverrideEnabled = 1U << 8, // new in OpenLoco
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ScreenFlags);

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
    ScreenFlags getScreenFlags();
    void setAllScreenFlags(ScreenFlags newScreenFlags);
    void setScreenFlag(ScreenFlags value);
    void clearScreenFlag(ScreenFlags value);
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
