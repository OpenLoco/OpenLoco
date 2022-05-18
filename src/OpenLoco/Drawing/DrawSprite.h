#pragma once
#include "../Graphics/ImageId.h"

namespace OpenLoco::Gfx
{
    struct PaletteMap;
    struct G1Element;
    struct Context;
}

namespace OpenLoco::Drawing
{
    struct DrawSpriteArgs
    {
        ImageId image;
        const Gfx::PaletteMap& palMap;
        const Gfx::G1Element& sourceImage;
        int32_t srcX;
        int32_t srcY;
        int32_t width;
        int32_t height;
        uint8_t* destinationBits;
        const Gfx::G1Element* treeWiltImage;
        DrawSpriteArgs(
            ImageId _image, const Gfx::PaletteMap& _palMap, const Gfx::G1Element& _sourceImage, int32_t _srcX, int32_t _srcY, int32_t _width,
            int32_t _height, uint8_t* _destinationBits, const Gfx::G1Element* _treeWiltImage)
            : image(_image)
            , palMap(_palMap)
            , sourceImage(_sourceImage)
            , srcX(_srcX)
            , srcY(_srcY)
            , width(_width)
            , height(_height)
            , destinationBits(_destinationBits)
            , treeWiltImage(_treeWiltImage)
        {
        }
    };

    void drawSpriteToBufferBMP(Gfx::Context& context, const DrawSpriteArgs& args);

    using DrawBlendOp = uint8_t;

    constexpr DrawBlendOp BLEND_NONE = 0;

    /**
     * Only supported by BITMAP. RLE images always encode transparency via the encoding.
     * Pixel value of 0 represents transparent.
     */
    constexpr DrawBlendOp BLEND_TRANSPARENT = 1 << 0;

    /**
     * Whether to use the pixel value from the source image.
     * This is usually only unset for glass images where there the src is only a transparency mask.
     */
    constexpr DrawBlendOp BLEND_SRC = 1 << 1;

    /**
     * Whether to use the pixel value of the destination image for blending.
     * This is used for any image that filters the target image, e.g. glass or water.
     */
    constexpr DrawBlendOp BLEND_DST = 1 << 2;

    /**
     * Whether to use the tree wilt image to prevent draws on certain parts of the image.
     */
    constexpr DrawBlendOp BLEND_TREEWILT = 1 << 3;

    DrawBlendOp getDrawBlendOp(const DrawSpriteArgs& args);
}
