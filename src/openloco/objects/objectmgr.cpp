#include "objectmgr.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::objectmgr
{
    loco_global<object_repository_item[64], 0x4FE0B8> object_repository;
    loco_global<interface_skin_object[1], 0x0050C3D0> _interfaceObjects;
    loco_global<sound_object * [128], 0x0050C3D4> _soundObjects;
    loco_global<currency_object * [1], 0x0050C5D4> _currencyObjects;
    loco_global<steam_object * [32], 0x0050C5D8> _steamObjects;
    loco_global<rock_object * [8], 0x0050C658> _rockObjects;
    loco_global<water_object * [1], 0x0050C678> _waterObjects;
    loco_global<land_object * [32], 0x0050C67C> _landObjects;
    loco_global<town_names_object * [1], 0x0050C6FC> _townNamesObjects;
    loco_global<cargo_object * [32], 0x0050C700> _cargoObjects;
    loco_global<wall_object * [32], 0x0050C780> _wallObjects;
    loco_global<train_signal_object * [16], 0x0050C800> _trainSignalObjects;
    loco_global<level_crossing_object * [4], 0x0050C840> _levelCrossingObjects;
    loco_global<street_light_object * [1], 0x0050C850> _streetLightObjects;
    loco_global<tunnel_object * [16], 0x0050C854> _tunnelObjects;
    loco_global<bridge_object * [8], 0x0050C894> _bridgeObjects;
    loco_global<train_station_object * [16], 0x0050C8B4> _trainStationObjects;
    loco_global<track_extra_object * [8], 0x0050C8F4> _trackExtraObjects;
    loco_global<track_object * [8], 0x0050C914> _trackObjects;
    loco_global<road_station_object * [16], 0x0050C934> _roadStationObjects;
    loco_global<road_extra_object * [4], 0x0050C974> _roadExtraObjects;
    loco_global<road_object * [8], 0x0050C984> _roadObjects;
    loco_global<airport_object * [8], 0x0050C9A4> _airportObjects;
    loco_global<dock_object * [8], 0x0050C9C4> _dockObjects;
    loco_global<vehicle_object * [224], 0x0050C9E4> _vehicleObjects;
    loco_global<tree_object * [64], 0x0050CD64> _treeObjects;
    loco_global<snow_object * [1], 0x0050CE64> _snowObjects;
    loco_global<climate_object * [1], 0x0050CE68> _climateObjects;
    loco_global<hill_shapes_object * [1], 0x0050CE6C> _hillShapeObjects;
    loco_global<building_object * [128], 0x0050CE70> _buildingObjects;
    loco_global<scaffolding_object * [1], 0x0050D070> _scaffoldingObjects;
    loco_global<industry_object * [16], 0x0050D074> _industryObjects;
    loco_global<region_object * [1], 0x0050D0B4> _regionObjects;
    loco_global<competitors_object * [32], 0x0050D0B8> _competitorsObjects;
    loco_global<scenario_text_object * [1], 0x0050D138> _scenarioTextObjects;

    // 0x00470F3C
    void load_index()
    {
        call(0x00470F3C);
    }

    cargo_object* get_cargo_object(size_t id)
    {
        return _cargoObjects[id];
    }

    vehicle_object* get_vehicle_object(size_t id)
    {
        return _vehicleObjects[id];
    }

    size_t get_max_objects(object_type type)
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
}
