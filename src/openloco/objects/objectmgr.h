#pragma once

namespace openloco
{
    struct cargo_object;
}

namespace openloco::objectmgr
{
    void load_index();
    cargo_object * get_cargo_object(size_t id);
}
