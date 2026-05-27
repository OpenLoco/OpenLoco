#pragma once

#include "Location.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <cstdint>
#include <limits>

namespace OpenLoco
{
    enum class EntityBaseType : uint8_t
    {
        vehicle = 0,
        effect,
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

    struct EntityBase
    {
        EntityBaseType baseType;
        uint8_t linkedListOffset;
        EntityId id;
        EntityId nextQuadrantId;
        EntityId nextEntityId;
        EntityId llPreviousId;
        World::Pos3 position;
        CompanyId owner;
        uint8_t spriteHeightNegative;
        uint8_t spriteHeightPositive;
        uint8_t spriteWidth;
        uint8_t spriteYaw;
        Pitch spritePitch;
        int16_t spriteLeft;
        int16_t spriteTop;
        int16_t spriteRight;
        int16_t spriteBottom;
        StringId name; // combined with ordinalNumber on vehicles

        void moveTo(const World::Pos3& loc);
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
    };

    // Max size of a Entity. Use when needing to know Entity size
    struct Entity : EntityBase
    {
    private:
        uint8_t pad_24[0x80 - 0x20];
    };
    static_assert(sizeof(Entity) == 0x80);
}
