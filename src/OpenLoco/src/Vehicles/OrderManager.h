#pragma once

#include "LabelFrame.h"
#include "Types.hpp"
#include <OpenLoco/Engine/World.hpp>

#include <cstdint>
#include <string>

namespace OpenLoco::Vehicles
{
    struct VehicleHead;
}

namespace OpenLoco::Vehicles::OrderManager
{
    struct NumDisplayFrame
    {
        uint32_t orderOffset; // 0x0
        LabelFrame frame;     // 0x4
        uint8_t lineNumber;   // 0x24
    };

    uint8_t* orders();
    uint32_t& numOrders();
    void freeOrders(VehicleHead* const head);

    void sub_470795(const uint32_t removeOrderTableOffset, const int16_t sizeOfRemovedOrderTable);
    std::pair<World::Pos3, std::string> generateOrderUiStringAndLoc(uint32_t orderOffset, uint8_t orderNum);
    void generateNumDisplayFrames(Vehicles::VehicleHead* head);
    const std::vector<NumDisplayFrame>& displayFrames();
}
