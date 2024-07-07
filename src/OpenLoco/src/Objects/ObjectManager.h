#pragma once

#include "Object.h"
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <optional>
#include <span>
#include <vector>

namespace OpenLoco
{
    class SawyerStreamWriter;

    namespace Gfx
    {
        class DrawingContext;
    }

    struct Object;
    struct ObjectEntryExtended;
    struct CargoObject;
    struct InterfaceSkinObject;
    struct SoundObject;
    struct CurrencyObject;
    struct SteamObject;
    struct CliffEdgeObject;
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

    constexpr size_t kMaxObjects = 859;

    Object* getAny(const LoadedObjectHandle& handle);

    template<typename T>
    const T* get()
    {
        static_assert(getMaxObjects(T::kObjectType) == 1);
        return reinterpret_cast<T*>(getAny({ T::kObjectType, 0 }));
    }

    template<typename T>
    const T* get(size_t id)
    {
        static_assert(getMaxObjects(T::kObjectType) != 1);
        return reinterpret_cast<T*>(getAny({ T::kObjectType, static_cast<LoadedObjectId>(id) }));
    }

#pragma pack(push, 1)
    struct ObjectHeader2
    {
        uint32_t decodedFileSize;
    };
    static_assert(sizeof(ObjectHeader2) == 0x4);

    struct ObjectHeader3
    {
        uint32_t numImages;      // 0x0
        uint8_t intelligence;    // 0x4 competitor stats
        uint8_t aggressiveness;  // 0x5
        uint8_t competitiveness; // 0x6
        uint8_t vehicleSubType;  // 0x7
        uint8_t pad_08[0x4];
    };
    static_assert(sizeof(ObjectHeader3) == 0xC);

#pragma pack(pop)

    struct LoadObjectsResult
    {
        bool success{};
        std::vector<ObjectHeader> problemObjects;
    };

    struct DependentObjects
    {
        std::vector<ObjectHeader> required;
        std::vector<ObjectHeader> willLoad;
    };

    struct TempLoadMetaData
    {
        ObjectHeader2 fileSizeHeader;
        ObjectHeader3 displayData;
    };

    void freeTemporaryObject();
    std::optional<TempLoadMetaData> loadTemporaryObject(const ObjectHeader& header);
    Object* getTemporaryObject();
    bool isTemporaryObjectLoad();

    std::optional<LoadedObjectHandle> findObjectHandle(const ObjectHeader& header);
    // Calls findObjectHandle and if can't find performs a secondary check with slightly looser
    // definitions of what a matching custom header is (no checksum, partial flags)
    std::optional<LoadedObjectHandle> findObjectHandleFuzzy(const ObjectHeader& header);
    void reloadAll();
    ObjectHeader& getHeader(const LoadedObjectHandle& handle);
    std::vector<ObjectHeader> getHeaders();

    LoadObjectsResult loadAll(std::span<ObjectHeader> objects);
    void writePackedObjects(SawyerStreamWriter& fs, const std::vector<ObjectHeader>& packedObjects);

    void unloadAll();
    // Only unloads the entry (clears entry for packing does not free)
    void unload(const LoadedObjectHandle& handle);
    // Unloads and frees the entry
    void unload(const ObjectHeader& header);
    bool load(const ObjectHeader& header);

    bool tryInstallObject(const ObjectHeader& object, std::span<const std::byte> data);

    size_t getByteLength(const LoadedObjectHandle& handle);

    void drawGenericDescription(Gfx::DrawingContext& drawingCtx, Ui::Point& rowPosition, const uint16_t designed, const uint16_t obsolete);

    void updateDefaultLevelCrossingType();
    void updateYearly2();

    void sub_4748FA();

    void registerHooks();

    // Calls function with the handle (LoadedObjectHandle) of each loaded object
    template<typename Function>
    void forEachLoadedObject(Function&& func)
    {
        for (uint8_t objectTypeU = 0; objectTypeU < kMaxObjectTypes; ++objectTypeU)
        {
            const auto objectType = static_cast<ObjectType>(objectTypeU);
            for (LoadedObjectId i = 0U; i < getMaxObjects(objectType); ++i)
            {
                LoadedObjectHandle handle{ objectType, i };
                if (getAny(handle) != nullptr)
                {
                    func(handle);
                }
            }
        }
    }
}
