#pragma once

#include "Economy/Currency.h"
#include "Economy/Expenditures.h"
#include "Map/Map.hpp"
#include "Types.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace OpenLoco
{
    namespace CompanyFlags
    {
        constexpr uint32_t unk0 = (1 << 0);                      //0x01
        constexpr uint32_t sorted = (1 << 3);                    // 0x08
        constexpr uint32_t increasedPerformance = (1 << 4);      // 0x10
        constexpr uint32_t decreasedPerformance = (1 << 5);      // 0x20
        constexpr uint32_t challengeCompleted = (1 << 6);        // 0x40
        constexpr uint32_t challengeFailed = (1 << 7);           // 0x80
        constexpr uint32_t challengeBeatenByOpponent = (1 << 8); // 0x100
        constexpr uint32_t bankrupt = (1 << 9);                  // 0x200
        constexpr uint32_t autopayLoan = (1 << 31);              // 0x80000000 new for OpenLoco
    }

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
        OwnerStatus(const Map::Pos2& pos)
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

#pragma pack(push, 1)
    struct Company
    {
        struct unk4A8
        {
            uint8_t var_00;
            uint8_t var_01;
            uint8_t pad_02[0x42];
            uint8_t var_44; // 0x4EC size of var_66
            uint8_t pad_45[0x66 - 0x45];
            EntityId var_66[11]; // 0x50E unsure on size
            currency32_t var_7C; // 0x524
            uint32_t var_80;     // 0x528
            uint32_t var_84;     // 0x52C
            uint8_t var_88;      // 0x530
            uint8_t pad_89[3];
        };
        static_assert(sizeof(unk4A8) == 0x8C);
        string_id name;
        string_id ownerName;
        uint32_t challengeFlags;          // 0x04
        currency48_t cash;                // 0x08
        currency32_t currentLoan;         // 0x0E
        uint32_t updateCounter;           // 0x12
        int16_t performanceIndex;         // 0x16
        uint8_t competitorId;             // 0x18
        uint8_t ownerEmotion;             // 0x19
        ColourScheme mainColours;         // 0x1A
        ColourScheme vehicleColours[10];  // 0x1C
        uint32_t customVehicleColoursSet; // 0x30
        uint32_t unlockedVehicles[7];     // 0x34 (bit field based on VehicleObject index)
        uint16_t availableVehicles;       // 0x50
        uint8_t pad_52[0x57 - 0x52];
        uint8_t numExpenditureMonths;                                                  // 0x57
        currency32_t expenditures[expenditureHistoryCapacity][ExpenditureType::Count]; // 0x58
        uint32_t startedDate;                                                          // 0x0498
        uint32_t var_49C;
        uint32_t var_4A0;
        uint8_t var_4A4;
        uint8_t var_4A5;
        uint8_t var_4A6;
        uint8_t var_4A7;
        unk4A8 var_4A8[60];
        uint8_t var_2578;
        uint8_t headquartersZ; // 0x2579
        coord_t headquartersX; // 0x257A -1 on no headquarter placed
        coord_t headquartersY; // 0x257C
        uint8_t pad_257E[0x259E - 0x257E];
        uint32_t var_259E;
        uint8_t pad_25A2[0x85C4 - 0x25A2];
        uint16_t var_85C4;
        uint16_t var_85C6;
        uint8_t pad_85C8[0x85F6 - 0x85C8];
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
        EntityId observationThing;           // 0x8BBC;
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
        void aiThink();
        bool isVehicleIndexUnlocked(const uint8_t vehicleIndex) const;
        void recalculateTransportCounts();
        void updateDaily();
        void updateDailyLogic();
        void updateDailyPlayer();
        void updateDailyControllingPlayer();
        void updateMonthlyHeadquarters();
        void updateLoanAutorepay();
        void updateQuarterly();
        void updateVehicleColours();
        void updateHeadquartersColour();
        void updateOwnerEmotion();
        std::vector<uint8_t> getAvailableRailTracks();

    private:
        uint8_t getHeadquarterPerformanceVariation() const;
        void setHeadquartersVariation(const uint8_t variation);
        void setHeadquartersVariation(const uint8_t variation, const Map::TilePos2& pos);

        void callThinkFunc2();
        bool tryPlaceVehicles();
        void sub_431295();
        void sub_43129D();
        void sub_4312AF();
        void sub_4312BF();

        void sub_4308D4();
        void sub_494805();
    };
#pragma pack(pop)

    static_assert(sizeof(Company) == 0x8FA8);
    static_assert(sizeof(Company::expenditures) == 0x440);
    static_assert(offsetof(Company, companyValueHistory[0]) == 0x88CE);
    static_assert(offsetof(Company, vehicleProfit) == 0x8B9E);
    static_assert(offsetof(Company, challengeProgress) == 0x8C4E);
    static_assert(offsetof(Company, var_8BB0) == 0x8BB0);

    string_id getCorporateRatingAsStringId(CorporateRating rating);
    constexpr CorporateRating performanceToRating(int16_t performanceIndex);
    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);
}
