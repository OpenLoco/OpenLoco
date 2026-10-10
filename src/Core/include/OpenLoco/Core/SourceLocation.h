#pragma once

#include <string_view>

namespace OpenLoco
{
    namespace Detail
    {
        // NOTE: Making this consteval triggers a bug in clang, leave it constexpr for now.
        constexpr std::string_view sanitizePath(std::string_view path)
        {
#if defined(OPENLOCO_PROJECT_PATH)
            constexpr std::string_view projectPath = OPENLOCO_PROJECT_PATH;
            // Removes also the first slash.
            return path.substr(projectPath.size() + 1);
#else
            return path;
#endif
        }
    }
}
