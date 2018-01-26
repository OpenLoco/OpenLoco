#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
    struct vehicle_object_unk
    {
        uint8_t length; // 0x00
        uint8_t pad_01[0x04 - 0x01];
        uint8_t sprite_ind; // 0x04
        uint8_t var_05;
    };

    struct vehicle_object_sprite
    {
        uint8_t num_dir; // 0x00
        uint8_t pad_01[0x03 - 0x01];
        uint8_t vehicle_type; // 0x03
        uint8_t num_units;    // 0x04
        uint8_t pad_05;
        uint8_t bogey_position; // 0x06
        uint8_t flags;          // 0x07
        uint8_t pad_08[0x0E - 0x08];
        uint8_t sprite_num; // 0x0E
        uint8_t pad_0F[0x1E - 0xF];
    };

    struct vehicle_object
    {
        string_id name;        // 0x00 probably not confirmed
        uint8_t vehicle_class; // 0x02
        uint8_t type;          // 0x03
        uint8_t pad_04[0x6 - 0x4];
        uint8_t num_mods;      // 0x06
        uint8_t cost_ind;      // 0x07
        int16_t cost_fact;     // 0x08
        uint8_t reliability;   // 0x0A
        uint8_t run_cost_ind;  // 0x0B
        int16_t run_cost_fact; // 0x0C
        uint8_t colour_type;   // 0x0E
        uint8_t num_compat;    // 0x0F
        uint8_t pad_10[0x24 - 0x10];
        vehicle_object_unk var_24[4];
        vehicle_object_sprite sprites[4]; // 0x3C
        uint8_t pad_B4[0xD8 - 0xB4];
        uint16_t power;      // 0xD8
        uint16_t speed;      // 0xDA
        uint16_t rack_speed; // 0xDC
        uint16_t weight;     // 0xDE
        uint16_t flags;      // 0xE0
        uint8_t pad_E1[0x10E - 0xE1];
        uint8_t vis_fx_height; // 0x10E
        uint8_t vis_fx_type;   // 0x10F
        uint8_t pad_110[0x112 - 0x110];
        uint8_t wake_fx_type; // 0x112
        uint8_t pad_113;
        uint16_t designed; // 0x114
        uint16_t obsolete; // 0x116
        uint8_t pad_118;
        uint8_t startsnd_type; // 0x119
        uint8_t pad_11A[0x15A - 0x11A];
        uint8_t numsnd; // 0x15A
        uint8_t pad_15B[0x15E - 0x15B];
    };
}
