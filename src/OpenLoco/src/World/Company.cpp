#include "Company.h"
#include "CompanyManager.h"
#include "Entities/EntityManager.h"
#include "GameCommands/Company/ChangeLoan.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "IndustryManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "MessageManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Random.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "StationManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Bound.hpp>
#include <algorithm>
#include <array>
#include <map>
#include <unordered_set>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    bool Company::empty() const
    {
        return name == StringIds::empty;
    }

    // 0x00437ED0
    void Company::recalculateTransportCounts()
    {
        // Reset all counts to 0
        for (auto& count : transportTypeCount)
        {
            count = 0;
        }

        auto companyId = id();
        for (auto* v : VehicleManager::VehicleList())
        {
            if (v->owner == companyId)
            {
                transportTypeCount[enumValue(v->vehicleType)]++;
            }
        }

        Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(companyId));
    }

    // 0x00437FC5
    void Company::updateDaily()
    {
        updateOwnerEmotion();
        for (auto& unk : activeEmotions)
        {
            unk = Math::Bound::sub(unk, 1u);
        }
        updateDailyLogic();
        observationTimeout = Math::Bound::sub(observationTimeout, 1u);
        if (jailStatus != 0)
        {
            jailStatus = Math::Bound::sub(jailStatus, 1u);
            if (jailStatus == 0)
            {
                Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(id()));
                Ui::WindowManager::invalidate(Ui::WindowType::news);
                Ui::WindowManager::invalidate(Ui::WindowType(0x2E));
            }
        }
        if (CompanyManager::isPlayerCompany(id()))
        {
            updateDailyPlayer();
        }
        if (CompanyManager::getControllingId() == id())
        {
            updateDailyControllingPlayer();
        }
    }

    // 0x00438205
    void Company::updateDailyLogic()
    {
        if (CompanyManager::isPlayerCompany(id()))
        {
            if (observationTimeout != 0)
            {
                return;
            }

            if (ownerStatus.isEmpty())
            {
                return;
            }
            if (ownerStatus.isEntity())
            {
                Vehicles::Vehicle train(ownerStatus.getEntity());
                if (train.veh2->position.x != Location::null)
                {
                    companySetObservation(id(), ObservationStatus::checkingServices, train.veh2->position, train.head->id, 0xFFFFU);
                }
            }
            else
            {
                companySetObservation(id(), ObservationStatus::surveyingLandscape, ownerStatus.getPosition(), EntityId::null, 0xFFFFU);
            }
        }
        else
        {
            setAiObservation(id());
        }
    }

    // 0x004387D0
    void Company::updateDailyPlayer()
    {
        if (isEditorMode() || isTitleMode())
        {
            return;
        }

        const auto newProgress = getNewChallengeProgress();
        if (newProgress != challengeProgress)
        {
            challengeProgress = newProgress;
            Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(id()));
        }

        constexpr auto requiredFlags = CompanyFlags::challengeBeatenByOpponent | CompanyFlags::challengeCompleted | CompanyFlags::challengeFailed;
        if ((challengeFlags & requiredFlags) != CompanyFlags::none)
        {
            return;
        }

        evaluateChallengeProgress();
    }

    // Split off from updateDailyPlayer
    void Company::evaluateChallengeProgress()
    {
        if (challengeProgress == 100)
        {
            challengeFlags |= CompanyFlags::challengeCompleted;
            if (CompanyManager::getControllingId() == id())
            {
                if (CompanyManager::getSecondaryPlayerId() != CompanyId::null)
                {
                    auto* secondaryPlayer = CompanyManager::get(CompanyManager::getSecondaryPlayerId());
                    secondaryPlayer->challengeFlags |= CompanyFlags::challengeBeatenByOpponent;
                    Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(secondaryPlayer->id()));
                }
                MessageManager::post(MessageType::congratulationsCompleted, id(), enumValue(id()), 0xFFFF);
                companyEmotionEvent(id(), Emotion::unk1);
                updateOwnerEmotion();
                Ui::Windows::CompanyWindow::openChallenge(id());
                Scenario::getObjectiveProgress().completedChallengeInMonths = Scenario::getObjectiveProgress().monthsInChallenge;
                ScenarioManager::saveNewScore(Scenario::getObjectiveProgress(), id());
            }
            else
            {
                if (CompanyManager::getControllingId() != CompanyId::null)
                {
                    auto* secondaryPlayer = CompanyManager::get(CompanyManager::getControllingId());
                    secondaryPlayer->challengeFlags |= CompanyFlags::challengeBeatenByOpponent;
                    Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(secondaryPlayer->id()));
                }
                MessageManager::post(MessageType::haveBeenBeaten, id(), enumValue(id()), 0xFFFF);
                companyEmotionEvent(id(), Emotion::unk5);
                updateOwnerEmotion();
                Ui::Windows::CompanyWindow::openChallenge(id());
                Scenario::getObjectiveProgress().completedChallengeInMonths = Scenario::getObjectiveProgress().monthsInChallenge;
            }
        }
        else if (challengeProgress == 255)
        {
            challengeFlags |= CompanyFlags::challengeFailed;
            if (CompanyManager::getControllingId() == id())
            {
                MessageManager::post(MessageType::failedObjectives, id(), enumValue(id()), 0xFFFF);
                companyEmotionEvent(id(), Emotion::unk4);
                updateOwnerEmotion();
                Ui::Windows::CompanyWindow::openChallenge(id());
            }
        }
    }

    // Converts performance index to rating
    // 0x00437D60
    // input:
    // ax = performanceIndex
    // output:
    // ax = return value, coprorate rating
    constexpr CorporateRating performanceToRating(int16_t performanceIndex)
    {
        return static_cast<CorporateRating>(std::min(9, performanceIndex / 100));
    }

    static std::map<CorporateRating, StringId> _ratingNames = {
        { CorporateRating::platelayer, StringIds::corporate_rating_platelayer },
        { CorporateRating::engineer, StringIds::corporate_rating_engineer },
        { CorporateRating::trafficManager, StringIds::corporate_rating_traffic_manager },
        { CorporateRating::transportCoordinator, StringIds::corporate_rating_transport_coordinator },
        { CorporateRating::routeSupervisor, StringIds::corporate_rating_route_supervisor },
        { CorporateRating::director, StringIds::corporate_rating_director },
        { CorporateRating::chiefExecutive, StringIds::corporate_rating_chief_executive },
        { CorporateRating::chairman, StringIds::corporate_rating_chairman },
        { CorporateRating::president, StringIds::corporate_rating_president },
        { CorporateRating::tycoon, StringIds::corporate_rating_tycoon },
    };

    StringId getCorporateRatingAsStringId(CorporateRating rating)
    {
        auto it = _ratingNames.find(rating);
        if (it != _ratingNames.end())
        {
            return it->second;
        }
        return StringIds::corporate_rating_platelayer;
    }

    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args)
    {
        args.push(performanceIndex);
        args.push(getCorporateRatingAsStringId(performanceToRating(performanceIndex)));
    }

    // 0x004F9462
    constexpr uint8_t kEmotionDurations[] = {
        0,
        31,
        10,
        7,
        31,
        10,
        31,
        31,
        11,
    };

    // 0x00437F29
    // companyId: ah
    // emotion: al
    void companyEmotionEvent(CompanyId companyId, Emotion emotion)
    {
        auto company = CompanyManager::get(companyId);
        company->activeEmotions[enumValue(emotion)] = kEmotionDurations[enumValue(emotion)];
    }

    static bool shouldSetObservation(Company& company, ObservationStatus status, World::Pos2 pos, EntityId entity, uint16_t object)
    {
        if (company.observationTimeout == 0)
        {
            return true;
        }
        if (status == ObservationStatus::checkingServices
            && company.observationStatus == ObservationStatus::checkingServices)
        {
            return false;
        }
        if (status != company.observationStatus)
        {
            return true;
        }
        if (pos.x != company.observationX)
        {
            return true;
        }
        if (pos.y != company.observationY)
        {
            return true;
        }
        if (object != company.observationObject)
        {
            return true;
        }
        if (entity != company.observationEntity)
        {
            return true;
        }
        return false;
    }

    // 0x00438167
    void companySetObservation(CompanyId id, ObservationStatus status, World::Pos2 pos, EntityId entity, uint16_t object)
    {
        auto* company = CompanyManager::get(id);
        if (shouldSetObservation(*company, status, pos, entity, object))
        {
            company->observationX = pos.x;
            company->observationY = pos.y;
            company->observationEntity = entity;
            company->observationObject = object;
            company->observationStatus = status;
            auto closestTown = TownManager::getClosestTownAndDensity(pos);
            if (closestTown.has_value())
            {
                company->observationTownId = closestTown->first;
            }
            Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(id));
            Ui::WindowManager::invalidate(Ui::WindowType::companyList);
        }
        company->observationTimeout = 5;
    }

    bool Company::isVehicleIndexUnlocked(const uint8_t vehicleIndex) const
    {
        return unlockedVehicles[vehicleIndex];
    }

    // 0x00487FCC
    void Company::updateQuarterly()
    {
        for (auto& thought : aiThoughts)
        {
            if (thought.type == AiThoughtType::null)
                continue;

            thought.var_88 = std::min(0xFF, thought.var_88 + 1);
            thought.var_84 = thought.var_80;
            thought.var_80 = 0;
            currency32_t totalRunCost = 0;
            for (auto i = 0; i < thought.var_44; ++i)
            {
                auto* vehHead = EntityManager::get<Vehicles::VehicleHead>(thought.var_66[i]);
                if (vehHead != nullptr)
                {
                    totalRunCost += vehHead->calculateRunningCost();
                }
            }
            thought.var_7C = totalRunCost;
        }
    }

    void Company::updateDailyControllingPlayer()
    {
        updateLoanAutorepay();
    }

    // 0x0042F220
    void Company::updateMonthlyHeadquarters()
    {
        setHeadquartersVariation(getHeadquarterPerformanceVariation());
    }

    void Company::updateLoanAutorepay()
    {
        if (currentLoan > 0 && cash > 0 && ((challengeFlags & CompanyFlags::autopayLoan) != CompanyFlags::none))
        {
            GameCommands::ChangeLoanArgs args{};
            args.newLoan = currentLoan - std::max<currency32_t>(0, std::min<currency32_t>(currentLoan, cash.asInt64()));

            GameCommands::setUpdatingCompanyId(id());
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }
    }

    // 0x004B8ED2
    void Company::updateVehicleColours()
    {
        for (auto* v : VehicleManager::VehicleList())
        {
            if (v->owner != id())
            {
                continue;
            }
            Vehicles::Vehicle train(*v);
            for (auto& car : train.cars)
            {
                const auto* vehObject = car.body->getObject();
                auto colour = mainColours;
                if (customVehicleColoursSet & (1 << vehObject->colourType))
                {
                    colour = vehicleColours[vehObject->colourType - 1];
                }
                car.applyToComponents([colour](auto& component) { component.colourScheme = colour; });
            }
        }
        Gfx::invalidateScreen();
    }

    // 0x0042F0C1
    static void updateHeadquartersColourAtTile(const World::TilePos2& pos, uint8_t zPos, Colour newColour)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& element : tile)
        {
            if (element.baseZ() != zPos)
                continue;

            auto* building = element.as<World::BuildingElement>();
            if (building == nullptr)
                continue;

            building->setColour(newColour);
            return;
        }
    }

    // 0x0042F07B
    void Company::updateHeadquartersColour()
    {
        if (headquartersX == -1)
            return;

        const auto colour = mainColours.primary;
        auto hqPos = World::toTileSpace(World::Pos2(headquartersX, headquartersY));
        updateHeadquartersColourAtTile(hqPos + World::TilePos2(0, 0), headquartersZ, colour);
        updateHeadquartersColourAtTile(hqPos + World::TilePos2(1, 0), headquartersZ, colour);
        updateHeadquartersColourAtTile(hqPos + World::TilePos2(1, 1), headquartersZ, colour);
        updateHeadquartersColourAtTile(hqPos + World::TilePos2(0, 1), headquartersZ, colour);
    }

    constexpr std::array<uint8_t, 9> emotionWeightings = {
        0,
        4,
        6,
        5,
        3,
        8,
        1,
        2,
        7,
    };

    // 0x00437F47
    void Company::updateOwnerEmotion()
    {
        Emotion newEmotion = Emotion::unk4;
        if ((challengeFlags & CompanyFlags::bankrupt) == CompanyFlags::none)
        {
            newEmotion = Emotion::unk0;
            uint8_t newEmotionWeight = 0;
            for (auto emotion = 0U; emotion < std::size(activeEmotions); ++emotion)
            {
                if (activeEmotions[emotion] == 0)
                {
                    continue;
                }
                const auto emotionWeight = emotionWeightings[emotion];
                if (newEmotionWeight <= emotionWeight)
                {
                    newEmotionWeight = emotionWeight;
                    newEmotion = static_cast<Emotion>(emotion);
                }
            }
        }

        if (newEmotion == ownerEmotion)
        {
            return;
        }

        ownerEmotion = newEmotion;
        Ui::WindowManager::invalidate(Ui::WindowType::company, enumValue(id()));
        if (id() == CompanyManager::getControllingId())
        {
            Ui::WindowManager::invalidate(Ui::WindowType::playerInfoToolbar);
        }
        Ui::WindowManager::invalidate(Ui::WindowType::vehicleList, enumValue(id()));
        Ui::WindowManager::invalidate(Ui::WindowType::stationList, enumValue(id()));
        Ui::WindowManager::invalidate(Ui::WindowType::news);
        Ui::WindowManager::invalidate(Ui::WindowType::companyList);
    }

    /* 0x004A6841
     * Creates a vector of all the available rail track (trains and trams) for a company
     * Tram track is marked with a (1<<7) flag within the uint8_t
     */
    std::vector<uint8_t> Company::getAvailableRailTracks()
    {
        std::vector<uint8_t> result;

        std::unordered_set<uint8_t> tracks;
        for (auto i = 0u; i < ObjectManager::getMaxObjects(ObjectType::vehicle); ++i)
        {
            const auto* vehObj = ObjectManager::get<VehicleObject>(i);
            if (vehObj == nullptr)
            {
                continue;
            }

            if (isVehicleIndexUnlocked(i) && vehObj->mode == TransportMode::rail)
            {
                tracks.insert(vehObj->trackType);
            }
        }

        std::copy_if(std::begin(tracks), std::end(tracks), std::back_inserter(result), [](uint8_t trackIdx) {
            const auto* trackObj = ObjectManager::get<TrackObject>(trackIdx);
            return !trackObj->hasFlags(TrackObjectFlags::unk_02);
        });

        std::unordered_set<uint8_t> roads;
        for (auto i = 0u; i < ObjectManager::getMaxObjects(ObjectType::vehicle); ++i)
        {
            const auto* vehObj = ObjectManager::get<VehicleObject>(i);
            if (vehObj == nullptr)
            {
                continue;
            }

            if (isVehicleIndexUnlocked(i) && vehObj->mode == TransportMode::road)
            {
                if (vehObj->trackType != 0xFF)
                {
                    roads.insert(vehObj->trackType | (1 << 7));
                }
            }
        }
        for (auto i = 0u; i < ObjectManager::getMaxObjects(ObjectType::road); ++i)
        {
            const auto* roadObj = ObjectManager::get<RoadObject>(i);
            if (roadObj == nullptr)
            {
                continue;
            }

            if (roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                roads.insert(i | (1 << 7));
            }
        }

        std::copy_if(std::begin(roads), std::end(roads), std::back_inserter(result), [](uint8_t trackIdx) {
            const auto* trackObj = ObjectManager::get<RoadObject>(trackIdx & ~(1 << 7));
            return trackObj->hasFlags(RoadObjectFlags::unk_01);
        });

        return result;
    }

    // 0x0042F042
    uint8_t Company::getHeadquarterPerformanceVariation() const
    {
        return std::min(performanceIndex / 200, 4);
    }

    // 0x0042F0FC
    void Company::setHeadquartersVariation(const uint8_t variation)
    {
        if (headquartersX == -1)
        {
            return;
        }
        const auto headPos = World::toTileSpace({ headquartersX, headquartersY });
        for (const auto& pos : World::TilePosRangeView(headPos, headPos + World::TilePos2{ 1, 1 }))
        {
            setHeadquartersVariation(variation, pos);
        }
    }

    // 0x0042F142
    void Company::setHeadquartersVariation(const uint8_t variation, const World::TilePos2& pos)
    {
        auto tile = World::TileManager::get(pos);
        for (auto& el : tile)
        {
            auto* elBuilding = el.as<World::BuildingElement>();
            if (elBuilding == nullptr)
            {
                continue;
            }
            if (elBuilding->baseZ() != headquartersZ)
            {
                continue;
            }
            if (elBuilding->variation() == variation)
            {
                break;
            }

            elBuilding->setVariation(variation);
            elBuilding->setAge(0);
            elBuilding->setConstructed(false);
            elBuilding->setUnk5u(0);

            Ui::ViewportManager::invalidate(World::toWorldSpace(pos), elBuilding->baseHeight(), elBuilding->clearHeight());

            const auto* buildingObj = elBuilding->getObject();
            auto totalHeight = 0;
            for (auto part : buildingObj->getBuildingParts(elBuilding->variation()))
            {
                totalHeight += buildingObj->partHeights[part];
            }
            elBuilding->setClearZ((totalHeight / 4) + elBuilding->baseZ());

            Ui::ViewportManager::invalidate(World::toWorldSpace(pos), elBuilding->baseHeight(), elBuilding->clearHeight());
            break;
        }
    }

    // 0x004385F6
    uint8_t Company::getNewChallengeProgress() const
    {
        registers regs;
        regs.esi = X86Pointer(this);
        regs.ebx = enumValue(id());
        call(0x004385F6, regs);
        return regs.al;
    }
}
