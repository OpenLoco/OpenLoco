#pragma once

#include "../Core/Optional.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

namespace OpenLoco
{
    namespace Gfx
    {
        struct drawpixelinfo_t;
        struct point_t;
    }

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
    struct SteamObject;
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

#pragma pack(push, 1)
    struct ObjectHeader
    {
    private:
        static constexpr char cFF = static_cast<char>(0xFF);

    public:
        uint32_t flags = 0xFFFFFFFF;
        char name[8] = { cFF, cFF, cFF, cFF, cFF, cFF, cFF, cFF };
        uint32_t checksum = 0xFFFFFFFF;

        std::string_view getName() const
        {
            return std::string_view(name, sizeof(name));
        }

        constexpr uint8_t getSourceGame() const
        {
            return (flags >> 6) & 0x3;
        }

        constexpr object_type getType() const
        {
            return static_cast<object_type>(flags & 0x3F);
        }

        constexpr bool isCustom() const
        {
            return getSourceGame() == 0;
        }

        bool isEmpty() const
        {
            auto ab = reinterpret_cast<const int64_t*>(this);
            return ab[0] == -1 && ab[1] == -1;
        }

        bool operator==(const ObjectHeader& rhs) const
        {
            return std::memcmp(this, &rhs, sizeof(ObjectHeader)) == 0;
        }
    };
    static_assert(sizeof(ObjectHeader) == 0x10);

    /**
     * Represents an index into the entire loaded object array. Not an index for
     * a specific object type.
     */
    using LoadedObjectIndex = size_t;
#pragma pack(pop)
}

namespace OpenLoco::ObjectManager
{
    void loadIndex();

    constexpr size_t getMaxObjects(object_type type)
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

    constexpr size_t maxObjects = 859;
    constexpr size_t maxObjectTypes = 34;

    template<typename T>
    T* get();

    template<typename T>
    T* get(size_t id);

    template<>
    object* get(size_t id);
    template<>
    interface_skin_object* get();
    template<>
    SteamObject* get(size_t id);
    template<>
    rock_object* get(size_t id);
    template<>
    cargo_object* get(size_t id);
    template<>
    train_signal_object* get(size_t id);
    template<>
    road_station_object* get(size_t id);
    template<>
    vehicle_object* get(size_t id);
    template<>
    tree_object* get(size_t id);
    template<>
    wall_object* get(size_t id);
    template<>
    building_object* get(size_t id);
    template<>
    industry_object* get(size_t id);
    template<>
    currency_object* get();
    template<>
    bridge_object* get();
    template<>
    train_station_object* get();
    template<>
    track_object* get(size_t id);
    template<>
    road_object* get(size_t id);
    template<>
    airport_object* get(size_t id);
    template<>
    dock_object* get(size_t id);
    template<>
    land_object* get(size_t id);
    template<>
    water_object* get();
    template<>
    region_object* get();
    template<>
    competitor_object* get(size_t id);
    template<>
    scenario_text_object* get();

#pragma pack(push, 1)
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
        ObjectHeader* _header;
        char* _filename;
        char* _name;

        static object_index_entry read(std::byte** ptr);
    };

    struct ObjIndexPair
    {
        int16_t index;
        object_index_entry object;
    };
#pragma pack(pop)

    uint32_t getNumInstalledObjects();
    std::vector<std::pair<uint32_t, object_index_entry>> getAvailableObjects(object_type type);
    void freeScenarioText();
    void getScenarioText(ObjectHeader& object);
    std::optional<LoadedObjectIndex> findIndex(const ObjectHeader& header);
    std::optional<LoadedObjectIndex> findIndex(const object_index_entry& object);
    void reloadAll();
    ObjIndexPair getActiveObject(object_type objectType, uint8_t* edi);
    ObjectHeader* getHeader(LoadedObjectIndex id);
    std::vector<ObjectHeader> getHeaders();

    void unload(LoadedObjectIndex index);

    size_t getByteLength(LoadedObjectIndex id);

    void drawGenericDescription(Gfx::drawpixelinfo_t& dpi, Gfx::point_t& rowPosition, const uint16_t designed, const uint16_t obsolete);
}
