#include "NewsPanelWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"
#include <cassert>

namespace OpenLoco::Ui::Widgets
{
    void NewsPanel::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto style = static_cast<Style>(widget.styleData);
        if (style == Style::old)
        {
            auto imageId = Gfx::recolour(ImageIds::news_background_old_left, ExtColour::translucentBrown1);
            drawingCtx.drawImage(widget.left, widget.top, imageId);

            imageId = Gfx::recolour(ImageIds::news_background_old_right, ExtColour::translucentBrown1);

            drawingCtx.drawImage(widget.left + (window->width / 2), widget.top, imageId);
        }
        else if (style == Style::new_)
        {
            drawingCtx.drawImage(widget.left, widget.top, ImageIds::news_background_new_left);

            drawingCtx.drawImage((window->width / 2), widget.top, ImageIds::news_background_new_right);
        }
        else
        {
            assert(false);
        }
    }
}
