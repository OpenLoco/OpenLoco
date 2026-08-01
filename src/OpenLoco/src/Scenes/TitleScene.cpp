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
        GameScene::tick();

        Title::update();
    }

    void tickInterface()
    {
        GameScene::tickInterface();
    }

    void update()
    {
    }

}
