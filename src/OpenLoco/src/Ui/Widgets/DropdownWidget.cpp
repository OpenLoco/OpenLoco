#include "DropdownWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "LabelWidget.h"
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

        auto point = Point(widget.left + 1, widget.top);
        int width = widget.right - widget.left - 2;

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.drawStringLeftClipped(point, width, colour, widget.text, formatArgs);
    }

    // 0x004CB164
    void ComboBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto flags = widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker;
        drawingCtx.fillRectInset(
            widget.left,
            widget.top,
            widget.right,
            widget.bottom,
            widgetState.colour,
            flags);

        drawText(drawingCtx, widget, widgetState);
    }
}
