#pragma once

#include "Economy/Currency.h"
#include "Economy/Expenditures.h"
#include "Types.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    namespace CompanyFlags
    {
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
        string_id owner_name;
        uint32_t challenge_flags;         // 0x04
        currency48_t cash;                // 0x08
        currency32_t current_loan;        // 0x0E
        uint32_t update_counter;          // 0x12
        int16_t performance_index;        // 0x16
        uint8_t competitor_id;            // 0x18
        uint8_t owner_emotion;            // 0x19
        ColourScheme mainColours;         // 0x1A
        ColourScheme vehicleColours[10];  // 0x1C
        uint32_t customVehicleColoursSet; // 0x30
        uint32_t unlocked_vehicles[7];    // 0x34 (bit field based on VehicleObject index)
        uint16_t available_vehicles;      // 0x50
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
        uint8_t pad_2578;
        uint8_t headquarters_z; // 0x2579
        coord_t headquarters_x; // 0x257A -1 on no headquarter placed
        coord_t headquarters_y; // 0x257C
        uint8_t pad_257E[0x85F8 - 0x257E];
        uint32_t cargoUnitsTotalDelivered;           // 0x85F8
        uint32_t cargo_units_delivered_history[120]; // 0x85FC
        int16_t performance_index_history[120];      // 0x87DC
        uint16_t history_size;                       // 0x88CC
        currency48_t companyValueHistory[120];       // 0x88CE
        currency48_t vehicleProfit;                  // 0x8B9E
        uint16_t transportTypeCount[6];              // 0x8BA4
        uint8_t var_8BB0[9];
        ObservationStatus observationStatus; // 0x8BB9;
        TownId observationTownId;            // 0x8BBA;
        EntityId observation_thing;          // 0x8BBC;
        int16_t observation_x;               // 0x8BBE;
        int16_t observation_y;               // 0x8BC0;
        uint16_t observationObject;          // 0x8BC2;
        uint16_t var_8BC4;
        uint8_t pad_8BC6[0x8BCE - 0x8BC6];
        uint32_t cargoDelivered[32]; // 0x8BCE;
        uint8_t challengeProgress;   // 0x8C4E - percent completed on challenge
        uint8_t pad_8C4F;
        uint32_t cargoUnitsTotalDistance;           // 0x8C50
        uint32_t cargo_units_distance_history[120]; // 0x8C54
        uint16_t jail_status;                       // 0x8E34
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
        void updateLoanAutorepay();
        void updateQuarterly();
        void updateVehicleColours();
        void updateHeadquartersColour();
        void updateOwnerEmotion();
    };
#pragma pack(pop)

    static_assert(sizeof(Company) == 0x8FA8);
    static_assert(sizeof(Company::expenditures) == 0x440);
    static_assert(offsetof(Company, companyValueHistory[0]) == 0x88CE);
    static_assert(offsetof(Company, vehicleProfit) == 0x8B9E);
    static_assert(offsetof(Company, challengeProgress) == 0x8C4E);
    static_assert(offsetof(Company, var_8BB0) == 0x8BB0);

    constexpr CorporateRating performanceToRating(int16_t performanceIndex);
    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);
}
