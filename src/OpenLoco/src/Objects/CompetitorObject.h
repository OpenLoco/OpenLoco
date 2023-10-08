#pragma once

#include "Object.h"
#include "Types.hpp"
#include <array>
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
    struct CompetitorObject
    {
        static constexpr auto kObjectType = ObjectType::competitor;

        StringId var_00;         // 0x00
        StringId var_02;         // 0x02
        uint32_t var_04;         // 0x04
        uint32_t var_08;         // 0x08
        uint32_t emotions;       // 0x0C
        uint32_t images[9];      // 0x10
        uint8_t intelligence;    // 0x34
        uint8_t aggressiveness;  // 0x35
        uint8_t competitiveness; // 0x36
        uint8_t var_37;          // 0x37

        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
        void drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const;
        bool validate() const;
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(CompetitorObject) == 0x38);

    [[nodiscard]] StringId aiRatingToLevel(const uint8_t rating);
}
