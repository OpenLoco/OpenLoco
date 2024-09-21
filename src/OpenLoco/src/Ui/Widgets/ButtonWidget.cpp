#include "ButtonWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CB164
    static void drawButton(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto* window = widgetState.window;

        int l = window->x + widget.left;
        int r = window->x + widget.right;
        int t = window->y + widget.top;
        int b = window->y + widget.bottom;

        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        drawingCtx.fillRectInset(l, t, r, b, widgetState.colour, flags);
    }

    // 0x004CB1BE, 0x004CB21D
    static void drawText(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
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

        auto* window = widgetState.window;
        int16_t centreX = window->x + (widget.left + widget.right + 1) / 2 - 1;
        int16_t y = window->y + std::max<int16_t>(widget.top, (widget.top + widget.bottom) / 2 - 5);
        int16_t width = widget.right - widget.left - 2;

        auto formatArgs = FormatArguments(widget.textArgs);
        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.drawStringCentredClipped(Point(centreX, y), width, colour, widget.text, formatArgs);
    }

    void Button::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawButton(drawingCtx, widget, widgetState);
        drawText(drawingCtx, widget, widgetState);
    }
}
