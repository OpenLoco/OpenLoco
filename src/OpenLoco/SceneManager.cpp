#include "SceneManager.h"
#include "Interop/Interop.hpp"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    loco_global<uint16_t, 0x00508F12> _screenAge;
    loco_global<uint16_t, 0x00508F14> _screenFlags;
    loco_global<uint8_t, 0x00508F17> pausedState;
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

    uint16_t getScreenFlags()
    {
        return _screenFlags;
    }

    void setAllScreenFlags(uint16_t newScreenFlags)
    {
        _screenFlags = newScreenFlags;
    }

    void setScreenFlag(uint16_t value)
    {
        *_screenFlags |= value;
    }

    void clearScreenFlag(uint16_t value)
    {
        *_screenFlags &= ~value;
    }

    bool isEditorMode()
    {
        return (getScreenFlags() & ScreenFlags::editor) != 0;
    }

    bool isTitleMode()
    {
        return (getScreenFlags() & ScreenFlags::title) != 0;
    }

    bool isNetworked()
    {
        return (getScreenFlags() & ScreenFlags::networked) != 0;
    }

    bool isNetworkHost()
    {
        return (getScreenFlags() & ScreenFlags::networkHost) != 0;
    }

    bool isProgressBarActive()
    {
        return (getScreenFlags() & ScreenFlags::progressBarActive) != 0;
    }

    bool isInitialised()
    {
        return (getScreenFlags() & ScreenFlags::initialised) != 0;
    }

    bool isDriverCheatEnabled()
    {
        return (getScreenFlags() & ScreenFlags::driverCheatEnabled) != 0;
    }

    bool isSandboxMode()
    {
        return (getScreenFlags() & ScreenFlags::sandboxMode) != 0;
    }

    bool isPauseOverrideEnabled()
    {
        return (getScreenFlags() & ScreenFlags::pauseOverrideEnabled) != 0;
    }

    bool isPaused()
    {
        return pausedState != 0;
    }

    uint8_t getPauseFlags()
    {
        return pausedState;
    }

    void setPauseFlag(uint8_t value)
    {
        *pausedState |= value;
    }

    void unsetPauseFlag(uint8_t value)
    {
        *pausedState &= ~(value);
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
