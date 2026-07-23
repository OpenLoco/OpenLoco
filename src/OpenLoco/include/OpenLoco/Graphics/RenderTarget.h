#pragma once

#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>
#include <optional>

namespace OpenLoco::Gfx
{
    // TODO: Convert this to a handle once everything is implemented.
    // Depending on the rendering engine this could be a buffer on GPU or RAM.
    struct RenderTarget
    {
        uint8_t* bits;      // 0x00
        int32_t x;          // 0x04
        int32_t y;          // 0x08
        int32_t width;      // 0x0C
        int32_t height;     // 0x10
        int16_t pitch;      // 0x14 note: this is actually (pitch - width)
        uint16_t zoomLevel; // 0x16

        Ui::Rect getUiRect() const;
        Ui::Rect getDrawableRect() const;
    };

    std::optional<RenderTarget> clipRenderTarget(const RenderTarget& src, const Ui::Rect& newRect);
}
