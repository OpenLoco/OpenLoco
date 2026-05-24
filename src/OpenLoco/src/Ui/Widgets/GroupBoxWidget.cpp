#include "GroupBoxWidget.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    void GroupBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto position = window->position() + widget.position();
        const auto size = widget.size();

        auto colour = widgetState.colour.opaque();
        int16_t textEndPos = position.x + 5;

        // Draw label text if present
        if (widget.text != StringIds::null)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);
            tr.setCurrentFont(widget.font);

            char buffer[512] = { 0 };
            StringManager::formatString(buffer, sizeof(buffer), widget.text);

            auto point = Point(position.x + 5, position.y);
            tr.drawString(point, colour, buffer);
            textEndPos = position.x + 5 + tr.getStringWidth(buffer) + 1;
        }

        // Prepare border position and size
        const auto borderPos = position + Ui::Point{ 0, 4 };
        const auto borderSize = Ui::Size{ size.width + 0, size.height - 4 };

        // Border left of text
        drawingCtx.fillRect(borderPos, { 5, 1 }, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(borderPos + Ui::Point{ 1, 1 }, { 4, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border right of text
        drawingCtx.fillRect(Ui::Point{ textEndPos, borderPos.y }, { borderSize.width - (textEndPos - borderPos.x) - 1, 1 }, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(Ui::Point{ textEndPos, borderPos.y } + Ui::Point(0, 1), { borderSize.width - (textEndPos - borderPos.x) - 2, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border right
        drawingCtx.fillRect(Ui::Point{ borderPos.x + borderSize.width - 1, borderPos.y + 1 }, { 1, borderSize.height - 1 }, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(Ui::Point{ borderPos.x + borderSize.width, borderPos.y }, { 1, borderSize.height + 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border bottom
        drawingCtx.fillRect(Ui::Point{ borderPos.x, borderPos.y + borderSize.height - 1 }, { borderSize.width - 1, 1 }, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(Ui::Point{ borderPos.x, borderPos.y + borderSize.height }, { borderSize.width + 0, 1 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);

        // Border left
        drawingCtx.fillRect(Ui::Point{ borderPos.x, borderPos.y + 1 }, { 1, borderSize.height - 2 }, Colours::getShade(colour.c(), 4), Gfx::RectFlags::none);
        drawingCtx.fillRect(Ui::Point{ borderPos.x + 1, borderPos.y + 2 }, { 1, borderSize.height - 3 }, Colours::getShade(colour.c(), 7), Gfx::RectFlags::none);
    }
}
