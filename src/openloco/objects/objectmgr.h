#pragma once

#include <cstdio>

namespace openloco
{
    enum class object_type
    {
        interface_skin,
        sound,
        currency,
        steam,
        rock,
        water,
        land,
        town_names,
        cargo,
        wall,
        track_signal,
        level_crossing,
        street_light,
        tunnel,
        bridge,
        track_station,
        track_extra,
        track,
        road_station,
        road_extra,
        road,
        airport,
        dock,
        vehicle,
        tree,
        snow,
        climate,
        hill_shapes,
        building,
        scaffolding,
        industry,
        region,
        competitors,
        scenario_text,
    };

    struct object;
    struct object_entry_extended;
    struct cargo_object;
    struct interface_skin_object;
    struct sound_object;
    struct currency_object;
    struct steam_object;
    struct rock_object;
    struct water_object;
    struct land_object;
    struct town_names_object;
    struct wall_object; 
    struct train_signal_object;
    struct level_crossing_object;
    struct street_light_object;
    struct tunnel_object;
    struct bridge_object;
    struct train_station_object;
    struct track_extra_object;
    struct track_object;
    struct road_station_object;
    struct road_extra_object;
    struct road_object;
    struct airport_object;
    struct dock_object;
    struct vehicle_object;
    struct tree_object;
    struct snow_object;
    struct climate_object;
    struct hill_shapes_object;
    struct building_object;
    struct scaffolding_object;
    struct industry_object;
    struct region_object;
    struct competitors_object;
    struct scenario_text_object;

    struct object_repository_item
    {
        object * objects;
        uint32_t * object_entry_extendeds;
    };
}

namespace openloco::objectmgr
{
    void load_index();
    cargo_object* get_cargo_object(size_t id);
    size_t get_max_objects(object_type type);
    vehicle_object * get_vehicle_object(size_t id);
}
