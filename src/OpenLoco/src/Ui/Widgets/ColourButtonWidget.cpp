#include "ColourButtonWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CB164
    void ColourButton::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        // TODO: Remove addition
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet);

        if (!widgetState.disabled && widgetState.activated && widgetState.hovered)
        {
            // TODO: Remove addition
            imageId = imageId.withIndexOffset(2);
        }
        else if (!widgetState.disabled && !widgetState.activated && widgetState.hovered)
        {
            // TODO: Remove addition
            imageId = imageId.withIndexOffset(1);
        }

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(widgetState.colour.c());
        }

        drawingCtx.drawImage({}, imageId);
    }
}
