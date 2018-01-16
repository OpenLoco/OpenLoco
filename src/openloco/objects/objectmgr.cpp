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
}
