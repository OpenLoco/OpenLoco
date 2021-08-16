#include "Gui.h"
#include "Graphics/Colour.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "OpenLoco.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "Widget.h"
#include "Window.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::Gui
{
    // 0x00438A6C
    void init()
    {
        Windows::Main::open();

        addr<0x00F2533F, int8_t>() = 0; // grid lines
        addr<0x0112C2e1, int8_t>() = 0;
        addr<0x009c870E, int8_t>() = 1;
        addr<0x009c870F, int8_t>() = 2;
        addr<0x009c8710, int8_t>() = 1;

        if (OpenLoco::isTitleMode())
        {
            Ui::Windows::TitleMenu::open();
            Ui::Windows::TitleExit::open();
            Ui::Windows::TitleLogo::open();
            Ui::Windows::TitleVersion::open();
            Ui::Windows::TitleOptions::open();
        }
        else
        {
            Windows::ToolbarTop::Game::open();

            Windows::PlayerInfoPanel::open();
            Windows::TimePanel::open();

            if (OpenLoco::Tutorial::state() != Tutorial::State::none)
            {
                Windows::Tutorial::open();
            }
        }

        resize();
    }

    // 0x004392BD
    void resize()
    {
        const int32_t uiWidth = Ui::width();
        const int32_t uiHeight = Ui::height();

        auto window = WindowManager::getMainWindow();
        if (window)
        {
            window->width = uiWidth;
            window->height = uiHeight;
            if (window->widgets.get())
            {
                window->widgets.get()[0].right = uiWidth;
                window->widgets.get()[0].bottom = uiHeight;
            }
            if (window->viewports[0].get())
            {
                window->viewports[0].get()->width = uiWidth;
                window->viewports[0].get()->height = uiHeight;
                window->viewports[0].get()->view_width = uiWidth << window->viewports[0].get()->zoom;
                window->viewports[0].get()->view_height = uiHeight << window->viewports[0].get()->zoom;
            }
        }

        window = WindowManager::find(WindowType::topToolbar);
        if (window)
        {
            window->width = std::max(uiWidth, 640);
        }

        window = WindowManager::find(WindowType::playerInfoToolbar);
        if (window)
        {
            window->y = uiHeight - window->height;
        }

        window = WindowManager::find(WindowType::timeToolbar);
        if (window)
        {
            window->y = uiHeight - window->height;
            window->x = std::max(uiWidth, 640) - window->width;
        }

        window = WindowManager::find(WindowType::editorToolbar);
        if (window)
        {
            window->y = uiHeight - window->height;
            window->width = std::max(uiWidth, 640);
        }

        window = WindowManager::find(WindowType::titleMenu);
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

        window = WindowManager::find(WindowType::tutorial);
        if (window)
        {
            if (Tutorial::state() == Tutorial::State::none)
            {
                WindowManager::close(window);
            }
        }
    }
}
