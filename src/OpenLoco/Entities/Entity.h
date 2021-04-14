#pragma once

#include "../Location.hpp"
#include "../Map/Map.hpp"
#include "../Types.hpp"

#include <cstdint>
#include <limits>

namespace OpenLoco::Vehicles
{
    struct VehicleBase;
}

namespace OpenLoco
{
    struct MiscBase;

    namespace EntityId
    {
        constexpr EntityId_t null = std::numeric_limits<EntityId_t>::max();
    }

    enum class EntityBaseType : uint8_t
    {
        vehicle = 0,
        misc
    };

    enum class Pitch : uint8_t
    {
        flat = 0,
        up6deg = 1,  // Transition
        up12deg = 2, // Gentle
        up18deg = 3, // Transition
        up25deg = 4, // Steep
        down6deg = 5,
        down12deg = 6,
        down18deg = 7,
        down25deg = 8,
        up10deg = 9, // Gentle Curve Up
        down10deg = 10,
        up20deg = 11, // Steep Curve Up
        down20deg = 12,
    };

#pragma pack(push, 1)
    struct EntityBase
    {
        EntityBaseType base_type;

    private:
        uint8_t type; // Use type specific getters/setters as this depends on base_type
    public:
        EntityId_t nextQuadrantId; // 0x02
        EntityId_t next_thing_id;  // 0x04
        EntityId_t llPreviousId;   // 0x06
        uint8_t linkedListOffset;  // 0x8
        uint8_t var_09;
        EntityId_t id; // 0xA
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
        Pitch sprite_pitch;    // 0x1F

        void moveTo(Map::map_pos3 loc);
        void invalidateSprite();

        Vehicles::VehicleBase* asVehicle() const { return asBase<Vehicles::VehicleBase, EntityBaseType::vehicle>(); }
        MiscBase* asMisc() const { return asBase<MiscBase, EntityBaseType::misc>(); }

    protected:
        uint8_t getSubType() const { return type; }
        void setSubType(const uint8_t newType) { type = newType; }

    private:
        template<typename TType, EntityBaseType TClass>
        TType* asBase() const
        {
            return base_type == TClass ? (TType*)this : nullptr;
        }
    };

    // Max size of a Entity. Use when needing to know Entity size
    struct Entity : EntityBase
    {
    private:
        uint8_t pad_20[0x80 - 0x20];
    };
    static_assert(sizeof(Entity) == 0x80);
#pragma pack(pop)
}
