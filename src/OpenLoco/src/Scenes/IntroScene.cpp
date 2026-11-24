#include "Scenes/IntroScene.h"
#include "Graphics/Gfx.h"
#include "Intro.h"
#include "SceneManager.h"
#include "Ui.h"
#include "Ui/WindowManager.h"

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
        Ui::WindowManager::tick();
    }

    void update()
    {
        Ui::handleInput();
    }
}
