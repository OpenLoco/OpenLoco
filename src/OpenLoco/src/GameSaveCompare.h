#pragma once

#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::GameSaveCompare
{
    void compareGameStates(const fs::path& path);
    void compareGameStates(const fs::path& path1, const fs::path& path2);
}
