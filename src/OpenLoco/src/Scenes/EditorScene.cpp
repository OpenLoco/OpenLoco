#include "Scenes/EditorScene.h"
#include "EditorController.h"
#include "OpenLoco.h"
#include "Scenes/GameScene.h"

namespace OpenLoco::Scenes::EditorScene
{
    void tick()
    {
        for (uint16_t i = 0; i < getNumTicks(); i++)
        {
            GameScene::tickWorld();
        }

        EditorController::tick();

        GameScene::tickInterface();
    }

    void update()
    {
    }
}
