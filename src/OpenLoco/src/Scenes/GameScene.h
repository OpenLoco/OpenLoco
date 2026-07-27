#pragma once

namespace OpenLoco::Scenes::GameScene
{
    void autosaveReset();

    // Advances the world by exactly one tick.
    void tickWorld();

    void tickInterface();
}
