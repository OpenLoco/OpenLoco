#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/StringIds.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"

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

    static constexpr Ui::Size32 kWindowSize = { 441, 91 };

    static constexpr auto widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 441, 91 }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 439, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::empty),
        Widgets::ImageButton({ 426, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 441, 76 }, WindowColour::secondary)

    );

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

        window->setWidgets(widgets);
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

    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
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

    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);

        auto origin = Point(self.x + (self.width / 2), self.y + (self.height / 2));
        auto width = self.width;

        tr.drawStringCentredClipped(origin, width, Colour::black, StringIds::buffer_1250);
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
