#include "CaptionWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"
#include <cassert>

namespace OpenLoco::Ui::Widgets
{
    // 0x004CA6AE
    static void drawBoxed(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        int l = window->x + widget.left;
        int r = window->x + widget.right;
        int t = window->y + widget.top;
        int b = window->y + widget.bottom;

        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.fillRectInset(
            l,
            t,
            r,
            b,
            widgetState.colour,
            widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        drawingCtx.fillRect(
            l + 1,
            t + 1,
            r - 1,
            b - 1,
            enumValue(ExtColour::unk2E),
            Gfx::RectFlags::transparent);

        int16_t width = r - l - 4 - 10;
        auto point = Point(l + 2 + (width / 2), t + 1);

        auto formatArgs = FormatArguments(widget.textArgs);
        tr.drawStringCentredClipped(
            point,
            width,
            AdvancedColour(Colour::white).outline(),
            widget.text,
            formatArgs);
    }

    // 0x004CF3EB
    static void drawStationNameBackground(Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const Window* window, [[maybe_unused]] const Widget* widget, int16_t x, int16_t y, AdvancedColour colour, int16_t width)
    {
        drawingCtx.drawImage(x - 4, y, Gfx::recolour(ImageIds::curved_border_left_medium, colour.c()));
        drawingCtx.drawImage(x + width, y, Gfx::recolour(ImageIds::curved_border_right_medium, colour.c()));
        drawingCtx.fillRect(x, y, x + width - 1, y + 11, Colours::getShade(colour.c(), 5), Gfx::RectFlags::none);
    }

    static constexpr uint8_t kTextColourByType[3] = {
        ControlCodes::Colour::black, // 23
        ControlCodes::windowColour1, // 24
        ControlCodes::Colour::white, // 25
    };

    static void drawSimple(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto formatArgs = FormatArguments(widget.textArgs);

        char stringBuffer[512];
        stringBuffer[0] = kTextColourByType[enumValue(widget.type) - enumValue(WidgetType::caption_23)];
        StringManager::formatString(&stringBuffer[1], widget.text, formatArgs);

        auto* window = widgetState.window;
        int16_t width = widget.right - widget.left - 4 - 14;
        int16_t x = widget.left + window->x + 2 + (width / 2);

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.setCurrentFont(Gfx::Font::medium_bold);
        int16_t stringWidth = tr.clipString(width - 8, stringBuffer);
        x -= (stringWidth - 1) / 2;

        if (widget.type == WidgetType::caption_23)
        {
            int16_t y = window->y + widget.top + 1;
            drawStationNameBackground(drawingCtx, window, &widget, x, y, widgetState.colour, stringWidth);
        }

        auto point = Point(x, window->y + widget.top + 1);
        tr.drawString(point, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    void Caption::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.type != WidgetType::caption_22)
        {
            drawSimple(drawingCtx, widget, widgetState);
        }
        else
        {
            drawBoxed(drawingCtx, widget, widgetState);
        }
    }
}
