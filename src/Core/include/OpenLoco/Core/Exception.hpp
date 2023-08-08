#pragma once

#include <exception>
#include <fmt/format.h>
#include <string>
#include <string_view>

namespace OpenLoco::Exception
{
    namespace Detail
    {
        // TODO: Make this consteval for C++20
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

    // Something like std::source_location except this works prior to C++20.
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

    namespace Detail
    {
        template<typename TExceptionTag>
        class ExceptionBase : public std::exception
        {
        private:
            SourceLocation _location;
            std::string _message;

        public:
            explicit ExceptionBase(const SourceLocation& location = SourceLocation{})
                : _location{ location }
            {
                _message = fmt::format("Exception thrown at '{}' - {}:{}", _location.function(), _location.file(), _location.line());
            }
            explicit ExceptionBase(const std::string& message, const SourceLocation& location = SourceLocation{})
                : _location{ location }
            {
                _message = fmt::format("Exception '{}', thrown at '{}' - {}:{}", message, _location.function(), _location.file(), _location.line());
            }
            const char* what() const noexcept override
            {
                return _message.c_str();
            }
        };
    }

    using RuntimeError = Detail::ExceptionBase<struct RuntimeErrorTag>;
    using InvalidArgument = Detail::ExceptionBase<struct InvalidArgumentTag>;
    using NotImplemented = Detail::ExceptionBase<struct NotImplementedTag>;
    using InvalidOperation = Detail::ExceptionBase<struct InvalidOperationTag>;
    using BadAllocation = Detail::ExceptionBase<struct BadAllocTag>;

}
