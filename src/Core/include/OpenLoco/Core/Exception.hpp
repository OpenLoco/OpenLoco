#pragma once

#include "SourceLocation.h"
#include <exception>
#include <fmt/format.h>
#include <source_location>
#include <string>

namespace OpenLoco::Exception
{
    namespace Detail
    {
        template<typename TExceptionTag>
        class ExceptionBase : public std::exception
        {
        private:
            std::source_location _location;
            std::string _message;

        public:
            explicit ExceptionBase(const std::source_location& location = std::source_location::current())
                : _location{ location }
            {
                _message = fmt::format("Exception thrown at '{}' - {}:{}", _location.function_name(), OpenLoco::Detail::sanitizePath(_location.file_name()), _location.line());
            }
            explicit ExceptionBase(const std::string& message, const std::source_location& location = std::source_location::current())
                : _location{ location }
            {
                _message = fmt::format("Exception '{}', thrown at '{}' - {}:{}", message, _location.function_name(), OpenLoco::Detail::sanitizePath(_location.file_name()), _location.line());
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
    using OutOfRange = Detail::ExceptionBase<struct OutOfRangeTag>;
    using OverflowError = Detail::ExceptionBase<struct OverflowErrorTag>;

}
