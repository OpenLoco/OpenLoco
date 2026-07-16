#pragma once

#include "CompanyAi/CompanyAi.h"
#include "Economy/Currency.h"
#include "Economy/Expenditures.h"
#include "Engine/Limits.h"
#include "Types.hpp"
#include <OpenLoco/Core/BitSet.hpp>
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sfl/static_vector.hpp>
#include <vector>

namespace OpenLoco
{
    enum class CompanyFlags : uint32_t
    {
        none = 0U,
        aiHasStarted = (1U << 0),              // 0x01 Set when an AI starts converting its first ai allocated to real assets
        unk1 = (1U << 1),                      // 0x02
        unk2 = (1U << 2),                      // 0x04
        sorted = (1U << 3),                    // 0x08; unused; previously used by the compnay list
        increasedPerformance = (1U << 4),      // 0x10
        decreasedPerformance = (1U << 5),      // 0x20
        challengeCompleted = (1U << 6),        // 0x40
        challengeFailed = (1U << 7),           // 0x80
        challengeBeatenByOpponent = (1U << 8), // 0x100
        bankrupt = (1U << 9),                  // 0x200
        autopayLoan = (1U << 31),              // 0x80000000 new for OpenLoco
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(CompanyFlags);

    enum class AiPlaystyleFlags : uint32_t
    {
        none = 0U,
        unk0 = (1U << 0),      // 0x01
        unk1 = (1U << 1),      // 0x02
        unk2 = (1U << 2),      // 0x04
        unk3 = (1U << 3),      // 0x08
        noAir = (1U << 4),     // 0x10
        noWater = (1U << 5),   // 0x20
        unk6 = (1U << 6),      // 0x40
        unk7 = (1U << 7),      // 0x80
        townIdSet = (1U << 8), // 0x100
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(AiPlaystyleFlags);

    enum class CorporateRating : uint8_t
    {
        platelayer,           // 0 - 9.9%
        engineer,             // 10 - 19.9%
        trafficManager,       // 20 - 29.9%
        transportCoordinator, // 30 - 39.9%
        routeSupervisor,      // 40 - 49.9%
        director,             // 50 - 59.9%
        chiefExecutive,       // 60 - 69.9%
        chairman,             // 70 - 79.9%
        president,            // 80 - 89.9%
        tycoon                // 90 - 100%
    };

    enum ObservationStatus : uint8_t
    {
        empty,
        buildingTrackRoad,
        buildingAirport,
        buildingDock,
        checkingServices,
        surveyingLandscape,
    };

    enum class Emotion : uint8_t
    {
        neutral,
        happy,
        worried,
        thinking,
        dejected,
        surprised,
        scared,
        angry,
        disgusted,
    };

    class OwnerStatus
    {
    public:
        int16_t data[2];
        OwnerStatus()
        {
            data[0] = -1;
            data[1] = 0;
        }
        OwnerStatus(EntityId id)
        {
            data[0] = -2;
            data[1] = enumValue(id);
        }
        OwnerStatus(const World::Pos2& pos)
        {
            data[0] = pos.x;
            data[1] = pos.y;
        }
        OwnerStatus(int16_t ax, int16_t cx)
        {
            data[0] = ax;
            data[1] = cx;
        }
        void getData(int16_t* res) const
        {
            res[0] = data[0];
            res[1] = data[1];
        }

        bool isEmpty() const { return data[0] == -1; }
        bool isEntity() const { return data[0] == -2; }
        EntityId getEntity() const
        {
            if (isEntity())
            {
                return static_cast<EntityId>(data[1]);
            }
            return EntityId::null;
        }
        World::Pos2 getPosition() const
        {
            if (isEntity())
            {
                return {};
            }
            return World::Pos2{ data[0], data[1] };
        }
    };

    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);

    constexpr size_t kExpenditureHistoryCapacity = 16;

    struct Company
    {
        struct TrackRoadHashTableEntry
        {
            uint16_t posX;                   // x
            uint16_t posYAndFlags;           // y + flags
            uint8_t posZ;                    // z
            uint8_t trackRoadIdAndDirection; // trackId | (direction << 6)

            TrackRoadHashTableEntry() = default;
            TrackRoadHashTableEntry(World::Pos3 pos, uint8_t trackRoadId, uint8_t direction);

            constexpr World::Pos3 getPosition() const
            {
                return World::Pos3(posX, posYAndFlags & 0xFFFE, posZ * 4 /*World::kSmallZStep*/);
            }
            constexpr uint8_t getTrackRoadId() const
            {
                return trackRoadIdAndDirection & 0x3F;
            }
            constexpr uint8_t getDirection() const
            {
                return (trackRoadIdAndDirection >> 6) & 0x03;
            }

            // When looking up the hash table entry if there is a hash collision check the next entries in
            // the table as well for matches until no hash collision or match is found
            constexpr bool hasHashCollision() const
            {
                return (posYAndFlags & (1U << 0)) != 0;
            }
            constexpr uint16_t calculateHash() const;
        };

        StringId name;
        StringId ownerName;
        CompanyFlags challengeFlags;                                                    // 0x04
        currency48_t cash;                                                              // 0x08
        currency32_t currentLoan;                                                       // 0x0E
        uint32_t updateCounter;                                                         // 0x12
        int16_t performanceIndex;                                                       // 0x16
        uint8_t competitorId;                                                           // 0x18
        Emotion ownerEmotion;                                                           // 0x19
        ColourScheme mainColours;                                                       // 0x1A
        ColourScheme vehicleColours[10];                                                // 0x1C
        uint32_t customVehicleColoursSet;                                               // 0x30
        BitSet<224> unlockedVehicles;                                                   // 0x34
        uint16_t availableVehicles;                                                     // 0x50
        AiPlaystyleFlags aiPlaystyleFlags;                                              // 0x52
        uint8_t aiPlaystyleTownId;                                                      // 0x56
        uint8_t numExpenditureYears;                                                    // 0x57
        currency32_t expenditures[kExpenditureHistoryCapacity][ExpenditureType::Count]; // 0x58
        uint32_t startedDate;                                                           // 0x0498
        uint32_t cargoTypesDelivered;                                                   // 0x49C
        uint32_t cargoTypesDelivered2;                                                  // 0x4A0
        AiThinkState aiThinkState;                                                      // 0x04A4
        uint8_t aiThinkSubState;                                                        // 0x4A5
        AiPlaceVehicleState aiPlaceVehicleState;                                        // 0x4A6
        uint8_t aiPlaceVehiclePad;                                                      // 0x4A7
        AiThought aiThoughts[kMaxAiThoughts];                                           // 0x04A8
        uint8_t activeThoughtId;                                                        // 0x2578
        World::SmallZ headquartersZ;                                                    // 0x2579
        coord_t headquartersX;                                                          // 0x257A -1 on no headquarter placed
        coord_t headquartersY;                                                          // 0x257C
        union
        {
            currency32_t activeThoughtRevenueEstimate; // 0x257E Also used for thoughtState2AiStationIdx in sub_430CEC TODO: Don't do this
            uint32_t thoughtState2AiStationIdx;        // 0x257E Also used mostly for activeThoughtRevenueEstimate TODO: Don't do this
        };
        uint32_t aiThoughtCooldown;        // 0x2582
        uint32_t aiBridgeSelectionCounter; // 0x2596
        uint8_t aiBridgeTypeLow;           // 0x259A
        uint8_t aiBridgeTypeMedium;        // 0x259B
        uint8_t aiBridgeTypeHigh;          // 0x259C
        uint32_t aiPlaceVehicleIndex;
        AiThoughtType aiSavedThoughtType;                      // 0x25BE
        CorporateRating currentRating;                         // 0x25BF
        TrackRoadHashTableEntry trackandRoadHashTable[0x1000]; // 0x25C0 Hash table entries
        uint16_t hashTableLength;                              // 0x85C0 Hash table length
        uint8_t aiStationIndex;                                // 0x85C2
        uint8_t aiStationFlags;                                // 0x85C3
        World::Pos2 aiPathfindTargetPos;                       // 0x85C4
        World::SmallZ aiPathfindTargetBaseZ;                   // 0x85C8
        World::Pos2 aiPathfindTargetPos2;                      // 0x85C9
        World::SmallZ aiPathfindTargetBaseZ2;                  // 0x85CD
        uint8_t aiPathfindTargetRot;                           // 0x85CE
        uint8_t aiPathfindTargetRot2;                          // 0x85CF
        World::Pos2 aiPathfindStartPos;                        // 0x85D0
        World::SmallZ aiPathfindStartBaseZ;                    // 0x85D4
        uint16_t aiPathfindStartTad;                           // 0x85D5
        World::Pos2 aiPathfindStartPos2;                       // 0x85D7
        World::SmallZ aiPathfindStartBaseZ2;                   // 0x85DB
        uint16_t aiPathfindStartTad2;                          // 0x85DC
        uint32_t aiPathfindWeighting1;                         // 0x85DE
        uint32_t aiPathfindWeighting2;                         // 0x85E2
        uint16_t aiBestTrackRoadId;                            // 0x85E6
        uint16_t aiPathfindPhase;                              // 0x85E8
        uint32_t aiPathfindMaxWeighting;                       // 0x85EA
        uint8_t aiPathfindUndoCount1;                          // 0x85EE
        uint8_t aiPathfindUndoCount2;                          // 0x85EF
        uint16_t aiPathfindIterCount;                          // 0x85F0
        currency32_t aiVehicleCost;                            // 0x85F2
        uint16_t aiThinkTimer;                                 // 0x85F6
        uint32_t cargoUnitsTotalDelivered;                     // 0x85F8
        uint32_t cargoUnitsDeliveredHistory[120];              // 0x85FC
        int16_t performanceIndexHistory[120];                  // 0x87DC
        uint16_t historySize;                                  // 0x88CC
        currency48_t companyValueHistory[120];                 // 0x88CE
        currency48_t vehicleProfit;                            // 0x8B9E
        uint16_t transportTypeCount[6];                        // 0x8BA4
        uint8_t activeEmotions[9];                             // 0x8BB0 duration in days that emotion is active 0 == not active
        ObservationStatus observationStatus;                   // 0x8BB9;
        TownId observationTownId;                              // 0x8BBA;
        EntityId observationEntity;                            // 0x8BBC;
        int16_t observationX;                                  // 0x8BBE;
        int16_t observationY;                                  // 0x8BC0;
        uint16_t observationObject;                            // 0x8BC2;
        uint16_t observationTimeout;                           // 0x8BC4
        OwnerStatus ownerStatus;                               // 0x8BC6
        uint32_t cargoDelivered[32];                           // 0x8BCE;
        uint8_t challengeProgress;                             // 0x8C4E - percent completed on challenge
        uint8_t numMonthsInTheRed;                             // 0x8C4F
        uint32_t cargoUnitsTotalDistance;                      // 0x8C50
        uint32_t cargoUnitsDistanceHistory[120];               // 0x8C54
        uint16_t jailStatus;                                   // 0x8E34

        CompanyId id() const;
        bool empty() const;
        bool isVehicleIndexUnlocked(const uint8_t vehicleIndex) const;
        void recalculateTransportCounts();
        void clearOwnerStatusForDeletedVehicle(EntityId vehicleId);
        void updateDaily();
        void updateDailyLogic();
        void updateDailyPlayer();
        void evaluateChallengeProgress();
        void updateDailyControllingPlayer();
        void updateMonthlyHeadquarters();
        void updateMonthly1();
        void updateLoanAutorepay();
        void updateQuarterly();
        void updateVehicleColours();
        void updateHeadquartersColour();
        void updateOwnerEmotion();
        uint8_t getHeadquarterPerformanceVariation() const;

        bool hashTableContains(const TrackRoadHashTableEntry& entry) const;
        bool addHashTableEntry(const TrackRoadHashTableEntry& entry);

    private:
        void setHeadquartersVariation(const uint8_t variation);
        void setHeadquartersVariation(const uint8_t variation, const World::TilePos2& pos);

        uint8_t getNewChallengeProgress() const;
    };

    StringId getCorporateRatingAsStringId(CorporateRating rating);
    constexpr CorporateRating performanceToRating(int16_t performanceIndex);
    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);
    void companyEmotionEvent(CompanyId companyId, Emotion emotion);
    void companySetObservation(CompanyId id, ObservationStatus status, World::Pos2 pos, EntityId entity, uint16_t object);

    // This is kMaxRoadObjects + kMaxTrackObjects as tram tracks are roads but are tracks
    // and vice versa there was capabilities for some unknown track type to be classed as a road
    using AvailableTracksAndRoads = sfl::static_vector<uint8_t, Limits::kMaxRoadObjects + Limits::kMaxTrackObjects>;

    AvailableTracksAndRoads companyGetAvailableRailTracks(const CompanyId id);
    AvailableTracksAndRoads companyGetAvailableRoads(const CompanyId id);
    void updateYearly(Company& company);
    struct ProfitAndValue
    {
        currency48_t vehicleProfit;
        currency48_t companyValue;
    };

    // 0x00437D79
    ProfitAndValue calculateCompanyValue(const Company& company);
}
