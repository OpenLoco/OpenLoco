#pragma once

#include "Graphics/PaletteMap.h"
#include "ImageId.h"
#include "Types.hpp"
#include "Ui/Rect.h"
#include "Ui/UiTypes.hpp"
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

    struct G1Element32
    {
        uint32_t offset;    // 0x00
        int16_t width;      // 0x04
        int16_t height;     // 0x06
        int16_t xOffset;    // 0x08
        int16_t yOffset;    // 0x0A
        uint16_t flags;     // 0x0C
        int16_t zoomOffset; // 0x0E
    };

    // A version that can be 64-bit when ready...
    struct G1Element
    {
        uint8_t* offset = nullptr;
        int16_t width = 0;
        int16_t height = 0;
        int16_t xOffset = 0;
        int16_t yOffset = 0;
        uint16_t flags = 0;
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
    };

#pragma pack(pop)
    namespace G1ElementFlags
    {
        constexpr uint16_t hasTransparancy = 1 << 0; // Image data contains transparent sections (when not set data is plain bmp)
        constexpr uint16_t unk1 = 1 << 1;            // Unknown function not used on any entry
        constexpr uint16_t isRLECompressed = 1 << 2; // Image data is encoded using CS's form of run length encoding
        constexpr uint16_t isR8G8B8Palette = 1 << 3; // Image data is a sequence of palette entries R8G8B8
        constexpr uint16_t hasZoomSprites = 1 << 4;  // Use a different sprite for higher zoom levels
        constexpr uint16_t noZoomDraw = 1 << 5;      // Does not get drawn at higher zoom levels (only zoom 0)
    }

    namespace ImageIdFlags
    {
        constexpr uint32_t remap = 1 << 29;
        constexpr uint32_t translucent = 1 << 30;
        constexpr uint32_t remap2 = 1 << 31;
    }

    void loadG1();
    void initialiseCharacterWidths();
    void initialiseNoiseMaskMap();
    void clear(RenderTarget& rt, uint32_t fill);
    void clearSingle(RenderTarget& rt, uint8_t paletteId);

    int16_t clipString(int16_t width, char* string);
    uint16_t getStringWidth(const char* buffer);
    uint16_t getMaxStringWidth(const char* buffer);

    Ui::Point drawString(RenderTarget& rt, int16_t x, int16_t y, AdvancedColour colour, void* str);

    int16_t drawStringLeftWrapped(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringLeft(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringLeft(
        RenderTarget& rt,
        Ui::Point* origin,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringLeftClipped(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringRight(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringRightUnderline(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args);
    void drawStringLeftUnderline(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentred(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredClipped(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    uint16_t drawStringCentredWrapped(
        RenderTarget& rt,
        Ui::Point& origin,
        uint16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredRaw(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        const void* args);
    void drawStringYOffsets(RenderTarget& rt, const Ui::Point& loc, AdvancedColour colour, const void* args, const int8_t* yOffsets);
    uint16_t getStringWidthNewLined(const char* buffer);
    std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth);

    void fillRect(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
    void drawRect(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour);
    void fillRectInset(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags);
    void drawRectInset(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags);
    void drawLine(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
    void drawImage(Gfx::RenderTarget* rt, int16_t x, int16_t y, uint32_t image);
    void drawImage(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image);
    void drawImageSolid(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex);
    void drawImagePaletteSet(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage);
    [[nodiscard]] uint32_t recolour(uint32_t image);
    [[nodiscard]] uint32_t recolour(uint32_t image, Colour colour);
    [[nodiscard]] uint32_t recolour(uint32_t image, ExtColour colour);
    [[nodiscard]] uint32_t recolour2(uint32_t image, Colour colour1, Colour colour2);
    [[nodiscard]] uint32_t recolour2(uint32_t image, ColourScheme colourScheme);
    [[nodiscard]] uint32_t recolourTranslucent(uint32_t image, ExtColour colour);
    [[nodiscard]] ImageId applyGhostToImage(uint32_t imageIndex);
    [[nodiscard]] constexpr uint32_t getImageIndex(uint32_t imageId) { return imageId & 0x7FFFF; }

    void invalidateScreen();
    void setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void drawDirtyBlocks();
    void render();

    void redrawScreenRect(Ui::Rect rect);
    void redrawScreenRect(int16_t left, int16_t top, int16_t right, int16_t bottom);

    G1Element* getG1Element(uint32_t id);

    // 0x0112C876
    int16_t getCurrentFontSpriteBase();
    void setCurrentFontSpriteBase(int16_t base);

    Drawing::SoftwareDrawingEngine& getDrawingEngine();

    void loadCurrency();
    void loadPalette();
}
