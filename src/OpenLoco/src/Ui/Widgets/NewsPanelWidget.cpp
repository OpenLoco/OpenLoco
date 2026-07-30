#include "Ui/Widgets/NewsPanelWidget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"
#include <cassert>

namespace OpenLoco::Ui::Widgets
{
    void NewsPanel::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, [[maybe_unused]] const WidgetState& widgetState)
    {
        const auto centerPos = Point(widget.width() / 2, 0);

        const auto style = static_cast<Style>(widget.styleData);
        if (style == Style::old)
        {
            auto imageId = Gfx::recolour(ImageIds::news_background_old_left, ExtColour::translucentBrown1);
            drawingCtx.drawImage(ZoomLevel::full, Ui::Point{}, imageId);

            imageId = Gfx::recolour(ImageIds::news_background_old_right, ExtColour::translucentBrown1);

            drawingCtx.drawImage(ZoomLevel::full, centerPos, imageId);
        }
        else if (style == Style::new_)
        {
            drawingCtx.drawImage(ZoomLevel::full, Ui::Point{}, ImageIds::news_background_new_left);

            drawingCtx.drawImage(ZoomLevel::full, centerPos, ImageIds::news_background_new_right);
        }
        else
        {
            assert(false);
        }
    }
}
