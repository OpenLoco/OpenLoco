#pragma once

#include "Economy/Currency.h"
#include "Effect.h"

namespace OpenLoco
{
#pragma pack(push, 1)

    struct MoneyEffect : EffectEntity
    {
        static constexpr uint32_t kLifetime = 160;        // windowCurrency
        static constexpr uint32_t kRedGreenLifetime = 55; // redGreen (RCT2 legacy) Note: due to delay it is technically 55 * 2

        uint8_t pad_24[0x26 - 0x24];
        union
        {
            uint16_t frame;     // 0x26
            uint16_t moveDelay; // 0x26 Note: this is only used by redGreen money (RCT2 legacy)
        };
        uint16_t numMovements; // 0x28 Note: this is only used by redGreen money (RCT2 legacy)
        int32_t amount;        // 0x2A - currency amount in British pounds - different currencies are probably getting recalculated
        CompanyId var_2E;      // company colour?
        uint8_t pad_2F[0x44 - 0x2F];
        int16_t offsetX; // 0x44
        uint16_t wiggle; // 0x46

        void update();

        static MoneyEffect* create(const World::Pos3& loc, const CompanyId company, const currency32_t amount);
    };
    static_assert(sizeof(MoneyEffect) == 0x48);

#pragma pack(pop)
}
