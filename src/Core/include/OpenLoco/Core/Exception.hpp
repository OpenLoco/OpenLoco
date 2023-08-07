#pragma once

#include <exception>
#include <fmt/format.h>
#include <string>

namespace OpenLoco::Exception
{
    // Something like std::source_location except this works prior to C++20.
    class SourceLocation
    {
        std::string _function;
        std::string _file;
        int _line;

    public:
        explicit SourceLocation(const char* func = __builtin_FUNCTION(), const char* file = __builtin_FILE(), int line = __builtin_LINE())
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
    using BadAlloc = Detail::ExceptionBase<struct BadAllocTag>;

}
