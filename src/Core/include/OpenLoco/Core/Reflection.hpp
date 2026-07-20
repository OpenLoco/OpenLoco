#pragma once

namespace OpenLoco
{
    template<auto Field>
    struct Serializable
    {
        static constexpr auto field = Field;
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
