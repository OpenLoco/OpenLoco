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

        uint16_t left = window->x + widget.left + 1;
        uint16_t top = window->y + widget.top + 1;
        uint16_t right = window->x + widget.right - 1;
        uint16_t bottom = window->y + widget.bottom - 1;

        top = bottom - 10;
        if (scrollArea.hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            right -= 11;
        }

        Gfx::RectInsetFlags f;
        auto tr = Gfx::TextRenderer(drawingCtx);

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarLeftPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(left, top, left + 9, bottom, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* hLeftStr = "\x90\xBE";
            tr.drawString(Point(left + 2, top), Colour::black, hLeftStr);
        }
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarRightPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(right - 9, top, right, bottom, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* hRightStr = "\x90\xAF";
            tr.drawString(Point(right - 7, top), Colour::black, hRightStr);
        }
        // popa

        const auto colour = widgetState.colour;
        // pusha
        drawingCtx.fillRect(left + 10, top, right - 10, bottom, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 10, top, right - 10, bottom, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);
        // popa

        // pusha
        drawingCtx.fillRect(left + 10, top + 2, right - 10, top + 2, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 10, top + 3, right - 10, top + 3, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 10, top + 7, right - 10, top + 7, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 10, top + 8, right - 10, top + 8, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarThumbPressed))
        {
            f = Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(left - 1 + scrollArea.hThumbLeft, top, left - 1 + scrollArea.hThumbRight, bottom, colour, f);
        // popa
    }

    static void drawVScroll(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState, const ScrollArea& scrollArea)
    {
        auto* window = widgetState.window;

        uint16_t left = window->x + widget.left + 1;
        uint16_t top = window->y + widget.top + 1;
        uint16_t right = window->x + widget.right - 1;
        uint16_t bottom = window->y + widget.bottom - 1;

        left = right - 10;
        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            bottom -= 11;
        }

        Gfx::RectInsetFlags f = Gfx::RectInsetFlags::none;

        auto tr = Gfx::TextRenderer(drawingCtx);

        // pusha
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarUpPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(left, top, right, top + 9, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* vTopStr = "\x90\xA0";
            tr.drawString(Point(left + 1, top - 1), Colour::black, vTopStr);
        }
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarDownPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(left, bottom - 9, right, bottom, widgetState.colour, f);
        // popa

        // pusha
        {
            const char* vBottomStr = "\x90\xAA";
            tr.drawString(Point(left + 1, bottom - 9), Colour::black, vBottomStr);
        }
        // popa

        const auto colour = widgetState.colour;
        // pusha
        drawingCtx.fillRect(left, top + 10, right, bottom - 10, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(left, top + 10, right, bottom - 10, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);
        // popa

        // pusha
        drawingCtx.fillRect(left + 2, top + 10, left + 2, bottom - 10, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 3, top + 10, left + 3, bottom - 10, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 7, top + 10, left + 7, bottom - 10, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(left + 8, top + 10, left + 8, bottom - 10, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        // popa

        // pusha
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarThumbPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(left, top - 1 + scrollArea.vThumbTop, right, top - 1 + scrollArea.vThumbBottom, colour, f);
        // popa
    }

    // 0x004CAB58
    void ScrollView::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        int16_t left = window->x + widget.left;
        int16_t top = window->y + widget.top;
        int16_t right = window->x + widget.right;
        int16_t bottom = window->y + widget.bottom;

        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.fillRectInset(left, top, right, bottom, widgetState.colour, widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        left++;
        top++;
        right--;
        bottom--;

        const auto& scrollArea = window->scrollAreas[widgetState.scrollviewIndex];

        tr.setCurrentFont(Gfx::Font::medium_bold);
        if (scrollArea.contentWidth > widget.width() && scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarVisible))
        {
            drawHScroll(drawingCtx, widget, widgetState, scrollArea);
            bottom -= 11;
        }

        if (scrollArea.contentHeight > widget.height() && scrollArea.hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            drawVScroll(drawingCtx, widget, widgetState, scrollArea);
            right -= 11;
        }

        Gfx::RenderTarget cropped = drawingCtx.currentRenderTarget();
        bottom++;
        right++;

        if (left > cropped.x)
        {
            int offset = left - cropped.x;
            cropped.width -= offset;
            cropped.x = left;
            cropped.pitch += offset;

            cropped.bits += offset;
        }

        int16_t bp = cropped.x + cropped.width - right;
        if (bp > 0)
        {
            cropped.width -= bp;
            cropped.pitch += bp;
        }

        if (top > cropped.y)
        {
            int offset = top - cropped.y;
            cropped.height -= offset;
            cropped.y = top;

            int aex = (cropped.pitch + cropped.width) * offset;
            cropped.bits += aex;
        }

        bp = cropped.y + cropped.height - bottom;
        if (bp > 0)
        {
            cropped.height -= bp;
        }

        if (cropped.width > 0 && cropped.height > 0)
        {
            cropped.x -= left - scrollArea.contentOffsetX;
            cropped.y -= top - scrollArea.contentOffsetY;

            drawingCtx.pushRenderTarget(cropped);

            window->callDrawScroll(drawingCtx, widgetState.scrollviewIndex);

            drawingCtx.popRenderTarget();
        }
    }
}
