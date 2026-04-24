#include "SceneManager.h"
#include "Audio/Audio.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::SceneManager
{
    static uint16_t _sceneAge;      // 0x00508F12
    static Flags _sceneFlags;       // 0x00508F14
    static PauseFlags _pausedState; // 0x00508F17
    static GameSpeed _gameSpeed;    // 0x00508F1A

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
        _sceneFlags |= value;
    }

    void removeSceneFlags(Flags value)
    {
        _sceneFlags &= ~value;
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
        return _pausedState != PauseFlags::none;
    }

    PauseFlags getPauseFlags()
    {
        return _pausedState;
    }

    static void onPause()
    {
        Audio::pauseSound();
        // Do not pause title screen music
        if (!isTitleMode())
        {
            Audio::pauseMusic();
        }
        Ui::Windows::TimePanel::invalidateFrame();
    }

    static void onUnpause()
    {
        Audio::unpauseSound();
        Audio::unpauseMusic();
        Ui::Windows::TimePanel::invalidateFrame();
    }

    void setPauseFlag(PauseFlags value)
    {
        if (_pausedState == PauseFlags::none)
        {
            onPause();
        }
        _pausedState |= value;
    }

    void unsetPauseFlag(PauseFlags value)
    {
        assert(_pausedState != PauseFlags::none);
        _pausedState &= ~(value);
        if (_pausedState == PauseFlags::none)
        {
            onUnpause();
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
