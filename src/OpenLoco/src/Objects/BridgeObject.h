#pragma once

#include "Map/Track/TrackEnum.h"
#include "Object.h"
#include "Speed.hpp"
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
        class DrawingContext;
    }

    enum class BridgeObjectFlags : uint8_t
    {
        none = 0U,
        hasRoof = 1U << 0,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(BridgeObjectFlags);

#pragma pack(push, 1)
    struct BridgeObject
    {
        static constexpr auto kObjectType = ObjectType::bridge;

        StringId name;
        BridgeObjectFlags flags; // 0x02
        uint8_t pad_03;
        uint16_t clearHeight;                            // 0x04
        int16_t deckDepth;                               // 0x06
        uint8_t spanLength;                              // 0x08
        uint8_t pillarSpacing;                           // 0x09
        Speed16 maxSpeed;                                // 0x0A
        World::MicroZ maxHeight;                         // 0x0C MicroZ!
        uint8_t costIndex;                               // 0x0D
        int16_t baseCostFactor;                          // 0x0E
        int16_t heightCostFactor;                        // 0x10
        int16_t sellCostFactor;                          // 0x12
        World::Track::CommonTraitFlags disabledTrackCfg; // 0x14
        uint32_t image;                                  // 0x16
        uint8_t trackNumCompatible;                      // 0x1A
        uint8_t trackMods[7];                            // 0x1B
        uint8_t roadNumCompatible;                       // 0x22
        uint8_t roadMods[7];                             // 0x23
        uint16_t designedYear;                           // 0x2A

        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(BridgeObject) == 0x2C);

    namespace Bridge::ImageIds
    {
        constexpr uint32_t previewImage = 0;

        constexpr uint32_t deckBaseNEspan0 = 36;
        constexpr uint32_t supportHeaderLhsNEspan0 = 37;
        constexpr uint32_t supportHeaderRhsNEspan0 = 38;
        constexpr uint32_t deckWallLhsNEspan0 = 39;
        constexpr uint32_t deckWallRhsNEspan0 = 40;
        constexpr uint32_t roofNEspan0 = 41;
        constexpr uint32_t deckBaseNEspan1 = 42;
        constexpr uint32_t supportHeaderLhsNEspan1 = 43;
        constexpr uint32_t supportHeaderRhsNEspan1 = 44;
        constexpr uint32_t deckWallLhsNEspan1 = 45;
        constexpr uint32_t deckWallRhsNEspan1 = 46;
        constexpr uint32_t roofNEspan1 = 47;
        constexpr uint32_t deckBaseNEspan2 = 48;
        constexpr uint32_t supportHeaderLhsNEspan2 = 49;
        constexpr uint32_t supportHeaderRhsNEspan2 = 50;
        constexpr uint32_t deckWallLhsNEspan2 = 51;
        constexpr uint32_t deckWallRhsNEspan2 = 52;
        constexpr uint32_t roofNEspan2 = 53;
        constexpr uint32_t deckBaseNEspan3 = 54;
        constexpr uint32_t supportHeaderLhsNEspan3 = 55;
        constexpr uint32_t supportHeaderRhsNEspan3 = 56;
        constexpr uint32_t deckWallLhsNEspan3 = 57;
        constexpr uint32_t deckWallRhsNEspan3 = 58;
        constexpr uint32_t roofNEspan3 = 59;
        constexpr uint32_t deckBaseSWspan0 = 60;
        constexpr uint32_t supportHeaderLhsSWspan0 = 61;
        constexpr uint32_t supportHeaderRhsSWspan0 = 62;
        constexpr uint32_t deckWallLhsSWspan0 = 63;
        constexpr uint32_t deckWallRhsSWspan0 = 64;
        constexpr uint32_t roofSWspan0 = 65;
        constexpr uint32_t deckBaseSWspan1 = 66;
        constexpr uint32_t supportHeaderLhsSWspan1 = 67;
        constexpr uint32_t supportHeaderRhsSWspan1 = 68;
        constexpr uint32_t deckWallLhsSWspan1 = 69;
        constexpr uint32_t deckWallRhsSWspan1 = 70;
        constexpr uint32_t roofSWspan1 = 71;
        constexpr uint32_t deckBaseSWspan2 = 72;
        constexpr uint32_t supportHeaderLhsSWspan2 = 73;
        constexpr uint32_t supportHeaderRhsSWspan2 = 74;
        constexpr uint32_t deckWallLhsSWspan2 = 75;
        constexpr uint32_t deckWallRhsSWspan2 = 76;
        constexpr uint32_t roofSWspan2 = 77;
        constexpr uint32_t deckBaseSWspan3 = 78;
        constexpr uint32_t supportHeaderLhsSWspan3 = 79;
        constexpr uint32_t supportHeaderRhsSWspan3 = 80;
        constexpr uint32_t deckWallLhsSWspan3 = 81;
        constexpr uint32_t deckWallRhsSWspan3 = 82;
        constexpr uint32_t roofSWspan3 = 83;
    }
}
