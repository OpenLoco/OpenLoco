#include "SceneManager.h"
#include "Interop/Interop.hpp"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    loco_global<uint16_t, 0x00508F12> _screen_age;
    loco_global<uint16_t, 0x00508F14> _screenFlags;
    loco_global<uint8_t, 0x00508F17> paused_state;
    loco_global<GameSpeed, 0x00508F1A> _gameSpeed;

    void resetScreenAge()
    {
        _screen_age = 0;
    }

    uint16_t getScreenAge()
    {
        return _screen_age;
    }

    void setScreenAge(uint16_t newAge)
    {
        _screen_age = newAge;
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
        return paused_state != 0;
    }

    uint8_t getPauseFlags()
    {
        return paused_state;
    }

    void setPauseFlag(uint8_t value)
    {
        *paused_state |= value;
    }

    void unsetPauseFlag(uint8_t value)
    {
        *paused_state &= ~(value);
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
