#include "Scenes/GameplayScene.h"
#include "SceneManager.h"
#include "Scenes/GameScene.h"

namespace OpenLoco::Scenes::GameplayScene
{
    void tick()
    {
        const auto numFrameUpdates = GameScene::tickWorld();

        if (SceneManager::isSceneTransitionPending())
        {
            return;
        }

        GameScene::tickInterface(numFrameUpdates);
    }
}
