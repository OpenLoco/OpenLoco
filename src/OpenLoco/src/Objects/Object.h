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
        static constexpr char cFF = static_cast<char>(0xFF);
        static constexpr uint32_t kFuzzyFlagsMask = 0xFFFE0000;

    public:
        uint32_t flags = 0xFFFFFFFF;
        char name[8] = { cFF, cFF, cFF, cFF, cFF, cFF, cFF, cFF };
        uint32_t checksum = 0xFFFFFFFF;

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

        bool isEmpty() const
        {
            auto ab = reinterpret_cast<const int64_t*>(this);
            return ab[0] == -1 && ab[1] == -1;
        }

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
