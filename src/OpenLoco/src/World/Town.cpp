#include "Town.h"
#include "Config.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoad.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/RoadElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Random.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
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
            return;

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
                history[i] = history[i + 1];
        }
        else
            historySize++;

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
            maxPopulation = std::max(maxPopulation, history[i]);

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
                history[i] += popOffset;
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
            args.unkFlags = 0;
            return GameCommands::doCommand(args, GameCommands::Flags::apply) == GameCommands::FAILURE;
        };
        squareSearch({ x, y }, 9, placeRoadAtTile);
    }
}
