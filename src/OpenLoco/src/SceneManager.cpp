#include "SceneManager.h"
#include "Audio/Audio.h"
#include "Logging.h"
#include "S5/S5.h"
#include "Scenes/BootScene.h"
#include "Scenes/EditorScene.h"
#include "Scenes/GameplayScene.h"
#include "Scenes/IntroScene.h"
#include "Scenes/TitleScene.h"
#include "Ui/WindowManager.h"
#include <optional>

using namespace OpenLoco::Diagnostics;
using namespace OpenLoco::Scenes;

namespace OpenLoco::SceneManager
{
    static uint16_t _sceneAge;      // 0x00508F12
    static Flags _sceneFlags;       // 0x00508F14
    static PauseFlags _pausedState; // 0x00508F17
    static GameSpeed _gameSpeed;    // 0x00508F1A

    static SceneId _currentScene = SceneId::boot;
    static std::optional<SceneId> _pendingScene;

    struct SceneLoad
    {
        fs::path path;
        S5::LoadFlags flags;
    };

    static std::optional<SceneLoad> _pendingLoad;

    static constexpr const char* sceneName(SceneId scene)
    {
        switch (scene)
        {
            case SceneId::boot:
                return "boot";
            case SceneId::intro:
                return "intro";
            case SceneId::title:
                return "title";
            case SceneId::gameplay:
                return "gameplay";
            case SceneId::editor:
                return "editor";
        }
        return "unknown";
    }

    SceneId getCurrentScene()
    {
        return _currentScene;
    }

    SceneId getPendingScene()
    {
        return _pendingScene.value_or(_currentScene);
    }

    bool isSceneTransitionPending()
    {
        return _pendingScene.has_value();
    }

    // Requests a scene change, this is deferred until the current tick has fully unwound.
    void requestScene(SceneId scene)
    {
        _pendingScene = scene;
    }

    void requestSceneLoad(SceneId scene, const fs::path& path, S5::LoadFlags flags)
    {
        _pendingLoad = SceneLoad{ path, flags };

        requestScene(scene);
    }

    static void applyPendingScene()
    {
        const auto previousScene = _currentScene;
        _currentScene = *_pendingScene;
        _pendingScene.reset();

        Logging::info("Scene transition: {} -> {}", sceneName(previousScene), sceneName(_currentScene));
    }

    static void enterScene(SceneId scene)
    {
        switch (scene)
        {
            case SceneId::boot:
                break;
            case SceneId::intro:
                IntroScene::onEnter();
                break;
            case SceneId::title:
                TitleScene::onEnter();
                break;
            case SceneId::gameplay:
                break;
            case SceneId::editor:
                break;
        }
    }

    static void exitScene(SceneId scene)
    {
        switch (scene)
        {
            case SceneId::boot:
                break;
            case SceneId::intro:
                IntroScene::onExit();
                break;
            case SceneId::title:
                break;
            case SceneId::gameplay:
                break;
            case SceneId::editor:
                break;
        }
    }

    static bool applyPendingLoad()
    {
        if (!_pendingLoad.has_value())
        {
            return true;
        }

        const auto load = *_pendingLoad;
        _pendingLoad.reset();

        if (!S5::importSaveToGameState(load.path, load.flags))
        {
            Logging::error("Failed to load '{}', returning to title", load.path.u8string());
            requestScene(SceneId::title);
            return false;
        }

        return true;
    }

    // Returns true if the current scene was changed.
    bool applySceneTransition()
    {
        if (!isSceneTransitionPending())
        {
            return false;
        }

        exitScene(_currentScene);
        applyPendingScene();

        if (!applyPendingLoad())
        {
            return true;
        }

        enterScene(_currentScene);

        return true;
    }

    void tick()
    {
        switch (_currentScene)
        {
            case SceneId::boot:
                BootScene::tick();
                break;
            case SceneId::intro:
                IntroScene::tick();
                break;
            case SceneId::title:
                TitleScene::tick();
                break;
            case SceneId::gameplay:
                GameplayScene::tick();
                break;
            case SceneId::editor:
                EditorScene::tick();
                break;
        }
    }

    void tickInterface()
    {
        switch (_currentScene)
        {
            case SceneId::boot:
                BootScene::tickInterface();
                break;
            case SceneId::intro:
                IntroScene::tickInterface();
                break;
            case SceneId::title:
                TitleScene::tickInterface();
                break;
            case SceneId::gameplay:
                GameplayScene::tickInterface();
                break;
            case SceneId::editor:
                EditorScene::tickInterface();
                break;
        }
    }

    void update()
    {
        switch (_currentScene)
        {
            case SceneId::boot:
                BootScene::update();
                break;
            case SceneId::intro:
                IntroScene::update();
                break;
            case SceneId::title:
                TitleScene::update();
                break;
            case SceneId::gameplay:
                GameplayScene::update();
                break;
            case SceneId::editor:
                EditorScene::update();
                break;
        }
    }

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
        Ui::Windows::TimePanel::invalidateFrame();
    }

    static void onUnpause()
    {
        Audio::unpauseSound();
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
