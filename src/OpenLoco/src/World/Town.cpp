#include "Town.h"
#include "Config.h"
#include "Date.h"
#include "GameCommands/Buildings/CreateBuilding.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Terraform/LowerLand.h"
#include "GameCommands/Terraform/RaiseLand.h"
#include "GameState.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TreeElement.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ObjectUtils.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/StreetLightObject.h"
#include "Random.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "ViewportManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <bit>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco
{
    constexpr auto kMaxTownBridgeLength = 15U;

    bool Town::empty() const
    {
        return name == StringIds::null;
    }

    /**
     * 0x0049742F
     * Update town
     *
     * @param this @<esi>
     */
    void Town::update()
    {
        recalculateSize();

        if (Config::get().townGrowthDisabled)
        {
            return;
        }

        static constexpr std::array<uint8_t, 12> kBuildSpeedToGrowthPerTick = { 0, 1, 3, 5, 7, 9, 12, 16, 22, 0, 0, 0 };
        auto growthPerTick = kBuildSpeedToGrowthPerTick[this->buildSpeed];
        if (growthPerTick == 0 || (growthPerTick == 1 && (gPrng1().randNext() & 7)))
        {
            grow(TownGrowFlags::buildInitialRoad | TownGrowFlags::roadUpdate | TownGrowFlags::neutralRoadTakeover);
        }
        else
        {
            for (int32_t counter = 0; counter < growthPerTick; ++counter)
            {
                grow(TownGrowFlags::buildInitialRoad | TownGrowFlags::roadUpdate | TownGrowFlags::neutralRoadTakeover | TownGrowFlags::allowRoadExpansion | TownGrowFlags::allowRoadBranching | TownGrowFlags::constructBuildings);
            }
        }
    }

    // 0x004FF6F4
    static constexpr std::array<Gfx::Font, 4> kZoomToTownFonts = {
        Gfx::Font::medium_bold,
        Gfx::Font::medium_bold,
        Gfx::Font::medium_normal,
        Gfx::Font::medium_normal,
    };

    void Town::drawLabel(Gfx::DrawingContext& drawingCtx, const Gfx::RenderTarget& rt)
    {
        if (!labelFrame.contains(rt.getDrawableRect(), rt.zoomLevel))
        {
            return;
        }

        auto tr = Gfx::TextRenderer(drawingCtx);

        char buffer[512]{};
        StringManager::formatString(buffer, name);
        tr.setCurrentFont(kZoomToTownFonts[rt.zoomLevel]);

        auto point = Ui::Point(labelFrame.left[rt.zoomLevel] + 1, labelFrame.top[rt.zoomLevel] + 1);
        tr.drawString(point, AdvancedColour(Colour::white).outline(), buffer);
    }

    // 0x00497616
    void Town::updateLabel()
    {
        char buffer[256]{};
        StringManager::formatString(buffer, 256, name);

        auto height = TileManager::getHeight(Pos2(x, y));
        auto pos = Pos3(x + kTileSize / 2, y + kTileSize / 2, height.landHeight);

        auto rotated = World::gameToScreen(pos, Ui::WindowManager::getCurrentRotation());
        rotated.y -= 48;

        for (auto zoomLevel = 0; zoomLevel < 4; zoomLevel++)
        {
            auto font = kZoomToTownFonts[zoomLevel];

            auto nameWidth = (Gfx::TextRenderer::getStringWidth(font, buffer) + 2) << zoomLevel;
            auto nameHeight = 11 << zoomLevel; // was a lookup on 0x4FF6FC; same for all zoom levels

            auto xOffset = rotated.x - (nameWidth / 2);
            auto yOffset = rotated.y - (nameHeight / 2);

            labelFrame.left[zoomLevel] = xOffset >> zoomLevel;
            labelFrame.right[zoomLevel] = (xOffset + nameWidth) >> zoomLevel;
            labelFrame.top[zoomLevel] = yOffset >> zoomLevel;
            labelFrame.bottom[zoomLevel] = (yOffset + nameHeight) >> zoomLevel;
        }
    }

    // 0x0049749B
    void Town::updateMonthly()
    {
        // Scroll history
        if (historySize == std::size(history))
        {
            for (size_t i = 0; i < std::size(history) - 1; i++)
            {
                history[i] = history[i + 1];
            }
        }
        else
        {
            historySize++;
        }

        // Compute population growth.
        uint32_t popSteps = std::max<int32_t>(population - historyMinPopulation, 0) / 50;
        uint32_t popGrowth = 0;
        while (popSteps > 255)
        {
            popSteps -= 20;
            popGrowth += 1000;
        }

        // Any population growth to account for?
        if (popGrowth != 0)
        {
            historyMinPopulation += popGrowth;

            uint8_t offset = (popGrowth / 50) & 0xFF;
            for (uint8_t i = 0; i < historySize; i++)
            {
                int16_t newHistory = history[i] - offset;
                history[i] = newHistory >= 0 ? static_cast<uint8_t>(newHistory) : 0;
            }
        }

        // Write new history point.
        auto histIndex = std::clamp<int32_t>(historySize - 1, 0, std::size(history));
        history[histIndex] = popSteps & 0xFF;

        // Find historical maximum population.
        uint8_t maxPopulation = 0;
        for (int i = 0; i < historySize; i++)
        {
            maxPopulation = std::max(maxPopulation, history[i]);
        }

        int32_t popOffset = historyMinPopulation;
        while (maxPopulation <= 235 && popOffset > 0)
        {
            maxPopulation += 20;
            popOffset -= 1000;
        }

        popOffset -= historyMinPopulation;
        if (popOffset != 0)
        {
            popOffset = -popOffset;
            historyMinPopulation -= popOffset;
            popOffset /= 50;

            for (int i = 0; i < historySize; i++)
            {
                history[i] += popOffset;
            }
        }

        // Work towards computing new build speed.
        // will be the smallest of the influence cargo delivered to the town
        // i.e. to get maximum growth max of the influence cargo must be delivered
        // every update. If no influence cargo the grows at max rate
        uint16_t minCargoDelivered = std::numeric_limits<uint16_t>::max();
        uint32_t cargoFlags = cargoInfluenceFlags;
        while (cargoFlags != 0)
        {
            uint32_t cargoId = Numerics::bitScanForward(cargoFlags);
            cargoFlags &= ~(1 << cargoId);

            minCargoDelivered = std::min(minCargoDelivered, monthlyCargoDelivered[cargoId]);
        }

        // Compute build speed (1=slow build speed, 4=fast build speed)
        buildSpeed = std::clamp((minCargoDelivered / 100) + 1, 1, 4);

        // Reset all monthlyCargoDelivered intermediaries to zero.
        memset(&monthlyCargoDelivered, 0, sizeof(monthlyCargoDelivered));
    }

    void Town::adjustCompanyRating(CompanyId cid, int amount)
    {
        companiesWithRating |= (1 << enumValue(cid));
        companyRatings[enumValue(cid)] = std::clamp(
            companyRatings[enumValue(cid)] + amount,
            kMinCompanyRating,
            kMaxCompanyRating);
    }

    // 0x004FF714
    constexpr uint32_t _populations[]{ 0, 150, 500, 2500, 8000 };

    /**
     * 0x004975E0
     * Recalculate size
     *
     * @param this @<esi>
     */
    void Town::recalculateSize()
    {
        auto newSize = enumValue(size);
        if (size < TownSize::metropolis
            && populationCapacity >= _populations[enumValue(size) + 1])
        {
            newSize++;
        }
        else if (size != TownSize::hamlet && populationCapacity + 100 < _populations[enumValue(size)])
        {
            newSize--;
        }

        if (static_cast<TownSize>(newSize) != size)
        {
            size = static_cast<TownSize>(newSize);
            Ui::WindowManager::invalidate(Ui::WindowType::townList);
        }
    }

    // 0x00497F1F
    static std::optional<uint8_t> getIdealTownRoadId(const Town& town)
    {
        struct Res
        {
            uint8_t roadObjId;  // dl
            TownSize foundSize; // cl
        };
        std::optional<Res> findResult;
        for (uint8_t roadObjId = 0; roadObjId < ObjectManager::getMaxObjects(ObjectType::road); ++roadObjId)
        {
            auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
            if (roadObj == nullptr)
            {
                continue;
            }
            if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                continue;
            }
            if (roadObj->hasFlags(RoadObjectFlags::isOneWay))
            {
                continue;
            }
            if (findResult.has_value())
            {
                bool setNewFound = false;
                if (roadObj->targetTownSize == town.size)
                {
                    setNewFound = true;
                }
                else
                {
                    if (roadObj->targetTownSize < town.size)
                    {
                        if (findResult->foundSize > town.size || roadObj->targetTownSize >= findResult->foundSize)
                        {
                            setNewFound = true;
                        }
                    }
                    else
                    {
                        if (findResult->foundSize > town.size && roadObj->targetTownSize < findResult->foundSize)
                        {
                            setNewFound = true;
                        }
                    }
                }
                if (!setNewFound)
                {
                    continue;
                }
            }
            findResult = Res{ roadObjId, roadObj->targetTownSize };
        }
        if (findResult.has_value())
        {
            return findResult->roadObjId;
        }
        return std::nullopt;
    }

    struct RoadInformation
    {
        CompanyId owner;                         // bl
        uint8_t roadObjId;                       // bh
        bool isStationRoadEnd;                   // ebx >> 23
        std::optional<uint8_t> streetLightStyle; // ebx >> 16 && ebx >> 24
    };

    // 0x00498D21
    // pos : ax, cx, dx
    // rotation : bl
    // return : flags Carry == found road
    static bool sub_498D21(const World::Pos3 pos, const uint8_t rotation)
    {
        if (!World::TileManager::validCoords(pos))
        {
            return false;
        }

        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseHeight() != pos.z)
            {
                continue;
            }
            if (!(getGameState().roadObjectIdIsNotTram & (1U << elRoad->roadObjectId())))
            {
                continue;
            }
            if (elRoad->roadId() != 0)
            {
                continue;
            }
            if (elRoad->isGhost() || elRoad->isAiAllocated())
            {
                continue;
            }

            if (rotation == elRoad->rotation() || (rotation ^ (1U << 1)) == elRoad->rotation())
            {
                return true;
            }
        }
        return false;
    }

    // 0x0042CEBF
    // year : ax
    // dx : dx
    // largeTile : bl
    // unk1 : 525D24
    // targetHeight : esi
    // return : ebp
    static sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::building)> sub_42CEBF(uint16_t year, uint16_t dx, bool largeTile, uint32_t unk1, uint16_t targetHeight)
    {
        sfl::static_vector<uint8_t, ObjectManager::getMaxObjects(ObjectType::building)> potentialBuildings;
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::building); ++i)
        {
            auto* buildingObj = ObjectManager::get<BuildingObject>(i);
            if (buildingObj == nullptr)
            {
                continue;
            }
            if (buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
            {
                continue;
            }
            if (!(buildingObj->generatorFunction & (1U << dx)))
            {
                continue;
            }
            if (year < buildingObj->designedYear || year > buildingObj->obsoleteYear)
            {
                continue;
            }
            if (largeTile != buildingObj->hasFlags(BuildingObjectFlags::largeTile))
            {
                continue;
            }
            if (buildingObj->var_AC != 0xFFU)
            {
                if (!(unk1 & (1U << buildingObj->var_AC)))
                {
                    continue;
                }
            }

            for (auto j = 0U; j < buildingObj->numVariations; ++j)
            {
                uint16_t height = 0;
                const auto parts = buildingObj->getBuildingParts(j);
                const auto partHeights = buildingObj->getBuildingPartHeights();
                for (const auto part : parts)
                {
                    height += partHeights[part];
                }

                if (height <= targetHeight)
                {
                    potentialBuildings.push_back(i);
                    break;
                }
            }
        }
        return potentialBuildings;
    }

    // 0x004F6CCC
    // index with tad side doesn't matter
    // Note: only valid for straight and very small curves
    static constexpr std::array<uint8_t, 80> kRoadTadConnectionEdge = {
        0b0101, // straight rotation 0 side 0
        0b1010, // straight rotation 1 side 0
        0b0101, // straight rotation 2 side 0
        0b1010, // straight rotation 3 side 0
        0b0101, // straight rotation 0 side 1
        0b1010, // straight rotation 1 side 1
        0b0101, // straight rotation 2 side 1
        0b1010, // straight rotation 3 side 1
        0b1100, // left curve very small rotation 0 side 0
        0b1001,
        0b0011,
        0b0110,
        0b1100, // left curve very small rotation 0 side 1
        0b1001,
        0b0011,
        0b0110,
        0b0110, // right curve very small rotation 0 side 0
        0b1100,
        0b1001,
        0b0011,
        0b0110, // right curve very small rotation 0 side 1
        0b1100,
        0b1001,
        0b0011,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };

    // 0x00498D9A
    // pos : ax, cx, dx
    // edge : ebx (must be below 32)
    // return : flags Carry == found road
    static bool sub_498D9A(const World::Pos3 pos, uint8_t edge)
    {
        if (!World::TileManager::validCoords(pos))
        {
            return false;
        }

        assert(edge < 32);

        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elRoad = el.as<World::RoadElement>();
            if (elRoad == nullptr)
            {
                continue;
            }
            if (elRoad->baseHeight() != pos.z)
            {
                continue;
            }
            if (!(getGameState().roadObjectIdIsNotTram & (1U << elRoad->roadObjectId())))
            {
                continue;
            }
            if (elRoad->isGhost() || elRoad->isAiAllocated())
            {
                continue;
            }

            const uint16_t tad = (elRoad->roadId() << 3) | elRoad->rotation();
            if (kRoadTadConnectionEdge[tad] & (1U << edge))
            {
                return true;
            }
        }
        return false;
    }

    static constexpr std::array<uint8_t, 8> k4F92A6 = {
        1,
        1,
        2,
        2,
        1,
        3,
        1,
        1,
    };

    // 0x0042DB35
    // Part of calculating the max height clearance for a new building on this tile
    // A collision in this function means that the building/obstacle will not be removed
    // Optionally `allowBuildingUpdate` ignores buildings that are old and not headquarters (to allow for their removal)
    static World::TileClearance::ClearFuncResult getBuildingMaxHeightClearFunc(World::TileElement& el, uint8_t baseZ, uint8_t& minZDiff, bool allowBuildingUpdate)
    {
        auto* elTree = el.as<World::TreeElement>();
        auto* elBuilding = el.as<World::BuildingElement>();
        if (elTree != nullptr)
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        else if (elBuilding != nullptr)
        {
            if (!allowBuildingUpdate)
            {
                return World::TileClearance::ClearFuncResult::collision;
            }

            if (!elBuilding->isConstructed())
            {
                return World::TileClearance::ClearFuncResult::collision;
            }
            auto* buildingObj = ObjectManager::get<BuildingObject>(elBuilding->objectId());
            if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
            {
                return World::TileClearance::ClearFuncResult::collision;
            }
            if (elBuilding->age() > 30)
            {
                return World::TileClearance::ClearFuncResult::noCollision;
            }
            return World::TileClearance::ClearFuncResult::collision;
        }
        else
        {
            const auto diff = el.baseZ() - baseZ;
            if (diff <= 0)
            {
                return World::TileClearance::ClearFuncResult::collision;
            }

            minZDiff = std::min<uint8_t>(minZDiff, diff);

            return World::TileClearance::ClearFuncResult::noCollision;
        }
    }

    // 0x0042D9FA
    // If 0 then no new building can be created here
    // pos : (ax, cx, dx)
    // isLargeTile : bh bit 0
    // allowBuildingUpdate : bh bit 2
    // return maxHeightOfBuilding: ebp
    static int16_t getMaxHeightOfNewBuilding(const World::Pos3 pos, bool isLargeTile, bool allowBuildingUpdate)
    {
        auto offsets = getBuildingTileOffsets(isLargeTile);
        int32_t maxHeightDiff = 0;
        for (auto& offset : offsets)
        {
            const auto loc = World::Pos2{ pos } + offset.pos;
            if (!World::TileManager::validCoords(loc))
            {
                continue;
            }
            const auto elSurface = World::TileManager::get(loc).surface();
            const auto heightDiff = std::abs(elSurface->baseHeight() - pos.z);
            maxHeightDiff = std::max(heightDiff, maxHeightDiff);
        }
        const auto smallMaxHeightDiff = maxHeightDiff / World::kSmallZStep;
        if (smallMaxHeightDiff > 8)
        {
            return 0;
        }
        uint8_t minClear = 0xFFU;
        for (auto& offset : offsets)
        {
            const auto loc = World::Pos2{ pos } + offset.pos;
            if (!World::TileManager::validCoords(loc))
            {
                return 0;
            }
            const auto elSurface = World::TileManager::get(loc).surface();
            const auto minZ = std::min(elSurface->baseHeight(), pos.z) / World::kSmallZStep;
            World::QuarterTile qt(0xF, 0xF);
            auto clearFunc = [baseZ = pos.z / World::kSmallZStep, &minClear, allowBuildingUpdate](TileElement& el) {
                return getBuildingMaxHeightClearFunc(el, baseZ, minClear, allowBuildingUpdate);
            };
            if (!World::TileClearance::applyClearAtStandardHeight(loc, minZ, 255, qt, clearFunc))
            {
                return 0;
            }
        }
        return minClear * World::kSmallZStep;
    }

    // 0x0042CF7C
    // pos : (ax, cx, di)
    // isLargeTile : bh bit 0
    // buildImmediately : bh bit 1
    // rotation : bl
    // targetHeight : ebp
    // return std::nullopt : Carry flag set
    //        See also BuildingPlacementArgs
    //
    // TODO: Pass in targetTownId when not hooking return nullopt
    // if nearby town is not targetTownId and remove the loco_global
    static std::optional<GameCommands::BuildingPlacementArgs> generateNewBuildingArgs(const World::Pos3 pos, int16_t targetHeight, uint8_t rotation, bool isLargeTile, bool buildImmediately)
    {
        const auto res = TownManager::getClosestTownAndDensity(pos);
        if (!res.has_value())
        {
            return std::nullopt;
        }

        const auto& [townId, townDensity] = res.value();
        // See TODO
        // if (targetTownId != townId)
        // {
        //     return std::nullopt;
        // }

        auto* town = TownManager::get(townId);
        // See TODO
        loco_global<Town*, 0x00525D20> _525D20;
        _525D20 = town;

        uint32_t unk525D24 = 0;
        const auto buildingsFactor = (town->numBuildings + 64) / 128;
        for (auto i = 0U; i < std::size(k4F92A6); ++i)
        {
            if (k4F92A6[i] * buildingsFactor > town->var_150[i])
            {
                unk525D24 |= (1U << i);
            }
        }

        const auto curYear = getCurrentYear();
        auto potentialBuildings = sub_42CEBF(curYear, townDensity, isLargeTile, unk525D24, targetHeight);
        if (potentialBuildings.empty())
        {
            if (townDensity == 0)
            {
                return std::nullopt;
            }
            potentialBuildings = sub_42CEBF(curYear, townDensity - 1, isLargeTile, unk525D24, targetHeight);
            if (potentialBuildings.empty())
            {
                return std::nullopt;
            }
        }

        const auto randBuilding = ((town->prng.randNext() & 0xFFFFU) * potentialBuildings.size()) / 65536;
        const auto buildingObjId = potentialBuildings[randBuilding];
        auto* buildingObj = ObjectManager::get<BuildingObject>(buildingObjId);
        sfl::static_vector<uint8_t, 32> potentialVariations;
        for (auto j = 0U; j < buildingObj->numVariations; ++j)
        {
            uint16_t height = 0;
            const auto parts = buildingObj->getBuildingParts(j);
            const auto partHeights = buildingObj->getBuildingPartHeights();
            for (const auto part : parts)
            {
                height += partHeights[part];
            }

            if (height <= targetHeight)
            {
                potentialVariations.push_back(j);
            }
        }
        const auto randVariationIndex = ((town->prng.randNext() & 0xFFFFU) * potentialVariations.size()) / 65536;
        const auto variation = potentialVariations[randVariationIndex];

        sfl::static_vector<Colour, 32> potentialColours;
        for (auto j = 0U; j < enumValue(Colour::max); ++j)
        {
            if (buildingObj->colours & (1U << j))
            {
                potentialColours.push_back(static_cast<Colour>(j));
            }
        }
        auto colour = Colour::black;
        if (!potentialColours.empty())
        {
            const auto randColourIndex = ((town->prng.randNext() & 0xFFFFU) * potentialColours.size()) / 65536;
            colour = potentialColours[randColourIndex];
        }

        GameCommands::BuildingPlacementArgs args{};
        args.buildImmediately = buildImmediately;
        args.pos = pos;
        args.colour = colour;
        args.rotation = rotation;
        args.type = buildingObjId;
        args.variation = variation;
        return args;
    }

    // 0x00498CB7
    // Searches a 3x3 square around the centerPos for any buildings
    // centerPos : ax, cx (Actually in World::Pos2 format)
    // return : flags Carry == building found
    static bool hasNearbyBuildings(World::TilePos2 centerPos)
    {
        const auto tileA = centerPos - World::TilePos2{ 2, 2 };
        const auto tileB = centerPos + World::TilePos2{ 2, 2 };
        for (const auto& tilePos : World::getClampedRange(tileA, tileB))
        {
            auto tile = World::TileManager::get(tilePos);
            for (const auto& el : tile)
            {
                auto* elBuilding = el.as<World::BuildingElement>();
                if (elBuilding == nullptr)
                {
                    continue;
                }
                return true;
            }
        }
        return false;
    }

    // 0x0047AD83
    // pos : ax, cx, dx
    // tad : ebp
    // newRoadObjId : bh
    // newOwner : bl
    // newStreetLightStyle : ebx >> 16
    static void updateAndTakeoverRoad(const World::Pos3 pos, const Vehicles::TrackAndDirection::_RoadAndDirection tad, const uint8_t newRoadObjId, const CompanyId newOwner, const uint8_t newStreetLightStyle)
    {
        auto* newRoadObj = ObjectManager::get<RoadObject>(newRoadObjId);
        const auto roadPiecesFlags = World::TrackData::getRoadMiscData(tad.id()).compatibleFlags;
        if ((roadPiecesFlags & newRoadObj->roadPieces) != roadPiecesFlags)
        {
            return;
        }

        const auto roadStart = [tad, &pos]() {
            if (tad.isReversed())
            {
                auto& roadSize = World::TrackData::getUnkRoad(tad._data);
                auto roadStart = pos + roadSize.pos;
                if (roadSize.rotationEnd < 12)
                {
                    roadStart -= World::Pos3{ kRotationOffset[roadSize.rotationEnd], 0 };
                }
                return roadStart;
            }
            else
            {
                return pos;
            };
        }();
        const auto pieces = World::TrackData::getRoadPiece(tad.id());
        for (auto& piece : pieces)
        {
            const auto roadPos = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, tad.cardinalDirection()), piece.z };

            auto tile = World::TileManager::get(roadPos);
            for (auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->baseHeight() != roadPos.z)
                {
                    continue;
                }
                if (elRoad->isGhost() || elRoad->isAiAllocated())
                {
                    continue;
                }
                auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
                {
                    continue;
                }
                elRoad->setOwner(newOwner);
                elRoad->setRoadObjectId(newRoadObjId);
                if (!elRoad->hasLevelCrossing())
                {
                    elRoad->setStreetLightStyle(newStreetLightStyle);
                }
                Ui::ViewportManager::invalidate(roadPos, elRoad->baseHeight(), elRoad->clearHeight(), ZoomLevel::half);
            }
        }
    }

    // 0x00498C6B
    // roadObjId : bh
    // townDensity : 0x01135C5F
    // return : edi
    static uint32_t getStreetLightStyle(uint8_t roadObjId, uint8_t townDensity)
    {
        auto* roadObj = ObjectManager::get<RoadObject>(roadObjId);
        if (!roadObj->hasFlags(RoadObjectFlags::unk_08) || townDensity == 0)
        {
            return 0;
        }

        return []() {
            auto* streetLightObj = ObjectManager::get<StreetLightObject>();
            const auto curYear = getCurrentYear();
            for (auto i = 0U; i < std::size(streetLightObj->designedYear); ++i)
            {
                if (curYear < streetLightObj->designedYear[i])
                {
                    return i;
                }
            }
            return static_cast<uint32_t>(std::size(streetLightObj->designedYear));
        }();
    }

    // 0x0047AC3E
    static RoadInformation getRoadInformation(const World::Pos3& loc, Vehicles::TrackAndDirection::_RoadAndDirection tad)
    {
        const auto roadStart = [tad, &loc]() {
            if (tad.isReversed())
            {
                auto& roadSize = World::TrackData::getUnkRoad(tad._data);
                auto roadStart = loc + roadSize.pos;
                if (roadSize.rotationEnd < 12)
                {
                    roadStart -= World::Pos3{ kRotationOffset[roadSize.rotationEnd], 0 };
                }
                return roadStart;
            }
            else
            {
                return loc;
            };
        }();
        const auto startDirection = tad.cardinalDirection();
        const auto& roadPieceZero = World::TrackData::getRoadPiece(tad.id())[0];
        const auto nextRoadStart = roadStart + World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPieceZero.x, roadPieceZero.y }, startDirection), roadPieceZero.z };
        auto* nextElRoad = [nextRoadStart, startDirection, tad]() -> World::RoadElement* {
            auto tile = World::TileManager::get(nextRoadStart);
            for (auto& el : tile)
            {
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }

                if (elRoad->baseHeight() != nextRoadStart.z)
                {
                    continue;
                }

                if (elRoad->rotation() != startDirection)
                {
                    continue;
                }

                if (elRoad->sequenceIndex() != 0)
                {
                    continue;
                }

                auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
                {
                    continue;
                }
                if (elRoad->roadId() != tad.id())
                {
                    continue;
                }
                return elRoad;
            }
            return nullptr;
        }();

        if (nextElRoad == nullptr)
        {
            // TODO
            return RoadInformation{ CompanyId::null, 0, false, std::nullopt };
        }

        RoadInformation result{};
        result.owner = nextElRoad->owner();
        result.roadObjId = nextElRoad->roadObjectId();
        result.streetLightStyle = std::nullopt;
        result.isStationRoadEnd = false;
        const bool hasLevelCrossing = nextElRoad->hasLevelCrossing();
        if (!hasLevelCrossing)
        {
            result.streetLightStyle = nextElRoad->streetLightStyle();
        }

        if (nextElRoad->hasStationElement())
        {
            auto* elStation = World::TileManager::get(nextRoadStart).roadStation(tad.id(), startDirection, nextRoadStart.z / World::kSmallZStep);
            if (elStation != nullptr)
            {
                auto* roadStationObj = ObjectManager::get<RoadStationObject>(elStation->objectId());
                if (roadStationObj->hasFlags(RoadStationFlags::roadEnd))
                {
                    result.isStationRoadEnd = true;
                }
            }
        }
        return result;
    }

    static constexpr std::array<World::Pos2, 4> k4FF704 = {
        World::Pos2{ 0, 0 },
        World::Pos2{ 0, -32 },
        World::Pos2{ -32, -32 },
        World::Pos2{ -32, 0 },
    };

    static constexpr std::array<std::array<World::TilePos2, 4>, 2> kIntersectionCheck = {
        std::array<World::TilePos2, 4>{
            World::TilePos2{ 0, -2 },
            World::TilePos2{ 0, -1 },
            World::TilePos2{ 0, 1 },
            World::TilePos2{ 0, 2 },
        },
        std::array<World::TilePos2, 4>{
            World::TilePos2{ -2, 0 },
            World::TilePos2{ -1, 0 },
            World::TilePos2{ 1, 0 },
            World::TilePos2{ 2, 0 },
        },
    };

    // 0x004984C5
    static bool addRoadJunction(Town& town, const World::Pos3 pos, const Vehicles::TrackAndDirection::_RoadAndDirection tad, const bool onBridge, const uint8_t roadObjId, const sfl::static_vector<uint16_t, 16>& connections)
    {
        const auto roadId = tad.id();
        if (roadId != 0 && roadId != 1 && roadId != 2)
        {
            return false;
        }
        if (onBridge)
        {
            return false;
        }
        auto randVal = town.prng.randNext();
        if ((randVal & 0xFFFFU) > 0x666)
        {
            return false;
        }
        auto* surface = TileManager::get(pos).surface();
        if (pos.z < surface->baseHeight())
        {
            return false;
        }

        uint8_t edges = 0;
        for (auto c : connections)
        {
            edges |= kRoadTadConnectionEdge[c & World::Track::AdditionalTaDFlags::basicTaDMask];
        }
        edges ^= 0xF;
        const auto numEdges = std::popcount(edges);
        if (numEdges == 0)
        {
            return false;
        }
        auto randEdge = [randVal, numEdges, edges]() {
            int32_t num = ((randVal >> 16) & 0xFFU) * numEdges / 256;
            auto i = 0U;
            for (; i < 4 && num >= 0; ++i)
            {
                if (edges & (1U << i))
                {
                    --num;
                }
            }
            return i - 1;
        }();

        bool hasIncompatibleRoad = [curPos = pos, randEdge]() {
            for (const auto& offset : kIntersectionCheck[randEdge & 1])
            {
                const auto pos = curPos + World::Pos3(World::toWorldSpace(offset), 0);
                if (sub_498D9A(pos, randEdge))
                {
                    return true;
                }
            }
            return false;
        }();

        if (hasIncompatibleRoad)
        {
            return false;
        }
        // 0x00498676
        uint8_t newRoadId = 0;
        if (edges & (1U << (randEdge ^ (1U << 1))))
        {
            if (edges & (1U << ((randEdge - 1) & 0x3)))
            {
                newRoadId = 1;
            }
            else
            {
                newRoadId = 2;
            }
        }

        GameCommands::RoadPlacementArgs args{};
        args.roadId = newRoadId;
        args.pos = pos;
        args.roadObjectId = roadObjId;
        args.bridge = 0xFFU;
        args.rotation = randEdge ^ (1U << 1);
        args.mods = 0;
        GameCommands::doCommand(args, GameCommands::Flags::apply);
        return true;
    }

    static constexpr std::array<Pos2, 6> kBuggedRotationOffset = {
        Pos2{ -25, 0 },
        Pos2{ -32, 0 },
        Pos2{ 0, 32 },
        Pos2{ 32, 0 },
        Pos2{ 0, -32 },
        Pos2{ -32, 0 },
    };

    // 0x00498801
    static bool placeRoadBridge(Town& town, const World::Pos3 pos, const uint8_t rotation, const uint8_t roadObjectId)
    {
        assert(rotation < 4);
        auto validBridgeTypes = getAvailableCompatibleBridges(roadObjectId, TransportMode::road);

        auto bridgePos = pos;
        auto bridgeLength = 1U;
        bool bridgesWater = false;
        bool bridgeIsHigh = false;
        // Bridge length is 1 based!
        for (; bridgeLength <= kMaxTownBridgeLength; ++bridgeLength)
        {
            sfl::static_vector<uint8_t, Limits::kMaxBridgeObjects> iterationValidBridgeTypes;
            for (auto bridgeObjId : validBridgeTypes)
            {
                GameCommands::RoadPlacementArgs args{};
                args.bridge = bridgeObjId;
                args.pos = bridgePos;
                args.mods = 0;
                args.roadId = 0;
                args.roadObjectId = roadObjectId;
                args.rotation = rotation;
                if (GameCommands::doCommand(args, 0) != GameCommands::FAILURE)
                {
                    iterationValidBridgeTypes.push_back(bridgeObjId);
                }
            }
            if (iterationValidBridgeTypes.empty())
            {
                return false;
            }
            validBridgeTypes = iterationValidBridgeTypes;

            auto tile = TileManager::get(bridgePos);
            auto* surface = tile.surface();
            if (surface->water())
            {
                bridgesWater = true;
            }
            auto bridgeHeight = bridgePos.z - surface->baseHeight();
            if (bridgeHeight >= 8 * kSmallZStep)
            {
                bridgeIsHigh = true;
            }

            bridgePos += World::Pos3{ World::kRotationOffset[rotation], 0 };
            if (!World::TileManager::validCoords(bridgePos))
            {
                return false;
            }
            auto nextTile = TileManager::get(bridgePos);
            auto* nextSurface = nextTile.surface();
            if (bridgePos.z <= nextSurface->baseHeight())
            {
                break;
            }
        }

        if ((!bridgeIsHigh && !bridgesWater) || bridgeLength > kMaxTownBridgeLength)
        {
            return false;
        }
        const auto randBridgeId = validBridgeTypes[((town.prng.randNext() & 0xFFU) * validBridgeTypes.size()) / 256];

        bridgePos = pos;
        for (auto i = 0U; i < bridgeLength; ++i)
        {
            GameCommands::RoadPlacementArgs args{};
            args.bridge = randBridgeId;
            args.pos = bridgePos;
            args.mods = 0;
            args.roadId = 0;
            args.roadObjectId = roadObjectId;
            args.rotation = rotation;
            GameCommands::doCommand(args, GameCommands::Flags::apply);

            bridgePos += World::Pos3{ World::kRotationOffset[rotation], 0 };
        }
        return true;
    }

    // 0x004986EA
    static void appendToRoadEnd(Town& town, const World::Pos3 pos, const uint8_t rotation, const uint8_t roadObjectId, const uint32_t iteration, const bool isOnBridge)
    {
        if (!World::TileManager::validCoords(pos))
        {
            return;
        }

        if (iteration >= 2 && !isOnBridge)
        {
            auto numBuildingsInArea = [pos]() {
                const auto tileA = World::toTileSpace(pos) - World::TilePos2{ 2, 2 };
                const auto tileB = tileA + World::TilePos2{ 4, 4 };
                uint32_t numBuildings = 0;
                for (const auto& tilePos : getClampedRange(tileA, tileB))
                {
                    auto tile = TileManager::get(tilePos);
                    for (const auto& el : tile)
                    {
                        if (el.as<BuildingElement>() != nullptr)
                        {
                            numBuildings++;
                        }
                    }
                }
                return numBuildings;
            }();
            if (numBuildingsInArea < 2)
            {
                return;
            }
        }

        for (auto j : { -1, 1 })
        {
            // NOTE: CS mistake here!
            const auto checkPos = pos + World::Pos3{ kBuggedRotationOffset[1 + (rotation + j)], 0 };
            // const auto checkPos = pos + World::Pos3{ World::kRotationOffset[(rotation + j) & 0x3], 0 };
            if (sub_498D21(checkPos, rotation))
            {
                return;
            }
        }

        const auto tile = TileManager::get(pos);
        auto* surface = tile.surface();
        if (pos.z > surface->baseHeight())
        {
            if (placeRoadBridge(town, pos, rotation, roadObjectId))
            {
                return;
            }
        }

        // 0x00498A06
        if (surface->water() != 0)
        {
            return;
        }

        auto heightDiff = pos.z / World::kSmallZStep - surface->baseZ();
        if (heightDiff > 0)
        {
            if (heightDiff == 4 && !surface->isSlopeDoubleHeight())
            {
                const auto normalisedCorners = Numerics::rotr4bit(surface->slopeCorners(), rotation);
                if (normalisedCorners == SurfaceSlope::SideUp::northeast)
                {
                    // Create straightSteepSlopeDown

                    GameCommands::RoadPlacementArgs args{};
                    args.pos = pos;
                    args.bridge = 0xFFU;
                    args.roadId = 8;
                    args.mods = 0;
                    args.roadObjectId = roadObjectId;
                    args.rotation = rotation;
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    return;
                }
                // TODO: Remove this its just due to a mistake by CS
                if (normalisedCorners > 8)
                {
                    return;
                }
            }
            if (heightDiff > 8)
            {
                return;
            }

            GameCommands::RaiseLandArgs args{};
            args.pointA = pos;
            args.pointB = pos;
            args.centre = pos;
            args.corner = MapSelectionType::full;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            return;
        }
        if (heightDiff < 0 && heightDiff >= -4)
        {
            GameCommands::LowerLandArgs args{};
            args.pointA = pos;
            args.pointB = pos;
            args.centre = pos;
            args.corner = MapSelectionType::full;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            return;
        }
        if (surface->slope() && heightDiff == 0)
        {
            const auto normalisedCorners = Numerics::rotr4bit(surface->slopeCorners(), rotation);
            if (!surface->isSlopeDoubleHeight() && normalisedCorners == SurfaceSlope::SideUp::southwest)
            {
                // Create straightSteepSlopeUp

                GameCommands::RoadPlacementArgs args{};
                args.pos = pos;
                args.bridge = 0xFFU;
                args.roadId = 7;
                args.mods = 0;
                args.roadObjectId = roadObjectId;
                args.rotation = rotation;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
                return;
            }
            GameCommands::LowerLandArgs args{};
            args.pointA = pos;
            args.pointB = pos;
            args.centre = pos;
            args.corner = MapSelectionType::full;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            return;
        }

        const auto randVar = town.prng.randNext() & 0xFFFFU;
        uint8_t roadId = 0;   // straight
        if (randVar < 0x028F) // 1%
        {
            roadId = 1; // left curve
        }
        else if (randVar < 0x051E) // 1%
        {
            roadId = 2; // right curve
        }
        auto* elRoad = [&tile, z = pos.z]() -> const World::RoadElement* {
            for (const auto& el : tile)
            {
                auto* elRoad = el.as<RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->baseHeight() != z)
                {
                    continue;
                }
                return elRoad;
            }
            return nullptr;
        }();
        if (elRoad != nullptr)
        {
            const auto existingRotation = elRoad->rotation();
            const auto existingRoadId = elRoad->roadId();
            if (roadId == 1)
            {
                if (existingRoadId == 1)
                {
                    if ((existingRotation ^ (1U << 1)) == rotation)
                    {
                        roadId = 0;
                    }
                }
                else if (existingRoadId == 2)
                {
                    if (((existingRotation + 1) & 0x3) == rotation)
                    {
                        roadId = 0;
                    }
                }
            }
            else if (roadId == 2)
            {
                if (existingRoadId == 1)
                {
                    if (((existingRotation - 1) & 0x3) == rotation)
                    {
                        roadId = 0;
                    }
                }
                else if (existingRoadId == 2)
                {
                    if ((existingRotation ^ (1U << 1)) == rotation)
                    {
                        roadId = 0;
                    }
                }
            }
            else
            {
                if (existingRoadId == 0)
                {
                    if ((existingRotation ^ (1U << 1)) != rotation)
                    {
                        roadId = 1;
                    }
                }
            }
        }
        GameCommands::RoadPlacementArgs args{};
        args.pos = pos;
        args.bridge = 0xFFU;
        args.roadId = roadId;
        args.mods = 0;
        args.roadObjectId = roadObjectId;
        args.rotation = rotation;
        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    // 0x00498320
    static void constructBuilding(Town& town, const World::Pos3 pos, const Vehicles::TrackAndDirection::_RoadAndDirection tad, const TownGrowFlags growFlags)
    {
        auto* surface = TileManager::get(pos).surface();
        if (pos.z < surface->baseHeight())
        {
            return;
        }

        const auto nextToData = TrackData::getRoadUnkNextTo(tad._data);

        const auto randItem = (nextToData.size() * (town.prng.randNext() & 0xFFU)) / 256;
        const auto& nextTo = nextToData[randItem];
        auto buildingPos = pos + World::Pos3(Math::Vector::rotate(nextTo.pos, tad.cardinalDirection()), nextTo.pos.z);
        const auto buildingRot = (nextTo.rotation + tad.cardinalDirection()) & 0x3;
        // TODO: Urgh dirty, use a normal randBool
        bool isLarge = false;
        if (town.prng.srand_0() & (1U << 31))
        {
            isLarge = true;
            buildingPos.x += k4FF704[buildingRot].x;
            buildingPos.y += k4FF704[buildingRot].y;
        }

        bool allowBuildingUpdate = false;
        // TODO: Even more dirty
        if ((growFlags & TownGrowFlags::alwaysUpdateBuildings) != TownGrowFlags::none || (isLarge && !(town.prng.srand_0() & 0x7C000000)))
        {
            allowBuildingUpdate = true;
        }

        const auto maxHeight = getMaxHeightOfNewBuilding(buildingPos, isLarge, allowBuildingUpdate);
        if (maxHeight == 0)
        {
            return;
        }

        loco_global<Town*, 0x00525D20> _525D20;
        auto args = generateNewBuildingArgs(buildingPos, maxHeight, buildingRot, isLarge, false);
        if (args.has_value() && _525D20 == &town)
        {
            if ((growFlags & TownGrowFlags::buildImmediately) != TownGrowFlags::none)
            {
                args->buildImmediately = true;
            }
            GameCommands::doCommand(args.value(), GameCommands::Flags::apply);
        }
    }

    template<size_t searchSize>
    constexpr auto kSquareSearchRange = []() {
        /* Searches in a square of increasing size
         * X, Y, Z, J, K
         *
         *     -4-3-2-1 0 1 2 3 4 5
         *     ____________________
         *  4 | K K K K K K K K K K
         *  3 | K J J J J J J J J K
         *  2 | K J Z Z Z Z Z Z J K
         *  1 | K J Z Y Y Y Y Z J K
         *  0 | K J Z Y X X Y Z J K
         * -1 | K J Z Y X X Y Z J K
         * -2 | K J Z Y Y Y Y Z J K
         * -3 | K J Z Z Z Z Z Z J K
         * -4 | K J J J J J J J J K
         * -5 | K K K K K K K K K K
         */
        static_assert((searchSize % 2) == 1, "Must not be an even value");
        std::array<World::TilePos2, (searchSize + 1) * (searchSize + 1)> range{};
        // 0x00503C6C
        std::array<World::TilePos2, 4> kDirections = {
            World::TilePos2{ -1, 0 },
            World::TilePos2{ 0, 1 },
            World::TilePos2{ 1, 0 },
            World::TilePos2{ 0, -1 },
        };

        World::TilePos2 pos{ 0, 0 };
        uint8_t k = 0;
        for (uint8_t i = 1; i <= searchSize; i += 2)
        {
            for (uint8_t direction = 0; direction < 4; ++direction)
            {
                for (auto j = i; j != 0; --j)
                {
                    range[k++] = pos;
                    pos += kDirections[direction];
                }
            }
            pos += World::TilePos2{ 1, -1 };
        }
        return range;
    }();

    // 0x00463BD2
    template<typename Func>
    static void squareSearch(const World::Pos2& centre, [[maybe_unused]] uint8_t searchSize, Func&& predicate)
    {
        assert(searchSize == 9);
        for (auto& offset : kSquareSearchRange<9>)
        {
            const World::Pos2 pos = World::toWorldSpace(offset) + centre;
            if (World::TileManager::validCoords(pos))
            {
                if (!predicate(pos))
                {
                    return;
                }
            }
        }
    }

    struct RoadExtentResult
    {
        World::Pos3 roadStart;
        uint16_t tad;
        bool isBridge;
    };

    // 0x00497FFC
    static std::optional<RoadExtentResult> findRoadExtent(const Town& town)
    {
        struct FindResult
        {
            World::Pos2 loc;
            World::RoadElement* elRoad;
        };

        std::optional<FindResult> res;

        // 0x00497F74
        auto validRoad = [randVal = town.prng.srand_0(), &res](const World::Pos2& loc) mutable {
            auto tile = World::TileManager::get(loc);
            bool hasPassedSurface = false;
            for (auto& el : tile)
            {
                auto* elSurface = el.as<World::SurfaceElement>();
                if (elSurface != nullptr)
                {
                    hasPassedSurface = true;
                    continue;
                }
                if (!hasPassedSurface)
                {
                    continue;
                }
                auto* elRoad = el.as<World::RoadElement>();
                if (elRoad == nullptr)
                {
                    continue;
                }
                if (elRoad->isGhost() || elRoad->isAiAllocated())
                {
                    continue;
                }
                if (elRoad->sequenceIndex() != 0)
                {
                    continue;
                }
                auto* roadObj = ObjectManager::get<RoadObject>(elRoad->roadObjectId());
                if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
                {
                    continue;
                }
                // There is a 50% chance that it will use a new result
                if (res.has_value())
                {
                    bool bitRes = randVal & 1;
                    randVal = std::rotr(randVal, 1);
                    if (bitRes)
                    {
                        return true;
                    }
                }
                res = FindResult{ loc, elRoad };
                return true;
            }
            return true;
        };
        squareSearch({ town.x, town.y }, 9, validRoad);

        if (!res.has_value())
        {
            return std::nullopt;
        }

        auto& roadPiece = World::TrackData::getRoadPiece(res->elRoad->roadId());
        return RoadExtentResult{
            World::Pos3(res->loc, res->elRoad->baseHeight() - roadPiece[0].z),
            static_cast<uint16_t>((res->elRoad->roadId() << 3) | res->elRoad->rotation()),
            res->elRoad->hasBridge()
        };
    }

    // 0x00498101
    static void buildInitialRoad(Town& town)
    {
        // 0x0049807D
        auto placeRoadAtTile = [&town](const World::Pos2& loc) {
            auto tile = World::TileManager::get(loc);
            auto* elSurface = tile.surface();
            auto height = elSurface->baseHeight();
            if (elSurface->slope())
            {
                height += 16;
                if (elSurface->isSlopeDoubleHeight())
                {
                    height += 16;
                }
            }

            auto roadObjId = getIdealTownRoadId(town);
            if (!roadObjId.has_value())
            {
                return true;
            }

            GameCommands::RoadPlacementArgs args{};
            args.pos = World::Pos3{ loc, height };
            args.rotation = town.prng.randNext(3);
            args.roadId = 0;
            args.mods = 0;
            args.bridge = 0xFF;
            args.roadObjectId = roadObjId.value();
            args.unkFlags = 0;
            return GameCommands::doCommand(args, GameCommands::Flags::apply) == GameCommands::FAILURE;
        };
        squareSearch({ town.x, town.y }, 9, placeRoadAtTile);
    }

    /**
     * 0x00498116
     * Grow
     *
     * @param this @<esi>
     * @param growFlags @<eax>
     */
    void Town::grow(TownGrowFlags growFlags)
    {
        const auto oldUpatingCompany = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(CompanyId::neutral);

        const auto extent = findRoadExtent(*this);
        if (!extent.has_value())
        {
            if ((growFlags & TownGrowFlags::buildInitialRoad) != TownGrowFlags::none)
            {
                buildInitialRoad(*this);
            }
            GameCommands::setUpdatingCompanyId(oldUpatingCompany);
            return;
        }

        auto roadStart = extent->roadStart;
        Vehicles::TrackAndDirection::_RoadAndDirection tad(0, 0);
        tad._data = extent->tad;
        if (prng.randBool())
        {
            auto& roadSize = World::TrackData::getUnkRoad(tad._data);
            roadStart += roadSize.pos;
            if (roadSize.rotationEnd < 12)
            {
                roadStart -= World::Pos3{ kRotationOffset[roadSize.rotationEnd], 0 };
            }
            tad.setReversed(!tad.isReversed());
        }

        const auto idealRoadId = getIdealTownRoadId(*this);
        if (!idealRoadId.has_value())
        {
            GameCommands::setUpdatingCompanyId(oldUpatingCompany);
            return;
        }

        auto curRoadPos = roadStart;
        bool curOnBridge = extent->isBridge;

        for (auto i = 0U; i < 75; ++i)
        {
            auto res = TownManager::getClosestTownAndDensity(curRoadPos);
            if (!res.has_value() || res->first != id())
            {
                break;
            }
            const auto curRoadInfo = getRoadInformation(curRoadPos, tad);
            if (curRoadInfo.owner != CompanyId::neutral)
            {
                if ((growFlags & TownGrowFlags::neutralRoadTakeover) != TownGrowFlags::none)
                {
                    if (curOnBridge || hasNearbyBuildings(World::toTileSpace(curRoadPos)))
                    {
                        updateAndTakeoverRoad(curRoadPos, tad, curRoadInfo.roadObjId, CompanyId::neutral, curRoadInfo.streetLightStyle.value_or(0U));
                        break;
                    }
                }
            }
            else
            {
                if (curRoadInfo.roadObjId != idealRoadId.value())
                {
                    const auto* curRoadObj = ObjectManager::get<RoadObject>(curRoadInfo.roadObjId);
                    if (!curRoadObj->hasFlags(RoadObjectFlags::isOneWay))
                    {
                        const auto* idealRoadObj = ObjectManager::get<RoadObject>(idealRoadId.value());

                        if (curRoadObj->maxSpeed <= idealRoadObj->maxSpeed)
                        {
                            if ((growFlags & TownGrowFlags::roadUpdate) != TownGrowFlags::none)
                            {
                                if (curOnBridge || hasNearbyBuildings(World::toTileSpace(curRoadPos)))
                                {
                                    updateAndTakeoverRoad(curRoadPos, tad, idealRoadId.value(), curRoadInfo.owner, curRoadInfo.streetLightStyle.value_or(0U));
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if ((growFlags & TownGrowFlags::roadUpdate) != TownGrowFlags::none)
                    {
                        if (curRoadInfo.streetLightStyle.has_value())
                        {
                            const auto newStyle = getStreetLightStyle(curRoadInfo.roadObjId, res->second);
                            if (newStyle != curRoadInfo.streetLightStyle.value())
                            {
                                if (curOnBridge || hasNearbyBuildings(World::toTileSpace(curRoadPos)))
                                {
                                    updateAndTakeoverRoad(curRoadPos, tad, curRoadInfo.roadObjId, curRoadInfo.owner, newStyle);
                                }
                            }
                        }
                    }
                }
            }

            if ((growFlags & TownGrowFlags::constructBuildings) != TownGrowFlags::none && !curOnBridge)
            {
                constructBuilding(*this, curRoadPos, tad, growFlags);
            }

            const auto roadEnd = World::Track::getRoadConnectionEnd(curRoadPos, tad._data);
            const auto rc = World::Track::getRoadConnections(roadEnd.nextPos, roadEnd.nextRotation, CompanyId::neutral, 0xFFU, 0, 0);
            if (rc.connections.empty())
            {
                if ((growFlags & TownGrowFlags::allowRoadExpansion) != TownGrowFlags::none && !curRoadInfo.isStationRoadEnd)
                {
                    appendToRoadEnd(*this, roadEnd.nextPos, roadEnd.nextRotation, idealRoadId.value(), i, curOnBridge);
                }
                break;
            }
            auto connection = rc.connections[0];
            // If there are multiple branches choose a random direction to walk
            if (rc.connections.size() > 1)
            {
                const auto randConnect = ((prng.randNext() & 0xFFFF) * rc.connections.size()) / 65536;
                connection = rc.connections[randConnect];
            }

            curRoadPos = roadEnd.nextPos;
            curOnBridge = connection & World::Track::AdditionalTaDFlags::hasBridge;
            tad._data = connection & World::Track::AdditionalTaDFlags::basicTaDMask;
            if ((growFlags & TownGrowFlags::allowRoadBranching) == TownGrowFlags::none)
            {
                continue;
            }

            if (!addRoadJunction(*this, curRoadPos, tad, curOnBridge, idealRoadId.value(), rc.connections))
            {
                continue;
            }
            break;
        }

        GameCommands::setUpdatingCompanyId(oldUpatingCompany);
    }

    StringId Town::getTownSizeString() const
    {
        static StringId townSizeNames[5] = {
            StringIds::town_size_hamlet,
            StringIds::town_size_village,
            StringIds::town_size_town,
            StringIds::town_size_city,
            StringIds::town_size_metropolis
        };

        if (static_cast<uint8_t>(size) < std::size(townSizeNames))
        {
            return townSizeNames[static_cast<uint8_t>(size)];
        }
        return StringIds::town_size_hamlet;
    }
}
