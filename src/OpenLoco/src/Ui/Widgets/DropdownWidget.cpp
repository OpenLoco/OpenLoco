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

        auto* window = widgetStated.window;

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        auto tr = Gfx::TextRenderer(drawingCtx);
        auto formatArgs = FormatArgumentsView(widget.textArgs);
        tr.drawStringLeftClipped(pos + Ui::Point{ 1, 1 }, size.width - 2, colour, widget.text, formatArgs);
    }

    // 0x004CB164
    void ComboBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto* window = widgetState.window;

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
