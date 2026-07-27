#include "Scenes/TitleScene.h"
#include "Graphics/Gfx.h"
#include "OpenLoco.h"
#include "Scenes/GameScene.h"
#include "Title.h"

namespace OpenLoco::Scenes::TitleScene
{
    void onEnter()
    {
        Gfx::loadDefaultPalette();
        Gfx::invalidateScreen();

        Title::start();
    }

    void tick()
    {
        for (uint16_t i = 0; i < getNumTicks(); i++)
        {
            GameScene::tickWorld();

            Title::update();
        }

        GameScene::tickInterface();
    }

    void update()
    {
    }
}
