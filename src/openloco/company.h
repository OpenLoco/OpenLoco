#pragma once

#include "localisation/stringmgr.h"
#include "management/Expenditures.h"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace openloco
{
    using company_id_t = uint8_t;

    namespace company_id
    {
        constexpr company_id_t neutral = 15;
        constexpr company_id_t null = std::numeric_limits<company_id_t>::max();
    }

    enum company_flags
    {
        increased_performance = (1 << 4),        // 0x10
        decreased_performance = (1 << 5),        // 0x20
        challenge_completed = (1 << 6),          // 0x40
        challenge_failed = (1 << 7),             // 0x80
        challenge_beaten_by_opponent = (1 << 8), // 0x100
        bankrupt = (1 << 9),                     // 0x200
    };

    constexpr size_t expenditureHistoryCapacity = 16;

#pragma pack(push, 1)
    struct company
    {
        string_id name;
        string_id owner_name;
        uint32_t challenge_flags;  // 0x04
        currency48_t cash;         // 0x08
        uint32_t current_loan;     // 0x0E
        uint32_t update_counter;   // 0x12
        int16_t performance_index; // 0x16
        uint8_t competitor_id;     // 0x18
        uint8_t owner_emotion;     // 0x19
        struct
        {
            uint8_t primary;   // 0x1A
            uint8_t secondary; // 0x1B
        } colour;
        uint8_t pad_1C[0x30 - 0x1C];
        uint32_t customVehicleColoursSet; // 0x30
        uint32_t unlocked_vehicles[7]; // 0x34 (bit field based on vehicle_object index)
        uint16_t available_vehicles;   // 0x50
        uint8_t pad_52[0x57 - 0x52];
        uint8_t numExpenditureMonths;                                                  // 0x57
        currency32_t expenditures[expenditureHistoryCapacity][ExpenditureType::Count]; // 0x58
        uint8_t pad_498[0x88CE - 0x498];
        currency48_t companyValue; // 0x88CE
        uint8_t pad_88D4[0x8B9E - 0x88D4];
        currency48_t vehicleProfit; // 0x8B9E
        uint8_t pad_8BA4[0x8BB0 - 0x8BA4];
        uint8_t var_8BB0[9];
        uint8_t pad_8BB9[0x8BBC - 0x8BB9];
        thing_id_t observation_thing; // 0x_8BBC;
        int16_t observation_x;        // 0x8BBE;
        int16_t observation_y;        // 0x8BC0;
        uint8_t pad_8BC2[0x8BCE - 0x8BC2];
        uint32_t cargoDelivered[32]; // 0x8BCE;
        uint8_t var_8C4E;
        uint8_t pad_8C4F[0x8E34 - 0x8C4F];
        uint16_t jail_status; // 0x8E34
        uint8_t pad_8E36[0x8FA8 - 0x8E36];

        company_id_t id() const;
        bool empty() const;
        void ai_think();
    };
#pragma pack(pop)

    static_assert(sizeof(company) == 0x8FA8);
    static_assert(sizeof(company::expenditures) == 0x440);
    static_assert(offsetof(company, companyValue) == 0x88CE);
    static_assert(offsetof(company, vehicleProfit) == 0x8B9E);
    static_assert(offsetof(company, var_8C4E) == 0x8C4E);
    static_assert(offsetof(company, var_8BB0) == 0x8BB0);

    bool is_player_company(company_id_t id);
}
