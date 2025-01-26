#include "TableHeaderWidget.h"
#include "ButtonWidget.h"

namespace OpenLoco::Ui::Widgets
{
    void TableHeader::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        Button::draw(drawingCtx, widget, widgetState);
    }
}
