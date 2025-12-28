#include "FrameWidget.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
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

        const auto resizeBarPos = pos + Ui::Point(size.width - kResizeHandleSize, size.height - kResizeHandleSize);

        uint32_t image = Gfx::recolour(ImageIds::window_resize_handle, colour.c());
        drawingCtx.drawImage(resizeBarPos, image);
    }

    // 0x004CAAB9
    void Frame::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        switch (Config::get().windowFrameStyle)
        {
            case Config::WindowFrameStyle::background:
                Frame::drawBackground(drawingCtx, widget, widgetState);
                break;
            case Config::WindowFrameStyle::solid:
                Frame::drawSolid(drawingCtx, widget, widgetState);
                break;
            case Config::WindowFrameStyle::transparent:
                Frame::drawTransparent(drawingCtx, widget, widgetState);
                break;
        }

        uint8_t shade;
        const auto* window = widgetState.window;
        if (window->hasFlags(WindowFlags::lighterFrame))
        {
            shade = Colours::getShade(widgetState.colour.c(), 3);
        }
        else
        {
            shade = Colours::getShade(widgetState.colour.c(), 1);
        }

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        // Shadow at the right side.
        drawingCtx.fillRect(
            pos + Point{ size.width - 1, 0 },
            Ui::Size{ 1, 40u },
            shade,
            Gfx::RectFlags::none);

        drawResizeHandle(drawingCtx, window, widget, widgetState.colour);
    }

    void Frame::drawBackground(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto* window = widgetState.window;
        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        const auto& rt = drawingCtx.currentRenderTarget();
        const auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(pos.x, pos.y, size.width, 41));
        if (clipped)
        {
            uint32_t imageId = widget.image;
            if (window->hasFlags(WindowFlags::lighterFrame))
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image, widgetState.colour.c());
            }
            else
            {
                imageId = Gfx::recolour(ImageIds::frame_background_image_alt, widgetState.colour.c());
            }

            drawingCtx.pushRenderTarget(*clipped);

            // Derive the number of background images to paint
            const auto backgroundImageWidth = Gfx::getG1Element(imageId)->width;
            const auto numPassesNeeded = (widget.width() + backgroundImageWidth - 1) / backgroundImageWidth;

            // Draw background image repeatedly to account for large windows
            // NB: starting on the right side to counter the border on the left side of the sprite
            for (auto i = numPassesNeeded; i >= 0; i--)
            {
                drawingCtx.drawImage(i * (backgroundImageWidth - 1), 0, imageId);
            }

            drawingCtx.popRenderTarget();
        }
    }

    void Frame::drawSolid(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto flags = widgetState.flags;
        const auto colour = widgetState.colour;
        const auto* window = widgetState.window;
        drawingCtx.fillRectInset(window->position() + widget.position(), widget.size(), colour, flags);
    }

    void Frame::drawTransparent(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto flags = widgetState.flags;
        const auto colour = widgetState.colour.translucent();
        const auto* window = widgetState.window;
        drawingCtx.fillRectInset(window->position() + widget.position(), widget.size(), colour, flags);
    }
}
