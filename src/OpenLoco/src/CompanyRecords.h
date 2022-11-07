#pragma once

#include "Speed.hpp"
#include "Types.hpp"

namespace OpenLoco::CompanyManager
{
#pragma pack(push, 1)
    struct Records
    {
        Speed16 speed[3];     // 0x000436 (0x0052624E)
        CompanyId company[3]; // 0x00043C (0x00526254)
        uint8_t pad_43A;
        uint32_t date[3]; // 0x000440 (0x00526258)
    };
#pragma pack(pop)

    constexpr CompanyManager::Records kZeroRecords{
        { kSpeedZero, kSpeedZero, kSpeedZero },
        { CompanyId::null, CompanyId::null, CompanyId::null },
        {},
        { 0, 0, 0 },
    };
    const Records& getRecords();
    void setRecords(const Records& records);
}
