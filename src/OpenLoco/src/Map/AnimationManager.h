#pragma once
#include <OpenLoco/Engine/Map.hpp>

namespace OpenLoco::Map::AnimationManager
{
    void createAnimation(uint8_t type, const Pos2& pos, tile_coord_t baseZ);
    void reset();
    void update();
    void registerHooks();
}
