#include "TextBoxWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x4CB2D6
    static void drawText(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetStated)
    {
        if (widget.content == Widget::kContentNull || widget.content == Widget::kContentUnk)
        {
            return;
        }

        auto colour = widgetStated.colour;
        if (widgetStated.disabled)
        {
            colour = colour.inset();
        }
        else
        {
            colour = colour.FD();
        }

        auto formatArgs = FormatArguments(widget.textArgs);
        auto* window = widgetStated.window;
        auto point = Point(window->x + widget.left + 1, window->y + widget.top);
        int width = widget.right - widget.left - 2;

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.drawStringLeftClipped(point, width, colour, widget.text, formatArgs);
    }

    // 0x4CB29C
    void TextBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto flags = widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker;
        drawingCtx.fillRectInset(
            window->x + widget.left,
            window->y + widget.top,
            window->x + widget.right,
            window->y + widget.bottom,
            widgetState.colour,
            flags);

        drawText(drawingCtx, widget, widgetState);
    }
}
