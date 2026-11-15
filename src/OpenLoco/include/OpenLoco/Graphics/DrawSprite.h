#pragma once
#include "Graphics/ImageId.h"
#include "Graphics/PaletteMap.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <OpenLoco/Engine/Ui/Size.hpp>
#include <OpenLoco/ZoomLevel.hpp>
#include <algorithm>

namespace OpenLoco::Gfx
{
    struct G1Element;
    struct RenderTarget;

    // The offset, in source pixels, from the start of a block of source pixels to the pixel that
    // represents it when a sprite is drawn zoomed out. The middle of the block is used, unless the
    // block is longer than the (clipped) sprite (then the middle of the sprite is used, or the start
    // of the block if there is not even room for that). Sampling the top left pixel instead would
    // sample the transparent corner of the diamond shaped sprites that the map is made of (surface
    // tiles, water, ...) and leave the map full of holes at the furthest zoom out levels. The
    // returned offset is small enough to keep every sample inside the sprite.
    constexpr int32_t zoomOutSampleOffset(const int32_t blockSize, const int32_t clippedSize)
    {
        return std::min({ blockSize / 2, clippedSize / 2, (clippedSize - 1) % blockSize });
    }

    struct DrawSpritePosArgs
    {
        Ui::Point srcPos;
        Ui::Point dstPos;
        Ui::Size size;
    };

    struct DrawSpriteArgs
    {
        const PaletteMap::View palMap;
        const G1Element& sourceImage;
        Ui::Point srcPos;
        Ui::Point dstPos;
        Ui::Size size;
        const G1Element* noiseImage;
        DrawSpriteArgs(
            const PaletteMap::View _palMap, const G1Element& _sourceImage, const Ui::Point& _srcPos, const Ui::Point& _dstPos, const Ui::Size& _size, const G1Element* _noiseImage)
            : palMap(_palMap)
            , sourceImage(_sourceImage)
            , srcPos(_srcPos)
            , dstPos(_dstPos)
            , size(_size)
            , noiseImage(_noiseImage)
        {
        }
    };

    enum class DrawBlendOp : uint8_t
    {
        none = 0U,

        /**
         * Only supported by BITMAP. RLE images always encode transparency via the encoding.
         * Pixel value of 0 represents transparent.
         */
        transparent = 1U << 0,

        /**
         * Whether to use the pixel value from the source image.
         * This is usually only unset for glass images where there the src is only a transparency mask.
         */
        src = 1U << 1,

        /**
         * Whether to use the pixel value of the destination image for blending.
         * This is used for any image that filters the target image, e.g. glass or water.
         */
        dst = 1U << 2,

        /**
         * Whether to use the noise image to prevent draws on certain parts of the image.
         */
        noiseMask = 1U << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(DrawBlendOp);

    DrawBlendOp getDrawBlendOp(const ImageId image, const DrawSpriteArgs& args);

    template<uint8_t TZoomLevel, bool TIsRLE>
    void drawSpriteToBuffer(const RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op);

    template<bool TIsRLE>
    void drawSpriteToBufferMagnify(const RenderTarget& rt, ZoomLevel zoom, const DrawSpriteArgs& args, const DrawBlendOp op);
}
