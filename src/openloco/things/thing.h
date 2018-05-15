#pragma once

#include "../types.hpp"
#include <cstdint>
#include <limits>

namespace openloco
{
    using thing_id_t = uint16_t;

    namespace thing_id
    {
        constexpr thing_id_t null = std::numeric_limits<thing_id_t>::max();
    }

    enum class thing_base_type : uint8_t
    {
        vehicle = 0,
        misc
    };

#pragma pack(push, 1)
    struct thing_base
    {
        thing_base_type base_type;

        void move_to(loc16 loc);
        void invalidate_sprite();
    };

    struct vehicle_base;
    struct misc_base;

    // Max size of a thing. Use when needing to know thing size
    struct thing : thing_base
    {
    private:
        uint8_t pad_01[128 - 0x01];
        template<typename TType, thing_base_type TClass>
        TType* as() const
        {
            return base_type == TClass ? (TType*)this : nullptr;
        }

    public:
        vehicle_base * as_vehilce() const { return as<vehicle_base, thing_base_type::vehicle>(); }
        misc_base * as_misc() const { return as<misc_base, thing_base_type::vehicle>(); }
    };
#pragma pack(pop)
}
