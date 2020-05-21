#include "objectmgr.h"
#include "../interop/interop.hpp"
#include <vector>

using namespace openloco::interop;

namespace openloco::objectmgr
{
    loco_global<object_repository_item[64], 0x4FE0B8> object_repository;
    loco_global<interface_skin_object* [1], 0x0050C3D0> _interfaceObjects;
    loco_global<sound_object* [128], 0x0050C3D4> _soundObjects;
    loco_global<currency_object* [1], 0x0050C5D4> _currencyObjects;
    loco_global<steam_object* [32], 0x0050C5D8> _steamObjects;
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
    void load_index()
    {
        call(0x00470F3C);
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
    steam_object* get(size_t id)
    {
        return _steamObjects[id];
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
    road_station_object* get(size_t id)
    {
        return _roadStationObjects[id];
    }

    template<>
    vehicle_object* get(size_t id)
    {
        return _vehicleObjects[id];
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
        if (_industryObjects[id] != (industry_object*)-1)
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
    track_extra_object* get(size_t id)
    {
        return _trackExtraObjects[id];
    }

    template<>
    track_object* get(size_t id)
    {
        return _trackObjects[id];
    }

    template<>
    road_extra_object* get(size_t id)
    {
        return _roadExtraObjects[id];
    }

    template<>
    road_object* get(size_t id)
    {
        return _roadObjects[id];
    }

    template<>
    airport_object* get(size_t id)
    {
        return _airportObjects[id];
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

        entry._header = (header*)*ptr;
        *ptr += sizeof(header);

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
            *ptr += sizeof(header);
        }

        uint8_t* countB = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countB; n++)
        {
            //header* subh = (header*)ptr;
            *ptr += sizeof(header);
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
            if (entry._header->get_type() == type)
                list.emplace_back(std::pair<uint32_t, object_index_entry>(i, entry));
        }

        return list;
    }
}
