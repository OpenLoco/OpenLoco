#pragma once

#include "stringmgr.h"

#define UNUSED_STR(x) (x)

namespace openloco::string_ids
{
    constexpr string_id empty = 0;

    constexpr string_id day_1st = 10;
    constexpr string_id day_2nd = 11;
    constexpr string_id day_3rd = 12;
    constexpr string_id day_4th = 13;
    constexpr string_id day_5th = 14;
    constexpr string_id day_6th = 15;
    constexpr string_id day_7th = 16;
    constexpr string_id day_8th = 17;
    constexpr string_id day_9th = 18;
    constexpr string_id day_10th = 19;
    constexpr string_id day_11th = 20;
    constexpr string_id day_12th = 21;
    constexpr string_id day_13th = 22;
    constexpr string_id day_14th = 23;
    constexpr string_id day_15th = 24;
    constexpr string_id day_16th = 25;
    constexpr string_id day_17th = 26;
    constexpr string_id day_18th = 27;
    constexpr string_id day_19th = 28;
    constexpr string_id day_20th = 29;
    constexpr string_id day_21st = 30;
    constexpr string_id day_22nd = 31;
    constexpr string_id day_23rd = 32;
    constexpr string_id day_24th = 33;
    constexpr string_id day_25th = 34;
    constexpr string_id day_26th = 35;
    constexpr string_id day_27th = 36;
    constexpr string_id day_28th = 37;
    constexpr string_id day_29th = 38;
    constexpr string_id day_30th = 39;
    constexpr string_id day_31st = 40;

    constexpr string_id month_short_january = 41;
    constexpr string_id month_short_february = 42;
    constexpr string_id month_short_march = 43;
    constexpr string_id month_short_april = 44;
    constexpr string_id month_short_may = 45;
    constexpr string_id month_short_june = 46;
    constexpr string_id month_short_july = 47;
    constexpr string_id month_short_august = 48;
    constexpr string_id month_short_september = 49;
    constexpr string_id month_short_october = 50;
    constexpr string_id month_short_november = 51;
    constexpr string_id month_short_december = 52;

    constexpr string_id close_window_cross = 56;

    constexpr string_id tooltip_close_window = 60;

    constexpr string_id display_resolution_dropdown_format = 65;
    constexpr string_id display_resolution_label_format = 66;

    constexpr string_id menu_about = 67;
    constexpr string_id about_locomotion_caption = 68;
    constexpr string_id about_locomotion_69 = 69;
    constexpr string_id about_locomotion_70 = 70;
    constexpr string_id about_locomotion_71 = 71;
    constexpr string_id about_locomotion_72 = 72;
    constexpr string_id about_locomotion_73 = 73;
    constexpr string_id about_locomotion_74 = 74;
    constexpr string_id about_locomotion_75 = 75;
    constexpr string_id about_locomotion_76 = 76;
    constexpr string_id about_locomotion_77 = 77;
    constexpr string_id about_locomotion_78 = UNUSED_STR(78);
    constexpr string_id about_locomotion_79 = UNUSED_STR(79);
    constexpr string_id about_locomotion_80 = UNUSED_STR(80);
    constexpr string_id about_locomotion_81 = UNUSED_STR(81);
    constexpr string_id about_locomotion_82 = UNUSED_STR(82);
    constexpr string_id about_locomotion_83 = UNUSED_STR(83);
    constexpr string_id about_locomotion_84 = UNUSED_STR(84);

    constexpr string_id stringid = 85;
    constexpr string_id arg2_stringid = 86;
    constexpr string_id arg4_stringid = 87;
    constexpr string_id arg6_stringid = 88;
    constexpr string_id arg8_stringid = 89;
    constexpr string_id arg10_stringid = 90;
    constexpr string_id arg12_stringid = 91;
    constexpr string_id arg14_stringid = 92;
    constexpr string_id arg16_stringid = 93;
    constexpr string_id arg18_stringid = 94;
    constexpr string_id arg20_stringid = 95;

    constexpr string_id dropdown = 96;

    constexpr string_id menu_load_game = 103;
    constexpr string_id menu_save_game = 104;
    constexpr string_id menu_quit_scenario_editor = 105;
    constexpr string_id menu_quit_game = 106;
    constexpr string_id screenshot_filename_template = 107;
    constexpr string_id menu_screenshot = 108;
    constexpr string_id screenshot_saved_as = 109;
    constexpr string_id screenshot_failed = 110;

    constexpr string_id menu_underground_view = 145;
    constexpr string_id menu_hide_foreground_tracks_roads = 146;

    constexpr string_id menu_rotate_clockwise = 172;
    constexpr string_id menu_rotate_anti_clockwise = 173;

    constexpr string_id label_button_cancel = 186;
    constexpr string_id label_button_ok = 187;

    constexpr string_id tooltip_scroll_left = 188;
    constexpr string_id tooltip_scroll_right = 189;
    constexpr string_id tooltip_scroll_left_fast = 190;
    constexpr string_id tooltip_scroll_right_fast = 191;
    constexpr string_id tooltip_scroll_left_right = 192;
    constexpr string_id tooltip_scroll_up = 193;
    constexpr string_id tooltip_scroll_down = 194;
    constexpr string_id tooltip_scroll_up_fast = 195;
    constexpr string_id tooltip_scroll_down_fast = 196;
    constexpr string_id tooltip_scroll_up_down = 197;

    constexpr string_id tooltip_scroll_map = 198;
    constexpr string_id tooltip_scroll_vehicle_list = 199;
    constexpr string_id tooltip_scroll_new_vehicle_list = 200;
    constexpr string_id tooltip_scroll_list = 201;
    constexpr string_id tooltip_scroll_trees_list = 202;
    constexpr string_id tooltip_scroll_orders_list = 203;
    constexpr string_id tooltip_scroll_station_list = 204;
    constexpr string_id tooltip_scroll_town_list = 205;
    constexpr string_id tooltip_scroll_industry_list = 206;
    constexpr string_id tooltip_scroll_new_industry_list = 207;
    constexpr string_id tooltip_scroll_building_list = 208;
    constexpr string_id tooltip_scroll_walls_list = 209;
    constexpr string_id tooltip_scroll_ratings_list = 210;
    constexpr string_id tooltip_scroll_cargo_list = 211;
    constexpr string_id tooltip_scroll_message_list = 212;
    constexpr string_id tooltip_scroll_company_list = 213;
    constexpr string_id tooltip_scroll_scenario_list = 214;
    constexpr string_id tooltip_scroll_credits_list = 215;

    constexpr string_id menu_zoom_in = 223;
    constexpr string_id menu_zoom_out = 224;

    constexpr string_id menu_towns = 225;
    constexpr string_id menu_industries = 226;
    constexpr string_id menu_airport = 227;
    constexpr string_id menu_ship_port = 228;

    constexpr string_id label_icons_none = 230;
    constexpr string_id label_icons_rail = 231;
    constexpr string_id label_icons_road = 232;
    constexpr string_id label_icons_rail_road = 233;
    constexpr string_id label_icons_air = 234;
    constexpr string_id label_icons_rail_air = 235;
    constexpr string_id label_icons_road_air = 236;
    constexpr string_id label_icons_rail_road_air = 237;
    constexpr string_id label_icons_water = 238;
    constexpr string_id label_icons_rail_water = 239;
    constexpr string_id label_icons_road_water = 240;
    constexpr string_id label_icons_rail_road_water = 241;
    constexpr string_id label_icons_air_water = 242;
    constexpr string_id label_icons_rail_air_water = 243;
    constexpr string_id label_icons_road_air_water = 244;
    constexpr string_id label_icons_rail_road_air_water = 245;

    constexpr string_id buffer_337 = 337;
    constexpr string_id buffer_338 = 338;

    constexpr string_id stringid_stringid = 347;

    constexpr string_id move_main_view_to_show_this = 355;

    constexpr string_id menu_mute = 396;
    constexpr string_id menu_play_music = 397;

    constexpr string_id menu_hide_foreground_scenery_buildings = 372;

    constexpr string_id dropdown_stringid = 421;
    constexpr string_id dropdown_stringptr = 424;

    constexpr string_id menu_height_marks_on_tracks_roads = 426;
    constexpr string_id menu_height_marks_on_land = 427;
    constexpr string_id menu_one_way_direction_arrows = 428;
    constexpr string_id menu_town_names_displayed = 429;
    constexpr string_id menu_station_names_displayed = 430;

    constexpr string_id dropdown_without_checkmark = 443;
    constexpr string_id dropdown_with_checkmark = 444;

    constexpr string_id outlined_wcolour2_stringid2 = 450;

    constexpr string_id white_stringid2 = 455;

    constexpr string_id wcolour2_stringid2 = 457;

    constexpr string_id audio_device_none = 479;
    constexpr string_id stringptr = 480;

    constexpr string_id spinner_up = 486;
    constexpr string_id spinner_down = 487;

    constexpr string_id status_num_stations_singular = 520;
    constexpr string_id status_num_stations_plural = 521;

    constexpr string_id date_monthyear = 584;

    constexpr string_id title_menu_new_game = 613;
    constexpr string_id title_menu_load_game = 614;
    constexpr string_id title_menu_show_tutorial = 615;
    constexpr string_id title_menu_exit_from_game = 616;
    constexpr string_id town_size_hamlet = 617;
    constexpr string_id town_size_village = 618;
    constexpr string_id town_size_town = 619;
    constexpr string_id town_size_city = 620;
    constexpr string_id town_size_metropolis = 621;

    constexpr string_id month_long_january = 635;
    constexpr string_id month_long_february = 636;
    constexpr string_id month_long_march = 637;
    constexpr string_id month_long_april = 638;
    constexpr string_id month_long_may = 639;
    constexpr string_id month_long_june = 640;
    constexpr string_id month_long_july = 641;
    constexpr string_id month_long_august = 642;
    constexpr string_id month_long_september = 643;
    constexpr string_id month_long_october = 644;
    constexpr string_id month_long_november = 645;
    constexpr string_id month_long_december = 646;

    constexpr string_id tooltip_daymonthyear_challenge = 647;

    constexpr string_id sound_quality = UNUSED_STR(650);
    constexpr string_id sound_quality_low = UNUSED_STR(651);
    constexpr string_id sound_quality_medium = UNUSED_STR(652);
    constexpr string_id sound_quality_high = UNUSED_STR(653);
    constexpr string_id options = 654;

    constexpr string_id distance_and_speed = 656;
    constexpr string_id heights = 657;
    constexpr string_id imperial = 658;
    constexpr string_id metric = 659;
    constexpr string_id height_units = 660;
    constexpr string_id height_real_values = 661;

    constexpr string_id display_resolution = 662;
    constexpr string_id landscape_smoothing = 663;
    constexpr string_id landscape_smoothing_tip = 664;
    constexpr string_id gridlines_on_landscape = 665;
    constexpr string_id gridlines_on_landscape_tip = 666;

    constexpr string_id clear_area = 679;
    constexpr string_id menu_clear_area = 680;
    constexpr string_id menu_adjust_land = 681;
    constexpr string_id menu_adjust_water = 682;
    constexpr string_id menu_plant_trees = 683;
    constexpr string_id menu_build_walls = 684;

    constexpr string_id challenge_failed = 695;
    constexpr string_id challenge_completed = 696;

    constexpr string_id challenge_progress = 699;
    constexpr string_id challenge_time_left = 700;

    constexpr string_id customise_keys = 701;
    constexpr string_id keyboard_shortcuts = 702;
    constexpr string_id reset_keys = 703;
    constexpr string_id reset_keys_tip = 704;
    constexpr string_id shortcut_close_topmost_window = 705;
    constexpr string_id shortcut_close_all_floating_windows = 706;
    constexpr string_id shortcut_cancel_construction_mode = 707;
    constexpr string_id shortcut_pause_unpause_game = 708;
    constexpr string_id shortcut_zoom_view_out = 709;
    constexpr string_id shortcut_zoom_view_in = 710;
    constexpr string_id shortcut_rotate_view = 711;
    constexpr string_id shortcut_rotate_construction_object = 712;
    constexpr string_id shortcut_toggle_underground_view = 713;
    constexpr string_id shortcut_toggle_hide_foreground_tracks = 714;
    constexpr string_id shortcut_toggle_hide_foreground_scenery = 715;
    constexpr string_id shortcut_toggle_height_marks_on_land = 716;
    constexpr string_id shortcut_toggle_height_marks_on_tracks = 717;
    constexpr string_id shortcut_toggle_dir_arrows_on_tracks = 718;
    constexpr string_id shortcut_adjust_land = 719;
    constexpr string_id shortcut_adjust_water = 720;
    constexpr string_id shortcut_plant_trees = 721;
    constexpr string_id shortcut_bulldoze_area = 722;
    constexpr string_id shortcut_build_tracks = 723;
    constexpr string_id shortcut_build_roads = 724;
    constexpr string_id shortcut_build_airports = 725;
    constexpr string_id shortcut_build_ship_ports = 726;
    constexpr string_id shortcut_build_new_vehicles = 727;
    constexpr string_id shortcut_show_vehicles_list = 728;
    constexpr string_id shortcut_show_stations_list = 729;
    constexpr string_id shortcut_show_towns_list = 730;
    constexpr string_id shortcut_show_industries_list = 731;
    constexpr string_id shortcut_show_map = 732;
    constexpr string_id shortcut_show_companies_list = 733;
    constexpr string_id shortcut_show_company_information = 734;
    constexpr string_id shortcut_show_finances = 735;
    constexpr string_id shortcut_show_announcements_list = 736;
    constexpr string_id shortcut_screenshot = 737;
    constexpr string_id shortcut_toggle_last_announcement = 738;
    constexpr string_id shortcut_send_message = 739;
    constexpr string_id shortcut_key_base = 740;

    constexpr string_id keyboard_shortcut_list_format = 996;
    constexpr string_id keyboard_shortcut_modifier_shift = 997;
    constexpr string_id keyboard_shortcut_modifier_ctrl = 998;
    constexpr string_id change_keyboard_shortcut = 999;
    constexpr string_id change_keyboard_shortcut_desc = 1000;
    constexpr string_id keyboard_shortcut_list_tip = 1001;

    constexpr string_id scroll_screen_edge = 1002;
    constexpr string_id scroll_screen_edge_tip = 1003;
    constexpr string_id customise_keys_tip = 1004;

    constexpr string_id forced_software_buffer_mixing = UNUSED_STR(1010);
    constexpr string_id forced_software_buffer_mixing_tip = UNUSED_STR(1011);

    constexpr string_id music_acknowledgements_btn = 1017;
    constexpr string_id music_acknowledgements_caption = 1018;
    constexpr string_id music_copyright = 1019;
    constexpr string_id locomotion_title = 1020;
    constexpr string_id locomotion_title_credit = 1021;
    constexpr string_id long_dusty_road = 1022;
    constexpr string_id long_dusty_road_credit = 1023;
    constexpr string_id flying_high = 1024;
    constexpr string_id flying_high_credit = 1025;
    constexpr string_id gettin_on_the_gas = 1026;
    constexpr string_id gettin_on_the_gas_credit = 1027;
    constexpr string_id jumpin_the_rails = 1028;
    constexpr string_id jumpin_the_rails_credit = 1029;
    constexpr string_id smooth_running = 1030;
    constexpr string_id smooth_running_credit = 1031;
    constexpr string_id traffic_jam = 1032;
    constexpr string_id traffic_jam_credit = 1033;
    constexpr string_id never_stop_til_you_get_there = 1034;
    constexpr string_id never_stop_til_you_get_there_credit = 1035;
    constexpr string_id soaring_away = 1036;
    constexpr string_id soaring_away_credit = 1037;
    constexpr string_id techno_torture = 1038;
    constexpr string_id techno_torture_credit = 1039;
    constexpr string_id everlasting_high_rise = 1040;
    constexpr string_id everlasting_high_rise_credit = 1041;
    constexpr string_id solace = 1042;
    constexpr string_id solace_credit = 1043;
    constexpr string_id chrysanthemum = 1044;
    constexpr string_id chrysanthemum_credit = 1045;
    constexpr string_id eugenia = 1046;
    constexpr string_id eugenia_credit = 1047;
    constexpr string_id the_ragtime_dance = 1048;
    constexpr string_id the_ragtime_dance_credit = 1049;
    constexpr string_id easy_winners = 1050;
    constexpr string_id easy_winners_credit = 1051;
    constexpr string_id setting_off = 1052;
    constexpr string_id setting_off_credit = 1053;
    constexpr string_id a_travellers_seranade = 1054;
    constexpr string_id a_travellers_seranade_credit = 1055;
    constexpr string_id latino_trip = 1056;
    constexpr string_id latino_trip_credit = 1057;
    constexpr string_id a_good_head_of_steam = 1058;
    constexpr string_id a_good_head_of_steam_credit = 1059;
    constexpr string_id hop_to_the_bop = 1060;
    constexpr string_id hop_to_the_bop_credit = 1061;
    constexpr string_id the_city_lights = 1062;
    constexpr string_id the_city_lights_credit = 1063;
    constexpr string_id steamin_down_town = 1064;
    constexpr string_id steamin_down_town_credit = 1065;
    constexpr string_id bright_expectations = 1066;
    constexpr string_id bright_expectations_credit = 1067;
    constexpr string_id mo_station = 1068;
    constexpr string_id mo_station_credit = 1069;
    constexpr string_id far_out = 1070;
    constexpr string_id far_out_credit = 1071;
    constexpr string_id running_on_time = 1072;
    constexpr string_id running_on_time_credit = 1073;
    constexpr string_id get_me_to_gladstone_bay = 1074;
    constexpr string_id get_me_to_gladstone_bay_credit = 1075;
    constexpr string_id chuggin_along = 1076;
    constexpr string_id chuggin_along_credit = 1077;
    constexpr string_id dont_lose_your_rag = 1078;
    constexpr string_id dont_lose_your_rag_credit = 1079;
    constexpr string_id sandy_track_blues = 1080;
    constexpr string_id sandy_track_blues_credit = 1081;
    constexpr string_id error_unable_to_load_saved_game = 1082;

    constexpr string_id loading = 1088;

    constexpr string_id white = 1090;
    constexpr string_id translucent = 1091;
    constexpr string_id construction_marker = 1092;
    constexpr string_id vehicles_min_scale = 1093;
    constexpr string_id station_names_min_scale = 1094;
    constexpr string_id full_scale = 1095;
    constexpr string_id half_scale = 1096;
    constexpr string_id quarter_scale = 1097;
    constexpr string_id eighth_scale = 1098;
    constexpr string_id vehicles_min_scale_tip = 1099;
    constexpr string_id station_names_min_scale_tip = 1100;

    constexpr string_id stringid_all_stations = 1128;
    constexpr string_id stringid_rail_stations = 1129;
    constexpr string_id stringid_road_stations = 1130;
    constexpr string_id stringid_airports = 1131;
    constexpr string_id stringid_ship_ports = 1132;
    constexpr string_id all_stations = 1133;
    constexpr string_id rail_stations = 1134;
    constexpr string_id road_stations = 1135;
    constexpr string_id airports = 1136;
    constexpr string_id ship_ports = 1137;

    constexpr string_id table_header_name = 1145;
    constexpr string_id table_header_name_desc = 1146;
    constexpr string_id table_header_monthly_profit = 1147;
    constexpr string_id table_header_monthly_profit_desc = 1148;
    constexpr string_id table_header_age = 1149;
    constexpr string_id table_header_age_desc = 1150;
    constexpr string_id table_header_reliability = 1151;
    constexpr string_id table_header_reliability_desc = 1152;

    constexpr string_id tooltip_sort_by_name = 1153;
    constexpr string_id tooltip_sort_by_profit = 1154;
    constexpr string_id tooltip_sort_by_age = 1155;
    constexpr string_id tooltip_sort_by_reliability = 1156;

    constexpr string_id tooltip_all_stations = 1200;
    constexpr string_id tooltip_rail_stations = 1201;
    constexpr string_id tooltip_road_stations = 1202;
    constexpr string_id tooltip_airports = 1203;
    constexpr string_id tooltip_ship_ports = 1204;

    constexpr string_id build_trains = 1240;
    constexpr string_id build_buses = 1241;
    constexpr string_id build_trucks = 1242;
    constexpr string_id build_trams = 1243;
    constexpr string_id build_aircraft = 1244;
    constexpr string_id build_ships = 1245;

    constexpr string_id buffer_1250 = 1250;

    constexpr string_id num_trains_singular = 1264;
    constexpr string_id num_buses_singular = 1265;
    constexpr string_id num_trucks_singular = 1266;
    constexpr string_id num_trams_singular = 1267;
    constexpr string_id num_aircrafts_singular = 1268;
    constexpr string_id num_ships_singular = 1269;
    constexpr string_id num_trains_plural = 1270;
    constexpr string_id num_buses_plural = 1271;
    constexpr string_id num_trucks_plural = 1272;
    constexpr string_id num_trams_plural = 1273;
    constexpr string_id num_aircrafts_plural = 1274;
    constexpr string_id num_ships_plural = 1275;

    constexpr string_id menu_sprite_stringid = 1287;
    constexpr string_id menu_nosprite_stringid = 1288;
    constexpr string_id menu_sprite_stringid_construction = 1289;

    constexpr string_id title_town_name = 1308;
    constexpr string_id prompt_type_new_town_name = 1309;
    constexpr string_id status_town_population = 1310;
    constexpr string_id error_cant_rename_town = 1311;

    constexpr string_id population_graph_people = 1357;
    constexpr string_id population_graph_year = 1358;
    constexpr string_id title_town = 1359;
    constexpr string_id title_town_population = 1360;
    constexpr string_id title_town_local_authority = 1361;

    constexpr string_id tooltip_town = 1386;
    constexpr string_id tooltip_population_graph = 1387;
    constexpr string_id tooltip_town_ratings_each_company = 1388;

    constexpr string_id demolish_this_town = 1392;
    constexpr string_id cant_remove_town = 1393;

    constexpr string_id town_size_1 = 1399;
    constexpr string_id town_size_2 = 1400;
    constexpr string_id town_size_3 = 1401;
    constexpr string_id town_size_4 = 1402;
    constexpr string_id town_size_5 = 1403;
    constexpr string_id town_size_6 = 1404;
    constexpr string_id town_size_7 = 1405;
    constexpr string_id town_size_8 = 1406;

    constexpr string_id expand_this_town = 1409;

    constexpr string_id station_cargo_rating_percent = 1423;

    constexpr string_id waiting_cargo_separator = 1430;
    constexpr string_id waiting = 1431;
    constexpr string_id nothing_waiting = 1432;
    constexpr string_id table_header_status = 1433;
    constexpr string_id table_header_status_desc = 1434;
    constexpr string_id table_header_total_waiting = 1435;
    constexpr string_id table_header_total_waiting_desc = 1436;
    constexpr string_id table_header_accepts = 1437;
    constexpr string_id table_header_accepts_desc = 1438;
    constexpr string_id tooltip_sort_by_station_status = 1439;
    constexpr string_id tooltip_sort_by_total_units_waiting = 1440;
    constexpr string_id tooltip_sort_by_cargo_accepted = 1441;
    constexpr string_id num_units = 1442;
    constexpr string_id unit_separator = 1443;

    constexpr string_id tooltip_select_company = 1465;

    constexpr string_id display_options = 1486;
    constexpr string_id sound_options = 1487;
    constexpr string_id music_options = 1488;
    constexpr string_id regional_options = 1489;
    constexpr string_id control_options = 1490;
    constexpr string_id miscellaneous_options = 1491;
    constexpr string_id options_title_display = 1492;
    constexpr string_id options_title_sound = 1493;
    constexpr string_id options_title_music = 1494;
    constexpr string_id options_title_regional = 1495;
    constexpr string_id options_title_controls = 1496;
    constexpr string_id options_title_miscellaneous = 1497;

    constexpr string_id use_preferred_currency_always = 1498;
    constexpr string_id use_preferred_currency_always_tip = 1499;
    constexpr string_id use_preferred_currency_new_game = 1500;
    constexpr string_id use_preferred_currency_new_game_tip = 1501;
    constexpr string_id current_game_currency_tip = 1502;
    constexpr string_id new_game_currency_tip = 1503;
    constexpr string_id current_game_currency = 1504;
    constexpr string_id new_game_currency = 1505;
    constexpr string_id preferred_currency_buffer = 1506;

    constexpr string_id forbid_trains = 1518;
    constexpr string_id forbid_buses = 1519;
    constexpr string_id forbid_trucks = 1520;
    constexpr string_id forbid_trams = 1521;
    constexpr string_id forbid_aircraft = 1522;
    constexpr string_id forbid_ships = 1523;

    constexpr string_id currently_playing = 1535;
    constexpr string_id music_controls_stop_tip = 1536;
    constexpr string_id music_controls_play_tip = 1537;
    constexpr string_id music_controls_next_tip = 1538;

    constexpr string_id play_only_music_from_current_era = 1539;
    constexpr string_id play_all_music = 1540;
    constexpr string_id play_custom_music_selection = 1541;

    constexpr string_id edit_music_selection = 1542;
    constexpr string_id edit_music_selection_tip = 1543;
    constexpr string_id music_selection_title = 1544;
    constexpr string_id music_selection_tooltip = 1545;
    constexpr string_id checkmark = 1546;
    constexpr string_id volume = 1547;
    constexpr string_id set_volume_tip = 1548;
    constexpr string_id menu_music_options = 1549;

    constexpr string_id owner_label = 1560;

    constexpr string_id title_multiplayer_toggle_tooltip = 1567;
    constexpr string_id single_player_mode = 1568;
    constexpr string_id two_player_mode_connected = 1569;
    constexpr string_id scenario_group_beginner = 1570;
    constexpr string_id scenario_group_easy = 1571;
    constexpr string_id scenario_group_medium = 1572;
    constexpr string_id scenario_group_challenging = 1573;
    constexpr string_id scenario_group_expert = 1574;

    constexpr string_id title_landscape_generation_options = 1585;
    constexpr string_id title_landscape_generation_land = 1586;
    constexpr string_id title_landscape_generation_forests = 1587;
    constexpr string_id title_landscape_generation_towns = 1588;
    constexpr string_id title_landscape_generation_industries = 1589;
    constexpr string_id tooltip_landscape_generation_options = 1590;
    constexpr string_id tooltip_landscape_generation_land = 1591;
    constexpr string_id tooltip_landscape_generation_forests = 1592;
    constexpr string_id tooltip_landscape_generation_towns = 1593;
    constexpr string_id tooltip_landscape_generation_industries = 1594;
    constexpr string_id music_none = 1595;
    constexpr string_id music_chuggin_along = 1596;
    constexpr string_id music_long_dusty_road = 1597;
    constexpr string_id music_flying_high = 1598;
    constexpr string_id music_gettin_on_the_gas = 1599;
    constexpr string_id music_jumpin_the_rails = 1600;
    constexpr string_id music_smooth_running = 1601;
    constexpr string_id music_traffic_jam = 1602;
    constexpr string_id music_never_stop_til_you_get_there = 1603;
    constexpr string_id music_soaring_away = 1604;
    constexpr string_id music_techno_torture = 1605;
    constexpr string_id music_everlasting_high_rise = 1606;
    constexpr string_id music_solace = 1607;
    constexpr string_id music_chrysanthemum = 1608;
    constexpr string_id music_eugenia = 1609;
    constexpr string_id music_the_ragtime_dance = 1610;
    constexpr string_id music_easy_winners = 1611;
    constexpr string_id music_setting_off = 1612;
    constexpr string_id music_a_travellers_seranade = 1613;
    constexpr string_id music_latino_trip = 1614;
    constexpr string_id music_a_good_head_of_steam = 1615;
    constexpr string_id music_hop_to_the_bop = 1616;
    constexpr string_id music_the_city_lights = 1617;
    constexpr string_id music_steamin_down_town = 1618;
    constexpr string_id music_bright_expectations = 1619;
    constexpr string_id music_mo_station = 1620;
    constexpr string_id music_far_out = 1621;
    constexpr string_id music_running_on_time = 1622;
    constexpr string_id music_get_me_to_gladstone_bay = 1623;
    constexpr string_id music_sandy_track_blues = 1624;
    constexpr string_id start_year_value = 1625;
    constexpr string_id start_year = 1626;
    constexpr string_id label_generate_random_landscape_when_game_starts = 1627;
    constexpr string_id tooltip_generate_random_landscape_when_game_starts = 1628;
    constexpr string_id button_generate_landscape = 1629;
    constexpr string_id tooltip_generate_random_landscape = 1630;
    constexpr string_id label_ok = 1631;
    constexpr string_id title_generate_new_landscape = 1632;
    constexpr string_id title_random_landscape_option = 1633;
    constexpr string_id prompt_confirm_generate_landscape = 1634;
    constexpr string_id prompt_confirm_random_landscape = 1635;
    constexpr string_id sea_level = 1636;
    constexpr string_id sea_level_units = 1637;
    constexpr string_id number_of_forests = 1638;
    constexpr string_id number_of_forests_value = 1639;
    constexpr string_id min_forest_radius = 1640;
    constexpr string_id min_forest_radius_blocks = 1641;
    constexpr string_id max_forest_radius = 1642;
    constexpr string_id max_forest_radius_blocks = 1643;
    constexpr string_id min_forest_density = 1644;
    constexpr string_id min_forest_density_percent = 1645;
    constexpr string_id max_forest_density = 1646;
    constexpr string_id max_forest_density_percent = 1647;
    constexpr string_id number_random_trees = 1648;
    constexpr string_id number_random_trees_value = 1649;
    constexpr string_id min_altitude_for_trees = 1650;
    constexpr string_id min_altitude_for_trees_height = 1651;
    constexpr string_id max_altitude_for_trees = 1652;
    constexpr string_id max_altitude_for_trees_height = 1653;
    constexpr string_id min_land_height = 1654;
    constexpr string_id min_land_height_units = 1655;
    constexpr string_id topography_style = 1656;
    constexpr string_id flat_land = 1657;
    constexpr string_id small_hills = 1658;
    constexpr string_id mountains = 1659;
    constexpr string_id half_mountains_half_hills = 1660;
    constexpr string_id half_mountains_half_flat = 1661;
    constexpr string_id hill_density = 1662;
    constexpr string_id hill_density_percent = 1663;
    constexpr string_id number_of_towns = 1664;
    constexpr string_id number_of_towns_value = 1665;
    constexpr string_id max_town_size = 1666;
    constexpr string_id industry_size_low = 1667;
    constexpr string_id industry_size_medium = 1668;
    constexpr string_id industry_size_high = 1669;
    constexpr string_id number_of_industries = 1670;

    constexpr string_id tooltip_scenario_options = 1674;
    constexpr string_id tooltip_scenario_challenge = 1675;
    constexpr string_id tooltip_company_options = 1676;
    constexpr string_id tooltip_financial_options = 1677;
    constexpr string_id title_scenario_options = 1678;
    constexpr string_id title_scenario_challenge = 1679;
    constexpr string_id title_company_options = 1680;
    constexpr string_id title_financial_options = 1681;
    constexpr string_id max_competing_companies = 1682;
    constexpr string_id max_competing_companies_value = 1683;
    constexpr string_id delay_before_competing_companies_start = 1684;
    constexpr string_id delay_before_competing_companies_start_months = 1685;
    constexpr string_id selection_of_competing_companies = 1686;
    constexpr string_id preferred_intelligence = 1687;
    constexpr string_id preferred_aggressiveness = 1688;
    constexpr string_id preferred_competitiveness = 1689;
    constexpr string_id preference_any = 1690;
    constexpr string_id preference_low = 1691;
    constexpr string_id preference_medium = 1692;
    constexpr string_id preference_high = 1693;
    constexpr string_id starting_loan = 1694;
    constexpr string_id starting_loan_value = 1695;
    constexpr string_id max_loan_size = 1696;
    constexpr string_id max_loan_size_value = 1697;
    constexpr string_id loan_interest_rate = 1698;
    constexpr string_id loan_interest_rate_value = 1699;
    constexpr string_id change = 1700;
    constexpr string_id scenario_name_stringid = 1701;
    constexpr string_id scenario_group = 1702;
    constexpr string_id scenario_details = 1703;
    constexpr string_id scenario_name_title = 1704;
    constexpr string_id enter_name_for_scenario = 1705;
    constexpr string_id scenario_details_title = 1706;
    constexpr string_id enter_description_of_this_scenario = 1707;
    constexpr string_id no_details_yet = 1708;
    constexpr string_id unnamed = 1709;

    constexpr string_id chat_send_message = 1716;
    constexpr string_id chat_title = 1717;
    constexpr string_id chat_instructions = 1718;
    constexpr string_id buffer_1719 = 1719;

    constexpr string_id land_distribution_everywhere = 1730;
    constexpr string_id land_distribution_nowhere = 1731;
    constexpr string_id land_distribution_far_from_water = 1732;
    constexpr string_id land_distribution_near_water = 1733;
    constexpr string_id land_distribution_on_mountains = 1734;
    constexpr string_id land_distribution_far_from_mountains = 1735;
    constexpr string_id land_distribution_in_small_random_areas = 1736;
    constexpr string_id land_distribution_in_large_random_areas = 1737;
    constexpr string_id land_distribution_around_cliffs = 1738;
    constexpr string_id create_hills_right_up_to_edge_of_map = 1739;
    constexpr string_id title_menu_scenario_editor = 1740;

    constexpr string_id menu_map = 1742;

    constexpr string_id toolbar_status_paused = 1800;

    constexpr string_id tooltip_speed_pause = 1817;
    constexpr string_id tooltip_speed_normal = 1818;
    constexpr string_id tooltip_speed_fast_forward = 1819;
    constexpr string_id tooltip_speed_extra_fast_forward = 1820;
    constexpr string_id randomly_generated_landscape = 1821;

    constexpr string_id local_authority_ratings_transport_companies = 1833;
    constexpr string_id town_rating_appalling = 1834;
    constexpr string_id town_rating_poor = 1835;
    constexpr string_id town_rating_average = 1836;
    constexpr string_id town_rating_good = 1837;
    constexpr string_id town_rating_excellent = 1838;
    constexpr string_id town_rating_company_percentage_rank = 1839;

    constexpr string_id and_be_the_top_company = 1848;
    constexpr string_id and_be_within_the_top_companies = 1849;
    constexpr string_id with_a_time_limit = 1850;
    constexpr string_id challenge_label = 1851;
    constexpr string_id challenge_value = 1852;
    constexpr string_id objective_achieve_a_certain_company_value = 1853;
    constexpr string_id objective_achieve_a_certain_monthly_profit_from_vehicles = 1854;
    constexpr string_id objective_achieve_a_certain_performance_index = 1855;
    constexpr string_id objective_deliver_a_certain_amount_of_cargo = 1856;
    constexpr string_id challenge_monetary_value = 1857;
    constexpr string_id challenge_performance_index = 1858;
    constexpr string_id challenge_delivered_cargo = 1859;
    constexpr string_id time_limit_years_value = 1860;

    constexpr string_id title_exit_game = 1869;

    constexpr string_id allow_industries_to_close_down_during_game = 1871;
    constexpr string_id allow_new_industries_to_start_up_during_game = 1872;

    constexpr string_id forbid_competing_companies_from_using = 1875;
    constexpr string_id forbid_player_companies_from_using = 1876;

    constexpr string_id tutorial_1 = 1879;
    constexpr string_id tutorial_2 = 1880;
    constexpr string_id tutorial_3 = 1881;

    constexpr string_id use_preferred_owner_name = 1919;
    constexpr string_id use_preferred_owner_name_tip = 1920;
    constexpr string_id wcolour2_preferred_owner_name = 1921;
    constexpr string_id preferred_owner_name = 1922;
    constexpr string_id enter_preferred_owner_name = 1923;

    constexpr string_id atari_inc_credits_btn = UNUSED_STR(1929);
    constexpr string_id licenced_to_atari_inc = 1930;
    constexpr string_id atari_inc_credits_caption = UNUSED_STR(1931);

    constexpr string_id title_menu_chat_tooltip = 1933;

    constexpr string_id the_other_player = 1934;

    // String ids 1943--1982 (some blank) were used in the Atari credits screen, but are now unused.

    constexpr string_id window_browse_input_caret = 2003;
    constexpr string_id window_browse_filename = 2004;
    constexpr string_id window_browse_folder = 2005;
    constexpr string_id window_browse_parent_folder_tooltip = 2006;
    constexpr string_id window_browse_company = 2007;
    constexpr string_id window_browse_date = 2008;
    constexpr string_id window_browse_challenge_progress = 2009;
    constexpr string_id window_browse_challenge_completed = 2010;
    constexpr string_id window_browse_challenge_failed = 2011;

    constexpr string_id error_invalid_filename = 2016;

    constexpr string_id buffer_2039 = 2039;
    constexpr string_id buffer_2040 = 2040;

    constexpr string_id buffer_2042 = 2042;

    constexpr string_id buffer_2045 = 2045;

    constexpr string_id export_plugin_objects = 2089;
    constexpr string_id export_plugin_objects_tip = 2090;

    constexpr string_id unit_mph = 2113;
    constexpr string_id unit_kmh = 2114;
    constexpr string_id unit_hour = 2115;
    constexpr string_id unit_hours = 2116;
    constexpr string_id unit_mins = 2117;
    constexpr string_id unit_min = 2118;
    constexpr string_id unit_secs = 2119;
    constexpr string_id unit_units = 2120;
    constexpr string_id unit_ft = 2121;
    constexpr string_id unit_m = 2122;
    constexpr string_id unit_hp = 2123;
    constexpr string_id unit_kW = 2124;

    constexpr string_id options_language = 2125;

    constexpr string_id disable_vehicle_breakdowns = 2126;

    constexpr string_id options_screen_mode = 2127;
    constexpr string_id options_mode_windowed = 2128;
    constexpr string_id options_mode_fullscreen = 2129;
    constexpr string_id options_mode_fullscreen_window = 2130;

    constexpr string_id default_audio_device_name = 2131;

    constexpr string_id window_scale_factor = 2132;
    constexpr string_id stepper_plus = 2133;
    constexpr string_id stepper_minus = 2134;
    constexpr string_id scale_formatted = 2135;

    constexpr string_id zoom_to_cursor = 2136;
    constexpr string_id zoom_to_cursor_tip = 2137;

    constexpr string_id play_title_music = 2138;
}
