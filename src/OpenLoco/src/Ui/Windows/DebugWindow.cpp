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
#include "Ui/Widgets/CheckboxWidget.h"
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

        constexpr auto checkbox_1 = WidgetId("checkbox_1");
        constexpr auto checkbox_2 = WidgetId("checkbox_2");
        constexpr auto checkbox_3 = WidgetId("checkbox_3");
        constexpr auto checkbox_4 = WidgetId("checkbox_4");

        // constexpr auto tab_4 = WidgetId("tab_4");
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

            Checkbox(widx::checkbox_1, { kMargin, kTitlebarHeight + kMargin + (5 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, StringIds::openloco),
            Checkbox(widx::checkbox_2, { kMargin, kTitlebarHeight + kMargin + (6 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, StringIds::openloco),
            Checkbox(widx::checkbox_3, { kMargin, kTitlebarHeight + kMargin + (7 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, StringIds::openloco),
            Checkbox(widx::checkbox_4, { kMargin, kTitlebarHeight + kMargin + (8 * (kRowSize + kMargin)) }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, StringIds::openloco),

            Tab(widx::tab_1, { kMargin + ((kTabWidth + kMargin) * 0), kTitlebarHeight + kMargin + (9 * (kRowSize + kMargin)) }, { kTabWidth, kTabHeight }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town),
            Tab(widx::tab_2, { kMargin + ((kTabWidth + kMargin) * 1), kTitlebarHeight + kMargin + (9 * (kRowSize + kMargin)) }, { kTabWidth, kTabHeight }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_population_graph),
            Tab(widx::tab_3, { kMargin + ((kTabWidth + kMargin) * 2), kTitlebarHeight + kMargin + (9 * (kRowSize + kMargin)) }, { kTabWidth, kTabHeight }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town_ratings_each_company)
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
        // window->disabledWidgets = 1U << widx::tab_3;
        window->initScrollWidgets();

        auto getWidgetById = [&](Window& window, const WidgetId id) -> Widget& {
            for (auto& widget : window.widgets)
            {
                if (widget.id == id)
                {
                    return widget;
                }
            }
            throw std::runtime_error("Widget not found");
        };

        auto& chkbox2 = getWidgetById(*window, widx::checkbox_2);
        chkbox2.activated = true;

        auto& chkbox3 = getWidgetById(*window, widx::checkbox_3);
        chkbox3.activated = false;
        chkbox3.disabled = true;

        auto& chkbox4 = getWidgetById(*window, widx::checkbox_4);
        chkbox4.activated = true;
        chkbox4.disabled = true;

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);

        return window;
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::Window& window, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        const auto& widget = window.widgets[widgetIndex];
        switch (widget.id)
        {
            case widx::close:
                WindowManager::close(window.type);
                break;
        }
    }

    // Basic prototype of requesting the user to click on stuff.
    static void drawInteractionRequest(Gfx::DrawingContext& drawingCtx, const Widget& widget, WidgetState& state)
    {
        auto* window = state.window;

        Ui::Point localPos = {};

        // Center of widget.
        localPos.x = widget.left + ((widget.right - widget.left) / 2);
        localPos.y = widget.top + ((widget.bottom - widget.top) / 2);

        auto radius = 4;
        radius += (window->frameNo / 4) % 12;

        Ui::Point windowPos = state.window->position();
        drawingCtx.drawCircle(windowPos + localPos, radius, 2, PaletteIndex::red5);
    }

    static void onUpdate(Ui::Window& window)
    {
        window.frameNo++;

        // Keep this going.
        window.invalidate();
    }

    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        window.draw(drawingCtx);

        // Draw interaction request on top of everything else.
        for (const auto& widget : window.widgets)
        {
            WidgetState widgetState{};
            widgetState.window = &window;

            drawInteractionRequest(drawingCtx, widget, widgetState);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onUpdate = onUpdate,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
