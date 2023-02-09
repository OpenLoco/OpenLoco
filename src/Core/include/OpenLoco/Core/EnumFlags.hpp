#pragma once

#include <cassert>
#include <cstdint>

namespace OpenLoco::Core
{

#define OPENLOCO_ENABLE_ENUM_OPERATORS(ENM)                                                                                 \
    static_assert(std::is_unsigned_v<std::underlying_type_t<ENM>> == true, "Underlying enum type must be unsignd");         \
    static_assert(static_cast<std::underlying_type_t<ENM>>(ENM::none) == 0U, "Enum must have a none value");                \
    inline constexpr ENM operator|(const ENM a, const ENM b) noexcept                                                       \
    {                                                                                                                       \
        return static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) | static_cast<std::underlying_type_t<ENM>>(b)); \
    }                                                                                                                       \
    inline constexpr ENM operator|=(ENM& a, const ENM b) noexcept                                                           \
    {                                                                                                                       \
        a = static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) | static_cast<std::underlying_type_t<ENM>>(b));    \
        return a;                                                                                                           \
    }                                                                                                                       \
    inline constexpr ENM operator^(const ENM a, const ENM b) noexcept                                                       \
    {                                                                                                                       \
        return static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) ^ static_cast<std::underlying_type_t<ENM>>(b)); \
    }                                                                                                                       \
    inline constexpr ENM operator^=(ENM& a, const ENM b) noexcept                                                           \
    {                                                                                                                       \
        a = static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) ^ static_cast<std::underlying_type_t<ENM>>(b));    \
        return a;                                                                                                           \
    }                                                                                                                       \
    inline constexpr ENM operator&(const ENM a, const ENM b) noexcept                                                       \
    {                                                                                                                       \
        return static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) & static_cast<std::underlying_type_t<ENM>>(b)); \
    }                                                                                                                       \
    inline constexpr ENM operator&=(ENM& a, const ENM b) noexcept                                                           \
    {                                                                                                                       \
        a = static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) & static_cast<std::underlying_type_t<ENM>>(b));    \
        return a;                                                                                                           \
    }                                                                                                                       \
    inline constexpr ENM operator~(const ENM a) noexcept                                                                    \
    {                                                                                                                       \
        return static_cast<ENM>(~static_cast<std::underlying_type_t<ENM>>(a));                                              \
    }                                                                                                                       \
    inline constexpr ENM operator-=(ENM& a, const ENM b) noexcept                                                           \
    {                                                                                                                       \
        a = static_cast<ENM>(static_cast<std::underlying_type_t<ENM>>(a) - static_cast<std::underlying_type_t<ENM>>(b));    \
        return a;                                                                                                           \
    }

}
