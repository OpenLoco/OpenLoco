#pragma once

#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>
#include <optional>

namespace OpenLoco::Gfx
{
    // TODO: Convert this to a handle once everything is implemented.
    // Depending on the rendering engine this could be a buffer on GPU or RAM.
    // All coordinates are in screen space.
    struct RenderTarget
    {
        uint8_t* bits;
        int16_t x;
        int16_t y;
        int16_t width;
        int16_t height;
        int16_t pitch; // note: this is actually (pitch - width)

        Ui::Rect getUiRect() const;
    };

    std::optional<RenderTarget> clipRenderTarget(const RenderTarget& src, const Ui::Rect& newRect);
}
