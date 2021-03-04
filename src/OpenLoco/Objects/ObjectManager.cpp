#include "ObjectManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Ptr.h"
#include <vector>

using namespace OpenLoco::Interop;

namespace OpenLoco::ObjectManager
{
#pragma pack(push, 1)
    struct ObjectEntry2 : public ObjectHeader
    {
        uint32_t dataSize;
    };
#if defined(__i386__) || defined(_M_IX86)
    static_assert(sizeof(ObjectEntry2) == 0x14);
#endif

    struct object_repository_item
    {
        object** objects;
        ObjectEntry2* object_entry_extendeds;
    };
#if defined(__i386__) || defined(_M_IX86)
    static_assert(sizeof(object_repository_item) == 8);
#endif
#pragma pack(pop)

    loco_global<ObjectEntry2[maxObjects], 0x1125A90> objectEntries;
    loco_global<object_repository_item[64], 0x4FE0B8> object_repository;
    loco_global<object* [maxObjects], 0x0050C3D0> _allObjects;
    loco_global<interface_skin_object* [1], 0x0050C3D0> _interfaceObjects;
    loco_global<sound_object* [128], 0x0050C3D4> _soundObjects;
    loco_global<currency_object* [1], 0x0050C5D4> _currencyObjects;
    loco_global<SteamObject* [32], 0x0050C5D8> _steamObjects;
    loco_global<rock_object* [8], 0x0050C658> _rockObjects;
    loco_global<water_object* [1], 0x0050C678> _waterObjects;
    loco_global<land_object* [32], 0x0050C67C> _landObjects;
    loco_global<town_names_object* [1], 0x0050C6FC> _townNamesObjects;
    loco_global<cargo_object* [32], 0x0050C700> _cargoObjects;
    loco_global<wall_object* [32], 0x0050C780> _wallObjects;
    loco_global<train_signal_object* [16], 0x0050C800> _trainSignalObjects;
    loco_global<level_crossing_object* [4], 0x0050C840> _levelCrossingObjects;
    loco_global<street_light_object* [1], 0x0050C850> _streetLightObjects;
    loco_global<tunnel_object* [16], 0x0050C854> _tunnelObjects;
    loco_global<bridge_object* [8], 0x0050C894> _bridgeObjects;
    loco_global<train_station_object* [16], 0x0050C8B4> _trainStationObjects;
    loco_global<track_extra_object* [8], 0x0050C8F4> _trackExtraObjects;
    loco_global<track_object* [8], 0x0050C914> _trackObjects;
    loco_global<road_station_object* [16], 0x0050C934> _roadStationObjects;
    loco_global<road_extra_object* [4], 0x0050C974> _roadExtraObjects;
    loco_global<road_object* [8], 0x0050C984> _roadObjects;
    loco_global<airport_object* [8], 0x0050C9A4> _airportObjects;
    loco_global<dock_object* [8], 0x0050C9C4> _dockObjects;
    loco_global<vehicle_object* [224], 0x0050C9E4> _vehicleObjects;
    loco_global<tree_object* [64], 0x0050CD64> _treeObjects;
    loco_global<snow_object* [1], 0x0050CE64> _snowObjects;
    loco_global<climate_object* [1], 0x0050CE68> _climateObjects;
    loco_global<hill_shapes_object* [1], 0x0050CE6C> _hillShapeObjects;
    loco_global<building_object* [128], 0x0050CE70> _buildingObjects;
    loco_global<scaffolding_object* [1], 0x0050D070> _scaffoldingObjects;
    loco_global<industry_object* [16], 0x0050D074> _industryObjects;
    loco_global<region_object* [1], 0x0050D0B4> _regionObjects;
    loco_global<competitor_object* [32], 0x0050D0B8> _competitorObjects;
    loco_global<scenario_text_object* [1], 0x0050D138> _scenarioTextObjects;

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
    interface_skin_object* get()
    {
        if (_interfaceObjects[0] == (void*)-1)
        {
            return nullptr;
        }

        return _interfaceObjects[0];
    }

    template<>
    sound_object* get(size_t id)
    {
        return _soundObjects[id];
    }

    template<>
    SteamObject* get(size_t id)
    {
        return _steamObjects[id];
    }

    template<>
    rock_object* get(size_t id)
    {
        if (_rockObjects[id] != reinterpret_cast<rock_object*>(-1))
            return _rockObjects[id];
        else
            return nullptr;
    }

    template<>
    cargo_object* get(size_t id)
    {
        if (_cargoObjects[id] != (cargo_object*)-1)
            return _cargoObjects[id];
        else
            return nullptr;
    }

    template<>
    train_signal_object* get(size_t id)
    {
        if (_trainSignalObjects[id] != reinterpret_cast<train_signal_object*>(-1))
            return _trainSignalObjects[id];
        else
            return nullptr;
    }

    template<>
    road_station_object* get(size_t id)
    {
        if (_roadStationObjects[id] != reinterpret_cast<road_station_object*>(-1))
            return _roadStationObjects[id];
        else
            return nullptr;
    }

    template<>
    vehicle_object* get(size_t id)
    {
        if (_vehicleObjects[id] != reinterpret_cast<vehicle_object*>(-1))
            return _vehicleObjects[id];
        else
            return nullptr;
    }

    template<>
    tree_object* get(size_t id)
    {
        if (_treeObjects[id] != reinterpret_cast<tree_object*>(-1))
            return _treeObjects[id];
        else
            return nullptr;
    }

    template<>
    wall_object* get(size_t id)
    {
        if (_wallObjects[id] != reinterpret_cast<wall_object*>(-1))
            return _wallObjects[id];
        else
            return nullptr;
    }

    template<>
    building_object* get(size_t id)
    {
        if (_buildingObjects[id] != reinterpret_cast<building_object*>(-1))
            return _buildingObjects[id];
        else
            return nullptr;
    }

    template<>
    industry_object* get(size_t id)
    {
        if (_industryObjects[id] != reinterpret_cast<industry_object*>(-1))
            return _industryObjects[id];
        else
            return nullptr;
    }

    template<>
    currency_object* get()
    {
        return _currencyObjects[0];
    }

    template<>
    bridge_object* get(size_t id)
    {
        if (_bridgeObjects[id] != reinterpret_cast<bridge_object*>(-1))
            return _bridgeObjects[id];
        else
            return nullptr;
    }

    template<>
    train_station_object* get(size_t id)
    {
        if (_trainStationObjects[id] != reinterpret_cast<train_station_object*>(-1))
            return _trainStationObjects[id];
        else
            return nullptr;
    }

    template<>
    track_extra_object* get(size_t id)
    {
        return _trackExtraObjects[id];
    }

    template<>
    track_object* get(size_t id)
    {
        if (_trackObjects[id] != reinterpret_cast<track_object*>(-1))
            return _trackObjects[id];
        else
            return nullptr;
    }

    template<>
    road_extra_object* get(size_t id)
    {
        return _roadExtraObjects[id];
    }

    template<>
    road_object* get(size_t id)
    {
        if (_roadObjects[id] != reinterpret_cast<road_object*>(-1))
            return _roadObjects[id];
        else
            return nullptr;
    }

    template<>
    airport_object* get(size_t id)
    {
        if (_airportObjects[id] != reinterpret_cast<airport_object*>(-1))
            return _airportObjects[id];
        else
            return nullptr;
    }

    template<>
    dock_object* get(size_t id)
    {
        if (_dockObjects[id] != reinterpret_cast<dock_object*>(-1))
            return _dockObjects[id];
        else
            return nullptr;
    }

    template<>
    land_object* get(size_t id)
    {
        if (_landObjects[id] != (land_object*)-1)
            return _landObjects[id];
        else
            return nullptr;
    }

    template<>
    water_object* get()
    {
        return _waterObjects[0];
    }

    template<>
    competitor_object* get(size_t id)
    {
        return _competitorObjects[id];
    }

    template<>
    scenario_text_object* get()
    {
        if (_scenarioTextObjects[0] != (scenario_text_object*)-1)
            return _scenarioTextObjects[0];
        else
            return nullptr;
    }

    template<>
    region_object* get()
    {
        if (_regionObjects[0] != reinterpret_cast<region_object*>(-1))
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
        regs.ebp = ToInt(&object);
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
                regs.esi = ToInt(obj);
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

    void drawGenericDescription(Gfx::drawpixelinfo_t& dpi, Gfx::point_t& rowPosition, const uint16_t designed, const uint16_t obsolete)
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
