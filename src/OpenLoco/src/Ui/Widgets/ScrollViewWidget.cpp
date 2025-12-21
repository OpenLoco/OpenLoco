#include "ScrollViewWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/ScrollView.h"
#include "Ui/Window.h"

using namespace OpenLoco::Ui::ScrollView;

namespace OpenLoco::Ui::Widgets
{
    static constexpr auto kArrowOffset = 2;

    static void drawHScroll(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState, const ScrollArea& scrollArea)
    {
        auto* window = widgetState.window;
        const auto position = window->position() + widget.position();
        const auto size = widget.size();
        const auto colour = widgetState.colour;

        // Calculate adjusted dimensions
        auto scrollPos = position + Point{ kScrollbarMargin, size.height - kScrollbarSize - kScrollbarMargin };
        auto scrollSize = Ui::Size{ size.width - (kScrollbarMargin * 2), kScrollbarSize };

        if (scrollArea.hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            scrollSize.width -= kScrollbarSize + kScrollbarMargin;
        }

        Gfx::RectInsetFlags f;
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Scroll track
        drawingCtx.fillRect(scrollPos, scrollSize, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos, scrollSize, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);

        // Track lines
        drawingCtx.fillRect(scrollPos + Point{ 0, 2 }, { +scrollSize.width, 1 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos + Point{ 0, 3 }, { +scrollSize.width, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos + Point{ 0, 7 }, { +scrollSize.width, 1 }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos + Point{ 0, 8 }, { +scrollSize.width, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Left button
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarLeftPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(scrollPos, kScrollButtonSize, widgetState.colour, f);

        // Left arrow
        {
            const char* hLeftStr = "\x90\xBE";
            tr.drawString(scrollPos + Point{ kArrowOffset, 0 }, Colour::black, hLeftStr);
        }

        // Right button
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarRightPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(
            scrollPos + Point{ scrollSize.width - kScrollButtonSize.width + kScrollbarMargin, 0 },
            kScrollButtonSize,
            widgetState.colour,
            f);

        // Right arrow
        {
            const char* hRightStr = "\x90\xAF";
            tr.drawString(
                scrollPos + Point{ scrollSize.width - kScrollButtonSize.width + kScrollbarMargin + kArrowOffset, 0 },
                Colour::black,
                hRightStr);
        }

        // Thumb
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarThumbPressed))
        {
            f = Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(
            scrollPos + Point{ scrollArea.hThumbLeft, 0 },
            { scrollArea.hThumbRight - scrollArea.hThumbLeft - kScrollbarMargin, +scrollSize.height },
            colour,
            f);
    }

    static void drawVScroll(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState, const ScrollArea& scrollArea)
    {
        auto* window = widgetState.window;
        const auto position = window->position() + widget.position();
        const auto size = widget.size();
        const auto colour = widgetState.colour;

        // Calculate adjusted dimensions
        auto scrollPos = position + Point{ size.width - kScrollbarSize - kScrollbarMargin, kScrollbarMargin };
        auto scrollSize = Ui::Size{ kScrollbarSize, size.height - (kScrollbarMargin * 2) };

        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            scrollSize.height -= kScrollbarSize;
        }

        Gfx::RectInsetFlags f = Gfx::RectInsetFlags::none;
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Scroll track
        drawingCtx.fillRect(scrollPos, scrollSize, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos, scrollSize, Colours::getShade(colour.c(), 3), Gfx::RectFlags::crossHatching);

        // Track lines
        drawingCtx.fillRect(scrollPos + Point{ 2, 0 }, { 1, +scrollSize.height }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos + Point{ 3, 0 }, { 1, +scrollSize.height }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos + Point{ 7, 0 }, { 1, +scrollSize.height }, Colours::getShade(colour.c(), 3), Gfx::RectFlags::none);
        drawingCtx.fillRect(scrollPos + Point{ 8, 0 }, { 1, +scrollSize.height }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Up button
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarUpPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(scrollPos, kScrollButtonSize, widgetState.colour, f);

        // Up arrow
        {
            const char* vTopStr = "\x90\xA0";
            tr.drawString(scrollPos + Point{ 1, -1 }, Colour::black, vTopStr);
        }

        // Down button
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarDownPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(
            scrollPos + Point{ 0, scrollSize.height - kScrollButtonSize.height },
            kScrollButtonSize,
            widgetState.colour,
            f);

        // Down arrow
        {
            const char* vBottomStr = "\x90\xAA";
            tr.drawString(
                scrollPos + Point{ kScrollbarMargin, scrollSize.height - kScrollButtonSize.height },
                Colour::black,
                vBottomStr);
        }

        // Thumb
        f = Gfx::RectInsetFlags::none;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarThumbPressed))
        {
            f = widgetState.flags | Gfx::RectInsetFlags::borderInset;
        }
        drawingCtx.fillRectInset(
            scrollPos + Point{ 0, scrollArea.vThumbTop },
            { +scrollSize.width, scrollArea.vThumbBottom - scrollArea.vThumbTop - kScrollbarMargin },
            colour,
            f);
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
        auto contentPos = position + Point{ kScrollbarMargin, kScrollbarMargin };
        auto contentSize = Ui::Size{ size.width - (kScrollbarMargin * 2), size.height - (kScrollbarMargin * 2) };

        const auto& scrollArea = window->scrollAreas[widgetState.scrollviewIndex];

        tr.setCurrentFont(Gfx::Font::medium_bold);
        if (scrollArea.contentWidth > size.width && scrollArea.hasFlags(Ui::ScrollFlags::hscrollbarVisible))
        {
            drawHScroll(drawingCtx, widget, widgetState, scrollArea);
            contentSize.height -= kScrollbarSize;
        }

        if (scrollArea.contentHeight > size.height && scrollArea.hasFlags(Ui::ScrollFlags::vscrollbarVisible))
        {
            drawVScroll(drawingCtx, widget, widgetState, scrollArea);
            contentSize.width -= kScrollbarSize;
        }

        Gfx::RenderTarget cropped = drawingCtx.currentRenderTarget();
        // Restore original dimensions for cropping calculations
        auto cropSize = Ui::Size{ contentSize.width, contentSize.height };

        if (contentPos.x > cropped.x)
        {
            int offset = contentPos.x - cropped.x;
            cropped.width -= offset;
            cropped.x = contentPos.x;
            cropped.pitch += offset;
            cropped.bits += offset;
        }

        auto bp = cropped.x + cropped.width - (contentPos.x + cropSize.width);
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
