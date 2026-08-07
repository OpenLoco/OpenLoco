#include "Scenes/IntroScene.h"
#include "Graphics/Gfx.h"
#include "Intro.h"
#include "SceneManager.h"

namespace OpenLoco::Scenes::IntroScene
{
    void onEnter()
    {
        Intro::state(Intro::State::begin);
    }

    void onExit()
    {
        Gfx::loadDefaultPalette();
        Gfx::invalidateScreen();
    }

    void tick()
    {
        Intro::tick();

        if (!Intro::isActive())
        {
            SceneManager::requestScene(SceneManager::SceneId::boot);
        }
    }

    void tickInterface()
    {
    }

    void update()
    {
    }
}
