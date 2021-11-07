#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include "../Window.h"

#include <string>
#include <string_view>

namespace OpenLoco::Ui::Windows::NetworkStatus
{
    enum Widx
    {
        frame,
        caption,
        closeBtn,
        panel,
    };

    constexpr Ui::Size windowSize = { 441, 91 };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 441, 91 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 439, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::empty),
        makeWidget({ 426, 2 }, { 13, 13 }, WidgetType::wt_9, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 441, 76 }, WidgetType::panel, WindowColour::secondary),
        widgetEnd(),
    };

    static WindowEventList _events;

    static std::string _text;
    static CloseCallback _cbClose;

    static void initEvents();

    Window* open(std::string_view text, CloseCallback cbClose)
    {
        _text = text;
        _cbClose = cbClose;

        initEvents();

        auto window = WindowManager::createWindowCentred(
            WindowType::networkStatus,
            windowSize,
            WindowFlags::flag_11 | WindowFlags::stick_to_front,
            &_events);

        window->widgets = widgets;
        window->enabled_widgets = (1 << Widx::closeBtn);

        window->initScrollWidgets();
        window->setColour(WindowColour::primary, Colour::black);
        window->setColour(WindowColour::secondary, Colour::black);

        return window;
    }

    void setText(std::string_view text)
    {
        _text = text;
        WindowManager::invalidate(WindowType::networkStatus);
    }

    void close()
    {
        WindowManager::close(WindowType::networkStatus);
        Gfx::invalidateScreen();
    }

    static void onClose(Ui::Window* window)
    {
        if (_cbClose != nullptr)
        {
            _cbClose();
        }
    }

    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::closeBtn:
                WindowManager::close(window);
                break;
        }
    }

    static void prepareDraw(Window* self)
    {
        StringManager::setString(StringIds::buffer_1250, _text.c_str());
    }

    static void draw(Window* self, Gfx::Context* context)
    {
        self->draw(context);

        uint16_t x = self->x + (self->width / 2);
        uint16_t y = self->y + (self->height / 2);
        uint16_t width = self->width;
        Gfx::drawStringCentredClipped(*context, x, y, width, Colour::black, StringIds::buffer_1250, nullptr);
    }

    static void initEvents()
    {
        _events.on_close = onClose;
        _events.on_mouse_up = onMouseUp;
        _events.draw = draw;
        _events.prepare_draw = prepareDraw;
    }
}
