#pragma once

#include <cstdint>

namespace OpenLoco::Scenes::GameScene
{
    uint16_t tickWorld();
    void tickInterface(uint16_t numFrameUpdates);
}
