#pragma once

#include "../compat.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

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
        competitor,
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
    struct competitor_object;
    struct scenario_text_object;

    struct object_repository_item
    {
        object* objects;
        uint32_t* object_entry_extendeds;
    };
}

namespace openloco::objectmgr
{
    void load_index();

    constexpr size_t get_max_objects(object_type type)
    {
        constexpr size_t counts[] = {
            1,   // interface,
            128, // sound,
            1,   // currency,
            32,  // steam,
            8,   // rock,
            1,   // water,
            32,  // surface,
            1,   // town_names,
            32,  // cargo,
            32,  // wall,
            16,  // train_signal,
            4,   // level_crossing,
            1,   // street_light,
            16,  // tunnel,
            8,   // bridge,
            16,  // train_station,
            8,   // track_extra,
            8,   // track,
            16,  // road_station,
            4,   // road_extra,
            8,   // road,
            8,   // airport,
            8,   // dock,
            224, // vehicle,
            64,  // tree,
            1,   // snow,
            1,   // climate,
            1,   // hill_shapes,
            128, // building,
            1,   // scaffolding,
            16,  // industry,
            1,   // region,
            32,  // competitor,
            1    // scenario_text,
        };
        return counts[(size_t)type];
    };

    template<typename T>
    T* get();

    template<typename T>
    T* get(size_t id);

    template<>
    interface_skin_object* get();
    template<>
    steam_object* get(size_t id);
    template<>
    cargo_object* get(size_t id);
    template<>
    road_station_object* get(size_t id);
    template<>
    vehicle_object* get(size_t id);
    template<>
    building_object* get(size_t id);
    template<>
    industry_object* get(size_t id);
    template<>
    currency_object* get();
    template<>
    track_object* get(size_t id);
    template<>
    road_object* get(size_t id);
    template<>
    airport_object* get(size_t id);
    template<>
    land_object* get(size_t id);
    template<>
    water_object* get();
    template<>
    competitor_object* get(size_t id);
    template<>
    scenario_text_object* get();

#pragma pack(push, 1)
    struct header
    {
        uint8_t type;
        uint8_t pad_01[3];
        uint8_t var_04[8];
        uint32_t checksum;

        constexpr object_type get_type()
        {
            return static_cast<object_type>(type & 0x3F);
        }
    };

    struct header2
    {
        uint8_t pad_00[0x04 - 0x00];
    };

    struct header3
    {
        uint32_t var_00;      // image count?
        uint8_t pad_04[0x08]; // competitor stats?
    };

    struct object_index_entry
    {
        header* _header;
        char* _filename;
        char* _name;

        static object_index_entry read(std::byte** ptr);
    };
#pragma pack(pop)

    uint32_t getNumInstalledObjects();
    std::vector<std::pair<uint32_t, object_index_entry>> getAvailableObjects(object_type type);
}
