#include "PaintIndustry.h"
#include "../Industry.h"
#include "../Map/Tile.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/ScaffoldingObject.h"
#include "../Objects/ObjectManager.h"
#include "../ScenarioManager.h"
#include "../Ui.h"
#include "Paint.h"

namespace OpenLoco::Paint
{
    // 0x00453C52
    void paintIndustry(PaintSession& session, const Map::IndustryElement& elIndustry)
    {
        session.setItemType(Ui::ViewportInteraction::InteractionItem::industry);
        auto* industry = elIndustry.industry();
        auto* indObj = industry->getObject();

        ImageId baseColour(0, elIndustry.var_6_F800());
        if (elIndustry.isGhost())
        {
            baseColour = Gfx::applyGhostToImage(0);
        }
        // Combine this with any imageId
        const uint8_t rotation = (session.getRotation() + elIndustry.rotation()) & 0x3;
        // 0xE0C3A0
        auto ticks = ScenarioManager::getScenarioTicks();
        uint8_t bl = 0xF0;
        uint8_t bh = 0;
        if (!elIndustry.hasHighTypeFlag())
        {
            ticks = 0;
            bl = elIndustry.var_6_003F();
            bh = elIndustry.var_5_E0();
        }

        // 0x00525D5C
        uint8_t unk525D5C = elIndustry.var_5_03();

        // 0x00525D4E
        const int16_t bbLengthZ = std::min(elIndustry.clearHeight() - elIndustry.baseHeight(), 128) - 2;

        // 0x00E0C3B0 (pointer to something like x,y bytes)
        uint32_t unkE0C3B0 = 0;
        if ((elIndustry.var_6_003F() & (1 << 5)) && (elIndustry.var_6_003F() & (1 << 4)))
        {
            unkE0C3B0 = indObj->var_28[elIndustry.var_6_003F() & 0x3];
        }

        // 0x00E0C3A4
        uint32_t buildingType = elIndustry.buildingType();
        uint8_t* esi = indObj->var_3C[buildingType];
        const auto baseHeight = elIndustry.baseHeight();
        int16_t bbZOffset = baseHeight;
        bool isMultiTile = indObj->buildingSizeFlags & (1 << buildingType);

        session.resetLastPS(); // Odd...
        if (indObj->flags & IndustryObjectFlags::hasShadows)
        {
            if (session.getRenderTarget()->zoomLevel <= 1)
            {
                const auto shadowImageOffset = buildingType * 4 + indObj->var_0E + rotation;
                const ImageId shadowImage = baseColour.withIndex(shadowImageOffset).withTranslucency(Colours::getShadow(elIndustry.var_6_F800()));
                if (isMultiTile)
                {
                    session.addToPlotListAsChild(shadowImage, { 0, 0, baseHeight }, { -8, -8, bbZOffset }, { 38, 38, bbLengthZ });
                }
                else
                {
                    session.addToPlotListAsChild(shadowImage, { 16, 16, baseHeight }, { 3, 3, bbZOffset }, { 26, 26, bbLengthZ });
                }
            }
        }

        // 0x00525D4F
        uint8_t totalSectionHeight = 0;
        if (bl != 0xF0)
        {
            int8_t sectionCount = bl;
            auto* edi = esi;
            while (*edi != 0xFF)
            {
                totalSectionHeight += indObj->var_20[*edi];
                sectionCount--;
                if (sectionCount == -1)
                {
                    totalSectionHeight = std::min<uint8_t>(1, totalSectionHeight);
                    break;
                }
            }
        }
        // 0x00525D30 (note this should be combined with a applyGhostToImage)
        const auto scaffoldingColour = indObj->scaffoldingColour;

        if (isMultiTile)
        {
            // 0x004540A6
        }
        else
        {
            // 0x00453E5B
            const auto scaffSegType = indObj->scaffoldingSegmentType;
            if (totalSectionHeight != 0 && scaffSegType != 0xFF)
            {
                const auto* scaffObj = ObjectManager::get<ScaffoldingObject>();
                const auto segmentHeight = scaffObj->segmentHeights[scaffSegType];
                uint32_t scaffImageIdx = scaffObj->image + scaffSegType * 12;
                ImageId scaffImage{};
                if (elIndustry.isGhost())
                {
                    scaffImage = Gfx::applyGhostToImage(scaffImageIdx);
                }
                else
                {
                    scaffImage = ImageId(scaffImageIdx, scaffoldingColour);
                }

            }
        }
    }
}
