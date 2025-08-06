#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::SceneManager
{
    loco_global<uint16_t, 0x00508F12> _sceneAge;
    loco_global<Flags, 0x00508F14> _sceneFlags;
    loco_global<PauseState, 0x00508F17> _pausedState;
    loco_global<GameSpeed, 0x00508F1A> _gameSpeed;

    void resetSceneAge()
    {
        _sceneAge = 0;
    }

    uint16_t getSceneAge()
    {
        return _sceneAge;
    }

    void setSceneAge(uint16_t newAge)
    {
        _sceneAge = newAge;
    }

    Flags getSceneFlags()
    {
        return _sceneFlags;
    }

    void setSceneFlags(Flags value)
    {
        _sceneFlags = value;
    }

    void addSceneFlags(Flags value)
    {
        *_sceneFlags |= value;
    }

    void removeSceneFlags(Flags value)
    {
        *_sceneFlags &= ~value;
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

    bool isPlayMode()
    {
        return isSceneInitialised() && !isEditorMode() && !isTitleMode();
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

    bool isSceneInitialised()
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
        return _pausedState != PauseState::none;
    }

    PauseState getPauseState()
    {
        return _pausedState;
    }

    void setPauseState(PauseState value)
    {
        if (_pausedState == PauseState::none)
        {
            _pausedState = value;
        }
    }

    void unsetPauseState(PauseState value)
    {
        if (_pausedState == value)
        {
            _pausedState = PauseState::none;
        }
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
