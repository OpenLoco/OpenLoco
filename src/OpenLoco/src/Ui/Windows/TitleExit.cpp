#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Intro.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::TitleExit
{
    static constexpr Ui::Size32 kWindowSize = { 40, 28 };

    namespace Widx
    {
        enum
        {
            exit_button
        };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::ImageButton({ 0, 0 }, kWindowSize, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_exit_from_game)

    );

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::titleExit,
            { Ui::width() - kWindowSize.width, Ui::height() - kWindowSize.height },
            kWindowSize,
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());

        return window;
    }

    static void prepareDraw(Ui::Window& self)
    {
        auto exitString = StringManager::getString(StringIds::title_exit_game);
        self.width = Gfx::TextRenderer::getStringWidthNewLined(Gfx::Font::medium_bold, exitString) + 10;

        self.x = Ui::width() - self.width;
        self.widgets[Widx::exit_button].right = self.width;
    }

    // 0x00439236
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw widgets.
        window.draw(drawingCtx);

        int16_t x = window.x + window.width / 2;
        int16_t y = window.y + window.widgets[Widx::exit_button].top + 8;
        Ui::Point origin = { x, y };
        tr.drawStringCentredWrapped(origin, window.width, Colour::black, StringIds::title_exit_game);
    }

    // 0x00439268
    static void onMouseUp([[maybe_unused]] Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
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
                args.loadQuitMode = LoadOrQuitMode::quitGamePrompt;
                args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
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
