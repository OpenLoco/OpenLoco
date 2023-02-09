#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    loco_global<uint16_t, 0x00508F12> _screenAge;
    loco_global<ScreenFlags, 0x00508F14> _screenFlags;
    loco_global<uint8_t, 0x00508F17> _pausedState;
    loco_global<GameSpeed, 0x00508F1A> _gameSpeed;

    void resetScreenAge()
    {
        _screenAge = 0;
    }

    uint16_t getScreenAge()
    {
        return _screenAge;
    }

    void setScreenAge(uint16_t newAge)
    {
        _screenAge = newAge;
    }

    ScreenFlags getScreenFlags()
    {
        return _screenFlags;
    }

    void setAllScreenFlags(ScreenFlags newScreenFlags)
    {
        _screenFlags = newScreenFlags;
    }

    void setScreenFlag(ScreenFlags value)
    {
        *_screenFlags |= value;
    }

    void clearScreenFlag(ScreenFlags value)
    {
        *_screenFlags &= ~value;
    }

    bool isEditorMode()
    {
        return (getScreenFlags() & ScreenFlags::editor) != ScreenFlags::none;
    }

    bool isTitleMode()
    {
        return (getScreenFlags() & ScreenFlags::title) != ScreenFlags::none;
    }

    bool isNetworked()
    {
        return (getScreenFlags() & ScreenFlags::networked) != ScreenFlags::none;
    }

    bool isNetworkHost()
    {
        return (getScreenFlags() & ScreenFlags::networkHost) != ScreenFlags::none;
    }

    bool isProgressBarActive()
    {
        return (getScreenFlags() & ScreenFlags::progressBarActive) != ScreenFlags::none;
    }

    bool isInitialised()
    {
        return (getScreenFlags() & ScreenFlags::initialised) != ScreenFlags::none;
    }

    bool isDriverCheatEnabled()
    {
        return (getScreenFlags() & ScreenFlags::driverCheatEnabled) != ScreenFlags::none;
    }

    bool isSandboxMode()
    {
        return (getScreenFlags() & ScreenFlags::sandboxMode) != ScreenFlags::none;
    }

    bool isPauseOverrideEnabled()
    {
        return (getScreenFlags() & ScreenFlags::pauseOverrideEnabled) != ScreenFlags::none;
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
