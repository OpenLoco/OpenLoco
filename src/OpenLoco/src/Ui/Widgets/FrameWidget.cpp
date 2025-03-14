#include "FrameWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
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

    // 0x004CAAB9
    void Frame::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        const auto& rt = drawingCtx.currentRenderTarget();
        const auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(pos.x, pos.y, size.width, 41));
        if (clipped)
        {
            uint32_t imageId = widget.image;
            if (window->hasFlags(WindowFlags::flag_11))
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image, widgetState.colour.c());
            }
            else
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image_alt, widgetState.colour.c());
            }

            drawingCtx.pushRenderTarget(*clipped);
            drawingCtx.drawImage(0, 0, imageId);
            drawingCtx.popRenderTarget();
        }

        uint8_t shade;
        if (window->hasFlags(WindowFlags::flag_11))
        {
            shade = Colours::getShade(widgetState.colour.c(), 3);
        }
        else
        {
            shade = Colours::getShade(widgetState.colour.c(), 1);
        }

        // Shadow at the right side.
        drawingCtx.fillRect(
            pos + Point{ size.width - 1, 0 },
            Ui::Size{ 1, 40u },
            shade,
            Gfx::RectFlags::none);

        drawResizeHandle(drawingCtx, window, widget, widgetState.colour);
    }
}
