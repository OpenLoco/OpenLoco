#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct InterfaceSkinObject
    {
        static constexpr auto kObjectType = ObjectType::interfaceSkin;

        string_id name; // 0x00
        uint32_t img;   // 0x02
        Colour2 colour_06;
        Colour2 colour_07;
        Colour2 colour_08;
        Colour2 colour_09;
        Colour2 colour_0A;
        Colour2 colour_0B;
        Colour2 colour_0C;
        Colour2 colour_0D;
        Colour2 colour_0E;
        Colour2 colour_0F;
        Colour2 colour_10;
        Colour2 colour_11;
        Colour2 colour_12;
        Colour2 colour_13;
        Colour2 colour_14;
        Colour2 colour_15;
        Colour2 colour_16;
        Colour2 colour_17;

        // 0x0043C888
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(InterfaceSkinObject) == 0x18);

    namespace InterfaceSkin::ImageIds
    {
        constexpr uint32_t preview_image = 0;
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
        constexpr uint32_t toolbar_cogwheels = 29;
        constexpr uint32_t toolbar_cogwheels_hover = 30;
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
        constexpr uint32_t tab_awards = 45;
        constexpr uint32_t toolbar_menu_airport = 46;
        constexpr uint32_t toolbar_menu_ship_port = 47;
        constexpr uint32_t tab_cargo_ratings = 48;
        constexpr uint32_t tab_colour_scheme_frame0 = 49;
        constexpr uint32_t tab_colour_scheme_frame1 = 50;
        constexpr uint32_t tab_colour_scheme_frame2 = 51;
        constexpr uint32_t tab_colour_scheme_frame3 = 52;
        constexpr uint32_t tab_colour_scheme_frame4 = 53;
        constexpr uint32_t tab_colour_scheme_frame5 = 54;
        constexpr uint32_t tab_colour_scheme_frame6 = 55;
        constexpr uint32_t tab_colour_scheme_frame7 = 56;
        constexpr uint32_t tab_population_frame0 = 57;
        constexpr uint32_t tab_population_frame1 = 58;
        constexpr uint32_t tab_population_frame2 = 59;
        constexpr uint32_t tab_population_frame3 = 60;
        constexpr uint32_t tab_population_frame4 = 61;
        constexpr uint32_t tab_population_frame5 = 62;
        constexpr uint32_t tab_population_frame6 = 63;
        constexpr uint32_t tab_population_frame7 = 64;
        constexpr uint32_t tab_performance_index_frame0 = 65;
        constexpr uint32_t tab_performance_index_frame1 = 66;
        constexpr uint32_t tab_performance_index_frame2 = 67;
        constexpr uint32_t tab_performance_index_frame3 = 68;
        constexpr uint32_t tab_performance_index_frame4 = 69;
        constexpr uint32_t tab_performance_index_frame5 = 70;
        constexpr uint32_t tab_performance_index_frame6 = 71;
        constexpr uint32_t tab_performance_index_frame7 = 72;
        constexpr uint32_t tab_cargo_units_frame0 = 73;
        constexpr uint32_t tab_cargo_units_frame1 = 74;
        constexpr uint32_t tab_cargo_units_frame2 = 75;
        constexpr uint32_t tab_cargo_units_frame3 = 76;
        constexpr uint32_t tab_cargo_units_frame4 = 77;
        constexpr uint32_t tab_cargo_units_frame5 = 78;
        constexpr uint32_t tab_cargo_units_frame6 = 79;
        constexpr uint32_t tab_cargo_units_frame7 = 80;
        constexpr uint32_t tab_cargo_distance_frame0 = 81;
        constexpr uint32_t tab_cargo_distance_frame1 = 82;
        constexpr uint32_t tab_cargo_distance_frame2 = 83;
        constexpr uint32_t tab_cargo_distance_frame3 = 84;
        constexpr uint32_t tab_cargo_distance_frame4 = 85;
        constexpr uint32_t tab_cargo_distance_frame5 = 86;
        constexpr uint32_t tab_cargo_distance_frame6 = 87;
        constexpr uint32_t tab_cargo_distance_frame7 = 88;
        constexpr uint32_t tab_production_frame0 = 89;
        constexpr uint32_t tab_production_frame1 = 90;
        constexpr uint32_t tab_production_frame2 = 91;
        constexpr uint32_t tab_production_frame3 = 92;
        constexpr uint32_t tab_production_frame4 = 93;
        constexpr uint32_t tab_production_frame5 = 94;
        constexpr uint32_t tab_production_frame6 = 95;
        constexpr uint32_t tab_production_frame7 = 96;
        constexpr uint32_t tab_wrench_frame0 = 97;
        constexpr uint32_t tab_wrench_frame1 = 98;
        constexpr uint32_t tab_wrench_frame2 = 99;
        constexpr uint32_t tab_wrench_frame3 = 100;
        constexpr uint32_t tab_wrench_frame4 = 101;
        constexpr uint32_t tab_wrench_frame5 = 102;
        constexpr uint32_t tab_wrench_frame6 = 103;
        constexpr uint32_t tab_wrench_frame7 = 104;
        constexpr uint32_t tab_wrench_frame8 = 105;
        constexpr uint32_t tab_wrench_frame9 = 106;
        constexpr uint32_t tab_wrench_frame10 = 107;
        constexpr uint32_t tab_wrench_frame11 = 108;
        constexpr uint32_t tab_wrench_frame12 = 109;
        constexpr uint32_t tab_wrench_frame13 = 110;
        constexpr uint32_t tab_wrench_frame14 = 111;
        constexpr uint32_t tab_wrench_frame15 = 112;
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
        constexpr uint32_t tab_transported_frame0 = 161;
        constexpr uint32_t tab_transported_frame1 = 162;
        constexpr uint32_t tab_transported_frame2 = 163;
        constexpr uint32_t tab_transported_frame3 = 164;
        constexpr uint32_t tab_transported_frame4 = 165;
        constexpr uint32_t tab_transported_frame5 = 166;
        constexpr uint32_t tab_transported_frame6 = 167;
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
        constexpr uint32_t tab_company_details = 181;
        constexpr uint32_t all_stations = 182;
        constexpr uint32_t rail_stations = 183;
        constexpr uint32_t road_stations = 184;
        constexpr uint32_t airports = 185;
        constexpr uint32_t ship_ports = 186;
        constexpr uint32_t toolbar_menu_build_walls = 187;
        constexpr uint32_t phone = 188;
        constexpr uint32_t toolbar_menu_towns = 189;
        constexpr uint32_t toolbar_menu_stations = 190;
        constexpr uint32_t toolbar_menu_industries = 191;
        constexpr uint32_t tab_routes_frame_0 = 192;
        constexpr uint32_t tab_routes_frame_1 = 193;
        constexpr uint32_t tab_routes_frame_2 = 194;
        constexpr uint32_t tab_routes_frame_3 = 195;
        constexpr uint32_t tab_messages = 196;
        constexpr uint32_t tab_message_settings = 197;
        constexpr uint32_t tab_cargo_delivered_frame0 = 198;
        constexpr uint32_t tab_cargo_delivered_frame1 = 199;
        constexpr uint32_t tab_cargo_delivered_frame2 = 200;
        constexpr uint32_t tab_cargo_delivered_frame3 = 201;
        constexpr uint32_t tab_cargo_payment_rates = 202;
        constexpr uint32_t tab_vehicle_train_frame0 = 203;
        constexpr uint32_t tab_vehicle_train_frame1 = 204;
        constexpr uint32_t tab_vehicle_train_frame2 = 205;
        constexpr uint32_t tab_vehicle_train_frame3 = 206;
        constexpr uint32_t tab_vehicle_train_frame4 = 207;
        constexpr uint32_t tab_vehicle_train_frame5 = 208;
        constexpr uint32_t tab_vehicle_train_frame6 = 209;
        constexpr uint32_t tab_vehicle_train_frame7 = 210;
        constexpr uint32_t tab_vehicle_aircraft_frame0 = 211;
        constexpr uint32_t tab_vehicle_aircraft_frame1 = 212;
        constexpr uint32_t tab_vehicle_aircraft_frame2 = 213;
        constexpr uint32_t tab_vehicle_aircraft_frame3 = 214;
        constexpr uint32_t tab_vehicle_aircraft_frame4 = 215;
        constexpr uint32_t tab_vehicle_aircraft_frame5 = 216;
        constexpr uint32_t tab_vehicle_aircraft_frame6 = 217;
        constexpr uint32_t tab_vehicle_aircraft_frame7 = 218;
        constexpr uint32_t tab_vehicle_bus_frame0 = 219;
        constexpr uint32_t tab_vehicle_bus_frame1 = 220;
        constexpr uint32_t tab_vehicle_bus_frame2 = 221;
        constexpr uint32_t tab_vehicle_bus_frame3 = 222;
        constexpr uint32_t tab_vehicle_bus_frame4 = 223;
        constexpr uint32_t tab_vehicle_bus_frame5 = 224;
        constexpr uint32_t tab_vehicle_bus_frame6 = 225;
        constexpr uint32_t tab_vehicle_bus_frame7 = 226;
        constexpr uint32_t tab_vehicle_tram_frame0 = 227;
        constexpr uint32_t tab_vehicle_tram_frame1 = 228;
        constexpr uint32_t tab_vehicle_tram_frame2 = 229;
        constexpr uint32_t tab_vehicle_tram_frame3 = 230;
        constexpr uint32_t tab_vehicle_tram_frame4 = 231;
        constexpr uint32_t tab_vehicle_tram_frame5 = 232;
        constexpr uint32_t tab_vehicle_tram_frame6 = 233;
        constexpr uint32_t tab_vehicle_tram_frame7 = 234;
        constexpr uint32_t tab_vehicle_truck_frame0 = 235;
        constexpr uint32_t tab_vehicle_truck_frame1 = 236;
        constexpr uint32_t tab_vehicle_truck_frame2 = 237;
        constexpr uint32_t tab_vehicle_truck_frame3 = 238;
        constexpr uint32_t tab_vehicle_truck_frame4 = 239;
        constexpr uint32_t tab_vehicle_truck_frame5 = 240;
        constexpr uint32_t tab_vehicle_truck_frame6 = 241;
        constexpr uint32_t tab_vehicle_truck_frame7 = 242;
        constexpr uint32_t tab_vehicle_ship_frame0 = 243;
        constexpr uint32_t tab_vehicle_ship_frame1 = 244;
        constexpr uint32_t tab_vehicle_ship_frame2 = 245;
        constexpr uint32_t tab_vehicle_ship_frame3 = 246;
        constexpr uint32_t tab_vehicle_ship_frame4 = 247;
        constexpr uint32_t tab_vehicle_ship_frame5 = 248;
        constexpr uint32_t tab_vehicle_ship_frame6 = 249;
        constexpr uint32_t tab_vehicle_ship_frame7 = 250;
        constexpr uint32_t build_vehicle_train_frame_0 = 251;
        constexpr uint32_t build_vehicle_train_frame_1 = 252;
        constexpr uint32_t build_vehicle_train_frame_2 = 253;
        constexpr uint32_t build_vehicle_train_frame_3 = 254;
        constexpr uint32_t build_vehicle_train_frame_4 = 255;
        constexpr uint32_t build_vehicle_train_frame_5 = 256;
        constexpr uint32_t build_vehicle_train_frame_6 = 257;
        constexpr uint32_t build_vehicle_train_frame_7 = 258;
        constexpr uint32_t build_vehicle_train_frame_8 = 259;
        constexpr uint32_t build_vehicle_train_frame_9 = 260;
        constexpr uint32_t build_vehicle_train_frame_10 = 261;
        constexpr uint32_t build_vehicle_train_frame_11 = 262;
        constexpr uint32_t build_vehicle_train_frame_12 = 263;
        constexpr uint32_t build_vehicle_train_frame_13 = 264;
        constexpr uint32_t build_vehicle_train_frame_14 = 265;
        constexpr uint32_t build_vehicle_train_frame_15 = 266;
        constexpr uint32_t build_vehicle_aircraft_frame_0 = 267;
        constexpr uint32_t build_vehicle_aircraft_frame_1 = 268;
        constexpr uint32_t build_vehicle_aircraft_frame_2 = 269;
        constexpr uint32_t build_vehicle_aircraft_frame_3 = 270;
        constexpr uint32_t build_vehicle_aircraft_frame_4 = 271;
        constexpr uint32_t build_vehicle_aircraft_frame_5 = 272;
        constexpr uint32_t build_vehicle_aircraft_frame_6 = 273;
        constexpr uint32_t build_vehicle_aircraft_frame_7 = 274;
        constexpr uint32_t build_vehicle_aircraft_frame_8 = 275;
        constexpr uint32_t build_vehicle_aircraft_frame_9 = 276;
        constexpr uint32_t build_vehicle_aircraft_frame_10 = 277;
        constexpr uint32_t build_vehicle_aircraft_frame_11 = 278;
        constexpr uint32_t build_vehicle_aircraft_frame_12 = 279;
        constexpr uint32_t build_vehicle_aircraft_frame_13 = 280;
        constexpr uint32_t build_vehicle_aircraft_frame_14 = 281;
        constexpr uint32_t build_vehicle_aircraft_frame_15 = 282;
        constexpr uint32_t build_vehicle_bus_frame_0 = 283;
        constexpr uint32_t build_vehicle_bus_frame_1 = 284;
        constexpr uint32_t build_vehicle_bus_frame_2 = 285;
        constexpr uint32_t build_vehicle_bus_frame_3 = 286;
        constexpr uint32_t build_vehicle_bus_frame_4 = 287;
        constexpr uint32_t build_vehicle_bus_frame_5 = 288;
        constexpr uint32_t build_vehicle_bus_frame_6 = 289;
        constexpr uint32_t build_vehicle_bus_frame_7 = 290;
        constexpr uint32_t build_vehicle_bus_frame_8 = 291;
        constexpr uint32_t build_vehicle_bus_frame_9 = 292;
        constexpr uint32_t build_vehicle_bus_frame_10 = 293;
        constexpr uint32_t build_vehicle_bus_frame_11 = 294;
        constexpr uint32_t build_vehicle_bus_frame_12 = 295;
        constexpr uint32_t build_vehicle_bus_frame_13 = 296;
        constexpr uint32_t build_vehicle_bus_frame_14 = 297;
        constexpr uint32_t build_vehicle_bus_frame_15 = 298;
        constexpr uint32_t build_vehicle_tram_frame_0 = 299;
        constexpr uint32_t build_vehicle_tram_frame_1 = 300;
        constexpr uint32_t build_vehicle_tram_frame_2 = 301;
        constexpr uint32_t build_vehicle_tram_frame_3 = 302;
        constexpr uint32_t build_vehicle_tram_frame_4 = 303;
        constexpr uint32_t build_vehicle_tram_frame_5 = 304;
        constexpr uint32_t build_vehicle_tram_frame_6 = 305;
        constexpr uint32_t build_vehicle_tram_frame_7 = 306;
        constexpr uint32_t build_vehicle_tram_frame_8 = 307;
        constexpr uint32_t build_vehicle_tram_frame_9 = 308;
        constexpr uint32_t build_vehicle_tram_frame_10 = 309;
        constexpr uint32_t build_vehicle_tram_frame_11 = 310;
        constexpr uint32_t build_vehicle_tram_frame_12 = 311;
        constexpr uint32_t build_vehicle_tram_frame_13 = 312;
        constexpr uint32_t build_vehicle_tram_frame_14 = 313;
        constexpr uint32_t build_vehicle_tram_frame_15 = 314;
        constexpr uint32_t build_vehicle_truck_frame_0 = 315;
        constexpr uint32_t build_vehicle_truck_frame_1 = 316;
        constexpr uint32_t build_vehicle_truck_frame_2 = 317;
        constexpr uint32_t build_vehicle_truck_frame_3 = 318;
        constexpr uint32_t build_vehicle_truck_frame_4 = 319;
        constexpr uint32_t build_vehicle_truck_frame_5 = 320;
        constexpr uint32_t build_vehicle_truck_frame_6 = 321;
        constexpr uint32_t build_vehicle_truck_frame_7 = 322;
        constexpr uint32_t build_vehicle_truck_frame_8 = 323;
        constexpr uint32_t build_vehicle_truck_frame_9 = 324;
        constexpr uint32_t build_vehicle_truck_frame_10 = 325;
        constexpr uint32_t build_vehicle_truck_frame_11 = 326;
        constexpr uint32_t build_vehicle_truck_frame_12 = 327;
        constexpr uint32_t build_vehicle_truck_frame_13 = 328;
        constexpr uint32_t build_vehicle_truck_frame_14 = 329;
        constexpr uint32_t build_vehicle_truck_frame_15 = 330;
        constexpr uint32_t build_vehicle_ship_frame_0 = 331;
        constexpr uint32_t build_vehicle_ship_frame_1 = 332;
        constexpr uint32_t build_vehicle_ship_frame_2 = 333;
        constexpr uint32_t build_vehicle_ship_frame_3 = 334;
        constexpr uint32_t build_vehicle_ship_frame_4 = 335;
        constexpr uint32_t build_vehicle_ship_frame_5 = 336;
        constexpr uint32_t build_vehicle_ship_frame_6 = 337;
        constexpr uint32_t build_vehicle_ship_frame_7 = 338;
        constexpr uint32_t build_vehicle_ship_frame_8 = 339;
        constexpr uint32_t build_vehicle_ship_frame_9 = 340;
        constexpr uint32_t build_vehicle_ship_frame_10 = 341;
        constexpr uint32_t build_vehicle_ship_frame_11 = 342;
        constexpr uint32_t build_vehicle_ship_frame_12 = 343;
        constexpr uint32_t build_vehicle_ship_frame_13 = 344;
        constexpr uint32_t build_vehicle_ship_frame_14 = 345;
        constexpr uint32_t build_vehicle_ship_frame_15 = 346;
        constexpr uint32_t build_industry_frame_0 = 347;
        constexpr uint32_t build_industry_frame_1 = 348;
        constexpr uint32_t build_industry_frame_2 = 349;
        constexpr uint32_t build_industry_frame_3 = 350;
        constexpr uint32_t build_industry_frame_4 = 351;
        constexpr uint32_t build_industry_frame_5 = 352;
        constexpr uint32_t build_industry_frame_6 = 353;
        constexpr uint32_t build_industry_frame_7 = 354;
        constexpr uint32_t build_industry_frame_8 = 355;
        constexpr uint32_t build_industry_frame_9 = 356;
        constexpr uint32_t build_industry_frame_10 = 357;
        constexpr uint32_t build_industry_frame_11 = 358;
        constexpr uint32_t build_industry_frame_12 = 359;
        constexpr uint32_t build_industry_frame_13 = 360;
        constexpr uint32_t build_industry_frame_14 = 361;
        constexpr uint32_t build_industry_frame_15 = 362;
        constexpr uint32_t build_town_frame_0 = 363;
        constexpr uint32_t build_town_frame_1 = 364;
        constexpr uint32_t build_town_frame_2 = 365;
        constexpr uint32_t build_town_frame_3 = 366;
        constexpr uint32_t build_town_frame_4 = 367;
        constexpr uint32_t build_town_frame_5 = 368;
        constexpr uint32_t build_town_frame_6 = 369;
        constexpr uint32_t build_town_frame_7 = 370;
        constexpr uint32_t build_town_frame_8 = 371;
        constexpr uint32_t build_town_frame_9 = 372;
        constexpr uint32_t build_town_frame_10 = 373;
        constexpr uint32_t build_town_frame_11 = 374;
        constexpr uint32_t build_town_frame_12 = 375;
        constexpr uint32_t build_town_frame_13 = 376;
        constexpr uint32_t build_town_frame_14 = 377;
        constexpr uint32_t build_town_frame_15 = 378;
        constexpr uint32_t build_buildings_frame_0 = 379;
        constexpr uint32_t build_buildings_frame_1 = 380;
        constexpr uint32_t build_buildings_frame_2 = 381;
        constexpr uint32_t build_buildings_frame_3 = 382;
        constexpr uint32_t build_buildings_frame_4 = 383;
        constexpr uint32_t build_buildings_frame_5 = 384;
        constexpr uint32_t build_buildings_frame_6 = 385;
        constexpr uint32_t build_buildings_frame_7 = 386;
        constexpr uint32_t build_buildings_frame_8 = 387;
        constexpr uint32_t build_buildings_frame_9 = 388;
        constexpr uint32_t build_buildings_frame_10 = 389;
        constexpr uint32_t build_buildings_frame_11 = 390;
        constexpr uint32_t build_buildings_frame_12 = 391;
        constexpr uint32_t build_buildings_frame_13 = 392;
        constexpr uint32_t build_buildings_frame_14 = 393;
        constexpr uint32_t build_buildings_frame_15 = 394;
        constexpr uint32_t build_misc_buildings_frame_0 = 395;
        constexpr uint32_t build_misc_buildings_frame_1 = 396;
        constexpr uint32_t build_misc_buildings_frame_2 = 397;
        constexpr uint32_t build_misc_buildings_frame_3 = 398;
        constexpr uint32_t build_misc_buildings_frame_4 = 399;
        constexpr uint32_t build_misc_buildings_frame_5 = 400;
        constexpr uint32_t build_misc_buildings_frame_6 = 401;
        constexpr uint32_t build_misc_buildings_frame_7 = 402;
        constexpr uint32_t build_misc_buildings_frame_8 = 403;
        constexpr uint32_t build_misc_buildings_frame_9 = 404;
        constexpr uint32_t build_misc_buildings_frame_10 = 405;
        constexpr uint32_t build_misc_buildings_frame_11 = 406;
        constexpr uint32_t build_misc_buildings_frame_12 = 407;
        constexpr uint32_t build_misc_buildings_frame_13 = 408;
        constexpr uint32_t build_misc_buildings_frame_14 = 409;
        constexpr uint32_t build_misc_buildings_frame_15 = 410;
        constexpr uint32_t build_additional_train = 411;
        constexpr uint32_t build_additional_bus = 412;
        constexpr uint32_t build_additional_truck = 413;
        constexpr uint32_t build_additional_tram = 414;
        constexpr uint32_t build_additional_aircraft = 415;
        constexpr uint32_t build_additional_ship = 416;
        constexpr uint32_t build_headquarters = 417;
        constexpr uint32_t vehicle_train_frame_0 = 418;
        constexpr uint32_t vehicle_train_frame_1 = 419;
        constexpr uint32_t vehicle_train_frame_2 = 420;
        constexpr uint32_t vehicle_train_frame_3 = 421;
        constexpr uint32_t vehicle_train_frame_4 = 422;
        constexpr uint32_t vehicle_train_frame_5 = 423;
        constexpr uint32_t vehicle_train_frame_6 = 424;
        constexpr uint32_t vehicle_train_frame_7 = 425;
        constexpr uint32_t vehicle_aircraft_frame_0 = 426;
        constexpr uint32_t vehicle_aircraft_frame_1 = 427;
        constexpr uint32_t vehicle_aircraft_frame_2 = 428;
        constexpr uint32_t vehicle_aircraft_frame_3 = 429;
        constexpr uint32_t vehicle_aircraft_frame_4 = 430;
        constexpr uint32_t vehicle_aircraft_frame_5 = 431;
        constexpr uint32_t vehicle_aircraft_frame_6 = 432;
        constexpr uint32_t vehicle_aircraft_frame_7 = 433;
        constexpr uint32_t vehicle_buses_frame_0 = 434;
        constexpr uint32_t vehicle_buses_frame_1 = 435;
        constexpr uint32_t vehicle_buses_frame_2 = 436;
        constexpr uint32_t vehicle_buses_frame_3 = 437;
        constexpr uint32_t vehicle_buses_frame_4 = 438;
        constexpr uint32_t vehicle_buses_frame_5 = 439;
        constexpr uint32_t vehicle_buses_frame_6 = 440;
        constexpr uint32_t vehicle_buses_frame_7 = 441;
        constexpr uint32_t vehicle_trams_frame_0 = 442;
        constexpr uint32_t vehicle_trams_frame_1 = 443;
        constexpr uint32_t vehicle_trams_frame_2 = 444;
        constexpr uint32_t vehicle_trams_frame_3 = 445;
        constexpr uint32_t vehicle_trams_frame_4 = 446;
        constexpr uint32_t vehicle_trams_frame_5 = 447;
        constexpr uint32_t vehicle_trams_frame_6 = 448;
        constexpr uint32_t vehicle_trams_frame_7 = 449;
        constexpr uint32_t vehicle_trucks_frame_0 = 450;
        constexpr uint32_t vehicle_trucks_frame_1 = 451;
        constexpr uint32_t vehicle_trucks_frame_2 = 452;
        constexpr uint32_t vehicle_trucks_frame_3 = 453;
        constexpr uint32_t vehicle_trucks_frame_4 = 454;
        constexpr uint32_t vehicle_trucks_frame_5 = 455;
        constexpr uint32_t vehicle_trucks_frame_6 = 456;
        constexpr uint32_t vehicle_trucks_frame_7 = 457;
        constexpr uint32_t vehicle_ships_frame_0 = 458;
        constexpr uint32_t vehicle_ships_frame_1 = 459;
        constexpr uint32_t vehicle_ships_frame_2 = 460;
        constexpr uint32_t vehicle_ships_frame_3 = 461;
        constexpr uint32_t vehicle_ships_frame_4 = 462;
        constexpr uint32_t vehicle_ships_frame_5 = 463;
        constexpr uint32_t vehicle_ships_frame_6 = 464;
        constexpr uint32_t vehicle_ships_frame_7 = 465;
        constexpr uint32_t toolbar_menu_map_north = 466;
        constexpr uint32_t toolbar_menu_map_west = 467;
        constexpr uint32_t toolbar_menu_map_south = 468;
        constexpr uint32_t toolbar_menu_map_east = 469;
    }
}
