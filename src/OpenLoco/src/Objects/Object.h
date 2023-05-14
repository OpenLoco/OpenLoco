#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>

namespace OpenLoco
{
    enum class ObjectType : uint8_t
    {
        interfaceSkin,
        sound,
        currency,
        steam,
        cliffEdge,
        water,
        land,
        townNames,
        cargo,
        wall,
        trackSignal,
        levelCrossing,
        streetLight,
        tunnel,
        bridge,
        trackStation,
        trackExtra,
        track,
        roadStation,
        roadExtra,
        road,
        airport,
        dock,
        vehicle,
        tree,
        snow,
        climate,
        hillShapes,
        building,
        scaffolding,
        industry,
        region,
        competitor,
        scenarioText,
    };

#pragma pack(push, 1)
    struct ObjectHeader
    {
    private:
        static constexpr uint32_t kFuzzyFlagsMask = 0xFFFE0000;

    public:
        uint32_t flags;
        char name[8];
        uint32_t checksum;

        std::string_view getName() const
        {
            return std::string_view(name, sizeof(name));
        }

        constexpr uint8_t getSourceGame() const
        {
            return (flags >> 6) & 0x3;
        }

        constexpr ObjectType getType() const
        {
            return static_cast<ObjectType>(flags & 0x3F);
        }

        constexpr uint32_t getFuzzyFlags() const
        {
            return flags & kFuzzyFlagsMask;
        }

        constexpr bool isCustom() const
        {
            return getSourceGame() == 0;
        }

        constexpr bool isEmpty() const;

        // The original game would check whether an object was part of the base game by means of the sourceGame attribute.
        // As many custom objects were being based on base game objects, most custom objects do not set this attribute
        // correctly. Therefore those objects would not be packed by Locomotion. We change this behaviour by explicitly
        // checking objects against a list of vanilla objects instead. The original logic is left in the isCustom method.
        bool isVanilla() const;

        bool operator==(const ObjectHeader& rhs) const
        {
            // Some vanilla objects reference other vanilla objects using a
            // ObjectHeader that is set as custom. To handle those we need
            // to check both the lhs and the rhs and only if both are Custom
            // do the full check.
            if (isCustom() && rhs.isCustom())
            {
                return std::memcmp(this, &rhs, sizeof(ObjectHeader)) == 0;
            }
            else
            {
                return getType() == rhs.getType() && getName() == rhs.getName();
            }
        }

        bool operator!=(const ObjectHeader& rhs) const
        {
            return !(*this == rhs);
        }
    };
    static_assert(sizeof(ObjectHeader) == 0x10);
#pragma pack(pop)
    constexpr ObjectHeader kEmptyObjectHeader = ObjectHeader{ 0xFFFFFFFFU, { '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF' }, 0xFFFFFFFFU };

    constexpr bool ObjectHeader::isEmpty() const
    {
        // No point checking the name as its already invalid if flags is all 0xFFFFFFFFU
        return this->flags == kEmptyObjectHeader.flags && this->checksum == kEmptyObjectHeader.checksum;
    }

    /**
     * Represents an index / ID of a specific object type.
     */
    using LoadedObjectId = uint16_t;

    /**
     * Represents an undefined index / ID for a specific object type.
     */
    static constexpr LoadedObjectId NullObjectId = std::numeric_limits<LoadedObjectId>::max();

    struct LoadedObjectHandle
    {
        ObjectType type;
        LoadedObjectId id;
    };
}
