#include "objectmgr.h"
#include "../industry.h"
#include "../interop/interop.hpp"
#include "../things/vehicle.h"
#include <vector>

using namespace openloco;
using namespace openloco::interop;

objectmanager openloco::g_objectmgr;

static loco_global<object_repository_item[64], 0x4FE0B8> object_repository;
static loco_global<interface_skin_object * [1], 0x0050C3D0> _interfaceObjects;
static loco_global<sound_object * [128], 0x0050C3D4> _soundObjects;
static loco_global<currency_object * [1], 0x0050C5D4> _currencyObjects;
static loco_global<steam_object * [32], 0x0050C5D8> _steamObjects;
static loco_global<rock_object * [8], 0x0050C658> _rockObjects;
static loco_global<water_object * [1], 0x0050C678> _waterObjects;
static loco_global<land_object * [32], 0x0050C67C> _landObjects;
static loco_global<town_names_object * [1], 0x0050C6FC> _townNamesObjects;
static loco_global<cargo_object * [32], 0x0050C700> _cargoObjects;
static loco_global<wall_object * [32], 0x0050C780> _wallObjects;
static loco_global<train_signal_object * [16], 0x0050C800> _trainSignalObjects;
static loco_global<level_crossing_object * [4], 0x0050C840> _levelCrossingObjects;
static loco_global<street_light_object * [1], 0x0050C850> _streetLightObjects;
static loco_global<tunnel_object * [16], 0x0050C854> _tunnelObjects;
static loco_global<bridge_object * [8], 0x0050C894> _bridgeObjects;
static loco_global<train_station_object * [16], 0x0050C8B4> _trainStationObjects;
static loco_global<track_extra_object * [8], 0x0050C8F4> _trackExtraObjects;
static loco_global<track_object * [8], 0x0050C914> _trackObjects;
static loco_global<road_station_object * [16], 0x0050C934> _roadStationObjects;
static loco_global<road_extra_object * [4], 0x0050C974> _roadExtraObjects;
static loco_global<road_object * [8], 0x0050C984> _roadObjects;
static loco_global<airport_object * [8], 0x0050C9A4> _airportObjects;
static loco_global<dock_object * [8], 0x0050C9C4> _dockObjects;
static loco_global<vehicle_object * [224], 0x0050C9E4> _vehicleObjects;
static loco_global<tree_object * [64], 0x0050CD64> _treeObjects;
static loco_global<snow_object * [1], 0x0050CE64> _snowObjects;
static loco_global<climate_object * [1], 0x0050CE68> _climateObjects;
static loco_global<hill_shapes_object * [1], 0x0050CE6C> _hillShapeObjects;
static loco_global<building_object * [128], 0x0050CE70> _buildingObjects;
static loco_global<scaffolding_object * [1], 0x0050D070> _scaffoldingObjects;
static loco_global<industry_object * [16], 0x0050D074> _industryObjects;
static loco_global<region_object * [1], 0x0050D0B4> _regionObjects;
static loco_global<competitors_object * [32], 0x0050D0B8> _competitorsObjects;
static loco_global<scenario_text_object * [1], 0x0050D138> _scenarioTextObjects;

// 0x00470F3C
void objectmanager::load_index()
{
    call(0x00470F3C);
}

template<>
interface_skin_object* objectmanager::get()
{
    if (_interfaceObjects[0] == (void*)-1)
    {
        return nullptr;
    }

    return _interfaceObjects[0];
}

template<>
sound_object* objectmanager::get(size_t id)
{
    return _soundObjects[id];
}

template<>
steam_object* objectmanager::get(size_t id)
{
    return _steamObjects[id];
}

template<>
cargo_object* objectmanager::get(size_t id)
{
    return _cargoObjects[id];
}

template<>
road_station_object* objectmanager::get(size_t id)
{
    return _roadStationObjects[id];
}

template<>
vehicle_object* objectmanager::get(size_t id)
{
    return _vehicleObjects[id];
}

template<>
building_object* objectmanager::get(size_t id)
{
    return _buildingObjects[id];
}

template<>
industry_object* objectmanager::get(size_t id)
{
    return _industryObjects[id];
}

template<>
currency_object* objectmanager::get()
{
    return _currencyObjects[0];
}

industry_object* objectmanager::get(const industry& i)
{
    return get<industry_object>(i.object_id);
}

vehicle_object* objectmanager::get(const vehicle& v)
{
    return get<vehicle_object>(v.object_id);
}

size_t objectmanager::get_max_objects(object_type type)
{
    static size_t counts[] = {
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
        32,  // competitors,
        1    // scenario_text,
    };
    return counts[(size_t)type];
};

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

objectmanager::object_index_entry objectmanager::object_index_entry::read(std::byte** ptr)
{
    object_index_entry entry = { 0 };

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

uint32_t objectmanager::getNumInstalledObjects()
{
    return *_installedObjectCount;
}

std::vector<std::pair<uint32_t, objectmanager::object_index_entry>> objectmanager::getAvailableObjects(object_type type)
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
