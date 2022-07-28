#include "ObjectManager.h"
#include "../Audio/Audio.h"
#include "../Console.h"
#include "../Core/FileSystem.hpp"
#include "../Environment.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../OpenLoco.h"
#include "../S5/SawyerStream.h"
#include "../Ui.h"
#include "../Ui/ProgressBar.h"
#include "../Utility/Numeric.hpp"
#include "../Utility/Stream.hpp"
#include "AirportObject.h"
#include "BridgeObject.h"
#include "BuildingObject.h"
#include "CargoObject.h"
#include "ClimateObject.h"
#include "CompetitorObject.h"
#include "CurrencyObject.h"
#include "DockObject.h"
#include "HillShapesObject.h"
#include "IndustryObject.h"
#include "InterfaceSkinObject.h"
#include "LandObject.h"
#include "LevelCrossingObject.h"
#include "RegionObject.h"
#include "RoadExtraObject.h"
#include "RoadObject.h"
#include "RoadStationObject.h"
#include "RockObject.h"
#include "ScaffoldingObject.h"
#include "ScenarioTextObject.h"
#include "SnowObject.h"
#include "SoundObject.h"
#include "SteamObject.h"
#include "StreetLightObject.h"
#include "TownNamesObject.h"
#include "TrackExtraObject.h"
#include "TrackObject.h"
#include "TrainSignalObject.h"
#include "TrainStationObject.h"
#include "TreeObject.h"
#include "TunnelObject.h"
#include "VehicleObject.h"
#include "WallObject.h"
#include "WaterObject.h"
#include <iterator>
#include <vector>

using namespace OpenLoco::Interop;

namespace OpenLoco::ObjectManager
{
    constexpr auto objectChecksumMagic = 0xF369A75B;
#pragma pack(push, 1)
    struct ObjectEntry2 : public ObjectHeader
    {
        uint32_t dataSize;
        ObjectEntry2(ObjectHeader head, uint32_t size)
            : ObjectHeader(head)
            , dataSize(size)
        {
        }
    };
    static_assert(sizeof(ObjectEntry2) == 0x14);

    struct ObjectRepositoryItem
    {
        Object** objects;
        ObjectEntry2* object_entry_extendeds;
    };
    assert_struct_size(ObjectRepositoryItem, 8);
#pragma pack(pop)

    loco_global<ObjectRepositoryItem[maxObjectTypes], 0x4FE0B8> object_repository;

    loco_global<uint32_t, 0x0050D154> _totalNumImages;

    static loco_global<std::byte*, 0x0050D158> _50D158;
    static loco_global<std::byte[0x2002], 0x0112A17F> _112A17F;
    static loco_global<bool, 0x0050AEAD> _isFirstTime;
    static loco_global<bool, 0x0050D161> _isPartialLoaded;
    static loco_global<uint32_t, 0x009D9D52> _decodedSize;    // return of getScenarioText (badly named)
    static loco_global<uint32_t, 0x0112A168> _numImages;      // return of getScenarioText (badly named)
    static loco_global<uint8_t, 0x0112C211> _intelligence;    // return of getScenarioText (badly named)
    static loco_global<uint8_t, 0x0112C212> _aggressiveness;  // return of getScenarioText (badly named)
    static loco_global<uint8_t, 0x0112C213> _competitiveness; // return of getScenarioText (badly named)

    static ObjectRepositoryItem& getRepositoryItem(ObjectType type)
    {
        return object_repository[enumValue(type)];
    }

    ObjectHeader& getHeader(const LoadedObjectHandle& handle)
    {
        return getRepositoryItem(handle.type).object_entry_extendeds[handle.id];
    }

    Object* getAny(const LoadedObjectHandle& handle)
    {
        auto obj = getRepositoryItem(handle.type).objects[handle.id];
        if (obj == (void*)-1)
        {
            obj = nullptr;
        }
        return obj;
    }

    /*
    static void printHeader(header data)
    {
        printf("(%02X | %02X << 6) ", data.type & 0x3F, data.type >> 6);
        printf("%02X ", data.pad_01[0]);
        printf("%02X ", data.pad_01[1]);
        printf("%02X ", data.pad_01[2]);

        char name[8 + 1] = { 0 };
        memcpy(name, data.var_04, 8);
        printf("'%s', ", name);

        printf("%08X ", data.checksum);
    }
    */

    ObjectIndexEntry ObjectIndexEntry::read(std::byte** ptr)
    {
        ObjectIndexEntry entry{};

        entry._header = (ObjectHeader*)*ptr;
        *ptr += sizeof(ObjectHeader);

        entry._filename = (char*)*ptr;
        *ptr += strlen(entry._filename) + 1;

        // decoded_chunk_size
        // ObjectHeader2* h2 = (ObjectHeader2*)ptr;
        *ptr += sizeof(ObjectHeader2);

        entry._name = (char*)*ptr;
        *ptr += strlen(entry._name) + 1;

        // ObjectHeader3* h3 = (ObjectHeader3*)ptr;
        *ptr += sizeof(ObjectHeader3);

        uint8_t* countA = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countA; n++)
        {
            // header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        uint8_t* countB = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countB; n++)
        {
            // header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        return entry;
    }

    // 0x00471B95
    void freeScenarioText()
    {
        call(0x00471B95);
    }

    // 0x0047176D
    void getScenarioText(ObjectHeader& object)
    {
        registers regs;
        regs.ebp = X86Pointer(&object);
        call(0x0047176D, regs);
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectHandle> findIndex(const ObjectHeader& header)
    {
        if ((header.flags & 0xFF) != 0xFF)
        {
            auto objectType = header.getType();
            const auto& typedObjectList = getRepositoryItem(objectType);
            auto maxObjectsForType = getMaxObjects(objectType);
            for (LoadedObjectId i = 0; i < maxObjectsForType; i++)
            {
                auto obj = typedObjectList.objects[i];
                if (obj != nullptr && obj != reinterpret_cast<Object*>(-1))
                {
                    const auto& objHeader = typedObjectList.object_entry_extendeds[i];

                    if (header == objHeader)
                    {
                        return { LoadedObjectHandle{ objectType, i } };
                    }
                }
            }
        }
        return std::nullopt;
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectHandle> findIndex(const ObjectIndexEntry& object)
    {
        return findIndex(*object._header);
    }

    // 0x0047237D
    void reloadAll()
    {
        call(0x0047237D);
    }

    enum class ObjectProcedure
    {
        load,
        unload,
        validate,
        drawPreview,
    };

    static bool callObjectValidate(const ObjectType type, Object& obj)
    {
        switch (type)
        {
            case ObjectType::interfaceSkin:
                return reinterpret_cast<InterfaceSkinObject*>(&obj)->validate();
            case ObjectType::sound:
                return reinterpret_cast<SoundObject*>(&obj)->validate();
            case ObjectType::currency:
                return reinterpret_cast<CurrencyObject*>(&obj)->validate();
            case ObjectType::steam:
                return reinterpret_cast<SteamObject*>(&obj)->validate();
            case ObjectType::rock:
                return reinterpret_cast<RockObject*>(&obj)->validate();
            case ObjectType::water:
                return reinterpret_cast<WaterObject*>(&obj)->validate();
            case ObjectType::land:
                return reinterpret_cast<LandObject*>(&obj)->validate();
            case ObjectType::townNames:
                return reinterpret_cast<TownNamesObject*>(&obj)->validate();
            case ObjectType::cargo:
                return reinterpret_cast<CargoObject*>(&obj)->validate();
            case ObjectType::wall:
                return reinterpret_cast<WallObject*>(&obj)->validate();
            case ObjectType::trackSignal:
                return reinterpret_cast<TrainSignalObject*>(&obj)->validate();
            case ObjectType::levelCrossing:
                return reinterpret_cast<LevelCrossingObject*>(&obj)->validate();
            case ObjectType::streetLight:
                return reinterpret_cast<StreetLightObject*>(&obj)->validate();
            case ObjectType::tunnel:
                return reinterpret_cast<TunnelObject*>(&obj)->validate();
            case ObjectType::bridge:
                return reinterpret_cast<BridgeObject*>(&obj)->validate();
            case ObjectType::trackStation:
                return reinterpret_cast<TrainStationObject*>(&obj)->validate();
            case ObjectType::trackExtra:
                return reinterpret_cast<TrackExtraObject*>(&obj)->validate();
            case ObjectType::track:
                return reinterpret_cast<TrackObject*>(&obj)->validate();
            case ObjectType::roadStation:
                return reinterpret_cast<RoadStationObject*>(&obj)->validate();
            case ObjectType::roadExtra:
                return reinterpret_cast<RoadExtraObject*>(&obj)->validate();
            case ObjectType::road:
                return reinterpret_cast<RoadObject*>(&obj)->validate();
            case ObjectType::airport:
                return reinterpret_cast<AirportObject*>(&obj)->validate();
            case ObjectType::dock:
                return reinterpret_cast<DockObject*>(&obj)->validate();
            case ObjectType::vehicle:
                return reinterpret_cast<VehicleObject*>(&obj)->validate();
            case ObjectType::tree:
                return reinterpret_cast<TreeObject*>(&obj)->validate();
            case ObjectType::snow:
                return reinterpret_cast<SnowObject*>(&obj)->validate();
            case ObjectType::climate:
                return reinterpret_cast<ClimateObject*>(&obj)->validate();
            case ObjectType::hillShapes:
                return reinterpret_cast<HillShapesObject*>(&obj)->validate();
            case ObjectType::building:
                return reinterpret_cast<BuildingObject*>(&obj)->validate();
            case ObjectType::scaffolding:
                return reinterpret_cast<ScaffoldingObject*>(&obj)->validate();
            case ObjectType::industry:
                return reinterpret_cast<IndustryObject*>(&obj)->validate();
            case ObjectType::region:
                return reinterpret_cast<RegionObject*>(&obj)->validate();
            case ObjectType::competitor:
                return reinterpret_cast<CompetitorObject*>(&obj)->validate();
            case ObjectType::scenarioText:
                return reinterpret_cast<ScenarioTextObject*>(&obj)->validate();
            default:
                assert(false);
                return false;
        }
    }

    static void callObjectUnload(const ObjectType type, Object& obj)
    {
        switch (type)
        {
            case ObjectType::interfaceSkin:
                reinterpret_cast<InterfaceSkinObject*>(&obj)->unload();
                break;
            case ObjectType::sound:
                reinterpret_cast<SoundObject*>(&obj)->unload();
                break;
            case ObjectType::currency:
                reinterpret_cast<CurrencyObject*>(&obj)->unload();
                break;
            case ObjectType::steam:
                reinterpret_cast<SteamObject*>(&obj)->unload();
                break;
            case ObjectType::rock:
                reinterpret_cast<RockObject*>(&obj)->unload();
                break;
            case ObjectType::water:
                reinterpret_cast<WaterObject*>(&obj)->unload();
                break;
            case ObjectType::land:
                reinterpret_cast<LandObject*>(&obj)->unload();
                break;
            case ObjectType::townNames:
                reinterpret_cast<TownNamesObject*>(&obj)->unload();
                break;
            case ObjectType::cargo:
                reinterpret_cast<CargoObject*>(&obj)->unload();
                break;
            case ObjectType::wall:
                reinterpret_cast<WallObject*>(&obj)->unload();
                break;
            case ObjectType::trackSignal:
                reinterpret_cast<TrainSignalObject*>(&obj)->unload();
                break;
            case ObjectType::levelCrossing:
                reinterpret_cast<LevelCrossingObject*>(&obj)->unload();
                break;
            case ObjectType::streetLight:
                reinterpret_cast<StreetLightObject*>(&obj)->unload();
                break;
            case ObjectType::tunnel:
                reinterpret_cast<TunnelObject*>(&obj)->unload();
                break;
            case ObjectType::bridge:
                reinterpret_cast<BridgeObject*>(&obj)->unload();
                break;
            case ObjectType::trackStation:
                reinterpret_cast<TrainStationObject*>(&obj)->unload();
                break;
            case ObjectType::trackExtra:
                reinterpret_cast<TrackExtraObject*>(&obj)->unload();
                break;
            case ObjectType::track:
                reinterpret_cast<TrackObject*>(&obj)->unload();
                break;
            case ObjectType::roadStation:
                reinterpret_cast<RoadStationObject*>(&obj)->unload();
                break;
            case ObjectType::roadExtra:
                reinterpret_cast<RoadExtraObject*>(&obj)->unload();
                break;
            case ObjectType::road:
                reinterpret_cast<RoadObject*>(&obj)->unload();
                break;
            case ObjectType::airport:
                reinterpret_cast<AirportObject*>(&obj)->unload();
                break;
            case ObjectType::dock:
                reinterpret_cast<DockObject*>(&obj)->unload();
                break;
            case ObjectType::vehicle:
                reinterpret_cast<VehicleObject*>(&obj)->unload();
                break;
            case ObjectType::tree:
                reinterpret_cast<TreeObject*>(&obj)->unload();
                break;
            case ObjectType::snow:
                reinterpret_cast<SnowObject*>(&obj)->unload();
                break;
            case ObjectType::climate:
                reinterpret_cast<ClimateObject*>(&obj)->unload();
                break;
            case ObjectType::hillShapes:
                reinterpret_cast<HillShapesObject*>(&obj)->unload();
                break;
            case ObjectType::building:
                reinterpret_cast<BuildingObject*>(&obj)->unload();
                break;
            case ObjectType::scaffolding:
                reinterpret_cast<ScaffoldingObject*>(&obj)->unload();
                break;
            case ObjectType::industry:
                reinterpret_cast<IndustryObject*>(&obj)->unload();
                break;
            case ObjectType::region:
                reinterpret_cast<RegionObject*>(&obj)->unload();
                break;
            case ObjectType::competitor:
                reinterpret_cast<CompetitorObject*>(&obj)->unload();
                break;
            case ObjectType::scenarioText:
                reinterpret_cast<ScenarioTextObject*>(&obj)->unload();
                break;
            default:
                assert(false);
                break;
        }
    }

    static bool callObjectFunction(const ObjectType type, Object& obj, const ObjectProcedure proc)
    {
        switch (proc)
        {
            case ObjectProcedure::validate:
                return callObjectValidate(type, obj);
            case ObjectProcedure::unload:
                callObjectUnload(type, obj);
                return true;
            default:
                throw std::runtime_error("Don't call this function with load/drawPreview.");
        }
    }

    static void callObjectLoad(const LoadedObjectHandle& handle, Object& obj, stdx::span<std::byte> data)
    {
        switch (handle.type)
        {
            case ObjectType::interfaceSkin:
                reinterpret_cast<InterfaceSkinObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::sound:
                reinterpret_cast<SoundObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::currency:
                reinterpret_cast<CurrencyObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::steam:
                reinterpret_cast<SteamObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::rock:
                reinterpret_cast<RockObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::water:
                reinterpret_cast<WaterObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::land:
                reinterpret_cast<LandObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::townNames:
                reinterpret_cast<TownNamesObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::cargo:
                reinterpret_cast<CargoObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::wall:
                reinterpret_cast<WallObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::trackSignal:
                reinterpret_cast<TrainSignalObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::levelCrossing:
                reinterpret_cast<LevelCrossingObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::streetLight:
                reinterpret_cast<StreetLightObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::tunnel:
                reinterpret_cast<TunnelObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::bridge:
                reinterpret_cast<BridgeObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::trackStation:
                reinterpret_cast<TrainStationObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::trackExtra:
                reinterpret_cast<TrackExtraObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::track:
                reinterpret_cast<TrackObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::roadStation:
                reinterpret_cast<RoadStationObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::roadExtra:
                reinterpret_cast<RoadExtraObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::road:
                reinterpret_cast<RoadObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::airport:
                reinterpret_cast<AirportObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::dock:
                reinterpret_cast<DockObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::vehicle:
                reinterpret_cast<VehicleObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::tree:
                reinterpret_cast<TreeObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::snow:
                reinterpret_cast<SnowObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::climate:
                reinterpret_cast<ClimateObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::hillShapes:
                reinterpret_cast<HillShapesObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::building:
                reinterpret_cast<BuildingObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::scaffolding:
                reinterpret_cast<ScaffoldingObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::industry:
                reinterpret_cast<IndustryObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::region:
                reinterpret_cast<RegionObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::competitor:
                reinterpret_cast<CompetitorObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::scenarioText:
                reinterpret_cast<ScenarioTextObject*>(&obj)->load(handle, data);
                break;
            default:
                assert(false);
                break;
        }
    }

    static bool callObjectFunction(const LoadedObjectHandle& handle, ObjectProcedure proc)
    {
        auto* obj = getAny(handle);
        if (obj != nullptr)
        {
            return callObjectFunction(handle.type, *obj, proc);
        }

        throw std::runtime_error("Object not loaded at this index");
    }

    bool computeObjectChecksum(const ObjectHeader& object, stdx::span<const uint8_t> data);

    // 0x00471BC5
    static bool load(const ObjectHeader& header, LoadedObjectId id)
    {
        // somewhat duplicates isObjectInstalled
        const auto installedObjects = getAvailableObjects(header.getType());
        auto res = std::find_if(std::begin(installedObjects), std::end(installedObjects), [&header](auto& obj) { return *obj.second._header == header; });
        if (res == std::end(installedObjects))
        {
            // Object is not installed
            return false;
        }

        const auto& installedObject = res->second;
        const auto filePath = Environment::getPath(Environment::PathId::objects) / fs::u8path(installedObject._filename);

        SawyerStreamReader stream(filePath);
        ObjectHeader loadingHeader;
        stream.read(&loadingHeader, sizeof(loadingHeader));
        if (loadingHeader != header)
        {
            // Something wrong has happened and installed object does not match index
            // Vanilla continued to search for subsequent matching installed headers.
            return false;
        }

        // Vanilla would branch and perform more efficient readChunk if size was known from installedObject.ObjectHeader2
        auto data = stream.readChunk();

        if (!computeObjectChecksum(loadingHeader, data))
        {
            // Something wrong has happened and installed object checksum is broken
            return false;
        }

        // Copy the object into Loco freeable memory (required for when load loads the object)
        auto* object = reinterpret_cast<Object*>(malloc(data.size()));
        std::copy(std::begin(data), std::end(data), reinterpret_cast<uint8_t*>(object));

        if (!callObjectFunction(loadingHeader.getType(), *object, ObjectProcedure::validate))
        {
            free(object);
            object = nullptr;
            // Object failed validation
            return false;
        }

        if (_totalNumImages >= Gfx::G1ExpectedCount::kObjects + Gfx::G1ExpectedCount::kDisc)
        {
            free(object);
            object = nullptr;
            // Too many objects loaded and no free image space
            return false;
        }

        object_repository[enumValue(loadingHeader.getType())].objects[id] = object;
        auto& extendedHeader = object_repository[enumValue(loadingHeader.getType())].object_entry_extendeds[id];
        extendedHeader = ObjectEntry2{
            loadingHeader, data.size()
        };

        if (!*_isPartialLoaded)
        {
            callObjectLoad({ loadingHeader.getType(), id }, *object, stdx::span<std::byte>(reinterpret_cast<std::byte*>(object), data.size()));
        }

        return true;
    }

    static std::optional<LoadedObjectId> findFreeObjectId(const ObjectType type)
    {
        for (LoadedObjectId id = 0; id < getMaxObjects(type); ++id)
        {
            if (getAny({ type, id }) == nullptr)
            {
                return id;
            }
        }
        return std::nullopt;
    }

    // 0x00471FF8
    void unload(const ObjectHeader& header)
    {
        auto handle = findIndex(header);
        if (!handle)
        {
            return;
        }
        unload(*handle);
        free(object_repository[enumValue(handle->type)].objects[handle->id]);
        object_repository[enumValue(handle->type)].objects[handle->id] = reinterpret_cast<Object*>(-1);
    }

    // 0x00471BCE
    bool load(const ObjectHeader& header)
    {
        auto id = findFreeObjectId(header.getType());
        if (!id)
        {
            return false;
        }
        return load(header, *id);
    }

    static LoadedObjectId getObjectId(LoadedObjectIndex index)
    {
        size_t objectType = 0;
        while (objectType < maxObjectTypes)
        {
            auto count = getMaxObjects(static_cast<ObjectType>(objectType));
            if (index < count)
            {
                return static_cast<LoadedObjectId>(index);
            }
            index -= count;
            objectType++;
        }
        return NullObjectId;
    }

    LoadObjectsResult loadAll(stdx::span<ObjectHeader> objects)
    {
        LoadObjectsResult result;
        result.success = true;

        unloadAll();

        LoadedObjectIndex index = 0;
        for (const auto& header : objects)
        {
            auto id = getObjectId(index);
            if (!header.isEmpty() && !load(header, id))
            {
                result.success = false;
                result.problemObject = header;
                std::string str(header.getName());
                Console::error("Failed to load: %s", str.c_str());
                // Could break early here but we want to list all of the failed objects
            }
            index++;
        }
        if (result.success == false)
        {
            unloadAll();
        }
        return result;
    }

    // 0x00472754
    static uint32_t computeChecksum(stdx::span<const uint8_t> data, uint32_t seed)
    {
        auto checksum = seed;
        for (auto d : data)
        {
            checksum = Utility::rol(checksum ^ d, 11);
        }
        return checksum;
    }

    // 0x0047270B
    bool computeObjectChecksum(const ObjectHeader& object, stdx::span<const uint8_t> data)
    {
        // Compute the checksum of header and data

        // Annoyingly the header you need to only compute the first byte of the flags
        stdx::span<const uint8_t> headerFlag(reinterpret_cast<const uint8_t*>(&object), 1);
        auto checksum = computeChecksum(headerFlag, objectChecksumMagic);

        // And then the name
        stdx::span<const uint8_t> headerName(reinterpret_cast<const uint8_t*>(&object.name), sizeof(ObjectHeader::name));
        checksum = computeChecksum(headerName, checksum);

        // Finally compute the datas checksum
        checksum = computeChecksum(data, checksum);

        return checksum == object.checksum;
    }

    static bool partialLoad(const ObjectHeader& header, stdx::span<uint8_t> objectData)
    {
        auto type = header.getType();
        size_t index = 0;
        for (; index < getMaxObjects(type); ++index)
        {
            if (getRepositoryItem(type).objects[index] == reinterpret_cast<Object*>(-1))
            {
                break;
            }
        }
        // No slot found. Not possible except if unload all had not been called
        if (index >= getMaxObjects(type))
        {
            return false;
        }

        auto* obj = reinterpret_cast<Object*>(objectData.data());
        getRepositoryItem(type).objects[index] = obj;
        getRepositoryItem(type).object_entry_extendeds[index] = ObjectEntry2(header, objectData.size());
        return true;
    }

    static constexpr SawyerEncoding getBestEncodingForObjectType(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::competitor:
                return SawyerEncoding::uncompressed;
            default:
                return SawyerEncoding::runLengthSingle;
            case ObjectType::currency:
                return SawyerEncoding::runLengthMulti;
            case ObjectType::townNames:
            case ObjectType::scenarioText:
                return SawyerEncoding::rotate;
        }
    }

    // 0x00472633
    // 0x004722FF
    void writePackedObjects(SawyerStreamWriter& fs, const std::vector<ObjectHeader>& packedObjects)
    {
        // TODO at some point, change this to just pack the object file directly from
        //      disc rather than using the in-memory version. This then avoids having
        //      to unload the object temporarily to save the S5.
        for (const auto& header : packedObjects)
        {
            auto index = ObjectManager::findIndex(header);
            if (index)
            {
                // Unload the object so that the object data is restored to
                // its original file state
                ObjectManager::unload(*index);

                auto encodingType = getBestEncodingForObjectType(header.getType());
                auto obj = ObjectManager::getAny(*index);
                auto objSize = ObjectManager::getByteLength(*index);

                fs.write(header);
                fs.writeChunk(encodingType, obj, objSize);
            }
            else
            {
                throw std::runtime_error("Unable to pack object: object not loaded");
            }
        }
    }

    // 0x00472031
    void unloadAll()
    {
        call(0x00472031);
    }

    void unload(const LoadedObjectHandle& handle)
    {
        callObjectFunction(handle, ObjectProcedure::unload);
    }

    size_t getByteLength(const LoadedObjectHandle& handle)
    {
        return getRepositoryItem(handle.type).object_entry_extendeds[handle.id].dataSize;
    }

    template<typename TObject>
    static void addAllInUseHeadersOfType(std::vector<ObjectHeader>& entries)
    {
        if constexpr (getMaxObjects(TObject::kObjectType) == 1)
        {
            auto* obj = ObjectManager::get<TObject>();
            if (obj != nullptr)
            {
                auto entry = getHeader({ TObject::kObjectType, 0 });
                entries.push_back(entry);
            }
            else
            {
                // Insert empty headers for any unused objects (required for save compatibility)
                // TODO: Move this into the S5 code.
                entries.emplace_back();
            }
        }
        else
        {
            for (LoadedObjectId i = 0; i < getMaxObjects(TObject::kObjectType); ++i)
            {
                auto* obj = ObjectManager::get<TObject>(i);
                if (obj != nullptr)
                {
                    auto entry = getHeader({ TObject::kObjectType, i });
                    entries.push_back(entry);
                }
                else
                {
                    // Insert empty headers for any unused objects (required for save compatibility)
                    // TODO: Move this into the S5 code.
                    entries.emplace_back();
                }
            }
        }
    }

    template<typename... TObject>
    static void addAllInUseHeadersOfTypes(std::vector<ObjectHeader>& entries)
    {
        (addAllInUseHeadersOfType<TObject>(entries), ...);
    }

    std::vector<ObjectHeader> getHeaders()
    {
        std::vector<ObjectHeader> entries;
        entries.reserve(ObjectManager::maxObjects);

        addAllInUseHeadersOfTypes<InterfaceSkinObject, SoundObject, CurrencyObject, SteamObject, RockObject, WaterObject, LandObject, TownNamesObject, CargoObject, WallObject, TrainSignalObject, LevelCrossingObject, StreetLightObject, TunnelObject, BridgeObject, TrainStationObject, TrackExtraObject, TrackObject, RoadStationObject, RoadExtraObject, RoadObject, AirportObject, DockObject, VehicleObject, TreeObject, SnowObject, ClimateObject, HillShapesObject, BuildingObject, ScaffoldingObject, IndustryObject, RegionObject, CompetitorObject, ScenarioTextObject>(entries);

        return entries;
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static const uint8_t descriptionRowHeight = 10;

    void drawGenericDescription(Gfx::Context& context, Ui::Point& rowPosition, const uint16_t designed, const uint16_t obsolete)
    {
        if (designed != 0)
        {
            FormatArguments args{};
            args.push(designed);
            Gfx::drawStringLeft(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_designed, &args);
            rowPosition.y += descriptionRowHeight;
        }

        if (obsolete != 0xFFFF)
        {
            FormatArguments args{};
            args.push(obsolete);
            Gfx::drawStringLeft(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_obsolete, &args);
            rowPosition.y += descriptionRowHeight;
        }
    }

    // 0x004796A9
    void updateYearly1()
    {
        // set default levelCrossing
        call(0x004796A9);
    }

    // 0x004C3A9E
    void updateYearly2()
    {
        // update available vehicles/roads/airports/etc.
        call(0x004C3A9E);
    }
}
