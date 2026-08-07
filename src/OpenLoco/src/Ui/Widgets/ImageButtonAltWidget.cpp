#include "Ui/Widgets/ImageButtonAltWidget.h"
#include "Graphics/DrawingContext.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    void ImageButtonAlt::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
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

        drawingCtx.drawImage(ZoomLevel::full, Ui::Point{}, imageId);
    }
}
