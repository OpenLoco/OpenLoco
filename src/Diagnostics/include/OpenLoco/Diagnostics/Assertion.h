#pragma once

#include <OpenLoco/Core/SourceLocation.h>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Platform/Debug.h>

#include <fmt/format.h>
#include <functional>
#include <memory>
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
        inline void assertionFailure(std::string_view op, const L& lhs, const R& rhs, const SourceLocation& loc)
        {
            Logging::error(
                "Assertion failure ({}:{}): {} {} {}",
                loc.file(),
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
    inline void eq(const U1& expected, const U2& actual, const SourceLocation& loc = SourceLocation{})
    {
        if (expected != actual)
        {
            Detail::assertionFailure("==", expected, actual, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void neq(const U1& expected, const U2& actual, const SourceLocation& loc = SourceLocation{})
    {
        if (expected == actual)
        {
            Detail::assertionFailure("!=", expected, actual, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void lt(const U1& lhs, const U2& rhs, const SourceLocation& loc = SourceLocation{})
    {
        if (!(lhs < rhs))
        {
            Detail::assertionFailure("<", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void le(const U1& lhs, const U2& rhs, const SourceLocation& loc = SourceLocation{})
    {
        if (!(lhs <= rhs))
        {
            Detail::assertionFailure("<=", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void gt(const U1& lhs, const U2& rhs, const SourceLocation& loc = SourceLocation{})
    {
        if (!(lhs > rhs))
        {
            Detail::assertionFailure(">", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<typename U1, typename U2>
    inline void ge(const U1& lhs, const U2& rhs, const SourceLocation& loc = SourceLocation{})
    {
        if (!(lhs >= rhs))
        {
            Detail::assertionFailure(">=", lhs, rhs, loc);
            OPENLOCO_DEBUG_BREAK();
        }
    }

    inline void isTrue(bool condition, const SourceLocation& loc = SourceLocation{})
    {
        if (!condition)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected true",
                loc.file(),
                loc.line());
            OPENLOCO_DEBUG_BREAK();
        }
    }

    inline void isFalse(bool condition, const SourceLocation& loc = SourceLocation{})
    {
        if (condition)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected false",
                loc.file(),
                loc.line());
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<Detail::NullablePointer Ptr>
    inline void isNull(Ptr&& ptr, const SourceLocation& loc = SourceLocation{})
    {
        if (ptr != nullptr)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected null pointer (got {})",
                loc.file(),
                loc.line(),
                Detail::formatValue(ptr));
            OPENLOCO_DEBUG_BREAK();
        }
    }

    template<Detail::NullablePointer Ptr>
    inline void notNull(Ptr&& ptr, const SourceLocation& loc = SourceLocation{})
    {
        if (ptr == nullptr)
        {
            Logging::error(
                "Assertion failure ({}:{}): expected non-null pointer",
                loc.file(),
                loc.line());
            OPENLOCO_DEBUG_BREAK();
        }
    }
}
