#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
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

    enum class RoadStationFlags : uint8_t
    {
        none = 0U,
        recolourable = 1U << 0,
        passenger = 1U << 1,
        freight = 1U << 2,
        roadEnd = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(RoadStationFlags);

#pragma pack(push, 1)
    struct RoadStationObject
    {
        static constexpr auto kObjectType = ObjectType::roadStation;

        string_id name;     // 0x00
        uint8_t paintStyle; // 0x02
        uint8_t pad_03;
        uint16_t roadPieces;     // 0x04
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint8_t costIndex;       // 0x0A
        RoadStationFlags flags;  // 0x0B
        uint32_t image;          // 0x0C
        uint32_t var_10;
        uint32_t var_14;
        uint32_t var_18;
        uint32_t var_1C;
        uint8_t numCompatible; // 0x20
        uint8_t mods[7];       // 0x21
        uint16_t designedYear; // 0x28
        uint16_t obsoleteYear; // 0x2A
        uint8_t var_2C;
        uint8_t pad_2D;
        uint32_t var_2E[16];

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();

        constexpr bool hasFlags(RoadStationFlags flagsToTest) const
        {
            return (flags & flagsToTest) != RoadStationFlags::none;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(RoadStationObject) == 0x6E);

    namespace RoadStation::ImageIds
    {
        constexpr uint32_t preview_image = 0;
        constexpr uint32_t preview_image_windows = 1;
    }
}
