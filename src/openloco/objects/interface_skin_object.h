#pragma once

#include "../localisation/stringmgr.h"

namespace openloco
{
#pragma pack(push, 1)
    struct interface_skin_object
    {
        string_id name;
        uint32_t img;
        uint8_t colour_06;
        uint8_t colour_07;
        uint8_t colour_08;
        uint8_t colour_09;
        uint8_t colour_0A;
        uint8_t colour_0B;
        uint8_t colour_0C;
        uint8_t colour_0D;
        uint8_t colour_0E;
        uint8_t colour_0F;
        uint8_t colour_10;
        uint8_t colour_11;
        uint8_t colour_12;
        uint8_t colour_13;
        uint8_t colour_14;
        uint8_t colour_15;
        uint8_t colour_16;
        uint8_t colour_17;
    };
#pragma pack(pop)

    namespace interface_skin::image_ids
    {
        constexpr uint32_t toolbar_loadsave = 3;
        constexpr uint32_t toolbar_loadsave_hover = 4;
        constexpr uint32_t toolbar_zoom = 5;
        constexpr uint32_t toolbar_zoom_hover = 6;
        constexpr uint32_t toolbar_rotate = 7;
        constexpr uint32_t toolbar_rotate_hover = 8;
        constexpr uint32_t toolbar_terraform = 9;
        constexpr uint32_t toolbar_terraform_hover = 10;
        constexpr uint32_t toolbar_audio_active = 11;
        constexpr uint32_t toolbar_audio_active_hover = 12;
        constexpr uint32_t toolbar_audio_inactive = 13;
        constexpr uint32_t toolbar_audio_inactive_hover = 14;
        constexpr uint32_t toolbar_view = 15;
        constexpr uint32_t toolbar_view_hover = 16;
        constexpr uint32_t toolbar_towns = 17;
        constexpr uint32_t toolbar_towns_hover = 18;
        constexpr uint32_t toolbar_empty_opaque = 19;
        constexpr uint32_t toolbar_empty_opaque_hover = 20;
        constexpr uint32_t toolbar_empty_transparent = 21;
        constexpr uint32_t toolbar_empty_transparent_hover = 22;
        constexpr uint32_t toolbar_industries = 23;
        constexpr uint32_t toolbar_industries_hover = 24;
        constexpr uint32_t toolbar_airports = 25;
        constexpr uint32_t toolbar_airports_hover = 26;
        constexpr uint32_t toolbar_ports = 27;
        constexpr uint32_t toolbar_ports_hover = 28;

        constexpr uint32_t toolbar_stations = 43;
        constexpr uint32_t toolbar_stations_hover = 44;

        constexpr uint32_t toolbar_menu_zoom_in = 175;
        constexpr uint32_t toolbar_menu_zoom_out = 176;
        constexpr uint32_t toolbar_menu_rotate_clockwise = 177;
        constexpr uint32_t toolbar_menu_rotate_anti_clockwise = 178;

        constexpr uint32_t phone = 188;

        constexpr uint32_t toolbar_menu_map_north = 466;
        constexpr uint32_t toolbar_menu_map_west = 467;
        constexpr uint32_t toolbar_menu_map_south = 468;
        constexpr uint32_t toolbar_menu_map_east = 469;
    }
}
