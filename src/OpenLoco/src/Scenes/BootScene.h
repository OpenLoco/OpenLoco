#pragma once

#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::Scenes::BootScene
{
    bool loadFile(const fs::path& path);
    void tick();
    void tickInterface();
    void update();
}
