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
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
        int32_t pitch; // note: this is actually (pitch - width)

        Ui::Rect getUiRect() const;
    };

    std::optional<RenderTarget> clipRenderTarget(const RenderTarget& src, const Ui::Rect& newRect);
}
