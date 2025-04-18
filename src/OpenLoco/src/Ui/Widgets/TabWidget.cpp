#include "TabWidget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui::Widgets
{
    // 0x004CADE8
    static void drawTabBackground(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();

        ImageId imageId = ImageId{ ImageIds::tab };

        // TODO: Separate content image and background image.
        // This is only done to keep everything as is for the time being.
        if (widget.image == ImageIds::wide_tab)
        {
            imageId = ImageId{ ImageIds::wide_tab };
        }

        if (widgetState.activated)
        {
            // TODO: remove image addition
            imageId = imageId.withIndexOffset(1);
        }

        auto colour = widgetState.colour;
        if (widgetState.disabled)
        {
            uint8_t c;
            if (colour.isTranslucent())
            {
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(pos + Ui::Point{ 1, 1 }, imageId, c);
                c = Colours::getShade(colour.c(), 2);
                drawingCtx.drawImageSolid(pos, imageId, c);
            }
            else
            {
                c = Colours::getShade(colour.c(), 6);
                drawingCtx.drawImageSolid(pos + Ui::Point{ 1, 1 }, imageId, c);
                c = Colours::getShade(colour.c(), 4);
                drawingCtx.drawImageSolid(pos, imageId, c);
            }

            return;
        }

        imageId = imageId.withPrimary(colour.c());

        drawingCtx.drawImage(pos, imageId);
    }

    static void drawTabContent(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        auto* window = widgetState.window;

        const auto pos = window->position() + widget.position();

        if (widgetState.disabled)
        {
            return; // 0x8000
        }

        bool isActivated = widgetState.activated;

        if (widget.image == Widget::kContentNull)
        {
            return;
        }

        if (isActivated)
        {
            if (widget.image != Widget::kContentNull)
            {
                drawingCtx.drawImage(pos.x, pos.y, widget.image);
            }
        }
        else
        {
            if (widget.image != Widget::kContentUnk)
            {
                drawingCtx.drawImage(pos.x, pos.y + 1, widget.image);
            }

            drawingCtx.drawImage(pos.x, pos.y, Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33));
            drawingCtx.drawRect(pos.x, pos.y + 26, 31, 1, Colours::getShade(window->getColour(WindowColour::secondary).c(), 7), Gfx::RectFlags::none);
        }
    }

    void Tab::draw(Gfx::DrawingContext& drawingCtx, const Widget& widget, const WidgetState& widgetState)
    {
        if (widget.content == Widget::kContentNull)
        {
            return;
        }

        drawTabBackground(drawingCtx, widget, widgetState);

        // Ugly hack to detect if the drawTab code is used or not.
        // We always draw the background as ImageIds::tab so only draw the content if the image is not the tab image.
        if (widget.image != ImageIds::tab && widget.image != ImageIds::wide_tab)
        {
            drawTabContent(drawingCtx, widget, widgetState);
        }
    }
}
