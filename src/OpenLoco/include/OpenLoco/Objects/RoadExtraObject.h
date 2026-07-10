#pragma once

#include "Map/Track/TrackEnum.h"
#include "Object.h"
#include "Types.hpp"
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

#pragma pack(push, 1)
    struct RoadExtraObject
    {
        static constexpr auto kObjectType = ObjectType::roadExtra;

        StringId name;                           // 0x00
        World::Track::RoadTraitFlags roadPieces; // 0x02
        uint8_t paintStyle;                      // 0x04
        uint8_t costIndex;                       // 0x05
        int16_t buildCostFactor;                 // 0x06
        int16_t sellCostFactor;                  // 0x08
        uint32_t image;                          // 0x0A
        uint32_t var_0E;

        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(RoadExtraObject) == 0x12);
    namespace RoadExtraObj::ImageIds
    {
        // Note: Style imageIds are relative to 0x0A so you need to +8 to get its
        // real id relative to object at rest
        namespace Style1
        {
            constexpr uint32_t kStraight0NE = 0;
            constexpr uint32_t kStraight0SE = 1;
            constexpr uint32_t kRightCurveSmall0NE = 2;
            constexpr uint32_t kRightCurveSmall3NE = 3;
            constexpr uint32_t kRightCurveSmall0SE = 4;
            constexpr uint32_t kRightCurveSmall3SE = 5;
            constexpr uint32_t kRightCurveSmall0SW = 6;
            constexpr uint32_t kRightCurveSmall3SW = 7;
            constexpr uint32_t kRightCurveSmall0NW = 8;
            constexpr uint32_t kRightCurveSmall3NW = 9;
            constexpr uint32_t kRightCurveVerySmall0NE = 10;
            constexpr uint32_t kRightCurveVerySmall0SE = 11;
            constexpr uint32_t kRightCurveVerySmall0SW = 12;
            constexpr uint32_t kRightCurveVerySmall0NW = 13;
            constexpr uint32_t kTurnaround0NE = 14;
            constexpr uint32_t kTurnaround0SE = 15;
            constexpr uint32_t kTurnaround0SW = 16;
            constexpr uint32_t kTurnaround0NW = 17;
            constexpr uint32_t kStraightSlopeUp0NE = 18;
            constexpr uint32_t kStraightSlopeUp0SE = 19;
            constexpr uint32_t kStraightSlopeUp0SW = 20;
            constexpr uint32_t kStraightSlopeUp0NW = 21;
            constexpr uint32_t kStraightSlopeUp1NE = 22;
            constexpr uint32_t kStraightSlopeUp1SE = 23;
            constexpr uint32_t kStraightSlopeUp1SW = 24;
            constexpr uint32_t kStraightSlopeUp1NW = 25;
            constexpr uint32_t kStraightSteepSlopeUp0NE = 26;
            constexpr uint32_t kStraightSteepSlopeUp0SE = 27;
            constexpr uint32_t kStraightSteepSlopeUp0SW = 28;
            constexpr uint32_t kStraightSteepSlopeUp0NW = 29;
            constexpr uint32_t kSupportStraight0SE = 30;
            constexpr uint32_t kSupportConnectorStraight0SE = 31;
            constexpr uint32_t kSupportStraight0SW = 32;
            constexpr uint32_t kSupportConnectorStraight0SW = 33;
            constexpr uint32_t kSupportStraight0NW = 34;
            constexpr uint32_t kSupportConnectorStraight0NW = 35;
            constexpr uint32_t kSupportStraight0NE = 36;
            constexpr uint32_t kSupportConnectorStraight0NE = 37;
        }
    }
}
