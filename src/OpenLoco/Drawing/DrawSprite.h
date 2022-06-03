#pragma once
#include "../Graphics/ImageId.h"
#include "../Ui/Types.hpp"

namespace OpenLoco::Gfx
{
    struct PaletteMap;
    struct G1Element;
    struct Context;
}

namespace OpenLoco::Drawing
{
    struct DrawSpritePosArgs
    {
        Ui::Point32 srcPos;
        Ui::Point32 dstPos;
        Ui::Size size;
    };
    struct DrawSpriteArgs
    {
        const Gfx::PaletteMap& palMap;
        const Gfx::G1Element& sourceImage;
        Ui::Point32 srcPos;
        Ui::Point32 dstPos;
        Ui::Size size;
        const Gfx::G1Element* noiseImage;
        DrawSpriteArgs(
            const Gfx::PaletteMap& _palMap, const Gfx::G1Element& _sourceImage, const Ui::Point32& _srcPos, const Ui::Point32& _dstPos, const Ui::Size& _size, const Gfx::G1Element* _noiseImage)
            : palMap(_palMap)
            , sourceImage(_sourceImage)
            , srcPos(_srcPos)
            , dstPos(_dstPos)
            , size(_size)
            , noiseImage(_noiseImage)
        {
        }
    };

    using DrawBlendOp = uint8_t;

    namespace BlendOp
    {
        constexpr DrawBlendOp none = 0;

        /**
         * Only supported by BITMAP. RLE images always encode transparency via the encoding.
         * Pixel value of 0 represents transparent.
         */
        constexpr DrawBlendOp transparent = 1 << 0;

        /**
         * Whether to use the pixel value from the source image.
         * This is usually only unset for glass images where there the src is only a transparency mask.
         */
        constexpr DrawBlendOp src = 1 << 1;

        /**
         * Whether to use the pixel value of the destination image for blending.
         * This is used for any image that filters the target image, e.g. glass or water.
         */
        constexpr DrawBlendOp dst = 1 << 2;

        /**
         * Whether to use the noise image to prevent draws on certain parts of the image.
         */
        constexpr DrawBlendOp noiseMask = 1 << 3;
    }

    DrawBlendOp getDrawBlendOp(const ImageId image, const DrawSpriteArgs& args);

    template<uint8_t TZoomLevel, bool TIsRLE>
    void drawSpriteToBuffer(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op);
}
