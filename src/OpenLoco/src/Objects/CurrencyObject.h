#pragma once

#include "Object.h"
#include "Types.hpp"
#include <OpenLoco/Core/Span.hpp>

namespace OpenLoco
{
    namespace Gfx
    {
        struct RenderTarget;
    }

#pragma pack(push, 1)
    struct CurrencyObject
    {
        static constexpr auto kObjectType = ObjectType::currency;

        string_id name;         // 0x00
        string_id prefixSymbol; // 0x02
        string_id suffixSymbol; // 0x04
        uint32_t objectIcon;    // 0x06
        uint8_t separator;      // 0x0A
        uint8_t factor;         // 0x0B

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(CurrencyObject) == 0xC);
}
