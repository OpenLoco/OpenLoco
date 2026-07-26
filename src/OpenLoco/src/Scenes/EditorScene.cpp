#include "Scenes/EditorScene.h"
#include "EditorController.h"
#include "SceneManager.h"
#include "Scenes/GameScene.h"

namespace OpenLoco::Scenes::EditorScene
{
    void tick()
    {
        const auto numFrameUpdates = GameScene::tickWorld();

        if (SceneManager::isSceneTransitionPending())
        {
            return;
        }

        EditorController::tick();

        GameScene::tickInterface(numFrameUpdates);
    }
}
