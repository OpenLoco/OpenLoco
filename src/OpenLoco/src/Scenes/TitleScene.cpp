#include "Scenes/TitleScene.h"
#include "Graphics/Gfx.h"
#include "SceneManager.h"
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
        const auto numFrameUpdates = GameScene::tickWorld();

        if (SceneManager::isSceneTransitionPending())
        {
            return;
        }

        GameScene::tickInterface(numFrameUpdates);
    }

    void tickLogic()
    {
        Title::update();
    }
}
