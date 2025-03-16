#include "CheckboxWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    static constexpr auto kCheckMarkSize = Ui::Size{ 10, 10 };
    static constexpr auto kLabelMarginLeft = 4;

    // 0x004CB00B
    static void drawCheckBox(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();

        drawingCtx.fillRectInset(
            pos,
            kCheckMarkSize,
            widgetState.colour,
            widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        if (widgetState.activated)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);
            static constexpr char strCheckmark[] = "\xAC";

            auto colour = widgetState.colour;
            if (widgetState.disabled)
            {
                colour = colour.inset();
            }

            tr.setCurrentFont(widget.font);
            tr.drawString(pos, colour.opaque(), strCheckmark);
        }
    }

    // 0x004CB080
    static void drawLabel(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
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
        tr.setCurrentFont(widget.font);

        const auto pos = window->position() + widget.position();
        tr.drawStringLeft(pos + Point{ kCheckMarkSize.width + kLabelMarginLeft, 0 }, colour, widget.text, formatArgs);
    }

    void Checkbox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawCheckBox(drawingCtx, widget, widgetState);
        drawLabel(drawingCtx, widget, widgetState);
    }
}
