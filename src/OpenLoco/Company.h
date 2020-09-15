#pragma once

#include "Localisation/StringManager.h"
#include "Management/Expenditures.h"
#include "Types.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace OpenLoco
{

    namespace company_id
    {
        constexpr company_id_t neutral = 15;
        constexpr company_id_t null = std::numeric_limits<company_id_t>::max();
    }

    enum company_flags
    {
        sorted = (1 << 3),                       // 0x08
        increased_performance = (1 << 4),        // 0x10
        decreased_performance = (1 << 5),        // 0x20
        challenge_completed = (1 << 6),          // 0x40
        challenge_failed = (1 << 7),             // 0x80
        challenge_beaten_by_opponent = (1 << 8), // 0x100
        bankrupt = (1 << 9),                     // 0x200
    };

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

    void formatPerformanceIndex(const int16_t performanceIndex, FormatArguments& args);

    constexpr size_t expenditureHistoryCapacity = 16;

    struct ColourScheme
    {
        uint8_t primary;   // 0x1A
        uint8_t secondary; // 0x1B
    };

#pragma pack(push, 1)
    struct company
    {
        string_id name;
        string_id owner_name;
        uint32_t challenge_flags;         // 0x04
        currency48_t cash;                // 0x08
        uint32_t current_loan;            // 0x0E
        uint32_t update_counter;          // 0x12
        int16_t performance_index;        // 0x16
        uint8_t competitor_id;            // 0x18
        uint8_t owner_emotion;            // 0x19
        ColourScheme mainColours;         // 0x1A
        ColourScheme vehicleColours[10];  // 0x1C
        uint32_t customVehicleColoursSet; // 0x30
        uint32_t unlocked_vehicles[7];    // 0x34 (bit field based on vehicle_object index)
        uint16_t available_vehicles;      // 0x50
        uint8_t pad_52[0x57 - 0x52];
        uint8_t numExpenditureMonths;                                                  // 0x57
        currency32_t expenditures[expenditureHistoryCapacity][ExpenditureType::Count]; // 0x58
        uint32_t startedDate;                                                          // 0x0498
        uint8_t pad_49C[0x2579 - 0x49C];
        uint8_t headquarters_z; // 0x2579
        coord_t headquarters_x; // 0x257A -1 on no headquarter placed
        coord_t headquarters_y; // 0x257C
        uint8_t pad_257E[0x85FC - 0x257E];
        uint32_t cargo_units_delivered_history[120]; // 0x85FC
        int16_t performance_index_history[120];      // 0x87DC
        uint16_t history_size;                       // 0x88CC
        currency48_t companyValueHistory[120];       // 0x88CE
        currency48_t vehicleProfit;                  // 0x8B9E
        uint16_t transportTypeCount[6];              // 0x8BA4
        uint8_t var_8BB0[9];
        uint8_t pad_8BB9[0x8BBC - 0x8BB9];
        thing_id_t observation_thing; // 0x8BBC;
        int16_t observation_x;        // 0x8BBE;
        int16_t observation_y;        // 0x8BC0;
        uint8_t pad_8BC2[0x8BCE - 0x8BC2];
        uint32_t cargoDelivered[32]; // 0x8BCE;
        uint8_t var_8C4E;
        uint8_t pad_8C4F[0x8C54 - 0x8C4F];
        uint32_t cargo_units_distance_history[120]; // 0x008C54
        uint16_t jail_status;                       // 0x8E34
        uint8_t pad_8E36[0x8FA8 - 0x8E36];

        company_id_t id() const;
        bool empty() const;
        void aiThink();
        bool isVehicleIndexUnlocked(const uint8_t vehicleIndex) const;
        void recalculateTransportCounts();
    };
#pragma pack(pop)

    static_assert(sizeof(company) == 0x8FA8);
    static_assert(sizeof(company::expenditures) == 0x440);
    static_assert(offsetof(company, companyValueHistory[0]) == 0x88CE);
    static_assert(offsetof(company, vehicleProfit) == 0x8B9E);
    static_assert(offsetof(company, var_8C4E) == 0x8C4E);
    static_assert(offsetof(company, var_8BB0) == 0x8BB0);

    bool isPlayerCompany(company_id_t id);
}
