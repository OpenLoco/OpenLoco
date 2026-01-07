#include "ToolbarButtonWidget.h"
#include "ImageButtonWidget.h"

namespace OpenLoco::Ui::Widgets
{
    void ToolbarButton::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        ImageButton::draw(drawingCtx, widget, widgetState);
    }
}
