#include "ButtonWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "LabelWidget.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CB164
    void Button::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        const auto* window = widgetState.window;

        auto flags = widgetState.flags;
        if (widgetState.activated)
        {
            flags |= Gfx::RectInsetFlags::borderInset;
        }

        drawingCtx.fillRectInset(window->position() + widget.position(), widget.size(), widgetState.colour, flags);

        Label::draw(drawingCtx, widget, widgetState);
    }
}
