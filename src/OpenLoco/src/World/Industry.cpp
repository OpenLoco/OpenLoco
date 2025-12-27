#include "Industry.h"
#include "Date.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Industries/RemoveIndustry.h"
#include "GameCommands/Terraform/CreateWall.h"
#include "IndustryManager.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/AnimationManager.h"
#include "Map/IndustryElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "MessageManager.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/ObjectManager.h"
#include "Random.h"
#include "StationManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Math/Bound.hpp>
#include <algorithm>

using namespace OpenLoco::World;

namespace OpenLoco
{
    const std::array<Unk4F9274, 1> word_4F9274 = {
        Unk4F9274{ { 0, 0 }, 0 },
    };
    const std::array<Unk4F9274, 4> word_4F927C = {
        Unk4F9274{ { 0, 0 }, 0 },
        Unk4F9274{ { 0, 32 }, 1 },
        Unk4F9274{ { 32, 32 }, 2 },
        Unk4F9274{ { 32, 0 }, 3 },
    };
    const std::span<const Unk4F9274> getBuildingTileOffsets(bool type)
    {
        if (type)
        {
            return word_4F927C;
        }
        return word_4F9274;
    }

    const IndustryObject* Industry::getObject() const
    {
        return ObjectManager::get<IndustryObject>(objectId);
    }

    bool Industry::empty() const
    {
        return name == StringIds::null;
    }

    bool Industry::canReceiveCargo() const
    {
        auto receiveCargoState = false;
        for (const auto& receivedCargo : ObjectManager::get<IndustryObject>(objectId)->requiredCargoType)
        {
            if (receivedCargo != 0xff)
            {
                receiveCargoState = true;
            }
        }
        return receiveCargoState;
    }

    bool Industry::canProduceCargo() const
    {
        auto produceCargoState = false;
        for (const auto& producedCargo : ObjectManager::get<IndustryObject>(objectId)->producedCargoType)
        {
            if (producedCargo != 0xff)
            {
                produceCargoState = true;
            }
        }
        return produceCargoState;
    }

    static bool findTree(SurfaceElement* surface)
    {
        auto element = surface;
        while (!element->isLast())
        {
            element++;
            if (element->type() == ElementType::tree)
            {
                return true;
            }
        }
        return false;
    }

    // 0x0045935F
    void Industry::getStatusString(const char* buffer)
    {
        char* ptr = (char*)buffer;
        *ptr = '\0';
        const auto* industryObj = getObject();

        // Closing Down
        if (hasFlags(IndustryFlags::closingDown))
        {
            ptr = StringManager::formatString(ptr, StringIds::industry_closing_down);
            return;
        }

        // Under Construction
        if (under_construction != 0xFF)
        {
            ptr = StringManager::formatString(ptr, StringIds::industry_under_construction);
            return;
        }

        // Produced Cargo Only
        if (!canReceiveCargo())
        {
            if (!canProduceCargo())
            {
                return;
            }

            ptr = StringManager::formatString(ptr, StringIds::industry_producing);

            ptr = industryObj->getProducedCargoString(ptr);

            return;
        }

        // Required Cargo
        ptr = StringManager::formatString(ptr, StringIds::industry_requires);

        ptr = industryObj->getRequiredCargoString(ptr);

        if (!canProduceCargo())
        {
            return;
        }

        // Production and Received Cargo
        ptr = StringManager::formatString(ptr, StringIds::cargo_to_produce);

        ptr = industryObj->getProducedCargoString(ptr);
    }

    // 0x00453275
    void Industry::update()
    {
        // Maybe worthwhile returning early if the industry does not use farm tiles
        if (!hasFlags(IndustryFlags::isGhost) && under_construction == 0xFF)
        {
            // Run tile loop for 100 iterations every tick; whole 384x384 map scans in 1475 ticks/15 days/37 seconds
            for (int i = 0; i < 100; i++)
            {
                isFarmTileProducing(tileLoop.current());

                // loc_453318
                if (tileLoop.next() == Pos2())
                {
                    calculateFarmProduction();
                    break;
                }
            }
        }
    }

    // 0x004534BD
    void Industry::updateDaily()
    {
        if (hasFlags(IndustryFlags::isGhost))
        {
            return;
        }

        auto* indObj = getObject();

        uint16_t amountToProduce = 0;
        if (indObj->hasFlags(IndustryObjectFlags::requiresAllCargo))
        {
            amountToProduce = std::numeric_limits<uint16_t>::max();
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj->requiredCargoType[i] != 0xFF)
                {
                    amountToProduce = std::min(amountToProduce, receivedCargoQuantityDailyTotal[i]);
                }
            }
            if (amountToProduce != 0)
            {
                for (auto i = 0; i < 3; ++i)
                {
                    if (indObj->requiredCargoType[i] != 0xFF)
                    {
                        receivedCargoQuantityDailyTotal[i] -= amountToProduce;
                    }
                }
            }
        }
        else
        {
            for (auto i = 0; i < 3; ++i)
            {
                if (indObj->requiredCargoType[i] != 0xFF)
                {
                    amountToProduce = Math::Bound::add(amountToProduce, receivedCargoQuantityDailyTotal[i]);
                    receivedCargoQuantityDailyTotal[i] = 0;
                }
            }
        }

        if (amountToProduce != 0)
        {
            for (auto i = 0; i < 2; ++i)
            {
                if (indObj->producedCargoType[i] != 0xFF)
                {
                    outputBuffer[i] = Math::Bound::add(outputBuffer[i], amountToProduce);
                }
            }
        }

        for (auto i = 0; i < 3; ++i)
        {
            // Lose a 16th of required cargo if requiresAllCargo and not equally satisfied
            receivedCargoQuantityDailyTotal[i] -= Math::Bound::add(receivedCargoQuantityDailyTotal[i], 15) / 16;
        }

        for (auto i = 0; i < 2; ++i)
        {
            if (indObj->producedCargoType[i] == 0xFF)
            {
                continue;
            }

            uint16_t realDailyProductionTarget = (dailyProductionTarget[i] * productionRate) / 256;
            if (realDailyProductionTarget < dailyProduction[i])
            {
                dailyProduction[i]--;
            }
            else if (realDailyProductionTarget > dailyProduction[i])
            {
                dailyProduction[i]++;
            }

            outputBuffer[i] = Math::Bound::add(dailyProduction[i], outputBuffer[i]);

            if (outputBuffer[i] >= 8)
            {
                const auto quantityToSend = std::min<uint16_t>(outputBuffer[i], 255);
                outputBuffer[i] -= quantityToSend;
                producedCargoQuantityMonthlyTotal[i] = Math::Bound::add(producedCargoQuantityMonthlyTotal[i], quantityToSend);

                std::vector<StationId> stations;
                for (auto stationId : producedCargoStatsStation[i])
                {
                    if (stationId != StationId::null)
                    {
                        stations.push_back(stationId);
                    }
                }

                const auto quantityDelivered = StationManager::deliverCargoToStations(stations, indObj->producedCargoType[i], quantityToSend);
                producedCargoQuantityDeliveredMonthlyTotal[i] = Math::Bound::add(quantityDelivered, producedCargoQuantityDeliveredMonthlyTotal[i]);
            }
        }
    }

    // 0x00453868
    void Industry::updateMonthly()
    {
        if (hasFlags(IndustryFlags::isGhost))
        {
            return;
        }

        if (hasFlags(IndustryFlags::closingDown) && dailyProduction[0] == 0 && dailyProduction[1] == 0)
        {
            GameCommands::IndustryRemovalArgs args;
            args.industryId = id();
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            return;
        }
        bool hasEvent = false;
        const auto* indObj = getObject();
        if (under_construction == 0xFF
            && (!hasFlags(IndustryFlags::closingDown))
            && indObj->requiredCargoType[0] == 0xFF)
        {
            if (isMonthlyProductionUp())
            {
                dailyProductionTarget[0] = std::min(100, dailyProductionTarget[0] * 2);
                dailyProductionTarget[1] = std::min(100, dailyProductionTarget[1] * 2);
                MessageManager::post(MessageType::industryProductionUp, CompanyId::null, enumValue(id()), 0xFFFF);
                hasEvent = true;
            }
            else if (isMonthlyProductionDown())
            {
                dailyProductionTarget[0] /= 2;
                dailyProductionTarget[1] /= 2;
                MessageManager::post(MessageType::industryProductionDown, CompanyId::null, enumValue(id()), 0xFFFF);
                hasEvent = true;
            }
        }
        if (!hasEvent
            && !IndustryManager::hasFlags(IndustryManager::Flags::disallowIndustriesCloseDown)
            && under_construction == 0xFF
            && !hasFlags(IndustryFlags::closingDown))
        {
            if (isMonthlyProductionClosing())
            {
                flags |= IndustryFlags::closingDown;
                dailyProductionTarget[0] = 0;
                dailyProductionTarget[1] = 0;
                MessageManager::post(MessageType::industryClosingDown, CompanyId::null, enumValue(id()), 0xFFFF);
            }
        }

        if (producedCargoMonthlyHistorySize[0] == std::size(producedCargoMonthlyHistory1))
        {
            std::rotate(std::begin(producedCargoMonthlyHistory1), std::begin(producedCargoMonthlyHistory1) + 1, std::end(producedCargoMonthlyHistory1));
        }
        else
        {
            producedCargoMonthlyHistorySize[0]++;
        }
        const auto newValue = static_cast<uint8_t>(std::min<uint32_t>(producedCargoQuantityMonthlyTotal[0], 12750u) / 50);
        producedCargoMonthlyHistory1[producedCargoMonthlyHistorySize[0] - 1] = newValue;

        if (producedCargoMonthlyHistorySize[1] == std::size(producedCargoMonthlyHistory2))
        {
            std::rotate(std::begin(producedCargoMonthlyHistory2), std::begin(producedCargoMonthlyHistory2) + 1, std::end(producedCargoMonthlyHistory2));
        }
        else
        {
            producedCargoMonthlyHistorySize[1]++;
        }
        const auto newValue2 = static_cast<uint8_t>(std::min<uint32_t>(producedCargoQuantityMonthlyTotal[1], 12750u) / 50);
        producedCargoMonthlyHistory2[producedCargoMonthlyHistorySize[1] - 1] = newValue2;

        producedCargoQuantityPreviousMonth[0] = producedCargoQuantityMonthlyTotal[0];
        producedCargoQuantityMonthlyTotal[0] = 0;
        producedCargoQuantityDeliveredPreviousMonth[0] = producedCargoQuantityDeliveredMonthlyTotal[0];
        producedCargoQuantityDeliveredMonthlyTotal[0] = 0;
        auto transported = std::min(producedCargoQuantityPreviousMonth[0], producedCargoQuantityDeliveredPreviousMonth[0]);
        if (producedCargoQuantityPreviousMonth[0] != 0)
        {
            transported = (transported * 100) / producedCargoQuantityPreviousMonth[0];
        }
        producedCargoPercentTransportedPreviousMonth[0] = transported;

        producedCargoQuantityPreviousMonth[1] = producedCargoQuantityMonthlyTotal[1];
        producedCargoQuantityMonthlyTotal[1] = 0;
        producedCargoQuantityDeliveredPreviousMonth[1] = producedCargoQuantityDeliveredMonthlyTotal[1];
        producedCargoQuantityDeliveredMonthlyTotal[1] = 0;
        auto transported2 = std::min(producedCargoQuantityPreviousMonth[1], producedCargoQuantityDeliveredPreviousMonth[1]);
        if (producedCargoQuantityPreviousMonth[1] != 0)
        {
            transported2 = (transported2 * 100) / producedCargoQuantityPreviousMonth[1];
        }
        producedCargoPercentTransportedPreviousMonth[1] = transported2;

        receivedCargoQuantityPreviousMonth[0] = receivedCargoQuantityMonthlyTotal[0];
        receivedCargoQuantityPreviousMonth[1] = receivedCargoQuantityMonthlyTotal[1];
        receivedCargoQuantityPreviousMonth[2] = receivedCargoQuantityMonthlyTotal[2];
        receivedCargoQuantityMonthlyTotal[0] = 0;
        receivedCargoQuantityMonthlyTotal[1] = 0;
        receivedCargoQuantityMonthlyTotal[2] = 0;
    }

    bool Industry::isMonthlyProductionUp()
    {
        auto* indObj = getObject();
        if (!indObj->hasFlags(IndustryObjectFlags::canIncreaseProduction))
        {
            return false;
        }
        return producedCargoPercentTransportedPreviousMonth[0] > 70
            && gPrng1().randNext(31) == 0
            && dailyProductionTarget[0] < 100
            && dailyProductionTarget[1] < 100;
    }

    bool Industry::isMonthlyProductionDown()
    {
        auto* indObj = getObject();
        if (!indObj->hasFlags(IndustryObjectFlags::canDecreaseProduction))
        {
            return false;
        }
        return (producedCargoPercentTransportedPreviousMonth[0] > 50
                && dailyProductionTarget[0] > 20
                && gPrng1().randNext(31) == 0)
            || (producedCargoPercentTransportedPreviousMonth[0] <= 50
                && dailyProductionTarget[0] > 10
                && gPrng1().randNext(15) == 0);
    }

    bool Industry::isMonthlyProductionClosing()
    {
        auto* indObj = getObject();
        // isObsolete or isTooLowProduction
        return (getCurrentYear() > indObj->obsoleteYear && prng.randNext(0xFFFF) < 102)
            || (indObj->monthlyClosureChance != 0 && indObj->monthlyClosureChance > prng.randNext(0xFFFF));
    }

    // 0x0045329B
    void Industry::isFarmTileProducing(const Pos2& pos)
    {
        const auto& surface = TileManager::get(pos).surface();
        if (surface != nullptr)
        {
            if (surface->isIndustrial())
            {
                if (surface->industryId() == id())
                {
                    uint8_t growthStage = surface->getGrowthStage();
                    const auto* obj = getObject();
                    if (growthStage == 0 || growthStage != obj->farmTileGrowthStageNoProduction)
                    {
                        // loc_4532E5
                        numFarmTiles++;
                        if ((!obj->hasFlags(IndustryObjectFlags::farmProductionIgnoresSnow) && surface->snowCoverage() != 0) || findTree(surface))
                        {
                            numIdleFarmTiles++;
                        }
                    }
                }
            }
        }
    }

    void Industry::calculateFarmProduction()
    {
        // 0x00453366
        // subtract 6% of farm tiles from idle farm tiles, prevents complete shutdown of production in winter
        int16_t adjustedIdleFarmTiles = std::max(0, numIdleFarmTiles - numFarmTiles / 16);
        int16_t productiveFarmTiles = numFarmTiles - adjustedIdleFarmTiles;
        int16_t relativeFarmSize = std::min(productiveFarmTiles / 25, 255);

        const auto* obj = getObject();
        if (relativeFarmSize < obj->farmNumFields)
        {
            productionRate = ((relativeFarmSize * 256) / obj->farmNumFields) & 0xFF;
        }
        else
        {
            // 0x0045335F; moved here.
            productionRate = 255;
        }

        // 0x004533B2
        numFarmTiles = 0;
        numIdleFarmTiles = 0;
        // if the farm is significantly smaller than ideal and 13% of cargo was transported last month, 1/2 chance to grow farm size by one field
        if (productionRate < 224)
        {
            if (producedCargoQuantityPreviousMonth[0] / 8 <= producedCargoQuantityDeliveredPreviousMonth[0] || producedCargoQuantityPreviousMonth[1] / 8 <= producedCargoQuantityDeliveredPreviousMonth[1])
            {
                if (prng.randBool())
                {
                    World::Pos2 randTile{ static_cast<coord_t>(x + (prng.randNext(-15, 16) * 32)), static_cast<coord_t>(y + (prng.randNext(-15, 16) * 32)) };
                    uint8_t wallType = obj->wallTypes[0];
                    uint8_t wallEntranceType = obj->wallTypes[1];
                    if (obj->wallTypes[2] != 0xFF && prng.randBool())
                    {
                        wallType = obj->wallTypes[2];
                        wallEntranceType = obj->wallTypes[3];
                    }
                    uint8_t updateTimerVal = prng.randNext(7);
                    expandGrounds(randTile, wallType, wallEntranceType, 0, updateTimerVal);
                }
            }
        }
    }

    // 0x0045510C bl == 0
    static bool isSurfaceClaimed(const World::TilePos2& pos)
    {
        if (!World::TileManager::validCoords(pos))
        {
            return false;
        }

        const auto tile = World::TileManager::get(pos);
        bool passedSurface = false;
        for (auto& el : tile)
        {
            auto* elSurface = el.as<World::SurfaceElement>();
            if (elSurface != nullptr)
            {
                if (elSurface->water() != 0)
                {
                    return false;
                }
                if (elSurface->hasType6Flag())
                {
                    return false;
                }
                passedSurface = true;
                continue;
            }
            if (!passedSurface)
            {
                continue;
            }
            if (el.isGhost())
            {
                continue;
            }
            if (el.as<World::WallElement>() != nullptr || el.as<World::TreeElement>() != nullptr)
            {
                continue;
            }
            return false;
        }

        return true;
    }

    // 0x0045510C bl == 1
    // originally argument 3 was a union of two fields 0bYYY00XXX; X is growthStage and Y is updateTimer
    bool claimSurfaceForIndustry(const World::TilePos2& pos, IndustryId industryId, uint8_t growthStage, uint8_t updateTimer)
    {
        if (!isSurfaceClaimed(pos))
        {
            return false;
        }

        const auto tile = World::TileManager::get(pos);
        World::SurfaceElement* surface = tile.surface();
        surface->setIsIndustrialFlag(true);
        surface->setIndustry(industryId);
        surface->setUpdateTimer(updateTimer);
        surface->setGrowthStage(growthStage);
        Ui::ViewportManager::invalidate(World::toWorldSpace(pos), surface->baseHeight(), surface->baseHeight() + 32);
        World::TileManager::removeAllWallsOnTileAbove(pos, surface->baseZ());

        return true;
    }

    // 0x00454A43
    // originally argument 3 was a union of two fields 0bYYY00XXX; X is growthStage and Y is updateTimer
    void Industry::expandGrounds(const Pos2& pos, uint8_t wallType, uint8_t wallEntranceType, uint8_t growthStage, uint8_t updateTimer)
    {
        std::size_t numBorders = 0;
        // Search a 5x5 area centred on Pos
        const auto initialTilePos = toTileSpace(pos);
        const auto topRight = initialTilePos - TilePos2{ 2, 2 };
        const auto bottomLeft = initialTilePos + TilePos2{ 2, 2 };

        for (const auto& tilePos : getClampedRange(topRight, bottomLeft))
        {
            if (isSurfaceClaimed(tilePos))
            {
                numBorders++;
            }
        }
        if (numBorders < 20)
        {
            return;
        }

        const auto* indObj = ObjectManager::get<IndustryObject>(objectId);

        std::optional<Core::Prng> isGrowthStageDesyncPrng;
        if (indObj->hasFlags(IndustryObjectFlags::farmTilesGrowthStageDesynchronized))
        {
            isGrowthStageDesyncPrng = prng;
        }
        std::optional<Core::Prng> isPartialCoveragePrng;
        if (indObj->hasFlags(IndustryObjectFlags::farmTilesPartialCoverage))
        {
            // Vanilla mistake here, prng was not set! It would just recycle from a previous isPartialCoveragePrng caller
            isPartialCoveragePrng = prng;
        }

        uint32_t randEntraceMask = 0;
        if (wallType != 0xFF && wallEntranceType != 0xFF)
        {
            randEntraceMask = 1U << (prng.srand_0() & 0xF);
            randEntraceMask |= 1U << ((prng.srand_0() >> 4) & 0x1F);
        }

        std::size_t i = 0;
        for (const auto& tilePos : getClampedRange(topRight, bottomLeft))
        {
            if (isGrowthStageDesyncPrng.has_value())
            {
                const auto randVal = isGrowthStageDesyncPrng->randNext();
                growthStage = ((randVal & 0xFF) * indObj->farmTileNumGrowthStages) / 256;
                updateTimer = (randVal >> 8) & 0x7;
            }
            bool skipBorderClear = false;
            if (isPartialCoveragePrng.has_value())
            {
                // 87% chance that farm tile is not generated. Note that 5x5 fields can overlap so this can be checked multiple times per tile.
                if (isPartialCoveragePrng->randNext() & 0x7)
                {
                    skipBorderClear = true;
                }
            }
            if (!skipBorderClear)
            {
                claimSurfaceForIndustry(tilePos, id(), growthStage, updateTimer);
            }
            if (wallType == 0xFF)
            {
                continue;
            }
            auto getWallPlacementArgs = [randEntraceMask, &i, wallEntranceType, wallType, &tilePos](const uint8_t rotation) {
                GameCommands::WallPlacementArgs args;
                args.pos = World::Pos3(World::toWorldSpace(tilePos), 0);
                args.rotation = rotation;
                args.type = randEntraceMask & (1U << i) ? wallEntranceType : wallType;
                i++;
                args.primaryColour = Colour::black;
                args.secondaryColour = Colour::black;
                args.tertiaryColour = Colour::black;
                return args;
            };
            // If on an edge add a wall
            if (tilePos.x == topRight.x)
            {
                GameCommands::doCommand(getWallPlacementArgs(0), GameCommands::Flags::apply);
            }
            // Must not be else if as corners have two walls
            if (tilePos.x == bottomLeft.x)
            {
                GameCommands::doCommand(getWallPlacementArgs(2), GameCommands::Flags::apply);
            }
            if (tilePos.y == topRight.y)
            {
                GameCommands::doCommand(getWallPlacementArgs(3), GameCommands::Flags::apply);
            }
            if (tilePos.y == bottomLeft.y)
            {
                GameCommands::doCommand(getWallPlacementArgs(1), GameCommands::Flags::apply);
            }
        }
    }

    // 0x00459D43
    void Industry::createMapAnimations()
    {
        for (size_t i = 0; i < numTiles; i++)
        {
            auto& tilePos = tiles[i];
            auto baseZ = (tilePos.z & ~Location::null) / 4;
            auto tile = TileManager::get(tilePos);

            for (auto& el : tile)
            {
                auto* industryEl = el.as<IndustryElement>();
                if (industryEl == nullptr)
                {
                    continue;
                }

                if (industryEl->baseZ() != baseZ)
                {
                    continue;
                }

                auto tileIndustry = industryEl->industry();
                if (tileIndustry != nullptr)
                {
                    const auto* industryObject = tileIndustry->getObject();
                    if (industryObject != nullptr)
                    {
                        auto animOffsets = getBuildingTileOffsets(industryObject->buildingSizeFlags & (1U << industryEl->buildingType()));
                        for (auto animOffset : animOffsets)
                        {
                            AnimationManager::createAnimation(3, animOffset.pos + tilePos, baseZ);
                        }
                    }
                }
            }
        }
    }

    // 0x004574F7
    void Industry::updateProducedCargoStats()
    {
        const auto* industryObj = getObject();

        for (auto cargoNum = 0; cargoNum < 2; ++cargoNum)
        {
            auto& indStatsStation = producedCargoStatsStation[cargoNum];
            auto& indStatsRating = producedCargoStatsRating[cargoNum];
            std::fill(std::begin(indStatsStation), std::end(indStatsStation), StationId::null);
            const auto cargoType = industryObj->producedCargoType[cargoNum];
            if (cargoType == 0xFF)
            {
                continue;
            }

            auto id = 0;
            for (const auto& hasBit : stationsInRange)
            {
                const auto stationId = static_cast<StationId>(id);
                id++;
                if (!hasBit)
                {
                    continue;
                }
                const auto* station = StationManager::get(stationId);
                if (station->empty())
                {
                    continue;
                }

                const auto& cargoStats = station->cargoStats[cargoType];
                if ((cargoStats.flags & StationCargoStatsFlags::acceptedFromProducer) == StationCargoStatsFlags::none)
                {
                    continue;
                }

                const auto rating = cargoStats.rating;
                for (auto index = 0; index < 4; ++index)
                {
                    if (indStatsStation[index] == StationId::null || indStatsRating[index] <= rating)
                    {
                        // This is an insertion sort.
                        // Rotate so that we overwrite the last entry
                        std::rotate(std::begin(indStatsStation) + index, std::end(indStatsStation) - 1, std::end(indStatsStation));
                        std::rotate(std::begin(indStatsRating) + index, std::end(indStatsRating) - 1, std::end(indStatsRating));
                        indStatsStation[index] = stationId;
                        indStatsRating[index] = rating;
                        break;
                    }
                }
            }

            uint8_t ratingFraction = 0xFF;
            for (auto& rating : indStatsRating)
            {
                rating = (rating * ratingFraction) / 256;
                ratingFraction = -rating;
            }
        }
        stationsInRange.reset();
    }
}
