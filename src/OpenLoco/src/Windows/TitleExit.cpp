#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Intro.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleExit
{
    static constexpr Ui::Size kWindowSize = { 40, 28 };

    namespace Widx
    {
        enum
        {
            exit_button
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_exit_from_game),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::titleExit,
            Ui::Point(Ui::width() - kWindowSize.width, Ui::height() - kWindowSize.height),
            kWindowSize,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            getEvents());

        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::exit_button);

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());

        return window;
    }

    static void prepareDraw(Ui::Window& self)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto exitString = StringManager::getString(StringIds::title_exit_game);
        drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);
        self.width = drawingCtx.getStringWidthNewLined(exitString) + 10;

        self.x = Ui::width() - self.width;
        self.widgets[Widx::exit_button].right = self.width;
    }

    // 0x00439236
    static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // Draw widgets.
        window.draw(rt);

        int16_t x = window.x + window.width / 2;
        int16_t y = window.y + window.widgets[Widx::exit_button].top + 8;
        Ui::Point origin = { x, y };
        drawingCtx.drawStringCentredWrapped(*rt, origin, window.width, Colour::black, StringIds::title_exit_game);
    }

    // 0x00439268
    static void onMouseUp([[maybe_unused]] Window& window, WidgetIndex_t widgetIndex)
    {
        if (Intro::isActive())
        {
            return;
        }

        switch (widgetIndex)
        {
            case Widx::exit_button:
                // Exit to desktop
                GameCommands::LoadSaveQuitGameArgs args{};
                args.option1 = GameCommands::LoadSaveQuitGameArgs::Options::save;
                args.option2 = LoadOrQuitMode::quitGamePrompt;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                break;
        }
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
