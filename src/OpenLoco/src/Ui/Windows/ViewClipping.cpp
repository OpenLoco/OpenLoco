#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/SliderWidget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::ViewClipping
{
    enum widx
    {
        frame,
        caption,
        closeButton,
        panel,
        slider,
    };

    namespace Widx
    {
        constexpr WidgetId kFrame{ "frame" };
        constexpr WidgetId kCaption{ "caption" };
        constexpr WidgetId kCloseButton{ "close_button" };
        constexpr WidgetId kPanel{ "panel" };
        constexpr WidgetId kSlider{ "slider" };
    }

    static constexpr Ui::Size kWindowSize = { 350, 60 };

    static constexpr auto kSliderSize = Size{ 100, 24 };

    static constexpr auto widgets = makeWidgets(
        Widgets::Frame(Widx::kFrame, { 0, 0 }, { 350, kWindowSize.height }, WindowColour::primary),
        Widgets::Caption(Widx::kCaption, { 1, 1 }, { 348, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::title_view_clipping),
        Widgets::ImageButton(Widx::kCloseButton, { kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(Widx::kPanel, { 0, 15 }, { 350, kWindowSize.height - 15 }, WindowColour::secondary),
        Widgets::Slider(Widx::kSlider, { (kWindowSize.width - kSliderSize.width) / 2, 25 }, kSliderSize, WindowColour::secondary)

    );

    static const WindowEventList& getEvents();

    Window* open()
    {
        if (auto* window = WindowManager::bringToFront(WindowType::viewClipping))
        {
            return window;
        }

        auto* window = WindowManager::createWindowCentred(
            WindowType::viewClipping,
            kWindowSize,
            WindowFlags::stickToFront,
            getEvents());

        window->setWidgets(widgets);
        window->holdableWidgets = (1 << widx::slider);

        window->setColour(WindowColour::primary, Colour::black);
        window->setColour(WindowColour::secondary, Colour::mutedSeaGreen);

        return window;
    }

    static void prepareDraw(Window& self)
    {
        auto sliderWidth = self.widgets[widx::slider].width() - 5;
        auto height = getMaxClipHeight() * sliderWidth / 255;

        self.widgets[widx::slider].content = height;
    }

    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
    }

    static void onMouseUp(Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, WidgetId widgetId)
    {
        if (widgetId == Widx::kCloseButton)
        {
            WindowManager::close(self.type);
        }
    }

    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, WidgetId widgetId)
    {
        if (widgetId == Widx::kSlider)
        {
            Input::setClickRepeatTicks(31);

            auto mousePos = Input::getScrollLastLocation();
            auto x = mousePos.x - self.x - self.widgets[widgetIndex].left + 5;

            auto sliderWidth = self.widgets[widgetIndex].width() - 5;
            auto height = std::clamp(x * 255 / sliderWidth, 0, 255);

            setMaxClipHeight(height);
            self.invalidate();
        }
    }

    static constexpr WindowEventList kEvents = {
        .prepareDraw = prepareDraw,
        .draw = draw,
        .onMouseDown = onMouseDown,
        .onMouseUp = onMouseUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
