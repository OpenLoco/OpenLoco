#include "GroupBoxWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    void GroupBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        auto colour = widgetState.colour;
        colour = colour.opaque();
        int32_t l = window->x + widget.left + 5;
        int32_t t = window->y + widget.top;
        int32_t r = window->x + widget.right;
        int32_t b = window->y + widget.bottom;
        int32_t textEndPos = l;

        // First, draw the label text, if any.
        if (widget.text != StringIds::null)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            char buffer[512] = { 0 };
            StringManager::formatString(buffer, sizeof(buffer), widget.text);

            auto point = Point(l, t);
            tr.drawString(point, colour, buffer);
            textEndPos = l + tr.getStringWidth(buffer) + 1;
        }

        // Prepare border dimensions
        l = window->x + widget.left;
        t = window->y + widget.top + 4;
        r = window->x + widget.right;
        b = window->y + widget.bottom;

        // Border left of text
        drawingCtx.fillRect(l, t, l + 4, t, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(l + 1, t + 1, l + 4, t + 1, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border right of text
        drawingCtx.fillRect(textEndPos, t, r - 1, t, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(textEndPos, t + 1, r - 2, t + 1, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border right
        drawingCtx.fillRect(r - 1, t + 1, r - 1, b - 1, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(r, t, r, b, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border bottom
        drawingCtx.fillRect(l, b - 1, r - 2, b - 1, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(l, b, r - 1, b, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border left
        drawingCtx.fillRect(l, t + 1, l, b - 2, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(l + 1, t + 2, l + 1, b - 2, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
    }
}
