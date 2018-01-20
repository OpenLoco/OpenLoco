#include "objectmgr.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::objectmgr
{
    loco_global_array<object_repository_item, 64, 0x4FE0B8> object_repository;
    // Array sizes to be confirmed
    loco_global_array<interface_object *, 1, 0x0050C3D0> _interfaceObjects;
    loco_global_array<sound_object *, 1, 0x0050C3D4> _soundObjects;
    loco_global_array<currency_object *, 1, 0x0050C5D4> _currencyObjects;
    loco_global_array<steam_object *, 1, 0x0050C5D8> _steamObjects;
    loco_global_array<rocks_object *, 1, 0x0050C658> _rockObjects;
    loco_global_array<water_object *, 1, 0x0050C678> _waterObjects;
    loco_global_array<surface_object *, 1, 0x0050C67C> _surfaceObjects;
    loco_global_array<town_name_object *, 1, 0x0050C6FC> _townNameObjects;
    loco_global_array<cargo_object *, 32, 0x0050C700> _cargoObjects;
    loco_global_array<wall_object *, 1, 0x0050C780> _wallObjects;
    loco_global_array<train_signal_object *, 1, 0x0050C800> _trainSignalObjects;
    loco_global_array<level_crossing_object *, 1, 0x0050C840> _levlCrossingObjects;
    loco_global_array<street_light_object *, 1, 0x0050C850> _streetLightObjects;
    loco_global_array<tunnel_object *, 1, 0x0050C854> _tunnelObjects;
    loco_global_array<bridge_object *, 1, 0x0050C894> _bridgeObjects;
    loco_global_array<train_station_object *, 1, 0x0050C8B4> _trainStationObjects;
    loco_global_array<track_extra_object *, 1, 0x0050C8F4> _trackExtraObjects;
    loco_global_array<track_object *, 1, 0x0050C914> _trackObjects;
    loco_global_array<road_station_object *, 1, 0x0050C934> _roadStationObjects;
    loco_global_array<road_extra_object *, 1, 0x0050C974> _roadExtraObjects;
    loco_global_array<road_object *, 1, 0x0050C984> _roadObjects;
    loco_global_array<airport_object *, 1, 0x0050C9A4> _airportObjects;
    loco_global_array<dock_object *, 1, 0x0050C9C4> _dockObjects;
    loco_global_array<vehicle_object *, 1, 0x0050C9E4> _vehicleObjects;
    loco_global_array<tree_object *, 1, 0x0050CD64> _treeObjects;
    loco_global_array<snow_object *, 1, 0x0050CE64> _snowObjects;
    loco_global_array<climate_object *, 1, 0x0050CE68> _cliamteObjects;
    loco_global_array<hill_shapes_object *, 1, 0x0050CE6C> _hillShapeObjects;
    loco_global_array<building_object *, 1, 0x0050CE70> _buildingObjects;
    loco_global_array<scaffolding_object *, 1, 0x0050D070> _scaffoldingObjects;
    loco_global_array<industry_object *, 1, 0x0050D074> _industryObjects;
    loco_global_array<region_object *, 1, 0x0050D0B4> _regionObjects;
    loco_global_array<competitors_object *, 1, 0x0050D0B8> _competitorsObjects;
    loco_global_array<scenario_text_object *, 1, 0x0050D138> _scenarioTextObjects;

    // 0x00470F3C
    void load_index()
    {
        call(0x00470F3C);
    }

    cargo_object* get_cargo_object(size_t id)
    {
        return _cargoObjects[id];
    }

    vehicle_object * get_vehicle_object(size_t id)
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
            1,   // town_name,
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
