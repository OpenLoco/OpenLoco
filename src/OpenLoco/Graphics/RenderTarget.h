#pragma once

#include "../Ui/Rect.h"
#include <cstdint>
#include <optional>

namespace OpenLoco::Gfx
{
#pragma pack(push, 1)

    // TODO: Convert this to a handle once everything is implemented.
    // Depending on the rendering engine this could be a buffer on GPU or RAM.
    struct RenderTarget
    {
        uint8_t* bits;      // 0x00
        int16_t x;          // 0x04
        int16_t y;          // 0x06
        int16_t width;      // 0x08
        int16_t height;     // 0x0A
        int16_t pitch;      // 0x0C note: this is actually (pitch - width)
        uint16_t zoomLevel; // 0x0E

        Ui::Rect getUiRect() const;
        Ui::Rect getDrawableRect() const;
    };

#pragma pack(pop)

    RenderTarget& getScreenRT();

    std::optional<Gfx::RenderTarget> clipRenderTarget(const Gfx::RenderTarget& src, const Ui::Rect& newRect);

}
