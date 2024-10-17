#include "TabWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // NOTE: Called drawTabImpl because Widget has a static function called drawTab
    // 0x004CADE8
    static void drawTabImpl(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;
        Ui::Point placeForImage(widget.left + window->x, widget.top + window->y);
        const bool isColourSet = widget.image & Widget::kImageIdColourSet;
        ImageId imageId = ImageId::fromUInt32(widget.image & ~Widget::kImageIdColourSet);
        if (widget.type == WidgetType::wt_6 || widget.type == WidgetType::toolbarTab || widget.type == WidgetType::tab || widget.type == WidgetType::wt_4)
        {
            if (widgetState.activated)
            {
                // TODO: remove image addition
                imageId = imageId.withIndexOffset(1);
            }
        }

        auto colour = widgetState.colour;
        if (widgetState.disabled)
        {
            // TODO: this is odd most likely this is another flag like Widget::kImageIdColourSet
            if (imageId.hasSecondary())
            {
                return;
            }

            // No colour applied image
            const auto pureImage = ImageId{ imageId.getIndex() };
            uint8_t c;
            if (colour.isTranslucent())
            {
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(placeForImage + Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 2);
                drawingCtx.drawImageSolid(placeForImage, pureImage, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                drawingCtx.drawImageSolid(placeForImage + Ui::Point{ 1, 1 }, pureImage, c);
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(placeForImage, pureImage, c);
            }

            return;
        }

        if (!isColourSet && imageId.hasSecondary())
        {
            imageId = imageId.withSecondary(colour.c());
        }

        if (!isColourSet && imageId.hasPrimary())
        {
            imageId = imageId.withPrimary(colour.c());
        }

        drawingCtx.drawImage(placeForImage, imageId);
    }

    void Tab::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        if (widgetState.disabled)
        {
            return;
        }

        drawTabImpl(drawingCtx, widget, widgetState);
    }
}
