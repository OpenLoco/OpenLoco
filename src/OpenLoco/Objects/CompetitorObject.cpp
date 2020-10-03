#include "CompetitorObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"

namespace OpenLoco
{
    // TODO: Should only be defined in ObjectSelectionWindow
    static const Gfx::point_t objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };
    static const uint8_t descriptionRowHeight = 10;

    // 0x00434D5B
    void competitor_object::drawPreviewImage(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::point_t pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
        Gfx::drawRect(&dpi, pos.x, pos.y, objectPreviewSize.width, objectPreviewSize.height, Colour::inset(Colour::dark_brown));

        auto image = Gfx::recolour(images[0], Colour::inset(Colour::dark_brown));
        Gfx::drawImage(&dpi, x - 32, y - 32, image);
    }

    // 0x00434DA7
    void competitor_object::drawDescription(Gfx::drawpixelinfo_t& dpi, const int16_t x, const int16_t y) const
    {
        Gfx::point_t rowPosition = { x, y };
        {
            auto args = FormatArguments();
            args.push<uint16_t>(intelligence);
            args.push(aiRatingToLevel(intelligence));

            Gfx::drawString_494B3F(dpi, rowPosition.x, rowPosition.y, Colour::black, StringIds::company_details_intelligence, &args);
            rowPosition.y += descriptionRowHeight;
        }
        {
            auto args = FormatArguments();
            args.push<uint16_t>(aggressiveness);
            args.push(aiRatingToLevel(aggressiveness));

            Gfx::drawString_494B3F(dpi, rowPosition.x, rowPosition.y, Colour::black, StringIds::company_details_aggressiveness, &args);
            rowPosition.y += descriptionRowHeight;
        }
        {
            auto args = FormatArguments();
            args.push<uint16_t>(competitiveness);
            args.push(aiRatingToLevel(competitiveness));

            Gfx::drawString_494B3F(dpi, rowPosition.x, rowPosition.y, Colour::black, StringIds::company_details_competitiveness, &args);
        }
    }
}
