#pragma once
#include "Graphics/ImageId.h"
#include "Graphics/PaletteMap.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <OpenLoco/Engine/Ui/Size.hpp>

namespace OpenLoco::Gfx
{
    struct G1Element;
    struct RenderTarget;
}

namespace OpenLoco::Gfx
{
    struct DrawSpritePosArgs
    {
        Ui::Point32 srcPos;
        Ui::Point32 dstPos;
        Ui::Size size;
    };
    struct DrawSpriteArgs
    {
        const Gfx::PaletteMap::View palMap;
        const Gfx::G1Element& sourceImage;
        Ui::Point32 srcPos;
        Ui::Point32 dstPos;
        Ui::Size size;
        const Gfx::G1Element* noiseImage;
        DrawSpriteArgs(
            const Gfx::PaletteMap::View _palMap, const Gfx::G1Element& _sourceImage, const Ui::Point32& _srcPos, const Ui::Point32& _dstPos, const Ui::Size& _size, const Gfx::G1Element* _noiseImage)
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
    void drawSpriteToBuffer(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op);
}
