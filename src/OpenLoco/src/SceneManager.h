#pragma once
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstdint>

namespace OpenLoco
{
    enum class GameSpeed : uint8_t
    {
        Normal = 0,
        FastForward = 1,
        ExtraFastForward = 2,
        MAX = ExtraFastForward,
    };

    namespace SceneManager
    {
        enum class Flags : uint16_t
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
        OPENLOCO_ENABLE_ENUM_OPERATORS(Flags);

        void resetSceneAge();
        uint16_t getSceneAge();
        void setSceneAge(uint16_t newAge);
        Flags getSceneFlags();
        void setSceneFlags(Flags value);
        void addSceneFlags(Flags value);
        void removeSceneFlags(Flags value);
        bool isEditorMode();
        bool isTitleMode();
        bool isPlayMode();
        bool isNetworked();
        bool isNetworkHost();
        bool isProgressBarActive();
        bool isSceneInitialised();
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

}
