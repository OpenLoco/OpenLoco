#pragma once

#include <OpenLoco/Core/SourceLocation.h>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Platform/Debug.h>

#include <fmt/format.h>
#include <functional>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>

namespace OpenLoco::Diagnostics::Assert
{
    namespace Detail
    {
        template<typename T>
        inline std::string formatValue(T&& value)
        {
            using DecayedType = std::remove_const_t<std::decay_t<T>>;

            // Check first if there is a formatter specialization.
            if constexpr (fmt::is_formattable<DecayedType>::value)
            {
                return fmt::format("{}", std::forward<T>(value));
            }
            else if constexpr (std::is_enum_v<DecayedType>)
            {
                return fmt::format("{}::{}", typeid(DecayedType).name(), static_cast<std::underlying_type_t<DecayedType>>(value));
            }
            else
            {
                return fmt::format("<{}>", typeid(DecayedType).name());
            }
        }

        template<typename L, typename R>
        inline void assertionFailure(std::string_view op, const L& lhs, const R& rhs, const std::source_location& loc)
        {
            Logging::error(
                "Assertion failure ({}:{}): {} {} {}",
                OpenLoco::Detail::sanitizePath(loc.file_name()),
                loc.line(),
                Detail::formatValue(lhs),
                op,
                Detail::formatValue(rhs));
        }

        template<typename Ptr>
        concept NullablePointer = requires(Ptr&& p) {
            { p == nullptr } -> std::convertible_to<bool>;
            { p != nullptr } -> std::convertible_to<bool>;
        };
    }

    template<typename U1, typename U2>
    inline void eq(const U1& expected, const U2& actual, const std::source_location& loc = std::source_location::current())
    {
        if (expected != actual)
        {
            Detail::assertionFailure("==", expected, actual, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void neq(const U1& expected, const U2& actual, const std::source_location& loc = std::source_location::current())
    {
        if (expected == actual)
        {
            Detail::assertionFailure("!=", expected, actual, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void lt(const U1& lhs, const U2& rhs, const std::source_location& loc = std::source_location::current())
    {
        if (!(lhs < rhs))
        {
            Detail::assertionFailure("<", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void le(const U1& lhs, const U2& rhs, const std::source_location& loc = std::source_location::current())
    {
        if (!(lhs <= rhs))
        {
            Detail::assertionFailure("<=", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void gt(const U1& lhs, const U2& rhs, const std::source_location& loc = std::source_location::current())
    {
        if (!(lhs > rhs))
        {
            Detail::assertionFailure(">", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void ge(const U1& lhs, const U2& rhs, const std::source_location& loc = std::source_location::current())
    {
        if (!(lhs >= rhs))
        {
            Detail::assertionFailure(">=", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    inline void isTrue(bool condition, const std::source_location& loc = std::source_location::current())
    {
        if (!condition)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected true",
                OpenLoco::Detail::sanitizePath(loc.file_name()),
                loc.line());
            OPENLOCO_DEBUG_BREAK();
        }
    }

    inline void isFalse(bool condition, const std::source_location& loc = std::source_location::current())
    {
        if (condition)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected false",
                OpenLoco::Detail::sanitizePath(loc.file_name()),
                loc.line());
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<Detail::NullablePointer Ptr>
    inline void isNull(Ptr&& ptr, const std::source_location& loc = std::source_location::current())
    {
        if (ptr != nullptr)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected null pointer (got {})",
                OpenLoco::Detail::sanitizePath(loc.file_name()),
                loc.line(),
                Detail::formatValue(ptr));
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<Detail::NullablePointer Ptr>
    inline void notNull(Ptr&& ptr, const std::source_location& loc = std::source_location::current())
    {
        if (ptr == nullptr)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected non-null pointer",
                OpenLoco::Detail::sanitizePath(loc.file_name()),
                loc.line());
            OPENLOCO_DEBUG_BREAK();
        }
    }
}
