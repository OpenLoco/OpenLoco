#include "ToolbarButtonWidget.h"
#include "Graphics/DrawingContext.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    void ToolbarButton::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();

        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet);

        auto colour = widgetState.colour;

        if (!isColourSet && imageId.hasSecondary())
        {
            imageId = imageId.withSecondary(colour.c());
        }

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(colour.c());
        }

        if (!isColourSet)
        {
            imageId = ImageId::fromUInt32(Gfx::recolour(imageId.getIndex(), colour.c()));
        }

        if (widgetState.activated)
        {
            imageId = imageId.withIndexOffset(1);
        }

        drawingCtx.drawImage(pos, imageId);
    }
}
