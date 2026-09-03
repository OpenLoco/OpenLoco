#include "Config.h"
#include "GameState.h"
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
#include "Ui/Widgets/LabelWidget.h"
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
        height,
    };

    namespace Widx
    {
        constexpr WidgetId kFrame{ "frame" };
        constexpr WidgetId kCaption{ "caption" };
        constexpr WidgetId kCloseButton{ "close_button" };
        constexpr WidgetId kPanel{ "panel" };
        constexpr WidgetId kSlider{ "slider" };
        constexpr WidgetId kHeight{ "height" };
    }

    static constexpr Ui::Size kWindowSize = { 200, 60 };
    static constexpr auto kSliderSize = Size{ 100, 24 };
    static constexpr auto kMaxClipHeight = 128 * World::kSmallZStep;

    static constexpr auto widgets = makeWidgets(
        Widgets::Frame(Widx::kFrame, { 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption(Widx::kCaption, { 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::title_view_clipping),
        Widgets::ImageButton(Widx::kCloseButton, { kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(Widx::kPanel, { 0, 15 }, kWindowSize - Size{ 0, 15 }, WindowColour::secondary),
        Widgets::Slider(Widx::kSlider, { 5, 25 }, kSliderSize, WindowColour::secondary),
        Widgets::Label(Widx::kHeight, { 5 + kSliderSize.width + 5, 29 }, { 100, 10 }, WindowColour::secondary, ContentAlign::left, StringIds::stringptr)

    );

    static char _heightLabel[100];

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

    static void updateHeightLabel(Window& self)
    {
        const auto height = (getMaxClipHeight() + 3) / World::kMicroZStep - getGameState().seaLevel;

        auto& config = Config::get();
        if (config.showHeightAsUnits)
        {
            std::snprintf(_heightLabel, sizeof(_heightLabel), "%d units", height);
        }
        else if (config.measurementFormat == Config::MeasurementFormat::imperial)
        {
            std::snprintf(_heightLabel, sizeof(_heightLabel), "%d ft", height * 16);
        }
        else if (config.measurementFormat == Config::MeasurementFormat::metric)
        {
            std::snprintf(_heightLabel, sizeof(_heightLabel), "%d m", height * 5);
        }

        FormatArguments args{ self.widgets[widx::height].textArgs };
        args.push(_heightLabel);
    }

    static void prepareDraw(Window& self)
    {
        auto sliderWidth = self.widgets[widx::slider].width() - 5;
        auto height = getMaxClipHeight() * sliderWidth / kMaxClipHeight;

        self.widgets[widx::slider].content = height;

        updateHeightLabel(self);
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
            auto height = std::clamp(x * kMaxClipHeight / sliderWidth, 0, kMaxClipHeight);

            setMaxClipHeight(height);
            self.invalidate();
        }
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
