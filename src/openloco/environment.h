#pragma once

#include <filesystem>

namespace openloco::environment
{
    namespace fs = std::experimental::filesystem;

    enum class path_id
    {

    };

    fs::path get_path(path_id id);
    void resolve_paths();
}
