#include "../interop/interop.hpp"
#include "objectmgr.h"

using namespace openloco::interop;

namespace openloco::objectmgr
{
    loco_global_array<cargo_object *, 32, 0x0050C700> _cargoObjects;

    // 0x00470F3C
    void load_index()
    {
        call(0x00470F3C);
    }

    cargo_object * get_cargo_object(size_t id)
    {
        return _cargoObjects[id];
    }

    size_t get_max_objects(object_type type)
    {
        static size_t counts[] =
        {
            1,      // interface,
            128,    // sound,
            1,      // currency,
            32,     // steam,
            8,      // rock,
            1,      // water,
            32,     // surface,
            1,      // town_name,
            32,     // cargo,
            32,     // wall,
            16,     // train_signal,
            4,      // level_crossing,
            1,      // street_light,
            16,     // tunnel,
            8,      // bridge,
            16,     // train_station,
            8,      // track_extra,
            8,      // track,
            16,     // road_station,
            4,      // road_extra,
            8,      // road,
            8,      // airport,
            8,      // dock,
            224,    // vehicle,
            64,     // tree,
            1,      // snow,
            1,      // climate,
            1,      // hill_shapes,
            128,    // building,
            1,      // scaffolding,
            16,     // industry,
            1,      // region,
            32,     // competitors,
            1       // scenario_text,
        };
        return counts[(size_t)type];
    };
}
