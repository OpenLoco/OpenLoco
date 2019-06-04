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
        constexpr uint32_t toolbar_map_generation = 29;
        constexpr uint32_t toolbar_map_generation_hover = 30;
        constexpr uint32_t toolbar_build_vehicle_train = 31;
        constexpr uint32_t toolbar_build_vehicle_train_hover = 32;
        constexpr uint32_t toolbar_build_vehicle_bus = 33;
        constexpr uint32_t toolbar_build_vehicle_bus_hover = 34;
        constexpr uint32_t toolbar_build_vehicle_truck = 35;
        constexpr uint32_t toolbar_build_vehicle_truck_hover = 36;
        constexpr uint32_t toolbar_build_vehicle_tram = 37;
        constexpr uint32_t toolbar_build_vehicle_tram_hover = 38;
        constexpr uint32_t toolbar_build_vehicle_airplane = 39;
        constexpr uint32_t toolbar_build_vehicle_airplane_hover = 40;
        constexpr uint32_t toolbar_build_vehicle_boat = 41;
        constexpr uint32_t toolbar_build_vehicle_boat_hover = 42;
        constexpr uint32_t toolbar_stations = 43;
        constexpr uint32_t toolbar_stations_hover = 44;

        constexpr uint32_t toolbar_menu_airport = 46;
        constexpr uint32_t toolbar_menu_ship_port = 47;

        constexpr uint32_t tab_population_frame0 = 57;
        constexpr uint32_t tab_population_frame1 = 58;
        constexpr uint32_t tab_population_frame2 = 59;
        constexpr uint32_t tab_population_frame3 = 60;
        constexpr uint32_t tab_population_frame4 = 61;
        constexpr uint32_t tab_population_frame5 = 62;
        constexpr uint32_t tab_population_frame6 = 63;
        constexpr uint32_t tab_population_frame7 = 64;

        constexpr uint32_t tab_finances_frame0 = 113;
        constexpr uint32_t tab_finances_frame1 = 114;
        constexpr uint32_t tab_finances_frame2 = 115;
        constexpr uint32_t tab_finances_frame3 = 116;
        constexpr uint32_t tab_finances_frame4 = 117;
        constexpr uint32_t tab_finances_frame5 = 118;
        constexpr uint32_t tab_finances_frame6 = 119;
        constexpr uint32_t tab_finances_frame7 = 120;
        constexpr uint32_t tab_finances_frame8 = 121;
        constexpr uint32_t tab_finances_frame9 = 122;
        constexpr uint32_t tab_finances_frame10 = 123;
        constexpr uint32_t tab_finances_frame11 = 124;
        constexpr uint32_t tab_finances_frame12 = 125;
        constexpr uint32_t tab_finances_frame13 = 126;
        constexpr uint32_t tab_finances_frame14 = 127;
        constexpr uint32_t tab_finances_frame15 = 128;
        constexpr uint32_t tab_cup_frame0 = 129;
        constexpr uint32_t tab_cup_frame1 = 130;
        constexpr uint32_t tab_cup_frame2 = 131;
        constexpr uint32_t tab_cup_frame3 = 132;
        constexpr uint32_t tab_cup_frame4 = 133;
        constexpr uint32_t tab_cup_frame5 = 134;
        constexpr uint32_t tab_cup_frame6 = 135;
        constexpr uint32_t tab_cup_frame7 = 136;
        constexpr uint32_t tab_cup_frame8 = 137;
        constexpr uint32_t tab_cup_frame9 = 138;
        constexpr uint32_t tab_cup_frame10 = 139;
        constexpr uint32_t tab_cup_frame11 = 140;
        constexpr uint32_t tab_cup_frame12 = 141;
        constexpr uint32_t tab_cup_frame13 = 142;
        constexpr uint32_t tab_cup_frame14 = 143;
        constexpr uint32_t tab_cup_frame15 = 144;
        constexpr uint32_t tab_ratings_frame0 = 145;
        constexpr uint32_t tab_ratings_frame1 = 146;
        constexpr uint32_t tab_ratings_frame2 = 147;
        constexpr uint32_t tab_ratings_frame3 = 148;
        constexpr uint32_t tab_ratings_frame4 = 149;
        constexpr uint32_t tab_ratings_frame5 = 150;
        constexpr uint32_t tab_ratings_frame6 = 151;
        constexpr uint32_t tab_ratings_frame7 = 152;
        constexpr uint32_t tab_ratings_frame8 = 153;
        constexpr uint32_t tab_ratings_frame9 = 154;
        constexpr uint32_t tab_ratings_frame10 = 155;
        constexpr uint32_t tab_ratings_frame11 = 156;
        constexpr uint32_t tab_ratings_frame12 = 157;
        constexpr uint32_t tab_ratings_frame13 = 158;
        constexpr uint32_t tab_ratings_frame14 = 159;
        constexpr uint32_t tab_ratings_frame15 = 160;

        constexpr uint32_t tab_cogs_frame0 = 168;
        constexpr uint32_t tab_cogs_frame1 = 169;
        constexpr uint32_t tab_cogs_frame2 = 170;
        constexpr uint32_t tab_cogs_frame3 = 171;
        constexpr uint32_t tab_scenario_details = 172;
        constexpr uint32_t tab_company = 173;
        constexpr uint32_t tab_companies = 174;
        constexpr uint32_t toolbar_menu_zoom_in = 175;
        constexpr uint32_t toolbar_menu_zoom_out = 176;
        constexpr uint32_t toolbar_menu_rotate_clockwise = 177;
        constexpr uint32_t toolbar_menu_rotate_anti_clockwise = 178;
        constexpr uint32_t toolbar_menu_plant_trees = 179;
        constexpr uint32_t toolbar_menu_bulldozer = 180;

        constexpr uint32_t all_stations = 182;
        constexpr uint32_t rail_stations = 183;
        constexpr uint32_t road_stations = 184;
        constexpr uint32_t airports = 185;
        constexpr uint32_t ship_ports = 186;
        constexpr uint32_t toolbar_menu_build_walls = 187;
        constexpr uint32_t phone = 188;
        constexpr uint32_t toolbar_menu_towns = 189;

        constexpr uint32_t toolbar_menu_industries = 191;

        constexpr uint32_t build_vehicle_train = 251;

        constexpr uint32_t build_vehicle_aircraft = 267;

        constexpr uint32_t build_vehicle_bus = 283;

        constexpr uint32_t build_vehicle_tram = 299;

        constexpr uint32_t build_vehicle_truck = 315;

        constexpr uint32_t build_vehicle_ship = 331;

        constexpr uint32_t vehicle_train = 418;

        constexpr uint32_t vehicle_aircraft = 426;

        constexpr uint32_t vehicle_bus = 434;

        constexpr uint32_t vehicle_truck = 450;

        constexpr uint32_t vehicle_tram = 442;

        constexpr uint32_t vehicle_ship = 458;

        constexpr uint32_t toolbar_menu_map_north = 466;
        constexpr uint32_t toolbar_menu_map_west = 467;
        constexpr uint32_t toolbar_menu_map_south = 468;
        constexpr uint32_t toolbar_menu_map_east = 469;
    }
}
