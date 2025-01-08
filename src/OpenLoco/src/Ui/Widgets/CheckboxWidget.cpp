#include "CheckboxWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    void Checkbox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawCheckMark(drawingCtx, widget, widgetState);
        drawLabel(drawingCtx, widget, widgetState);
    }

    // 0x004CB00B
    void Checkbox::drawCheckMark(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        if (widgetState.enabled)
        {
            drawingCtx.fillRectInset(
                window->x + widget.left,
                window->y + widget.top,
                window->x + widget.left + 9,
                window->y + widget.bottom - 1,
                widgetState.colour,
                widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);
        }

        if (widgetState.activated)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);
            static constexpr char strCheckmark[] = "\xAC";
            auto point = Point(window->x + widget.left, window->y + widget.top);

            auto color = widgetState.colour;
            tr.setCurrentFont(Gfx::Font::medium_bold);
            tr.drawString(point, color.opaque(), strCheckmark);
        }
    }

    // 0x004CB080
    void Checkbox::drawLabel(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        auto colour = widgetState.colour;
        colour = colour.opaque();

        if (widgetState.disabled)
        {
            colour = colour.inset();
        }

        auto formatArgs = FormatArguments(widget.textArgs);
        auto* window = widgetState.window;

        auto tr = Gfx::TextRenderer(drawingCtx);
        auto point = Point(window->x + widget.left + 14, window->y + widget.top);
        tr.drawStringLeft(point, colour, widget.text, formatArgs);
    }
}
