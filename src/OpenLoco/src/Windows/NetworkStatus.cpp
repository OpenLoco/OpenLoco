#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "Window.h"

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

    static constexpr Ui::Size kWindowSize = { 441, 91 };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 441, 91 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 439, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::empty),
        makeWidget({ 426, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 441, 76 }, WidgetType::panel, WindowColour::secondary),
        widgetEnd(),
    };

    static std::string _text;
    static CloseCallback _cbClose;

    static const WindowEventList& getEvents();

    Window* open(std::string_view text, CloseCallback cbClose)
    {
        _text = text;
        _cbClose = cbClose;

        auto window = WindowManager::createWindowCentred(
            WindowType::networkStatus,
            kWindowSize,
            WindowFlags::flag_11 | WindowFlags::stickToFront,
            getEvents());

        window->widgets = widgets;
        window->enabledWidgets = (1 << Widx::closeBtn);

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

    void setText(std::string_view text, CloseCallback cbClose)
    {
        _text = text;
        _cbClose = cbClose;
        WindowManager::invalidate(WindowType::networkStatus);
    }

    void close()
    {
        WindowManager::close(WindowType::networkStatus);
        Gfx::invalidateScreen();
    }

    static void onClose([[maybe_unused]] Ui::Window& window)
    {
        if (_cbClose)
        {
            _cbClose();
        }
    }

    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::closeBtn:
                WindowManager::close(&window);
                break;
        }
    }

    static void prepareDraw([[maybe_unused]] Window& self)
    {
        StringManager::setString(StringIds::buffer_1250, _text.c_str());
    }

    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        self.draw(rt);

        uint16_t x = self.x + (self.width / 2);
        uint16_t y = self.y + (self.height / 2);
        uint16_t width = self.width;
        drawingCtx.drawStringCentredClipped(*rt, x, y, width, Colour::black, StringIds::buffer_1250, nullptr);
    }

    static constexpr WindowEventList _events = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return _events;
    }
}
