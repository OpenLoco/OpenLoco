#include "PanelWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CAB8E
    static void drawResizeHandle(Gfx::DrawingContext& drawingCtx, const Window* window, const Widget& widget, AdvancedColour colour)
    {
        if (!window->hasFlags(WindowFlags::resizable))
        {
            return;
        }

        if (window->minHeight == window->maxHeight || window->minWidth == window->maxWidth)
        {
            return;
        }

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        const auto resizeBarPos = pos + Ui::Point(size.width - 18, size.height - 18);

        uint32_t image = Gfx::recolour(ImageIds::window_resize_handle, colour.c());
        drawingCtx.drawImage(resizeBarPos, image);
    }

    // 0x004CAB58
    void Panel::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        drawingCtx.fillRectInset(
            pos,
            size,
            widgetState.colour,
            widgetState.flags);

        drawResizeHandle(drawingCtx, window, widget, widgetState.colour);
    }
}
