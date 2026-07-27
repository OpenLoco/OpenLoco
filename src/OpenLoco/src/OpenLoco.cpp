#include "Scenario/Scenario.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
// timeGetTime is unavailable if we use lean and mean
// #define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <windows.h>

// `small` is used as a type in `windows.h`
#undef small
#endif

#include "Audio/Audio.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Entities/EntityTweener.h"
#include "Environment.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Gui.h"
#include "Input.h"
#include "Input/Shortcuts.h"
#include "Localisation/Formatting.h"
#include "Localisation/LanguageFiles.h"
#include "Localisation/Languages.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/TileManager.h"
#include "MessageManager.h"
#include "MultiPlayer.h"
#include "Network/Network.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Scenario/ScenarioManager.h"
#include "SceneManager.h"
#include "Scenes/BootScene.h"
#include "Scenes/GameScene.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "Ui/ToolTip.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Platform/Crash.h>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Version.hpp>

using namespace OpenLoco::Ui;
using namespace OpenLoco::Input;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco
{
    using Clock = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;

    static double _accumulator = 0.0;
    static Timepoint _lastUpdate = Clock::now();
    static CrashHandler::Handle _exHandler = nullptr;

    static uint32_t _time_since_last_tick; // 0x0050C19C
    static uint32_t _last_tick_time;       // 0x0050C19E
    static uint16_t _numFrameUpdates;      // 0x00F253A0

    // 0x004BE621
    [[noreturn]] void exitWithError(StringId titleStringId, StringId messageStringId)
    {
        char titleBuffer[256] = { 0 };
        char messageBuffer[256] = { 0 };
        StringManager::formatString(titleBuffer, 255, titleStringId);
        StringManager::formatString(messageBuffer, 255, messageStringId);
        Ui::showMessageBox(titleBuffer, messageBuffer);

        exitCleanly();
    }

    // 0x004BE65E
    [[noreturn]] void exitCleanly()
    {
        Audio::close();
        Audio::disposeDSound();
        Ui::disposeCursors();
        Localisation::unloadLanguageFile();

        auto tempFilePath = Environment::getPathNoWarning(Environment::PathId::_1tmp);
        if (fs::exists(tempFilePath))
        {
            auto path8 = tempFilePath.u8string();
            Logging::info("Removing temp file '{}'", path8.c_str());
            fs::remove(tempFilePath);
        }
        CrashHandler::shutdown(_exHandler);

        // Logging should be the last before terminating.
        Logging::shutdown();

        // SDL_Quit();
        exit(0);
    }

    // 0x00441400
    static void startupChecks()
    {
        const auto& config = Config::get();
        if (!config.allowMultipleInstances && !Platform::lockSingleInstance())
        {
            exitWithError(StringIds::game_init_failure, StringIds::loco_already_running);
        }

        // Originally the game would check that all the game
        // files exist are some have the correct checksum. We
        // do not need to do this anymore, the game should work
        // with g1 alone and some objects?
    }

    // 0x004C57C0
    void resetSubsystems()
    {
        Ui::Windows::MapToolTip::reset();

        Colours::initColourMap();
        Ui::WindowManager::init();
        Ui::ViewportManager::init();

        Input::init();
        Input::initMouse();

        // tooltip-related
        Ui::ToolTip::set_52336E(false);

        Ui::Windows::TextInput::cancel();

        // TODO Move this to a more generic, initialise game state function when
        //      we have one hooked / implemented.
        Scenes::GameScene::autosaveReset();
    }

    void initialise()
    {
        _last_tick_time = Platform::getTime();

        std::srand(std::time(nullptr));

        // Do this first since some shutdown logic might otherwise read bad data.
        EntityManager::reset();

        Localisation::enumerateLanguages();
        Localisation::loadLanguageFile();

        startupChecks();

        Input::Shortcuts::initialize();
        World::TileManager::allocateMapElements();

        Gfx::loadG1();
        Gfx::initialise();

        Ui::initialise();
        resetSubsystems();
        Gui::init();

        MessageManager::reset();
        Scenario::reset();

        ObjectManager::loadIndex();
        ScenarioManager::loadIndex();
    }

    void sub_431695(uint16_t var_F253A0)
    {
        GameCommands::setUpdatingCompanyId(CompanyManager::getControllingId());
        for (auto i = 0; i < var_F253A0; i++)
        {
            MessageManager::sub_428E47();
            WindowManager::dispatchUpdateAll();
        }

        Input::processKeyboardInput();
        WindowManager::tick();
        Ui::handleInput();
        CompanyManager::updateOwnerStatus();
    }

    // The remainder of a tick is abandoned when a scene transition was requested, the game
    // state it was operating on has been replaced by that point.
    static void applyPendingScene()
    {
        if (!SceneManager::applySceneTransition())
        {
            return;
        }

        EntityTweener::get().reset();
    }

    // Decides how many ticks the world is advanced by this fixed update.
    static uint32_t calculateNumTicks()
    {
        uint32_t numUpdates = std::clamp<uint32_t>(_time_since_last_tick / 31U, 1, 3);

        if (WindowManager::find(Ui::WindowType::multiplayer, 0) != nullptr)
        {
            numUpdates = 1;
        }
        if (SceneManager::isNetworked())
        {
            numUpdates = 1;
        }

        if (Input::hasPendingMouseInputUpdate())
        {
            Input::clearPendingMouseInputUpdate();
            numUpdates = 1;
        }
        else
        {
            switch (Input::state())
            {
                case State::reset:
                case State::normal:
                case State::dropdownActive:
                    if (Input::hasFlag(Flags::viewportScrolling))
                    {
                        Input::resetFlag(Flags::viewportScrolling);
                        numUpdates = 1;
                    }
                    break;
                case State::widgetPressed: break;
                case State::positioningWindow: break;
                case State::viewportRight: break;
                case State::viewportLeft: break;
                case State::scrollLeft: break;
                case State::resizing: break;
                case State::scrollRight: break;
            }
        }

        Ui::WindowManager::setVehiclePreviewRotationFrame(Ui::WindowManager::getVehiclePreviewRotationFrame() + numUpdates);

        if (SceneManager::isPaused())
        {
            numUpdates = 0;
        }

        _numFrameUpdates = std::max<uint16_t>(1, numUpdates);
        SceneManager::setSceneAge(std::min(0xFFFF, (int32_t)SceneManager::getSceneAge() + _numFrameUpdates));

        if (SceneManager::getGameSpeed() != GameSpeed::Normal)
        {
            numUpdates *= 3;
            if (SceneManager::getGameSpeed() != GameSpeed::FastForward)
            {
                numUpdates *= 3;
            }
        }

        // Catch up to server (usually after we have just joined the game)
        auto numTicksBehind = Network::getServerTick() - ScenarioManager::getScenarioTicks();
        if (numTicksBehind > 4)
        {
            numUpdates = 4;
        }

        return numUpdates;
    }

    // 0x0046A794
    static void tick()
    {
        uint32_t time = Platform::getTime();
        _time_since_last_tick = (uint16_t)std::min(time - _last_tick_time, 500U);
        _last_tick_time = time;

        if (Tutorial::state() != Tutorial::State::none)
        {
            _time_since_last_tick = 31;
        }

        GameCommands::resetCommandNestLevel();
        Ui::tick();

        // Original called 0x00440DEC here which handled legacy cmd line options
        // like installing scenarios and handling multiplayer.

        Input::handleKeyboard();
        Input::processMouseMovement();
        Audio::tick();

        // Network messages are handled outside of the scenes, they can request a scene transition.
        Network::tick();

        applyPendingScene();

        const auto numTicks = calculateNumTicks();
        for (auto i = 0U; i < numTicks; i++)
        {
            SceneManager::tick();
        }

        SceneManager::tickInterface();

        applyPendingScene();
    }

    static void tickWait()
    {
        // Idle loop for a 40 FPS
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (Platform::getTime() - _last_tick_time < Engine::UpdateRateInMs);
    }

    bool promptTickLoop(std::function<bool()> tickAction)
    {
        while (true)
        {
            _last_tick_time = Platform::getTime();
            _time_since_last_tick = 31;
            if (!Input::processMessages())
            {
                return false;
            }
            if (!tickAction())
            {
                break;
            }
            Ui::render();
            tickWait();
        }
        return true;
    }

    constexpr auto MaxUpdateTime = static_cast<double>(Engine::MaxTimeDeltaMs) / 1000.0;
    constexpr auto UpdateTime = static_cast<double>(Engine::UpdateRateInMs) / 1000.0;
    constexpr auto TimeScale = 1.0;

    static void variableUpdate()
    {
        auto& tweener = EntityTweener::get();

        const auto alpha = std::min<float>(_accumulator / UpdateTime, 1.0);

        while (_accumulator > UpdateTime)
        {
            tweener.preTick();

            tick();
            _accumulator -= UpdateTime;

            tweener.postTick();
        }

        tweener.tween(alpha);

        SceneManager::update();

        Ui::render();
    }

    static void fixedUpdate()
    {
        auto& tweener = EntityTweener::get();
        tweener.reset();

        if (_accumulator < UpdateTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else
        {
            tick();
            _accumulator -= UpdateTime;

            SceneManager::update();

            Ui::render();
        }
    }

    void update()
    {
        auto timeNow = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(timeNow - _lastUpdate).count() / 1'000'000.0;

        elapsed *= TimeScale;

        _accumulator = std::min(_accumulator + elapsed, MaxUpdateTime);
        _lastUpdate = timeNow;

        if (Config::get().uncapFPS)
        {
            variableUpdate();
        }
        else
        {
            fixedUpdate();
        }
    }

    uint16_t getTimeSinceLastTick()
    {
        return _time_since_last_tick;
    }

    uint16_t getNumFrameUpdates()
    {
        return _numFrameUpdates;
    }

    void simulateGame(const fs::path& savePath, int32_t ticks)
    {
        try
        {
            initialise();
            Scenes::BootScene::loadFile(savePath);

            // The load itself is performed as part of the scene transition.
            SceneManager::applySceneTransition();
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to simulate park: {}", e.what());
        }

        if (SceneManager::getCurrentScene() != SceneManager::SceneId::gameplay)
        {
            Logging::error("Unable to simulate park!");
            return;
        }

        Logging::info("File loaded. Starting simulation.");

        for (int32_t i = 0; i < ticks; i++)
        {
            if (SceneManager::isSceneTransitionPending())
            {
                break;
            }

            Scenes::GameScene::tick();
        }
    }

}
