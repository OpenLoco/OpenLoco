#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"
#include <vector>

namespace OpenLoco::Ui::Windows::Debug
{
    static constexpr Ui::Size32 kWindowSize = { 400, 260 };
    static constexpr int32_t kMargin = 2;

    static constexpr int32_t kTitlebarHeight = 13;
    static constexpr int32_t kLabelHeight = 12;
    static constexpr int32_t kButtonHeight = 12;
    static constexpr int32_t kRowSize = 24;

    static constexpr int32_t kTabWidth = 31;
    static constexpr int32_t kTabHeight = 27;

    namespace widx
    {
        constexpr auto frame = WidgetId("frame");
        constexpr auto title = WidgetId("title");
        constexpr auto close = WidgetId("close");
        constexpr auto panel = WidgetId("panel");
        constexpr auto button_1 = WidgetId("button_1");
        constexpr auto button_2 = WidgetId("button_2");
        constexpr auto label_1 = WidgetId("label_1");
        constexpr auto label_2 = WidgetId("label_2");
        constexpr auto label_3 = WidgetId("label_3");
        constexpr auto tab_1 = WidgetId("tab_1");
        constexpr auto tab_2 = WidgetId("tab_2");
        constexpr auto tab_3 = WidgetId("tab_3");
        constexpr auto tab_4 = WidgetId("tab_4");
    }

    static const WindowEventList& getEvents();

    namespace
    {
        using namespace Widgets;

        static constexpr auto _widgets = makeWidgets(
            Frame(widx::frame, { 0, 0 }, kWindowSize, WindowColour::primary),
            Caption(widx::title, { 1, 1 }, { kWindowSize.width - 2, kTitlebarHeight }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::openloco),
            ImageButton(widx::close, { kWindowSize.width - 15, kMargin }, { 13, kTitlebarHeight }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),

            Panel(widx::panel, { 0, kTitlebarHeight + kMargin }, { kWindowSize.width, 245 }, WindowColour::secondary),

            Button(widx::button_1, { kMargin, kTitlebarHeight + kMargin + (0 * (kRowSize + kMargin)) }, { kWindowSize.width / 2, kButtonHeight }, WindowColour::secondary, StringIds::openloco),
            ImageButton(widx::button_2, { kMargin, kTitlebarHeight + kMargin + (1 * (kRowSize + kMargin)) }, { 24, 24 }, WindowColour::secondary, ImageIds::red_flag, StringIds::tooltip_stop_start),

            Label(widx::label_1, { kMargin, kTitlebarHeight + kMargin + (2 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, ContentAlign::left, StringIds::openloco),
            Label(widx::label_2, { kMargin, kTitlebarHeight + kMargin + (3 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, ContentAlign::center, StringIds::openloco),
            Label(widx::label_3, { kMargin, kTitlebarHeight + kMargin + (4 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, ContentAlign::right, StringIds::openloco),

            Tab(widx::tab_1, { kMargin + ((kTabWidth + kMargin) * 0), kTitlebarHeight + kMargin + (5 * (kRowSize + kMargin)) }, { kTabWidth, kTabHeight }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town),
            Tab(widx::tab_2, { kMargin + ((kTabWidth + kMargin) * 1), kTitlebarHeight + kMargin + (5 * (kRowSize + kMargin)) }, { kTabWidth, kTabHeight }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_population_graph),
            Tab(widx::tab_3, { kMargin + ((kTabWidth + kMargin) * 2), kTitlebarHeight + kMargin + (5 * (kRowSize + kMargin)) }, { kTabWidth, kTabHeight }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town_ratings_each_company)
            //
        );
    }

    // 0x0043B26C
    Window* open()
    {
        if (auto* wnd = WindowManager::bringToFront(WindowType::debug); wnd != nullptr)
        {
            return wnd;
        }

        auto window = WindowManager::createWindowCentred(
            WindowType::debug,
            kWindowSize,
            WindowFlags::none,
            getEvents());

        window->setWidgets(_widgets);
        window->enabledWidgets = ~0ULL;
        // window->disabledWidgets = 1U << widx::tab_3;
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);

        return window;
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::Window& window, const WidgetIndex_t widgetIndex)
    {
        const auto& widget = window.widgets[widgetIndex];
        switch (widget.id)
        {
            case widx::close:
                WindowManager::close(window.type);
                break;
        }
    }

    // 0x0043B2E4
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        window.draw(drawingCtx);
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
