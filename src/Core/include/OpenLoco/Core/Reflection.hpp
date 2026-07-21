#pragma once

#include <cstddef>
#include <string_view>

namespace OpenLoco
{
    template<std::size_t N>
    struct FixedString
    {
        char value[N]{};

        consteval FixedString(const char (&str)[N])
        {
            for (std::size_t i = 0; i < N; ++i)
            {
                value[i] = str[i];
            }
        }
    };

    template<auto Ptr, FixedString Name>
    struct Serializable
    {
        static constexpr auto field = Ptr;
        static constexpr std::string_view name = Name.value;
    };

    template<auto Ptr, FixedString Name>
    struct Field
    {
        static constexpr auto field = Ptr;
        static constexpr std::string_view name = Name.value;
    };

    template<typename... Fields>
    struct FieldList
    {
    };

    template<typename T>
    struct Reflection
    {
    };
}

#define REFL_FIELD(TYPE, NAME) ::OpenLoco::Field<&TYPE::NAME, #NAME>
#define REFL_SERIALIZABLE_FIELD(TYPE, NAME) ::OpenLoco::Serializable<&TYPE::NAME, #NAME>
