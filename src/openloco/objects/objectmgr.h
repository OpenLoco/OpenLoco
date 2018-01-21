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

    struct cargo_object;
}

namespace openloco::objectmgr
{
    void load_index();
    cargo_object * get_cargo_object(size_t id);
    size_t get_max_objects(object_type type);
}
