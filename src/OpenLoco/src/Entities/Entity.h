#pragma once

#include "Location.hpp"
#include "Map/Map.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    enum class EntityBaseType : uint8_t
    {
        vehicle = 0,
        misc,
        null = 0xFF
    };

    enum class Pitch : uint8_t
    {
        //                               actual angle (for trig)
        flat = 0,
        up6deg = 1,  // Transition,      5.75 deg
        up12deg = 2, // Gentle,          11.75 deg
        up18deg = 3, // Transition,      17 deg
        up25deg = 4, // Steep,           22.5 deg
        down6deg = 5,
        down12deg = 6,
        down18deg = 7,
        down25deg = 8,
        up10deg = 9, // Gentle Curve Up, 10 deg
        down10deg = 10,
        up20deg = 11, // Steep Curve Up, 19.25 deg
        down20deg = 12,
    };

    enum class EntityBaseFlags : uint16_t // commands?
    {
        none = 0U,
        unk_0 = 1U << 0,
        commandStop = 1U << 1, // commanded to stop??
        sorted = 1U << 3,      // vehicle list
        unk_5 = 1U << 5,
        manualControl = 1U << 6,
        shuntCheat = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(EntityBaseFlags);

#pragma pack(push, 1)
    struct EntityBase
    {
        EntityBaseType baseType;

    private:
        uint8_t type; // Use type specific getters/setters as this depends on baseType
    public:
        EntityId nextQuadrantId;  // 0x02
        EntityId nextThingId;     // 0x04
        EntityId llPreviousId;    // 0x06
        uint8_t linkedListOffset; // 0x8
        uint8_t var_09;
        EntityId id; // 0xA
        EntityBaseFlags var_0C;
        Map::Pos3 position; // 0x0E
        uint8_t var_14;
        uint8_t var_15;
        int16_t spriteLeft;   // 0x16
        int16_t spriteTop;    // 0x18
        int16_t spriteRight;  // 0x1A
        int16_t spriteBottom; // 0x1C
        uint8_t spriteYaw;    // 0x1E
        Pitch spritePitch;    // 0x1F
        uint8_t pad_20;
        CompanyId owner; // 0x21
        string_id name;  // 0x22, combined with ordinalNumber on vehicles

        void moveTo(const Map::Pos3& loc);
        void invalidateSprite();

        template<typename T>
        bool isBase() const
        {
            return baseType == T::kBaseType;
        }
        template<typename BaseType>
        BaseType* asBase()
        {
            return isBase<BaseType>() ? reinterpret_cast<BaseType*>(this) : nullptr;
        }
        template<typename BaseType>
        const BaseType* asBase() const
        {
            return isBase<BaseType>() ? reinterpret_cast<const BaseType*>(this) : nullptr;
        }
        bool empty() const;

    protected:
        constexpr uint8_t getSubType() const { return type; }
        void setSubType(const uint8_t newType) { type = newType; }
    };

    // Max size of a Entity. Use when needing to know Entity size
    struct Entity : EntityBase
    {
    private:
        uint8_t pad_24[0x80 - 0x24];
    };
    static_assert(sizeof(Entity) == 0x80);
#pragma pack(pop)
}
