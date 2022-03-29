#pragma once
#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct WaterObject
    {
        static constexpr auto kObjectType = ObjectType::water;

        string_id name;
        uint8_t cost_index; // 0x02
        uint8_t var_03;
        int8_t cost_factor; //0x04
        uint8_t var_05;
        uint32_t image; // 0x06
        uint32_t var_0A;

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(WaterObject) == 0xE);

    namespace Water::ImageIds
    {
        constexpr uint32_t toolbar_terraform_water = 42;
    }
}
