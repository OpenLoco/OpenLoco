#include "Scenes/EditorScene.h"
#include "EditorController.h"
#include "Scenes/GameScene.h"

namespace OpenLoco::Scenes::EditorScene
{
    void tick()
    {
        GameScene::tick();

        EditorController::tick();
    }

    void tickInterface()
    {
        GameScene::tickInterface();
    }

    void update()
    {
    }
}
