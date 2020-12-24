#pragma once

#include "../Types.hpp"
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    struct vehicle_base;
    struct MiscBase;

    namespace ThingId
    {
        constexpr thing_id_t null = std::numeric_limits<thing_id_t>::max();
    }

    enum class thing_base_type : uint8_t
    {
        vehicle = 0,
        misc
    };

    enum class VehicleThingType : uint8_t;
    enum class MiscThingType : uint8_t;

#pragma pack(push, 1)
    struct thing_base
    {
        thing_base_type base_type;
        union
        {
            VehicleThingType vType; // Vehicle Thing type (only valid for vehicle_base things)
            MiscThingType mType;    // Misc Thing type (only valid for MiscBase things)
        };
        uint8_t pad_02;
        uint8_t pad_03;
        thing_id_t next_thing_id; // 0x04
        uint8_t pad_06[0x09 - 0x06];
        uint8_t var_09;
        thing_id_t id;
        uint16_t var_0C;
        int16_t x; // 0x0E
        int16_t y; // 0x10
        int16_t z; // 0x12
        uint8_t var_14;
        uint8_t var_15;
        int16_t sprite_left;   // 0x16
        int16_t sprite_top;    // 0x18
        int16_t sprite_right;  // 0x1A
        int16_t sprite_bottom; // 0x1C
        uint8_t sprite_yaw;    // 0x1E
        uint8_t sprite_pitch;  // 0x1F

        void moveTo(loc16 loc);
        void invalidateSprite();

        vehicle_base* asVehicle() const { return asBase<vehicle_base, thing_base_type::vehicle>(); }
        MiscBase* asMisc() const { return asBase<MiscBase, thing_base_type::misc>(); }

    private:
        template<typename TType, thing_base_type TClass>
        TType* asBase() const
        {
            return base_type == TClass ? (TType*)this : nullptr;
        }
    };

    // Max size of a thing. Use when needing to know thing size
    struct Thing : thing_base
    {
    private:
        uint8_t pad_20[0x80 - 0x20];
    };
    static_assert(sizeof(Thing) == 0x80);
#pragma pack(pop)
}
