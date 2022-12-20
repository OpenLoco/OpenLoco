#pragma once

#include "Object.h"
#include "Ui/UiTypes.hpp"
#include <OpenLoco/Core/Span.hpp>
#include <optional>
#include <vector>

namespace OpenLoco
{
    class SawyerStreamWriter;

    namespace Gfx
    {
        struct RenderTarget;
    }

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

    /**
     * Represents an index into the entire loaded object array. Not an index for
     * a specific object type. DO NOT USE
     */
    using LoadedObjectIndex = size_t;
}

namespace OpenLoco::ObjectManager
{
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

    Object* getAny(const LoadedObjectHandle& handle);

    template<typename T>
    T* get()
    {
        static_assert(getMaxObjects(T::kObjectType) == 1);
        return reinterpret_cast<T*>(getAny({ T::kObjectType, 0 }));
    }

    template<typename T>
    T* get(size_t id)
    {
        static_assert(getMaxObjects(T::kObjectType) != 1);
        return reinterpret_cast<T*>(getAny({ T::kObjectType, static_cast<LoadedObjectId>(id) }));
    }

#pragma pack(push, 1)
    struct ObjectHeader2
    {
        uint32_t decodedFileSize;
    };

    struct ObjectHeader3
    {
        uint32_t numImages;      // 0x0
        uint8_t intelligence;    // 0x4 competitor stats
        uint8_t aggressiveness;  // 0x5
        uint8_t competitiveness; // 0x6
        uint8_t pad_07[0x5];
    };

#pragma pack(pop)

    struct LoadObjectsResult
    {
        bool success{};
        ObjectHeader problemObject;
    };

    struct DependentObjects
    {
        std::vector<ObjectHeader> required;
        std::vector<ObjectHeader> willLoad;
    };

    void freeTemporaryObject();
    bool loadTemporaryObject(ObjectHeader& header);
    Object* getTemporaryObject();
    bool isTemporaryObjectLoad();

    std::optional<LoadedObjectHandle> findObjectHandle(const ObjectHeader& header);
    void reloadAll();
    ObjectHeader& getHeader(const LoadedObjectHandle& handle);
    std::vector<ObjectHeader> getHeaders();

    LoadObjectsResult loadAll(stdx::span<ObjectHeader> objects);
    void writePackedObjects(SawyerStreamWriter& fs, const std::vector<ObjectHeader>& packedObjects);

    void unloadAll();
    // Only unloads the entry (clears entry for packing does not free)
    void unload(const LoadedObjectHandle& handle);
    // Unloads and frees the entry
    void unload(const ObjectHeader& header);
    bool load(const ObjectHeader& header);

    bool tryInstallObject(const ObjectHeader& object, stdx::span<const uint8_t> data);

    size_t getByteLength(const LoadedObjectHandle& handle);

    void drawGenericDescription(Gfx::RenderTarget& rt, Ui::Point& rowPosition, const uint16_t designed, const uint16_t obsolete);

    void updateYearly1();
    void updateYearly2();

    void sub_4748FA();

    void registerHooks();
}
