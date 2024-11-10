#include "LabelWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"
#include <cassert>

namespace OpenLoco::Ui::Widgets
{
    // 0x004CB21D
    void Label::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.text == StringIds::null || widget.text == StringIds::empty)
        {
            return;
        }

        auto colour = widgetState.colour;
        colour = colour.opaque();
        if (widgetState.disabled)
        {
            colour = colour.inset();
        }

        auto* window = widgetState.window;

        auto formatArgs = FormatArguments(widget.textArgs);
        auto tr = Gfx::TextRenderer(drawingCtx);

        const int16_t x = [&]() {
            if (widget.contentAlign == ContentAlign::Left)
            {
                return window->x + widget.left;
            }
            else if (widget.contentAlign == ContentAlign::Center)
            {
                return window->x + (widget.left + widget.right + 1) / 2 - 1;
            }
            else if (widget.contentAlign == ContentAlign::Right)
            {
                // TODO: This is not ideal, add drawStringRightClipped to TextRenderer.
                char buffer[512]{};
                StringManager::formatString(buffer, std::size(buffer), widget.text, formatArgs);

                const auto stringWidth = tr.getStringWidthNewLined(buffer);

                return window->x + widget.left + (widget.width() - stringWidth) - 1;
            }
            assert(false);
            return 0;
        }();

        int16_t y = window->y + std::max<int16_t>(widget.top, (widget.top + widget.bottom) / 2 - 5);
        int16_t width = widget.right - widget.left - 2;

        if (widget.contentAlign == ContentAlign::Left)
        {
            tr.drawStringLeftClipped(Point(x, y), width, colour, widget.text, formatArgs);
        }
        else if (widget.contentAlign == ContentAlign::Center)
        {
            tr.drawStringCentredClipped(Point(x, y), width, colour, widget.text, formatArgs);
        }
        else if (widget.contentAlign == ContentAlign::Right)
        {
            tr.drawStringLeftClipped(Point(x, y), width, colour, widget.text, formatArgs);
        }
    }
}
