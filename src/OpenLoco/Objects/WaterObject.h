#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct water_object
    {
        string_id name;
        uint8_t cost_index; // 0x02
        uint8_t var_03;
        uint8_t cost_factor; //0x04
        uint8_t var_05;
        uint32_t var_06;
    };
#pragma pack(pop)

    namespace Water::ImageIds
    {
        constexpr uint32_t toolbar_terraform_water = 42;
    }
}
