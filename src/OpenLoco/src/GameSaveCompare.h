#pragma once

#include <OpenLoco/Core/FileSystem.hpp>

namespace OpenLoco::GameSaveCompare
{
    bool compareGameStates(const fs::path& path);
    bool compareGameStates(const fs::path& path1, const fs::path& path2, bool displayAllDivergences);
}
