#include "ViewportWidget.h"
#include "Graphics/RenderTarget.h"
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
        auto point = Point(1, 0);
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

        auto* vp0 = viewports[0];
        auto* vp1 = viewports[1];
        auto hasViewport = vp0 != nullptr || vp1 != nullptr;

        // TODO: This is a hack, we need to step back in the render target stack to render the viewports,
        // viewports currently store their position which is used to render in screen space coordinates
        // also relative to the current invalidated region, it's a bit of a mess.
        Gfx::RenderTarget savedRT;
        if (hasViewport)
        {
            savedRT = drawingCtx.currentRenderTarget();
            drawingCtx.popRenderTarget();
        }

        if (vp0 != nullptr)
        {
            vp0->render(drawingCtx);
        }

        if (vp1 != nullptr)
        {
            vp1->render(drawingCtx);
        }

        if (hasViewport)
        {
            drawingCtx.pushRenderTarget(savedRT);
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
