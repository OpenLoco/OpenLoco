#pragma once

#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>

namespace OpenLoco::Gfx
{
    // GPU-based render target for hardware rendering
    struct GpuRenderTarget
    {
        uint32_t textureId;    // OpenGL texture ID
        uint32_t framebufferId; // OpenGL framebuffer ID
        uint32_t depthBufferId; // OpenGL depth buffer ID (optional)
        int16_t x;              // Position offset
        int16_t y;              // Position offset
        int16_t width;          // Width in pixels
        int16_t height;         // Height in pixels
        uint16_t zoomLevel;     // Zoom level (0 = 1:1, 1 = 2:1, etc.)
        bool hasDepth;          // Whether this RT has depth buffering

        GpuRenderTarget()
            : textureId(0)
            , framebufferId(0)
            , depthBufferId(0)
            , x(0)
            , y(0)
            , width(0)
            , height(0)
            , zoomLevel(0)
            , hasDepth(false)
        {
        }

        Ui::Rect getUiRect() const
        {
            return Ui::Rect(x, y, width, height);
        }

        Ui::Rect getDrawableRect() const
        {
            return Ui::Rect(0, 0, width, height);
        }

        bool isValid() const
        {
            return textureId != 0 && framebufferId != 0;
        }
    };
}
