#include "Ui/Widgets/SliderWidget.h"
#include "Graphics/DrawingContext.h"

namespace OpenLoco::Ui::Widgets
{
    void Slider::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        drawingCtx.drawImage(ZoomLevel::full, Point(0, 0), Gfx::recolour(ImageIds::volume_slider_track, widgetState.colour.c()));
        drawingCtx.drawImage(ZoomLevel::full, Point(widget.content, 0), Gfx::recolour(ImageIds::volume_slider_thumb, widgetState.colour.c()));
    }
}
