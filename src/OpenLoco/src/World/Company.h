#pragma once

#include "Economy/Currency.h"
#include "Economy/Expenditures.h"
#include "Types.hpp"
#include <OpenLoco/Core/BitSet.hpp>
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace OpenLoco
{
    enum class CompanyFlags : uint32_t
    {
        none = 0U,
        unk0 = (1U << 0),                      // 0x01
        unk1 = (1U << 1),                      // 0x02
        unk2 = (1U << 2),                      // 0x04
        sorted = (1U << 3),                    // 0x08
        increasedPerformance = (1U << 4),      // 0x10
        decreasedPerformance = (1U << 5),      // 0x20
        challengeCompleted = (1U << 6),        // 0x40
        challengeFailed = (1U << 7),           // 0x80
        challengeBeatenByOpponent = (1U << 8), // 0x100
        bankrupt = (1U << 9),                  // 0x200
        autopayLoan = (1U << 31),              // 0x80000000 new for OpenLoco
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(CompanyFlags);

    enum class CorporateRating
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

    class OwnerStatus
    {
        int16_t data[2];

    public:
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
    };

    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);

    constexpr size_t expenditureHistoryCapacity = 16;

    enum class AiThinkState : uint8_t
    {
        unk0,
        unk1,
        unk2,
        unk3,
        unk4,
        unk5,
        unk6,
        unk7,
        unk8,
        unk9,
        unk10,
    };

    enum class AiThoughtType : uint8_t
    {
        unk0,
        unk1,
        unk2,
        unk3,
        unk4,
        unk5,
        unk6,
        unk7,
        unk8,
        unk9,
        unk10,
        unk11,
        unk12,
        unk13,
        unk14,
        unk15,
        unk16,
        unk17,
        unk18,
        unk19,

        null = 0xFF
    };

    constexpr auto kAiThoughtCount = 20;

#pragma pack(push, 1)
    struct Company
    {
        struct AiThought
        {
            struct unk4AE
            {
                StationId var_00; // 0x0
                uint8_t pad_02;
                uint8_t rotation; // 0x3
                World::Pos2 pos;  // 0x4
                uint8_t baseZ;    // 0x8
                uint8_t pad_9[0xE - 0x9];
            };
            static_assert(sizeof(unk4AE) == 0xE);
            AiThoughtType type; // 0x00 0x4A8
            uint8_t var_01;     // 0x4A9
            uint8_t pad_02;
            uint8_t var_03; // 0x4AB size of var_06
            uint8_t pad_04[2];
            unk4AE var_06[4];   // 0x4AE
            uint8_t trackObjId; // 0x3E 0x4E6 track or road (with high bit set)
            uint8_t pad_3F;
            uint8_t mods; // 0x40 0x4E8 track or road
            uint8_t pad_41;
            uint8_t cargoType;           // 0x42 0x4EA
            uint8_t var_43;              // 0x4EB
            uint8_t var_44;              // 0x4EC size of var_66
            uint8_t var_45;              // 0x4ED size of var_46
            uint8_t pad_46[0x66 - 0x46]; // array of uint16_t object id's unsure of size
            EntityId var_66[8];          // 0x50E
            currency32_t var_76;         // 0x51E
            uint8_t pad_7A[0x7C - 0x7A];
            currency32_t var_7C; // 0x524
            currency32_t var_80; // 0x528
            currency32_t var_84; // 0x52C
            uint8_t var_88;      // 0x530
            uint8_t pad_89[2];
            uint8_t var_8B; // 0x533
        };
        static_assert(sizeof(AiThought) == 0x8C);
        StringId name;
        StringId ownerName;
        CompanyFlags challengeFlags;      // 0x04
        currency48_t cash;                // 0x08
        currency32_t currentLoan;         // 0x0E
        uint32_t updateCounter;           // 0x12
        int16_t performanceIndex;         // 0x16
        uint8_t competitorId;             // 0x18
        uint8_t ownerEmotion;             // 0x19
        ColourScheme mainColours;         // 0x1A
        ColourScheme vehicleColours[10];  // 0x1C
        uint32_t customVehicleColoursSet; // 0x30
        BitSet<224> unlockedVehicles;     // 0x34
        uint16_t availableVehicles;       // 0x50
        uint8_t pad_52[0x57 - 0x52];
        uint8_t numExpenditureMonths;                                                  // 0x57
        currency32_t expenditures[expenditureHistoryCapacity][ExpenditureType::Count]; // 0x58
        uint32_t startedDate;                                                          // 0x0498
        uint32_t var_49C;
        uint32_t var_4A0;
        AiThinkState var_4A4; // 0x04A4
        uint8_t var_4A5;
        uint8_t var_4A6;
        uint8_t var_4A7;
        AiThought aiThoughts[60];    // 0x04A8
        uint8_t var_2578;            // 0x2578 activeThought?
        World::SmallZ headquartersZ; // 0x2579
        coord_t headquartersX;       // 0x257A -1 on no headquarter placed
        coord_t headquartersY;       // 0x257C
        uint8_t pad_257E[0x259E - 0x257E];
        uint32_t var_259E;
        uint8_t pad_25A2[0x85C2 - 0x25A2];
        uint8_t var_85C2;
        uint8_t var_85C3;
        uint16_t var_85C4;
        uint16_t var_85C6;
        uint8_t pad_85C8[0x85F0 - 0x85C8];
        uint16_t var_85F0;
        uint8_t pad_85F2[0x85F6 - 0x85F2];
        uint16_t var_85F6;
        uint32_t cargoUnitsTotalDelivered;        // 0x85F8
        uint32_t cargoUnitsDeliveredHistory[120]; // 0x85FC
        int16_t performanceIndexHistory[120];     // 0x87DC
        uint16_t historySize;                     // 0x88CC
        currency48_t companyValueHistory[120];    // 0x88CE
        currency48_t vehicleProfit;               // 0x8B9E
        uint16_t transportTypeCount[6];           // 0x8BA4
        uint8_t var_8BB0[9];
        ObservationStatus observationStatus; // 0x8BB9;
        TownId observationTownId;            // 0x8BBA;
        EntityId observationEntity;          // 0x8BBC;
        int16_t observationX;                // 0x8BBE;
        int16_t observationY;                // 0x8BC0;
        uint16_t observationObject;          // 0x8BC2;
        uint16_t var_8BC4;
        OwnerStatus ownerStatus; // 0x8BC6
        uint8_t pad_8BCA[0x8BCE - 0x8BCA];
        uint32_t cargoDelivered[32]; // 0x8BCE;
        uint8_t challengeProgress;   // 0x8C4E - percent completed on challenge
        uint8_t pad_8C4F;
        uint32_t cargoUnitsTotalDistance;        // 0x8C50
        uint32_t cargoUnitsDistanceHistory[120]; // 0x8C54
        uint16_t jailStatus;                     // 0x8E34
        uint8_t pad_8E36[0x8FA8 - 0x8E36];

        CompanyId id() const;
        bool empty() const;
        bool isVehicleIndexUnlocked(const uint8_t vehicleIndex) const;
        void recalculateTransportCounts();
        void updateDaily();
        void updateDailyLogic();
        void updateDailyPlayer();
        void evaluateChallengeProgress();
        void updateDailyControllingPlayer();
        void updateMonthlyHeadquarters();
        void updateLoanAutorepay();
        void updateQuarterly();
        void updateVehicleColours();
        void updateHeadquartersColour();
        void updateOwnerEmotion();
        std::vector<uint8_t> getAvailableRailTracks();
        uint8_t getHeadquarterPerformanceVariation() const;

    private:
        void setHeadquartersVariation(const uint8_t variation);
        void setHeadquartersVariation(const uint8_t variation, const World::TilePos2& pos);

        uint8_t getNewChallengeProgress() const;
    };
#pragma pack(pop)

    static_assert(sizeof(Company) == 0x8FA8);
    static_assert(sizeof(Company::expenditures) == 0x440);
    static_assert(offsetof(Company, companyValueHistory[0]) == 0x88CE);
    static_assert(offsetof(Company, vehicleProfit) == 0x8B9E);
    static_assert(offsetof(Company, challengeProgress) == 0x8C4E);
    static_assert(offsetof(Company, var_8BB0) == 0x8BB0);

    StringId getCorporateRatingAsStringId(CorporateRating rating);
    constexpr CorporateRating performanceToRating(int16_t performanceIndex);
    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);
}
