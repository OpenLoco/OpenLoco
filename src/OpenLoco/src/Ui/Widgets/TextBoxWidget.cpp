#include "TextBoxWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x4CB29C
    void TextBox::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto flags = widgetState.flags | Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillDarker;
        drawingCtx.fillRectInset(
            window->x + widget.left,
            window->y + widget.top,
            window->x + widget.right,
            window->y + widget.bottom,
            widgetState.colour,
            flags);
    }
}
