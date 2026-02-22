#pragma once

#include <fmt/format.h>
#include <string>
#include <string_view>

namespace OpenLoco
{
    namespace Detail
    {
        consteval std::string_view sanitizePath(std::string_view path)
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

    // Something like std::source_location except it uses relative paths when possible.
    class SourceLocation
    {
        std::string _function;
        std::string _file;
        int _line;

    public:
        explicit SourceLocation(std::string_view func = __builtin_FUNCTION(), std::string_view file = Detail::sanitizePath(__builtin_FILE()), int line = __builtin_LINE())
            : _function(func)
            , _file(file)
            , _line(line)
        {
        }
        const std::string& file() const noexcept
        {
            return _file;
        }
        const std::string& function() const noexcept
        {
            return _function;
        }
        int line() const noexcept
        {
            return _line;
        }
    };
}
