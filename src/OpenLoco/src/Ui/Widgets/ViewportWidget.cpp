#include "ViewportWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "TextBoxWidget.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
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

    static void drawViewports(Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const Widget& widget, const WidgetState& widgetState)
    {
        // TODO: Move the viewport into the widget.
        auto* window = widgetState.window;

        // TODO: Move viewports into the Widget
        auto& viewports = window->viewports;

        if (viewports[0] != nullptr)
        {
            viewports[0]->render(drawingCtx);
        }

        if (viewports[1] != nullptr)
        {
            viewports[1]->render(drawingCtx);
        }
    }

    void Viewport::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        // TODO: Not an actual TextBox, this is just inset border and background.
        //       Add border style to the Widget.
        TextBox::draw(drawingCtx, widget, widgetState);
        drawText(drawingCtx, widget, widgetState);
        drawViewports(drawingCtx, widget, widgetState);
    }

}
