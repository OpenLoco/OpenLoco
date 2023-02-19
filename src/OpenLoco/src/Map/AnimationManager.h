#pragma once
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::World::AnimationManager
{
    void createAnimation(uint8_t type, const Pos2& pos, tile_coord_t baseZ);
    void reset();
    void update();
    void registerHooks();
}
