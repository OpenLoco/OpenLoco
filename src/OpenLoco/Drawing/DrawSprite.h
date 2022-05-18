namespace OpenLoco::Gfx
{
    struct PaletteMap;
    struct G1Element;
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
        DrawSpriteArgs(
            ImageId _image, const Gfx::PaletteMap& _palMap, const Gfx::G1Element& _sourceImage, int32_t _srcX, int32_t _srcY, int32_t _width,
            int32_t _height, uint8_t* _destinationBits)
            : image(_image)
            , palMap(_palMap)
            , sourceImage(_sourceImage)
            , srcX(_srcX)
            , srcY(_srcY)
            , width(_width)
            , height(_height)
            , destinationBits(_destinationBits)
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
    constexpr DrawBlendOp BLEND_DST = 2 << 2;

}
