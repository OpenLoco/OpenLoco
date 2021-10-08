#pragma once

#include "../Core/Optional.hpp"
#include "../Core/Span.hpp"
#include "../Ui/Types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

namespace OpenLoco
{
    class SawyerStreamWriter;

    namespace Gfx
    {
        struct Context;
    }

    enum class ObjectType
    {
        interfaceSkin,
        sound,
        currency,
        steam,
        rock,
        water,
        land,
        townNames,
        cargo,
        wall,
        trackSignal,
        levelCrossing,
        streetLight,
        tunnel,
        bridge,
        trackStation,
        trackExtra,
        track,
        roadStation,
        roadExtra,
        road,
        airport,
        dock,
        vehicle,
        tree,
        snow,
        climate,
        hillShapes,
        building,
        scaffolding,
        industry,
        region,
        competitor,
        scenarioText,
    };

    struct Object;
    struct ObjectEntryExtended;
    struct CargoObject;
    struct InterfaceSkinObject;
    struct SoundObject;
    struct CurrencyObject;
    struct SteamObject;
    struct RockObject;
    struct WaterObject;
    struct LandObject;
    struct TownNamesObject;
    struct WallObject;
    struct TrainSignalObject;
    struct LevelCrossingObject;
    struct StreetLightObject;
    struct TunnelObject;
    struct BridgeObject;
    struct TrainStationObject;
    struct TrackExtraObject;
    struct TrackObject;
    struct RoadStationObject;
    struct RoadExtraObject;
    struct RoadObject;
    struct AirportObject;
    struct DockObject;
    struct VehicleObject;
    struct TreeObject;
    struct SnowObject;
    struct ClimateObject;
    struct HillShapesObject;
    struct BuildingObject;
    struct ScaffoldingObject;
    struct IndustryObject;
    struct RegionObject;
    struct CompetitorObject;
    struct ScenarioTextObject;

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

        constexpr ObjectType getType() const
        {
            return static_cast<ObjectType>(flags & 0x3F);
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

    /**
     * Represents an index / ID of a specific object type.
     */
    using LoadedObjectId = size_t;
#pragma pack(pop)
}

namespace OpenLoco::ObjectManager
{
    void loadIndex();

    constexpr size_t getMaxObjects(ObjectType type)
    {
        // 0x004FE250
        constexpr size_t counts[] = {
            1,   // interface,
            128, // sound,
            1,   // currency,
            32,  // steam,
            8,   // rock,
            1,   // water,
            32,  // surface,
            1,   // townNames,
            32,  // cargo,
            32,  // wall,
            16,  // train_signal,
            4,   // levelCrossing,
            1,   // streetLight,
            16,  // tunnel,
            8,   // bridge,
            16,  // train_station,
            8,   // trackExtra,
            8,   // track,
            16,  // roadStation,
            4,   // roadExtra,
            8,   // road,
            8,   // airport,
            8,   // dock,
            224, // vehicle,
            64,  // tree,
            1,   // snow,
            1,   // climate,
            1,   // hillShapes,
            128, // building,
            1,   // scaffolding,
            16,  // industry,
            1,   // region,
            32,  // competitor,
            1    // scenarioText,
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
    Object* get(size_t id);
    template<>
    InterfaceSkinObject* get();
    template<>
    SteamObject* get(size_t id);
    template<>
    RockObject* get(size_t id);
    template<>
    CargoObject* get(size_t id);
    template<>
    TrainSignalObject* get(size_t id);
    template<>
    RoadStationObject* get(size_t id);
    template<>
    VehicleObject* get(size_t id);
    template<>
    TreeObject* get(size_t id);
    template<>
    WallObject* get(size_t id);
    template<>
    BuildingObject* get(size_t id);
    template<>
    IndustryObject* get(size_t id);
    template<>
    CurrencyObject* get();
    template<>
    BridgeObject* get();
    template<>
    TrainStationObject* get();
    template<>
    TrackObject* get(size_t id);
    template<>
    RoadObject* get(size_t id);
    template<>
    AirportObject* get(size_t id);
    template<>
    DockObject* get(size_t id);
    template<>
    LandObject* get(size_t id);
    template<>
    WaterObject* get();
    template<>
    RegionObject* get();
    template<>
    CompetitorObject* get(size_t id);
    template<>
    ScenarioTextObject* get();

#pragma pack(push, 1)
    struct ObjectHeader2
    {
        uint32_t var_00;
    };

    struct ObjectHeader3
    {
        uint32_t var_00;         // image count?
        uint8_t intelligence;    // 0x4 competitor stats
        uint8_t aggressiveness;  // 0x5
        uint8_t competitiveness; // 0x6
        uint8_t pad_07[0x5];
    };

    struct ObjectIndexEntry
    {
        ObjectHeader* _header;
        char* _filename;
        char* _name;

        static ObjectIndexEntry read(std::byte** ptr);
    };

    struct ObjIndexPair
    {
        int16_t index;
        ObjectIndexEntry object;
    };
#pragma pack(pop)

    struct LoadObjectsResult
    {
        bool success{};
        ObjectHeader problemObject;
    };

    uint32_t getNumInstalledObjects();
    std::vector<std::pair<uint32_t, ObjectIndexEntry>> getAvailableObjects(ObjectType type);
    void freeScenarioText();
    void getScenarioText(ObjectHeader& object);
    std::optional<LoadedObjectIndex> findIndex(const ObjectHeader& header);
    std::optional<LoadedObjectIndex> findIndex(const ObjectIndexEntry& object);
    void reloadAll();
    ObjIndexPair getActiveObject(ObjectType objectType, uint8_t* edi);
    ObjectHeader* getHeader(LoadedObjectIndex id);
    std::vector<ObjectHeader> getHeaders();

    LoadObjectsResult loadAll(stdx::span<ObjectHeader> objects);
    bool tryInstallObject(const ObjectHeader& object, stdx::span<const uint8_t> data);
    void writePackedObjects(SawyerStreamWriter& fs, const std::vector<ObjectHeader>& packedObjects);

    void unloadAll();
    void unload(LoadedObjectIndex index);

    size_t getByteLength(LoadedObjectIndex id);

    void drawGenericDescription(Gfx::Context& context, Ui::Point& rowPosition, const uint16_t designed, const uint16_t obsolete);
}
