#include "TextBoxWidget.h"
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

        auto* window = widgetStated.window;

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        auto tr = Gfx::TextRenderer(drawingCtx);
        auto formatArgs = FormatArgumentsView(widget.textArgs);
        tr.drawStringLeftClipped(pos + Point{ 1, 1 }, size.width - 2, colour, widget.text, formatArgs);
    }

    // 0x4CB29C
    void TextBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        const auto flags = widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker;
        drawingCtx.fillRectInset(
            pos,
            size,
            widgetState.colour,
            flags);

        drawText(drawingCtx, widget, widgetState);
    }
}
