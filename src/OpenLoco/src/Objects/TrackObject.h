#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

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

    namespace Flags22
    {
        constexpr uint8_t unk_00 = 1 << 0;
        constexpr uint8_t unk_01 = 1 << 1;
        constexpr uint8_t unk_02 = 1 << 2;
    }

    namespace TrackPieceFlags
    {
        constexpr uint16_t diagonal = 1 << 0;
        constexpr uint16_t largeCurve = 1 << 1;
        constexpr uint16_t normalCurve = 1 << 2;
        constexpr uint16_t smallCurve = 1 << 3;
        constexpr uint16_t verySmallCurve = 1 << 4;
        constexpr uint16_t slope = 1 << 5;
        constexpr uint16_t steepSlope = 1 << 6;
        constexpr uint16_t oneSided = 1 << 7;
        constexpr uint16_t slopedCurve = 1 << 8;
        constexpr uint16_t sBend = 1 << 9;
        constexpr uint16_t junction = 1 << 10;
    }

#pragma pack(push, 1)
    struct TrackObject
    {
        static constexpr auto kObjectType = ObjectType::track;

        string_id name;
        uint16_t trackPieces;        // 0x02
        uint16_t stationTrackPieces; // 0x04
        uint8_t var_06;
        uint8_t numCompatible; // 0x07
        uint8_t numMods;       // 0x08
        uint8_t numSignals;    // 0x09
        uint8_t mods[4];       // 0x0A
        uint16_t var_0E;       // ?compatible signals bitset?
        uint16_t var_10;
        uint8_t pad_12[0x14 - 0x12];
        int16_t buildCostFactor;  // 0x14
        int16_t sellCostFactor;   // 0x16
        int16_t tunnelCostFactor; // 0x18
        uint8_t costIndex;        // 0x1A
        uint8_t var_1B;
        uint16_t curveSpeed;   // 0x1C
        uint32_t image;        // 0x1E
        uint16_t flags;        // 0x22
        uint8_t numBridges;    // 0x24
        uint8_t bridges[7];    // 0x25
        uint8_t numStations;   // 0x2C
        uint8_t stations[7];   // 0x2D
        uint8_t displayOffset; // 0x34
        uint8_t pad_35;

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(TrackObject) == 0x36);
}
