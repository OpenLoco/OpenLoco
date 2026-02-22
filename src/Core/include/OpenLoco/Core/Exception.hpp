#pragma once

#include "SourceLocation.h"
#include <exception>
#include <fmt/format.h>
#include <string>
#include <string_view>

namespace OpenLoco::Exception
{
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
    using OutOfRange = Detail::ExceptionBase<struct OutOfRangeTag>;
    using OverflowError = Detail::ExceptionBase<struct OverflowErrorTag>;

}
