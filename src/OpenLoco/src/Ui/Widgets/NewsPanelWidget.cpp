#include "NewsPanelWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"
#include <cassert>

namespace OpenLoco::Ui::Widgets
{
    void NewsPanel::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, [[maybe_unused]] const WidgetState& widgetState)
    {
        const auto pos = widget.position();
        const auto centerPos = pos + Point(widget.width() / 2, 0);

        const auto style = static_cast<Style>(widget.styleData);
        if (style == Style::old)
        {
            auto imageId = Gfx::recolour(ImageIds::news_background_old_left, ExtColour::translucentBrown1);
            drawingCtx.drawImage(pos, imageId);

            imageId = Gfx::recolour(ImageIds::news_background_old_right, ExtColour::translucentBrown1);

            drawingCtx.drawImage(centerPos, imageId);
        }
        else if (style == Style::new_)
        {
            drawingCtx.drawImage(pos, ImageIds::news_background_new_left);

            drawingCtx.drawImage(centerPos, ImageIds::news_background_new_right);
        }
        else
        {
            assert(false);
        }
    }
}
