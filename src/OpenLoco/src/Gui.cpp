#include "Gui.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Map/Tile.h"
#include "SceneManager.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"

using namespace OpenLoco::Ui;

namespace OpenLoco::Gui
{
    // 0x00438A6C
    void init()
    {
        Windows::Main::open();

        Windows::Terraform::setAdjustLandToolSize(1);
        Windows::Terraform::setAdjustWaterToolSize(1);
        Windows::Terraform::setClearAreaToolSize(2);

        if (SceneManager::isTitleMode())
        {
            Ui::Windows::TitleMenu::open();
            Ui::Windows::TitleExit::open();
            Ui::Windows::TitleLogo::open();
            Ui::Windows::TitleVersion::open();
            Ui::Windows::TitleOptions::open();
        }
        else if (SceneManager::isPlayMode())
        {
            Windows::ToolbarTop::open();

            Windows::CompanyInfoPanel::open();
            Windows::TimePanel::open();

            if (OpenLoco::Tutorial::state() != Tutorial::State::none)
            {
                Windows::Tutorial::open();
            }
        }

        resize();
    }

    static void resizeEditorScene(const int32_t uiWidth, const int32_t uiHeight)
    {
        using Windows::EditorStepController::StepDirection;

        auto* topToolbar = WindowManager::find(WindowType::topToolbar);

        const bool infoPanelsOnTop = Config::get().infoPanelsOnTop && uiWidth > 640;
        const bool infoPanelsJuxtaposed = infoPanelsOnTop && Config::get().infoPanelsJuxtaposed && topToolbar != nullptr && uiWidth > 640;

        auto* window = WindowManager::find(WindowType::editorStepController, enumValue(StepDirection::previous));
        if (window)
        {
            window->y = infoPanelsOnTop ? 0 : uiHeight - window->height;
            window->x = infoPanelsJuxtaposed ? topToolbar->x - window->width - 10 : 0;
        }

        window = WindowManager::find(WindowType::editorStepController, enumValue(StepDirection::next));
        if (window)
        {
            window->y = infoPanelsOnTop ? 0 : uiHeight - window->height;
            window->x = infoPanelsJuxtaposed ? topToolbar->x + topToolbar->width + 10 : std::max(uiWidth, 640) - window->width;
        }

        window = WindowManager::find(WindowType::editorStatusLine);
        if (window)
        {
            window->y = uiHeight - window->height;
            window->x = (std::max(uiWidth, 640) - window->width) / 2;
        }
    }

    static void resizeGameScene(const int32_t uiWidth, const int32_t uiHeight)
    {
        auto* topToolbar = WindowManager::find(WindowType::topToolbar);
        if (topToolbar)
        {
            topToolbar->width = std::max(uiWidth, 640);
            topToolbar->callPrepareDraw();
        }

        const bool infoPanelsOnTop = Config::get().infoPanelsOnTop && uiWidth > 640;
        const bool infoPanelsJuxtaposed = infoPanelsOnTop && Config::get().infoPanelsJuxtaposed && uiWidth > 640;

        auto* companyInfo = WindowManager::find(WindowType::companyInfoPanel);
        if (companyInfo)
        {
            companyInfo->y = infoPanelsOnTop ? 0 : uiHeight - companyInfo->height;
            companyInfo->x = infoPanelsJuxtaposed ? topToolbar->x - companyInfo->width - 10 : 0;
        }

        auto* timeInfo = WindowManager::find(WindowType::timePanel);
        if (timeInfo)
        {
            timeInfo->y = infoPanelsOnTop ? 0 : uiHeight - timeInfo->height;
            timeInfo->x = infoPanelsJuxtaposed ? topToolbar->x + topToolbar->width + 10 : std::max(uiWidth, 640) - timeInfo->width;
        }
    }

    static void resizeMain(const int32_t uiWidth, const int32_t uiHeight)
    {
        auto* window = WindowManager::getMainWindow();
        if (window)
        {
            window->width = uiWidth;
            window->height = uiHeight;
            if (!window->widgets.empty())
            {
                window->widgets[0].right = uiWidth;
                window->widgets[0].bottom = uiHeight;
            }
            if (window->viewports[0])
            {
                window->viewports[0]->width = uiWidth;
                window->viewports[0]->height = uiHeight;
                window->viewports[0]->viewWidth = window->viewports[0]->zoom.applyTo(uiWidth);
                window->viewports[0]->viewHeight = window->viewports[0]->zoom.applyTo(uiHeight);
            }
        }
    }

    static void resizeMisc()
    {
        auto* window = WindowManager::find(WindowType::tutorial);
        if (window)
        {
            if (Tutorial::state() == Tutorial::State::none)
            {
                WindowManager::close(window);
            }
        }

        window = WindowManager::find(WindowType::options);
        if (window != nullptr)
        {
            window->moveToCentre();
        }
    }

    static void resizeTitleScene(const int32_t uiWidth, const int32_t uiHeight)
    {
        auto* window = WindowManager::find(WindowType::titleMenu);
        if (window)
        {
            window->x = uiWidth / 2 - 148;
            window->y = uiHeight - 117;
        }

        window = WindowManager::find(WindowType::titleExit);
        if (window)
        {
            window->x = uiWidth - 40;
            window->y = uiHeight - 28;
        }

        window = WindowManager::find(WindowType::openLocoVersion);
        if (window)
        {
            window->y = uiHeight - window->height;
        }

        window = WindowManager::find(WindowType::titleOptions);
        if (window)
        {
            window->x = uiWidth - window->width;
        }
    }

    // 0x004392BD
    void resize()
    {
        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        resizeMain(uiWidth, uiHeight);

        if (SceneManager::isEditorMode())
        {
            resizeEditorScene(uiWidth, uiHeight);
        }
        else if (SceneManager::isTitleMode())
        {
            resizeTitleScene(uiWidth, uiHeight);
        }
        else
        {
            resizeGameScene(uiWidth, uiHeight);
        }

        resizeMisc();
    }
}
