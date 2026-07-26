#include "Scenes/GameScene.h"
#include "Audio/Audio.h"
#include "GameState.h"
#include "Input.h"
#include "Network/Network.h"
#include "OpenLoco.h"
#include "Scenario/ScenarioManager.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <algorithm>

using namespace OpenLoco::Input;
using namespace OpenLoco::Ui;

namespace OpenLoco::Scenes::GameScene
{
    uint16_t tickWorld()
    {
        uint16_t numUpdates = std::clamp<uint16_t>(getTimeSinceLastTick() / (uint16_t)31, 1, 3);
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
        const uint16_t numFrameUpdates = std::max<uint16_t>(1, numUpdates);
        SceneManager::setSceneAge(std::min(0xFFFF, (int32_t)SceneManager::getSceneAge() + numFrameUpdates));
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

        tickLogic(numUpdates);

        if (SceneManager::isSceneTransitionPending())
        {
            return numFrameUpdates;
        }

        getGameState().var_014A++;

        return numFrameUpdates;
    }

    void tickInterface(uint16_t numFrameUpdates)
    {
        Audio::playBackgroundMusic();

        sub_431695(numFrameUpdates);
    }
}
