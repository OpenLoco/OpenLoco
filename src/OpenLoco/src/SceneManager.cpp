#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::SceneManager
{
    loco_global<uint16_t, 0x00508F12> _screenAge;
    loco_global<Flags, 0x00508F14> _screenFlags;
    loco_global<uint8_t, 0x00508F17> _pausedState;
    loco_global<GameSpeed, 0x00508F1A> _gameSpeed;

    void resetSceneAge()
    {
        _screenAge = 0;
    }

    uint16_t getSceneAge()
    {
        return _screenAge;
    }

    void setSceneAge(uint16_t newAge)
    {
        _screenAge = newAge;
    }

    Flags getSceneFlags()
    {
        return _screenFlags;
    }

    void setSceneFlags(Flags value)
    {
        _screenFlags = value;
    }

    void addSceneFlags(Flags value)
    {
        *_screenFlags |= value;
    }

    void removeSceneFlags(Flags value)
    {
        *_screenFlags &= ~value;
    }

    static inline bool hasSceneFlags(Flags value)
    {
        return (getSceneFlags() & value) != Flags::none;
    }

    bool isEditorMode()
    {
        return hasSceneFlags(Flags::editor);
    }

    bool isTitleMode()
    {
        return hasSceneFlags(Flags::title);
    }

    bool isNetworked()
    {
        return hasSceneFlags(Flags::networked);
    }

    bool isNetworkHost()
    {
        return hasSceneFlags(Flags::networkHost);
    }

    bool isProgressBarActive()
    {
        return hasSceneFlags(Flags::progressBarActive);
    }

    bool isInitialised()
    {
        return hasSceneFlags(Flags::initialised);
    }

    bool isDriverCheatEnabled()
    {
        return hasSceneFlags(Flags::driverCheatEnabled);
    }

    bool isSandboxMode()
    {
        return hasSceneFlags(Flags::sandboxMode);
    }

    bool isPauseOverrideEnabled()
    {
        return hasSceneFlags(Flags::pauseOverrideEnabled);
    }

    bool isPaused()
    {
        return _pausedState != 0;
    }

    uint8_t getPauseFlags()
    {
        return _pausedState;
    }

    void setPauseFlag(uint8_t value)
    {
        *_pausedState |= value;
    }

    void unsetPauseFlag(uint8_t value)
    {
        *_pausedState &= ~(value);
    }

    GameSpeed getGameSpeed()
    {
        return _gameSpeed;
    }

    // 0x00439A70 (speed: 0)
    // 0x00439A93 (speed: 1)
    // 0x00439AB6 (speed: 2)
    void setGameSpeed(const GameSpeed speed)
    {
        assert(speed <= GameSpeed::MAX);
        if (_gameSpeed != speed)
        {
            _gameSpeed = speed;
            Ui::WindowManager::invalidate(Ui::WindowType::timeToolbar);
        }
    }
}
