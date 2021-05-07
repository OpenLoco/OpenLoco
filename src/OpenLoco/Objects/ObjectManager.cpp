#include "ObjectManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include <vector>

using namespace OpenLoco::Interop;

namespace OpenLoco::ObjectManager
{
#pragma pack(push, 1)
    struct ObjectEntry2 : public ObjectHeader
    {
        uint32_t dataSize;
    };
    static_assert(sizeof(ObjectEntry2) == 0x14);

    struct object_repository_item
    {
        object** objects;
        ObjectEntry2* object_entry_extendeds;
    };
    static_assert(sizeof(object_repository_item) == 8);
#pragma pack(pop)

    loco_global<ObjectEntry2[maxObjects], 0x1125A90> objectEntries;
    loco_global<object_repository_item[64], 0x4FE0B8> object_repository;
    loco_global<object* [maxObjects], 0x0050C3D0> _allObjects;
    loco_global<InterfaceSkinObject* [1], 0x0050C3D0> _interfaceObjects;
    loco_global<SoundObject* [128], 0x0050C3D4> _soundObjects;
    loco_global<CurrencyObject* [1], 0x0050C5D4> _currencyObjects;
    loco_global<SteamObject* [32], 0x0050C5D8> _steamObjects;
    loco_global<RockObject* [8], 0x0050C658> _rockObjects;
    loco_global<WaterObject* [1], 0x0050C678> _waterObjects;
    loco_global<LandObject* [32], 0x0050C67C> _landObjects;
    loco_global<town_names_object* [1], 0x0050C6FC> _townNamesObjects;
    loco_global<CargoObject* [32], 0x0050C700> _cargoObjects;
    loco_global<WallObject* [32], 0x0050C780> _wallObjects;
    loco_global<TrainSignalObject* [16], 0x0050C800> _trainSignalObjects;
    loco_global<LevelCrossingObject* [4], 0x0050C840> _levelCrossingObjects;
    loco_global<StreetLightObject* [1], 0x0050C850> _streetLightObjects;
    loco_global<TunnelObject* [16], 0x0050C854> _tunnelObjects;
    loco_global<BridgeObject* [8], 0x0050C894> _bridgeObjects;
    loco_global<TrainStationObject* [16], 0x0050C8B4> _trainStationObjects;
    loco_global<TrackExtraObject* [8], 0x0050C8F4> _trackExtraObjects;
    loco_global<TrackObject* [8], 0x0050C914> _trackObjects;
    loco_global<RoadStationObject* [16], 0x0050C934> _roadStationObjects;
    loco_global<RoadExtraObject* [4], 0x0050C974> _roadExtraObjects;
    loco_global<RoadObject* [8], 0x0050C984> _roadObjects;
    loco_global<AirportObject* [8], 0x0050C9A4> _airportObjects;
    loco_global<DockObject* [8], 0x0050C9C4> _dockObjects;
    loco_global<VehicleObject* [224], 0x0050C9E4> _vehicleObjects;
    loco_global<TreeObject* [64], 0x0050CD64> _treeObjects;
    loco_global<SnowObject* [1], 0x0050CE64> _snowObjects;
    loco_global<ClimateObject* [1], 0x0050CE68> _climateObjects;
    loco_global<HillShapesObject* [1], 0x0050CE6C> _hillShapeObjects;
    loco_global<BuildingObject* [128], 0x0050CE70> _buildingObjects;
    loco_global<ScaffoldingObject* [1], 0x0050D070> _scaffoldingObjects;
    loco_global<IndustryObject* [16], 0x0050D074> _industryObjects;
    loco_global<RegionObject* [1], 0x0050D0B4> _regionObjects;
    loco_global<CompetitorObject* [32], 0x0050D0B8> _competitorObjects;
    loco_global<ScenarioTextObject* [1], 0x0050D138> _scenarioTextObjects;

    // 0x00470F3C
    void loadIndex()
    {
        call(0x00470F3C);
    }

    ObjectHeader* getHeader(LoadedObjectIndex id)
    {
        return &objectEntries[id];
    }

    template<>
    object* get(size_t id)
    {
        auto obj = _allObjects[id];
        if (obj == (void*)-1)
        {
            obj = nullptr;
        }
        return obj;
    }

    template<>
    InterfaceSkinObject* get()
    {
        if (_interfaceObjects[0] == (void*)-1)
        {
            return nullptr;
        }

        return _interfaceObjects[0];
    }

    template<>
    SoundObject* get(size_t id)
    {
        return _soundObjects[id];
    }

    template<>
    SteamObject* get(size_t id)
    {
        return _steamObjects[id];
    }

    template<>
    RockObject* get(size_t id)
    {
        if (_rockObjects[id] != reinterpret_cast<RockObject*>(-1))
            return _rockObjects[id];
        else
            return nullptr;
    }

    template<>
    CargoObject* get(size_t id)
    {
        if (_cargoObjects[id] != (CargoObject*)-1)
            return _cargoObjects[id];
        else
            return nullptr;
    }

    template<>
    TrainSignalObject* get(size_t id)
    {
        if (_trainSignalObjects[id] != reinterpret_cast<TrainSignalObject*>(-1))
            return _trainSignalObjects[id];
        else
            return nullptr;
    }

    template<>
    RoadStationObject* get(size_t id)
    {
        if (_roadStationObjects[id] != reinterpret_cast<RoadStationObject*>(-1))
            return _roadStationObjects[id];
        else
            return nullptr;
    }

    template<>
    VehicleObject* get(size_t id)
    {
        if (_vehicleObjects[id] != reinterpret_cast<VehicleObject*>(-1))
            return _vehicleObjects[id];
        else
            return nullptr;
    }

    template<>
    TreeObject* get(size_t id)
    {
        if (_treeObjects[id] != reinterpret_cast<TreeObject*>(-1))
            return _treeObjects[id];
        else
            return nullptr;
    }

    template<>
    WallObject* get(size_t id)
    {
        if (_wallObjects[id] != reinterpret_cast<WallObject*>(-1))
            return _wallObjects[id];
        else
            return nullptr;
    }

    template<>
    BuildingObject* get(size_t id)
    {
        if (_buildingObjects[id] != reinterpret_cast<BuildingObject*>(-1))
            return _buildingObjects[id];
        else
            return nullptr;
    }

    template<>
    IndustryObject* get(size_t id)
    {
        if (_industryObjects[id] != reinterpret_cast<IndustryObject*>(-1))
            return _industryObjects[id];
        else
            return nullptr;
    }

    template<>
    CurrencyObject* get()
    {
        return _currencyObjects[0];
    }

    template<>
    BridgeObject* get(size_t id)
    {
        if (_bridgeObjects[id] != reinterpret_cast<BridgeObject*>(-1))
            return _bridgeObjects[id];
        else
            return nullptr;
    }

    template<>
    TrainStationObject* get(size_t id)
    {
        if (_trainStationObjects[id] != reinterpret_cast<TrainStationObject*>(-1))
            return _trainStationObjects[id];
        else
            return nullptr;
    }

    template<>
    TrackExtraObject* get(size_t id)
    {
        return _trackExtraObjects[id];
    }

    template<>
    TrackObject* get(size_t id)
    {
        if (_trackObjects[id] != reinterpret_cast<TrackObject*>(-1))
            return _trackObjects[id];
        else
            return nullptr;
    }

    template<>
    RoadExtraObject* get(size_t id)
    {
        return _roadExtraObjects[id];
    }

    template<>
    RoadObject* get(size_t id)
    {
        if (_roadObjects[id] != reinterpret_cast<RoadObject*>(-1))
            return _roadObjects[id];
        else
            return nullptr;
    }

    template<>
    AirportObject* get(size_t id)
    {
        if (_airportObjects[id] != reinterpret_cast<AirportObject*>(-1))
            return _airportObjects[id];
        else
            return nullptr;
    }

    template<>
    DockObject* get(size_t id)
    {
        if (_dockObjects[id] != reinterpret_cast<DockObject*>(-1))
            return _dockObjects[id];
        else
            return nullptr;
    }

    template<>
    LandObject* get(size_t id)
    {
        if (_landObjects[id] != (LandObject*)-1)
            return _landObjects[id];
        else
            return nullptr;
    }

    template<>
    WaterObject* get()
    {
        return _waterObjects[0];
    }

    template<>
    CompetitorObject* get(size_t id)
    {
        return _competitorObjects[id];
    }

    template<>
    ClimateObject* get()
    {
        if (_climateObjects[0] == (void*)-1)
        {
            return nullptr;
        }

        return _climateObjects[0];
    }

    template<>
    ScenarioTextObject* get()
    {
        if (_scenarioTextObjects[0] != (ScenarioTextObject*)-1)
            return _scenarioTextObjects[0];
        else
            return nullptr;
    }

    template<>
    RegionObject* get()
    {
        if (_regionObjects[0] != reinterpret_cast<RegionObject*>(-1))
            return _regionObjects[0];
        else
            return nullptr;
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

    object_index_entry object_index_entry::read(std::byte** ptr)
    {
        object_index_entry entry{};

        entry._header = (ObjectHeader*)*ptr;
        *ptr += sizeof(ObjectHeader);

        entry._filename = (char*)*ptr;
        *ptr += strlen(entry._filename) + 1;

        // decoded_chunk_size
        //header2* h2 = (header2*)ptr;
        *ptr += sizeof(header2);

        entry._name = (char*)*ptr;
        *ptr += strlen(entry._name) + 1;

        //header3* h3 = (header3*)ptr;
        *ptr += sizeof(header3);

        uint8_t* countA = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countA; n++)
        {
            //header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        uint8_t* countB = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countB; n++)
        {
            //header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        return entry;
    }

    static loco_global<std::byte*, 0x0050D13C> _installedObjectList;
    static loco_global<uint32_t, 0x0112A110> _installedObjectCount;

    uint32_t getNumInstalledObjects()
    {
        return *_installedObjectCount;
    }

    std::vector<std::pair<uint32_t, object_index_entry>> getAvailableObjects(object_type type)
    {
        auto ptr = (std::byte*)_installedObjectList;
        std::vector<std::pair<uint32_t, object_index_entry>> list;

        for (uint32_t i = 0; i < _installedObjectCount; i++)
        {
            auto entry = object_index_entry::read(&ptr);
            if (entry._header->getType() == type)
                list.emplace_back(std::pair<uint32_t, object_index_entry>(i, entry));
        }

        return list;
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
        regs.ebp = reinterpret_cast<int32_t>(&object);
        call(0x0047176D, regs);
    }

    static LoadedObjectIndex getLoadedObjectIndex(object_type objectType, size_t index)
    {
        auto baseIndex = 0;
        size_t type = 0;
        while (type != static_cast<size_t>(objectType))
        {
            auto maxObjectsForType = getMaxObjects(static_cast<object_type>(type));
            baseIndex += maxObjectsForType;
            type++;
        }
        return baseIndex + index;
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectIndex> findIndex(const ObjectHeader& header)
    {
        if ((header.flags & 0xFF) != 0xFF)
        {
            auto objectType = header.getType();
            const auto& typedObjectList = object_repository[static_cast<size_t>(objectType)];
            auto maxObjectsForType = getMaxObjects(objectType);
            for (size_t i = 0; i < maxObjectsForType; i++)
            {
                auto obj = typedObjectList.objects[i];
                if (obj != nullptr && obj != reinterpret_cast<object*>(-1))
                {
                    const auto& objHeader = typedObjectList.object_entry_extendeds[i];
                    if (objHeader.isCustom())
                    {
                        if (header == objHeader)
                        {
                            return getLoadedObjectIndex(objectType, i);
                        }
                    }
                    else
                    {
                        if (header.getType() == objHeader.getType() && header.getName() == objHeader.getName())
                        {
                            return getLoadedObjectIndex(objectType, i);
                        }
                    }
                }
            }
        }
        return {};
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectIndex> findIndex(const object_index_entry& object)
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

    static bool callObjectFunction(LoadedObjectIndex index, ObjectProcedure proc)
    {
        auto objectHeader = getHeader(index);
        if (objectHeader != nullptr)
        {
            auto obj = get<object>(index);
            if (obj != nullptr)
            {
                auto objectType = objectHeader->getType();
                auto objectProcTable = (const uintptr_t*)0x004FE1C8;
                auto objectProc = objectProcTable[static_cast<size_t>(objectType)];

                registers regs;
                regs.al = static_cast<uint8_t>(proc);
                regs.esi = reinterpret_cast<uint32_t>(obj);
                return (call(objectProc, regs) & X86_FLAG_CARRY) != 0;
            }
        }
        throw std::runtime_error("Object not loaded at this index");
    }

    void unload(LoadedObjectIndex index)
    {
        callObjectFunction(index, ObjectProcedure::unload);
    }

    size_t getByteLength(LoadedObjectIndex id)
    {
        return objectEntries[id].dataSize;
    }

    std::vector<ObjectHeader> getHeaders()
    {
        std::vector<ObjectHeader> entries;
        entries.reserve(ObjectManager::maxObjects);

        for (size_t i = 0; i < ObjectManager::maxObjects; i++)
        {
            auto obj = ObjectManager::get<object>(i);
            if (obj != nullptr)
            {
                auto entry = getHeader(i);
                entries.push_back(*entry);
            }
            else
            {
                entries.emplace_back();
            }
        }

        return entries;
    }

    // 0x00472AFE
    ObjIndexPair getActiveObject(object_type objectType, uint8_t* edi)
    {
        const auto objects = getAvailableObjects(objectType);

        for (auto [index, object] : objects)
        {
            if (edi[index] & (1 << 0))
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        return { -1, object_index_entry{} };
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static const uint8_t descriptionRowHeight = 10;

    void drawGenericDescription(Gfx::Context& dpi, Gfx::point_t& rowPosition, const uint16_t designed, const uint16_t obsolete)
    {
        if (designed != 0)
        {
            FormatArguments args{};
            args.push(designed);
            Gfx::drawString_494B3F(dpi, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_designed, &args);
            rowPosition.y += descriptionRowHeight;
        }

        if (obsolete != 0xFFFF)
        {
            FormatArguments args{};
            args.push(obsolete);
            Gfx::drawString_494B3F(dpi, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_obsolete, &args);
            rowPosition.y += descriptionRowHeight;
        }
    }
}
