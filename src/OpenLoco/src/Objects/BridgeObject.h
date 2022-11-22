#pragma once

#include "Core/Span.hpp"
#include "Object.h"
#include "Speed.hpp"
#include "Types.hpp"

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    struct BridgeObject
    {
        static constexpr auto kObjectType = ObjectType::bridge;

        string_id name;
        uint8_t noRoof; // 0x02
        uint8_t pad_03[0x06 - 0x03];
        uint16_t var_06;            // 0x06
        uint8_t spanLength;         // 0x08
        uint8_t pillarSpacing;      // 0x09
        Speed16 maxSpeed;           // 0x0A
        uint8_t maxHeight;          // 0x0C
        uint8_t costIndex;          // 0x0D
        int16_t baseCostFactor;     // 0x0E
        int16_t heightCostFactor;   // 0x10
        int16_t sellCostFactor;     // 0x12
        uint16_t disabledTrackCfg;  // 0x14
        uint32_t image;             // 0x16
        uint8_t trackNumCompatible; // 0x1A
        uint8_t trackMods[7];       // 0x1B
        uint8_t roadNumCompatible;  // 0x22
        uint8_t roadMods[7];        // 0x23
        uint16_t designedYear;      // 0x2A

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(BridgeObject) == 0x2C);
}
