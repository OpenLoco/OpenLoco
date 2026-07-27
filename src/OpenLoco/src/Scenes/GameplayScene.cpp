#include "Scenes/GameplayScene.h"
#include "OpenLoco.h"
#include "Scenes/GameScene.h"

namespace OpenLoco::Scenes::GameplayScene
{
    void tick()
    {
        for (uint16_t i = 0; i < getNumTicks(); i++)
        {
            GameScene::tickWorld();
        }

        GameScene::tickInterface();
    }

    void update()
    {
    }
}
