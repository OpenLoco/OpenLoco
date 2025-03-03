#include "ScrollViewWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    static void drawHScroll(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState, const ScrollArea& scrollArea)
    {
        auto* window = widgetState.window;
        const auto position = window->position() + widget.position();
        const auto size = widget.size();

        // Calculate adjusted dimensions
        auto scrollPos = Ui::Point{ position.x + 1, position.y + size.height - 11 }; // top = bottom - 10
        auto scrollSize = Ui::Size{ size.width - 2, 10 };                            // Account for 1px offset on each side

        if (scrollArea.hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            scrollSize.width -= 11;
        }

        Gfx::RectInsetFlags f;
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Left button
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarLeftPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset({ scrollPos.x, scrollPos.y }, { 10u, scrollSize.height }, widgetState.colour, f);

        // Left arrow
        {
            const char* hLeftStr = "\x90\xBE";
            tr.drawString(Point(scrollPos.x + 2, scrollPos.y), Colour::black, hLeftStr);
        }

        // Right button
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarRightPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset({ scrollPos.x + scrollSize.width - 10, scrollPos.y }, { 10U, scrollSize.height }, widgetState.colour, f);

        // Right arrow
        {
            const char* hRightStr = "\x90\xAF";
            tr.drawString(Point(scrollPos.x + scrollSize.width - 7, scrollPos.y), Colour::black, hRightStr);
        }

        const auto colour = widgetState.colour;
        // Scroll track
        drawingCtx.fillRect({ scrollPos.x + 10, scrollPos.y }, { scrollSize.width - 20, +scrollSize.height }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 10, scrollPos.y }, { scrollSize.width - 20, +scrollSize.height }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);

        // Track lines
        drawingCtx.fillRect({ scrollPos.x + 10, scrollPos.y + 2 }, { scrollSize.width - 20, 1 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 10, scrollPos.y + 3 }, { scrollSize.width - 20, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 10, scrollPos.y + 7 }, { scrollSize.width - 20, 1 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 10, scrollPos.y + 8 }, { scrollSize.width - 20, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Thumb
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarThumbPressed))
        {
            f = Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset({ scrollPos.x - 1 + scrollArea.hThumbLeft, scrollPos.y }, { scrollArea.hThumbRight - scrollArea.hThumbLeft, +scrollSize.height }, colour, f);
    }

    static void drawVScroll(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState, const ScrollArea& scrollArea)
    {
        auto* window = widgetState.window;
        const auto position = window->position() + widget.position();
        const auto size = widget.size();

        // Calculate adjusted dimensions
        auto scrollPos = Ui::Point{ position.x + size.width - 11, position.y + 1 }; // left = right - 10
        auto scrollSize = Ui::Size{ 10, size.height - 1 };                          // Account for 1px offset on each side

        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            scrollSize.height -= 10;
        }

        Gfx::RectInsetFlags f = Gfx::RectInsetFlags::none;
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Up button
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarUpPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset({ scrollPos.x, scrollPos.y }, { scrollSize.width, 10U }, widgetState.colour, f);

        // Up arrow
        {
            const char* vTopStr = "\x90\xA0";
            tr.drawString(Point(scrollPos.x + 1, scrollPos.y - 1), Colour::black, vTopStr);
        }

        // Down button
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarDownPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset({ scrollPos.x, scrollPos.y + scrollSize.height - 10 }, { scrollSize.width, 10U }, widgetState.colour, f);

        // Down arrow
        {
            const char* vBottomStr = "\x90\xAA";
            tr.drawString(Point(scrollPos.x + 1, scrollPos.y + scrollSize.height - 9), Colour::black, vBottomStr);
        }

        const auto colour = widgetState.colour;
        // Scroll track
        drawingCtx.fillRect({ scrollPos.x, scrollPos.y + 10 }, { +scrollSize.width, scrollSize.height - 20 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x, scrollPos.y + 10 }, { +scrollSize.width, scrollSize.height - 20 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);

        // Track lines
        drawingCtx.fillRect({ scrollPos.x + 2, scrollPos.y + 10 }, { 1, scrollSize.height - 20 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 3, scrollPos.y + 10 }, { 1, scrollSize.height - 20 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 7, scrollPos.y + 10 }, { 1, scrollSize.height - 20 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect({ scrollPos.x + 8, scrollPos.y + 10 }, { 1, scrollSize.height - 20 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Thumb
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarThumbPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset({ scrollPos.x, scrollPos.y - 1 + scrollArea.vThumbTop }, { +scrollSize.width, scrollArea.vThumbBottom - scrollArea.vThumbTop + 2 }, colour, f);
    }

    // 0x004CAB58
    void ScrollView::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        const auto position = window->position() + widget.position();
        const auto size = widget.size();

        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw background with inset
        drawingCtx.fillRectInset(position, size, widgetState.colour, widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        // Adjusted content area (1px inset)
        auto contentPos = Ui::Point{ position.x + 1, position.y + 1 };
        auto contentSize = Ui::Size{ size.width - 2, size.height - 2 };

        const auto& scrollArea = window->scrollAreas[widgetState.scrollviewIndex];

        tr.setCurrentFont(Gfx::Font::medium_bold);
        if (scrollArea.contentWidth > size.width && scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarVisible))
        {
            drawHScroll(drawingCtx, widget, widgetState, scrollArea);
            contentSize.height -= 11;
        }

        if (scrollArea.contentHeight > size.height && scrollArea.hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            drawVScroll(drawingCtx, widget, widgetState, scrollArea);
            contentSize.width -= 11;
        }

        Gfx::RenderTarget cropped = drawingCtx.currentRenderTarget();
        // Restore original dimensions for cropping calculations
        auto cropSize = Ui::Size{ contentSize.width + 1, contentSize.height + 1 };

        if (contentPos.x > cropped.x)
        {
            int offset = contentPos.x - cropped.x;
            cropped.width -= offset;
            cropped.x = contentPos.x;
            cropped.pitch += offset;
            cropped.bits += offset;
        }

        int16_t bp = cropped.x + cropped.width - (contentPos.x + cropSize.width);
        if (bp > 0)
        {
            cropped.width -= bp;
            cropped.pitch += bp;
        }

        if (contentPos.y > cropped.y)
        {
            int offset = contentPos.y - cropped.y;
            cropped.height -= offset;
            cropped.y = contentPos.y;
            int aex = (cropped.pitch + cropped.width) * offset;
            cropped.bits += aex;
        }

        bp = cropped.y + cropped.height - (contentPos.y + cropSize.height);
        if (bp > 0)
        {
            cropped.height -= bp;
        }

        if (cropped.width > 0 && cropped.height > 0)
        {
            cropped.x -= contentPos.x - scrollArea.contentOffsetX;
            cropped.y -= contentPos.y - scrollArea.contentOffsetY;

            drawingCtx.pushRenderTarget(cropped);
            window->callDrawScroll(drawingCtx, widgetState.scrollviewIndex);
            drawingCtx.popRenderTarget();
        }
    }
}
