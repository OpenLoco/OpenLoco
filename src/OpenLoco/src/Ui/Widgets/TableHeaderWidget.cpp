#include "TableHeaderWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"
#include <cassert>

namespace OpenLoco::Ui::Widgets
{
    void TableHeader::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        StringId labelText = StringIds::table_header;
        switch (Style(widget.styleData))
        {
            case Style::notSorted:
                break;
            case Style::sortedAscending:
                labelText = StringIds::table_header_ascending;
                break;
            case Style::sortedDescending:
                labelText = StringIds::table_header_descending;
                break;
        }

        // the following is adapted from ButtonWidget and LabelWidget
        // TODO make functions for common code instead of duplicating logic
        
        const auto* window = widgetState.window;

        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        drawingCtx.fillRectInset(window->position() + widget.position(), widget.size(), widgetState.colour, flags);




        if (widget.text == StringIds::null || widget.text == StringIds::empty)
        {
            return;
        }

        auto colour = widgetState.colour.opaque();
        if (widgetState.disabled)
        {
            colour = colour.inset();
        }

        const auto position = window->position() + widget.position();
        const auto size = widget.size();

        auto formatArgs = FormatArguments(widget.textArgs); // Hello

        formatArgs.push(widget.text);

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.setCurrentFont(widget.font);

        const auto fontHeight = tr.getLineHeight(tr.getCurrentFont());
        // NOTE: -1 is an ugly hack for buttons with inset border, remove that when all buttons have consistent height.
        const int16_t yOffset = std::max<int16_t>(0, (size.height - fontHeight) / 2 - 1);
        const int16_t x = position.x;
        const int16_t y = position.y + yOffset;
        const int16_t width = size.width - 2;

        tr.drawStringLeftClipped(Point(x, y), width, colour, labelText, formatArgs);
    }
}
