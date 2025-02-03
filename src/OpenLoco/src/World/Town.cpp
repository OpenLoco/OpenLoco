#include "Town.h"
#include "Config.h"
#include "Date.h"
#include "GameCommands/Buildings/CreateBuilding.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoad.h"
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
#include "Map/Track/TrackData.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
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

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco
{
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
            grow(0x07);
        }
        else
        {
            for (int32_t counter = 0; counter < growthPerTick; ++counter)
            {
                grow(0x3F);
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

    /**
     * 0x00498116
     * Grow
     *
     * @param this @<esi>
     * @param growFlags @<eax>
     */
    void Town::grow(int32_t growFlags)
    {
        registers regs;
        regs.eax = growFlags;
        regs.esi = X86Pointer(this);
        call(0x00498116, regs);
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
            if (World::validCoords(pos))
            {
                if (!predicate(pos))
                {
                    return;
                }
            }
        }
    }

    // 0x00497FFC
    std::optional<RoadExtentResult> Town::findRoadExtent() const
    {
        struct FindResult
        {
            World::Pos2 loc;
            World::RoadElement* elRoad;
        };

        std::optional<FindResult> res;

        // 0x00497F74
        auto validRoad = [randVal = prng.srand_0(), &res](const World::Pos2& loc) mutable {
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
        squareSearch({ x, y }, 9, validRoad);

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
            if (roadObj->hasFlags(RoadObjectFlags::unk_00))
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
            findResult = Res{ roadObjId, town.size };
        }
        if (findResult.has_value())
        {
            return findResult->roadObjId;
        }
        return std::nullopt;
    }

    // 0x00498101
    void Town::buildInitialRoad()
    {
        // 0x0049807D
        auto placeRoadAtTile = [&town = *this](const World::Pos2& loc) {
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
            return GameCommands::doCommand(args, GameCommands::Flags::apply) == GameCommands::FAILURE;
        };
        squareSearch({ x, y }, 9, placeRoadAtTile);
    }

    static loco_global<World::Pos2[16], 0x00503C6C> _503C6C;

    struct NextRoadResult
    {
        CompanyId owner;                              // bl
        uint8_t roadObjId;                            // bh
        bool isStationRoadEnd;                        // ebx >> 23
        std::optional<uint8_t> levelCrossingObjectId; // ebx >> 16 && ebx >> 24
    };

    // 0x00498D21
    // pos : ax, cx, dx
    // rotation : bl
    // return : flags Carry == found road
    static bool sub_498D21(const World::Pos3 pos, const uint8_t rotation)
    {
        if (!World::validCoords(pos))
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
                auto parts = buildingObj->getBuildingParts(j);
                for (const auto part : parts)
                {
                    height += buildingObj->partHeights[part];
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
    constexpr std::array<uint8_t, 80> kRoadTadConnectionEdge = {
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
    // bl : ebx (must be below 32)
    // return : flags Carry == found road
    static bool sub_498D9A(const World::Pos3 pos, uint8_t bl)
    {
        if (!World::validCoords(pos))
        {
            return false;
        }

        assert(bl < 32);

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
            if (kRoadTadConnectionEdge[tad] & (1U << bl))
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
    // A collision in this function means that the building will not be removed
    // Finds buildings that are old and not headquarters for removal
    static World::TileClearance::ClearFuncResult sub_42DB35(World::TileElement& el, uint8_t baseZ, uint8_t& minZDiff, bool unkFlag)
    {
        auto* elTree = el.as<World::TreeElement>();
        auto* elBuilding = el.as<World::BuildingElement>();
        if (elTree != nullptr)
        {
            return World::TileClearance::ClearFuncResult::noCollision;
        }
        else if (elBuilding != nullptr)
        {
            if (!unkFlag)
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
                return World::TileClearance::ClearFuncResult::noCollision;
            }
            minZDiff = std::min<uint8_t>(minZDiff, diff);
        }
    }

    // 0x0042D9FA
    // If 0 then no new building can be created here
    // pos : (ax, cx, dx)
    // isLargeTile : bh bit 0
    // unkFlag : bh bit 2
    // return maxHeightOfBuilding: ebp
    static int16_t getMaxHeightOfNewBuilding(const World::Pos3 pos, bool isLargeTile, bool unkFlag)
    {
        auto offsets = getBuildingTileOffsets(isLargeTile);
        int32_t maxHeightDiff = 0;
        for (auto& offset : offsets)
        {
            const auto loc = World::Pos2{ pos } + offset.pos;
            if (!World::validCoords(loc))
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
            if (!World::validCoords(loc))
            {
                return 0;
            }
            const auto elSurface = World::TileManager::get(loc).surface();
            const auto minZ = std::min(elSurface->baseHeight(), pos.z) / World::kSmallZStep;
            World::QuarterTile qt(0xF, 0xF);
            auto clearFunc = [baseZ = pos.z / World::kSmallZStep, &minClear, unkFlag](TileElement& el) {
                return sub_42DB35(el, baseZ, minClear, unkFlag);
            };
            if (!World::TileClearance::applyClearAtStandardHeight(pos, minZ, 255, qt, clearFunc))
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
            auto potentialBuildings = sub_42CEBF(curYear, townDensity - 1, isLargeTile, unk525D24, targetHeight);
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
            auto parts = buildingObj->getBuildingParts(j);
            for (const auto part : parts)
            {
                height += buildingObj->partHeights[part];
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
        auto* roadObj = ObjectManager::get<RoadObject>(newRoadObjId);
        const auto roadPiecesFlags = World::TrackData::getRoadMiscData(tad.id()).compatibleFlags;
        if ((roadPiecesFlags & roadObj->roadPieces) != roadObj->roadPieces)
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
                    roadStart -= World::Pos3{ _503C6C[roadSize.rotationEnd], 0 };
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
            return std::size(streetLightObj->designedYear);
        }();
    }

    // 0x0047AC3E
    static NextRoadResult sub_47AC3E(const World::Pos3& loc, Vehicles::TrackAndDirection::_RoadAndDirection tad)
    {
        const auto roadStart = [tad, &loc]() {
            if (tad.isReversed())
            {
                auto& roadSize = World::TrackData::getUnkRoad(tad._data);
                auto roadStart = loc + roadSize.pos;
                if (roadSize.rotationEnd < 12)
                {
                    roadStart -= World::Pos3{ _503C6C[roadSize.rotationEnd], 0 };
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
            return NextRoadResult{ CompanyId::null, 0, false, std::nullopt };
        }

        NextRoadResult result{};
        result.owner = nextElRoad->owner();
        result.roadObjId = nextElRoad->roadObjectId();
        result.levelCrossingObjectId = std::nullopt;
        result.isStationRoadEnd = false;
        const bool hasLevelCrossing = nextElRoad->hasLevelCrossing();
        if (hasLevelCrossing)
        {
            result.levelCrossingObjectId = nextElRoad->levelCrossingObjectId();
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

    void townRegisterHooks()
    {
        registerHook(
            0x0047AC3E,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                const auto loc = World::Pos3(regs.ax, regs.cx, regs.dx);
                Vehicles::TrackAndDirection::_RoadAndDirection tad(0, 0);
                tad._data = regs.ebp;
                const auto res = sub_47AC3E(loc, tad);
                const auto levelCrossingObjectId = res.levelCrossingObjectId.value_or(0);
                const uint32_t ebx = enumValue(res.owner) | (res.roadObjId << 8) | (levelCrossingObjectId << 16) | ((res.isStationRoadEnd ? 1 : 0) << 23) | ((res.levelCrossingObjectId.has_value() ? 1 : 0) << 24);
                regs = backup;
                regs.ebx = ebx;
                return 0;
            });
    }
}
