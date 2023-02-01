#pragma once

#include "Graphics/PaletteMap.h"
#include "ImageId.h"
#include "Types.hpp"
#include "Ui/Rect.h"
#include "Ui/UiTypes.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <array>
#include <cstdint>
#include <optional>

namespace OpenLoco
{
    using PaletteIndex_t = uint8_t;
    struct AdvancedColour;
    enum class ExtColour : uint8_t;
}

namespace OpenLoco::Drawing
{
    class SoftwareDrawingEngine;
}

namespace OpenLoco::Gfx
{
    struct RenderTarget;

    namespace G1ExpectedCount
    {
        constexpr uint32_t kDisc = 0x101A; // And GOG
        constexpr uint32_t kSteam = 0x0F38;
        constexpr uint32_t kObjects = 0x40000;
    }
#pragma pack(push, 1)

    struct G1Header
    {
        uint32_t numEntries;
        uint32_t totalSize;
    };

    enum class G1ElementFlags : uint16_t
    {
        none = 0U,
        hasTransparancy = 1U << 0,   // Image data contains transparent sections (when not set data is plain bmp)
        unk1 = 1U << 1,              // Unknown function not used on any entry
        isRLECompressed = 1U << 2,   // Image data is encoded using CS's form of run length encoding
        isR8G8B8Palette = 1U << 3,   // Image data is a sequence of palette entries R8G8B8
        hasZoomSprites = 1U << 4,    // Use a different sprite for higher zoom levels
        noZoomDraw = 1U << 5,        // Does not get drawn at higher zoom levels (only zoom 0)
        duplicatePrevious = 1U << 6, // Duplicates the previous element but with adjusted x/y offsets
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(G1ElementFlags);

    struct G1Element32
    {
        uint32_t offset;      // 0x00
        int16_t width;        // 0x04
        int16_t height;       // 0x06
        int16_t xOffset;      // 0x08
        int16_t yOffset;      // 0x0A
        G1ElementFlags flags; // 0x0C
        int16_t zoomOffset;   // 0x0E
    };

    // A version that can be 64-bit when ready...
    struct G1Element
    {
        uint8_t* offset = nullptr;
        int16_t width = 0;
        int16_t height = 0;
        int16_t xOffset = 0;
        int16_t yOffset = 0;
        G1ElementFlags flags = G1ElementFlags::none;
        int16_t zoomOffset = 0;

        G1Element() = default;
        G1Element(const G1Element32& src)
            : offset((uint8_t*)src.offset)
            , width(src.width)
            , height(src.height)
            , xOffset(src.xOffset)
            , yOffset(src.yOffset)
            , flags(src.flags)
            , zoomOffset(src.zoomOffset)
        {
        }

        constexpr bool hasFlags(G1ElementFlags flagsToTest) const
        {
            return (flags & flagsToTest) != G1ElementFlags::none;
        }
    };

#pragma pack(pop)

    struct ImageExtents
    {
        uint8_t width;
        uint8_t heightNegative;
        uint8_t heightPositive;
    };

    namespace ImageIdFlags
    {
        constexpr uint32_t remap = 1 << 29;
        constexpr uint32_t translucent = 1 << 30;
        constexpr uint32_t remap2 = 1 << 31;
    }

    void loadG1();
    void initialiseCharacterWidths();
    void initialiseNoiseMaskMap();

    [[nodiscard]] uint32_t recolour(uint32_t image);
    [[nodiscard]] uint32_t recolour(uint32_t image, Colour colour);
    [[nodiscard]] uint32_t recolour(uint32_t image, ExtColour colour);
    [[nodiscard]] uint32_t recolour2(uint32_t image, Colour colour1, Colour colour2);
    [[nodiscard]] uint32_t recolour2(uint32_t image, ColourScheme colourScheme);
    [[nodiscard]] uint32_t recolourTranslucent(uint32_t image, ExtColour colour);
    [[nodiscard]] ImageId applyGhostToImage(uint32_t imageIndex);
    [[nodiscard]] constexpr uint32_t getImageIndex(uint32_t imageId) { return imageId & 0x7FFFF; }

    // Invalidates the entire screen.
    void invalidateScreen();

    // Invalidate a region of the screen.
    void invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom);

    // Renders all invalidated regions the next frame.
    void render();

    // Renders a region of the screen.
    void render(Ui::Rect rect);
    void render(int16_t left, int16_t top, int16_t right, int16_t bottom);

    // Renders all invalidated regions and processes new messages.
    void renderAndUpdate();

    G1Element* getG1Element(uint32_t id);

    Drawing::SoftwareDrawingEngine& getDrawingEngine();

    void loadCurrency();
    void loadPalette();

    ImageExtents getImagesMaxExtent(const ImageId baseImageId, const size_t numImages);
}
