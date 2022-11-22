#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "Industry.h"
#include "IndustryManager.h"
#include "Localisation/StringManager.h"
#include "Map/IndustryElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "MessageManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "S5/S5.h"
#include "StationManager.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "GameCommands.h"

namespace OpenLoco::GameCommands
{
    // 0x00455916
    // NOTE: Element is invalid after this call and all elements on tile
    static void removeElement(const Map::Pos2& pos, Map::TileElement& el)
    {
        Ui::ViewportManager::invalidate(pos, el.baseHeight(), el.clearHeight());
        Map::TileManager::removeElement(el);
    }

    // 0x0045579F
    static void removeIndustryElement(const Map::Pos3& pos)
    {
        auto tile = Map::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elIndustry = el.as<Map::IndustryElement>();
            if (elIndustry == nullptr)
            {
                continue;
            }
            if (elIndustry->baseHeight() != pos.z)
            {
                continue;
            }
            auto* industry = elIndustry->industry();
            if (industry == nullptr)
            {
                continue;
            }
            auto* indObj = industry->getObject();
            if (indObj == nullptr)
            {
                continue;
            }
            const auto buildingType = elIndustry->buildingType();
            const auto buildingTiles = getBuildingTileOffsets(indObj->buildingSizeFlags & (1 << buildingType));
            for (auto& buildTilePos : buildingTiles)
            {
                auto buildPos = buildTilePos.pos + pos;
                auto buildTile = Map::TileManager::get(buildPos);
                for (auto& elB : buildTile)
                {
                    auto* elBIndustry = elB.as<Map::IndustryElement>();
                    if (elBIndustry == nullptr)
                    {
                        continue;
                    }
                    if (elBIndustry->baseHeight() != pos.z)
                    {
                        continue;
                    }
                    removeElement(buildPos, elB);
                    break;
                }
            }
            for (auto i = 0; i < industry->numTiles; ++i)
            {
                const auto& indTile = industry->tiles[i];
                const auto indPos = Map::Pos3{ Map::Pos2(indTile),
                                               static_cast<coord_t>(indTile.z & ~(1 << 15)) };
                if (indPos == pos)
                {
                    if (industry->numTiles > 1)
                    {
                        std::copy(&industry->tiles[i + 1], &industry->tiles[industry->numTiles], &industry->tiles[i]);
                    }
                    industry->numTiles--;
                    break;
                }
            }
            break;
        }
    }

    // 0x00455A5C
    static void revokeAllSurfaceClaims(const IndustryId id)
    {
        for (auto& pos : Map::TilePosRangeView({ 1, 1 }, { Map::kMapRows - 1, Map::kMapColumns - 1 }))
        {
            auto tile = Map::TileManager::get(pos);
            auto* surface = tile.surface();
            if (surface == nullptr)
            {
                continue;
            }
            if (!surface->isIndustrial())
            {
                continue;
            }
            if (surface->industryId() != id)
            {
                continue;
            }
            surface->setIsIndustrialFlag(false);
            surface->setVar6SLR5(0);
            surface->setVariation(0);
            Ui::ViewportManager::invalidate(pos, surface->baseHeight(), surface->baseHeight() + 32);
            Map::TileManager::removeAllWallsOnTile(pos, surface->baseZ());
        }
    }

    // 0x00455943
    static uint32_t removeIndustry(IndustryId id, uint8_t flags)
    {
        setExpenditureType(ExpenditureType::Miscellaneous);
        auto* industry = IndustryManager::get(id);
        if (industry == nullptr)
        {
            return FAILURE;
        }
        auto* indObj = industry->getObject();
        if (indObj == nullptr)
        {
            return FAILURE;
        }

        const auto height = Map::TileManager::getHeight(Map::Pos2{ industry->x, industry->y } + Map::Pos2{ 16, 16 });
        setPosition({ industry->x, industry->y, height.landHeight });
        if (flags & Flags::apply)
        {
            for (auto i = industry->numTiles; i != 0; --i)
            {
                auto tile = industry->tiles[0];
                tile.z &= static_cast<coord_t>(~(1 << 15));
                removeIndustryElement(tile);
            }

            S5::getOptions().madeAnyChanges = 1;
            Ui::WindowManager::close(Ui::WindowType::industry, enumValue(id));
            StringManager::emptyUserString(industry->name);
            industry->name = StringIds::null;
            Ui::Windows::IndustryList::removeIndustry(id);
            revokeAllSurfaceClaims(id);

            for (auto& station : StationManager::stations())
            {
                for (auto& stat : station.cargoStats)
                {
                    if (stat.industryId == id)
                    {
                        stat.industryId = IndustryId::null;
                    }
                }
            }

            MessageManager::removeAllSubjectRefs(enumValue(id), MessageItemArgumentType::industry);
        }

        return Economy::getInflationAdjustedCost(indObj->clearCostFactor, indObj->costIndex, 3);
    }

    void removeIndustry(registers& regs)
    {
        IndustryRemovalArgs args(regs);
        regs.ebx = removeIndustry(args.industryId, regs.bl);
    }
}
