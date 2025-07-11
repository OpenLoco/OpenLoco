#include "ButtonWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "LabelWidget.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CB164
    void Button::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        drawingCtx.fillRectInset({}, widget.size(), widgetState.colour, flags);

        Label::draw(drawingCtx, widget, widgetState);
    }
}
