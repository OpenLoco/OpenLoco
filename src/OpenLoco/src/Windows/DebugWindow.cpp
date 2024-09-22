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
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"
#include <vector>

namespace OpenLoco::Ui::Windows::Debug
{
    static constexpr Ui::Size32 kWindowSize = { 400, 260 };
    static constexpr int32_t kMargin = 2;

    static constexpr int32_t kTitlebarHeight = 13;
    static constexpr int32_t kLabelHeight = 12;
    static constexpr int32_t kButtonHeight = 12;

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            button_1,
        };
    }

    static const WindowEventList& getEvents();

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        makeWidget({ 1, 1 }, { kWindowSize.width - 2, kTitlebarHeight }, WidgetType::caption_25, WindowColour::primary, StringIds::openloco),
        makeWidget({ kWindowSize.width - 15, kMargin }, { 13, kTitlebarHeight }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),

        Widgets::Panel({ 0, kTitlebarHeight + kMargin }, { kWindowSize.width, 245 }, WindowColour::secondary),
        Widgets::Button({ kMargin, kTitlebarHeight + kMargin }, { kWindowSize.width / 2, kButtonHeight }, WindowColour::secondary, StringIds::openloco),
        Widgets::Label({ kMargin, kTitlebarHeight + kMargin + kButtonHeight + kMargin }, { kWindowSize.width - (kMargin * 2), kLabelHeight }, WindowColour::secondary, StringIds::openloco));

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
        window->enabledWidgets = (1 << widx::close) | (1 << widx::button_1);
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);

        return window;
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::Window& window, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
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
