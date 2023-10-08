#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <span>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    enum class AirportMovementNodeFlags : uint16_t
    {
        none = 0U,
        terminal = 1 << 0,
        takeoffEnd = 1U << 1,
        flag2 = 1U << 2,
        taxiing = 1U << 3,
        inFlight = 1U << 4,
        heliTakeoffBegin = 1U << 5,
        takeoffBegin = 1U << 6,
        heliTakeoffEnd = 1U << 7,
        touchdown = 1U << 8,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(AirportMovementNodeFlags);

    struct AirportObject
    {
        static constexpr auto kObjectType = ObjectType::airport;

        struct MovementNode
        {
            int16_t x;                      // 0x00
            int16_t y;                      // 0x02
            int16_t z;                      // 0x04
            AirportMovementNodeFlags flags; // 0x06

            constexpr bool hasFlags(AirportMovementNodeFlags flagsToTest) const
            {
                return (flags & flagsToTest) != AirportMovementNodeFlags::none;
            }
        };

        struct MovementEdge
        {
            uint8_t var_00;
            uint8_t curNode;  // 0x01
            uint8_t nextNode; // 0x02
            uint8_t var_03;
            uint32_t mustBeClearEdges;     // 0x04 Which edges must be clear to use the transition edge
            uint32_t atLeastOneClearEdges; // 0x08 Which edges must have at least one clear to use transition edge
        };

        string_id name;
        int16_t buildCostFactor; // 0x02
        int16_t sellCostFactor;  // 0x04
        uint8_t costIndex;       // 0x06
        uint8_t var_07;
        uint32_t image; // 0x08
        uint32_t var_0C;
        uint16_t allowedPlaneTypes; // 0x10
        uint8_t numSpriteSets;      // 0x12
        uint8_t numTiles;           // 0x13
        const uint8_t* var_14;
        const uint16_t* var_18;
        const uint8_t* var_1C[32];
        const uint32_t* var_9C;
        uint32_t largeTiles;               // 0xA0
        int8_t minX;                       // 0xA4
        int8_t minY;                       // 0xA5
        int8_t maxX;                       // 0xA6
        int8_t maxY;                       // 0xA7
        uint16_t designedYear;             // 0xA8
        uint16_t obsoleteYear;             // 0xAA
        uint8_t numMovementNodes;          // 0xAC
        uint8_t numMovementEdges;          // 0xAD
        const MovementNode* movementNodes; // 0xAE
        const MovementEdge* movementEdges; // 0xB2
        uint8_t pad_B6[0xBA - 0xB6];

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
        std::pair<World::TilePos2, World::TilePos2> getAirportExtents(const World::TilePos2& centrePos, const uint8_t rotation) const;
    };
#pragma pack(pop)

    static_assert(sizeof(AirportObject) == 0xBA);
}
