#include "TabWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CADE8
    static void drawTabBackground(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();

        // TODO: This is always ImageIds::tab at the moment, we should make this implicit.
        ImageId imageId = ImageId{ widget.image };
        if (widgetState.activated)
        {
            // TODO: remove image addition
            imageId = imageId.withIndexOffset(1);
        }

        auto colour = widgetState.colour;
        if (widgetState.disabled)
        {
            uint8_t c;
            if (colour.isTranslucent())
            {
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(pos + Ui::Point{ 1, 1 }, imageId, c);
                c = Colours::getShade(colour.c(), 2);
                drawingCtx.drawImageSolid(pos, imageId, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                drawingCtx.drawImageSolid(pos + Ui::Point{ 1, 1 }, imageId, c);
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(pos, imageId, c);
            }

            return;
        }

        imageId = imageId.withPrimary(colour.c());

        drawingCtx.drawImage(pos, imageId);
    }

    void Tab::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        drawTabBackground(drawingCtx, widget, widgetState);

        // TODO: Draw the content of the tab once the background is implicit.
    }
}
