#include "SliderWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "TabWidget.h"

namespace OpenLoco::Ui::Widgets
{
    void Slider::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        Tab::draw(drawingCtx, widget, widgetState);
    }
}
