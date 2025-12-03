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

        auto tr = Gfx::TextRenderer(drawingCtx);

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        drawingCtx.fillRectInset(
            pos,
            size,
            widgetState.colour,
            widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker);

        drawingCtx.fillRect(
            pos + Ui::Point(1, 1),
            size - Ui::Size(2, 2),
            enumValue(ExtColour::unk2E),
            Gfx::RectFlags::transparent);

        int16_t width = size.width - 4 - 10;
        auto centerPos = pos + Point(2 + (width / 2), 1);

        auto formatArgs = FormatArguments(widget.textArgs);
        tr.drawStringCentredClipped(
            centerPos,
            width,
            AdvancedColour(Colour::white).outline(),
            widget.text,
            formatArgs);
    }

    // 0x004CF3EB
    static void drawStationNameBackground(Gfx::DrawingContext& drawingCtx, const Ui::Point& origin, AdvancedColour colour, int32_t width)
    {
        drawingCtx.drawImage(origin - Ui::Point{ 4, 0 }, Gfx::recolour(ImageIds::curved_border_left_medium, colour.c()));
        drawingCtx.drawImage(origin + Ui::Point(width, 0), Gfx::recolour(ImageIds::curved_border_right_medium, colour.c()));
        drawingCtx.fillRect(origin, Ui::Size{ width, 11 }, Colours::getShade(colour.c(), 5), Gfx::RectFlags::none);
    }

    static void drawSimple(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState, const Caption::Style captionStyle)
    {
        auto formatArgs = FormatArguments(widget.textArgs);

        char stringBuffer[512];
        if (captionStyle == Caption::Style::blackText)
        {
            stringBuffer[0] = ControlCodes::Colour::black;
        }
        else if (captionStyle == Caption::Style::colourText)
        {
            stringBuffer[0] = ControlCodes::windowColour1;
        }
        else if (captionStyle == Caption::Style::whiteText)
        {
            stringBuffer[0] = ControlCodes::Colour::white;
        }
        else
        {
            assert(false);
        }

        StringManager::formatString(&stringBuffer[1], widget.text, formatArgs);

        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();
        const auto size = widget.size();

        int16_t width = size.width - 4 - 14;

        auto stationNamePos = pos + Ui::Point(2 + (width / 2), 1);

        auto tr = Gfx::TextRenderer(drawingCtx);
        tr.setCurrentFont(Gfx::Font::medium_bold);

        int16_t stringWidth = tr.clipString(width - 8, stringBuffer);
        stationNamePos.x -= (stringWidth - 1) / 2;

        if (captionStyle == Caption::Style::blackText)
        {
            drawStationNameBackground(drawingCtx, stationNamePos, widgetState.colour, stringWidth);
        }

        auto textPos = stationNamePos;
        tr.drawString(textPos, AdvancedColour(Colour::black).outline(), stringBuffer);
    }

    void Caption::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto captionStyle = static_cast<Caption::Style>(widget.styleData);

        if (captionStyle != Caption::Style::boxed)
        {
            drawSimple(drawingCtx, widget, widgetState, captionStyle);
        }
        else
        {
            drawBoxed(drawingCtx, widget, widgetState);
        }
    }
}
