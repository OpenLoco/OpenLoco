#pragma once

#include "../Core/Optional.hpp"
#include "../OpenLoco.h"
#include "../Types.hpp"
#include "../Ui/Rect.h"
#include "../Ui/Types.hpp"
#include <cstdint>

namespace OpenLoco
{
    using Colour_t = uint8_t;
}

namespace OpenLoco::Gfx
{
#pragma pack(push, 1)

    struct Context
    {
        uint8_t* bits;       // 0x00
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t width;       // 0x08
        int16_t height;      // 0x0A
        int16_t pitch;       // 0x0C note: this is actually (pitch - width)
        uint16_t zoom_level; // 0x0E

        Ui::Rect getUiRect() const;
        Ui::Rect getDrawableRect() const;
    };

    Context& screenContext();

    struct G1Header
    {
        uint32_t num_entries;
        uint32_t total_size;
    };

    struct G1Element32
    {
        uint32_t offset;  // 0x00
        int16_t width;    // 0x04
        int16_t height;   // 0x06
        int16_t x_offset; // 0x08
        int16_t y_offset; // 0x0A
        uint16_t flags;   // 0x0C
        int16_t unused;   // 0x0E
    };

    // A version that can be 64-bit when ready...
    struct G1Element
    {
        uint8_t* offset = nullptr;
        int16_t width = 0;
        int16_t height = 0;
        int16_t x_offset = 0;
        int16_t y_offset = 0;
        uint16_t flags = 0;
        int16_t unused = 0;

        G1Element() = default;
        G1Element(const G1Element32& src)
            : offset((uint8_t*)src.offset)
            , width(src.width)
            , height(src.height)
            , x_offset(src.x_offset)
            , y_offset(src.y_offset)
            , flags(src.flags)
            , unused(src.unused)
        {
        }
    };

#pragma pack(pop)
    namespace ImageIdFlags
    {
        constexpr uint32_t remap = 1 << 29;
        constexpr uint32_t translucent = 1 << 30;
        constexpr uint32_t remap2 = 1 << 31;
    }

    /**
     * Represents an 8-bit indexed map that maps from one palette index to another.
     */
    struct PaletteMap
    {
    private:
        uint8_t* _data{};
        uint32_t _dataLength{};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
        uint16_t _numMaps;
#pragma clang diagnostic pop
        uint16_t _mapLength;

    public:
        static const PaletteMap& getDefault();

        PaletteMap() = default;

        PaletteMap(uint8_t* data, uint16_t numMaps, uint16_t mapLength)
            : _data(data)
            , _dataLength(numMaps * mapLength)
            , _numMaps(numMaps)
            , _mapLength(mapLength)
        {
        }

        template<std::size_t TSize>
        PaletteMap(uint8_t (&map)[TSize])
            : _data(map)
            , _dataLength(static_cast<uint32_t>(std::size(map)))
            , _numMaps(1)
            , _mapLength(static_cast<uint16_t>(std::size(map)))
        {
        }

        uint8_t& operator[](size_t index);
        uint8_t operator[](size_t index) const;
        uint8_t* data() const { return _data; }
        uint8_t blend(uint8_t src, uint8_t dst) const;
        void copy(size_t dstIndex, const PaletteMap& src, size_t srcIndex, size_t length);
    };

    std::optional<uint32_t> getPaletteG1Index(Colour_t paletteId);
    std::optional<PaletteMap> getPaletteMapForColour(Colour_t paletteId);

    Context& screenContext();

    void loadG1();
    void clear(Context& context, uint32_t fill);
    void clearSingle(Context& context, uint8_t paletteId);

    int16_t clipString(int16_t width, char* string);
    uint16_t getStringWidth(const char* buffer);
    uint16_t getMaxStringWidth(const char* buffer);

    Ui::Point drawString(Context& context, int16_t x, int16_t y, uint8_t colour, void* str);

    int16_t drawString_495224(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494B3F(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494B3F(
        Context& context,
        Ui::Point* origin,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494BBF(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawString_494C78(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringUnderline(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args);
    void drawStringLeftUnderline(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentred(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredClipped(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    uint16_t drawStringCentredWrapped(
        Context& context,
        Ui::Point& origin,
        uint16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args = nullptr);
    void drawStringCentredRaw(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        const void* args);
    uint16_t getStringWidthNewLined(const char* buffer);
    std::pair<uint16_t, uint16_t> wrapString(const char* buffer, uint16_t stringWidth);

    void fillRect(Gfx::Context& context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
    void drawRect(Gfx::Context& context, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour);
    void fillRectInset(Gfx::Context& context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags);
    void drawRectInset(Gfx::Context& context, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags);
    void drawLine(Gfx::Context& context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour);
    void drawImage(Gfx::Context* context, int16_t x, int16_t y, uint32_t image);
    void drawImageSolid(Gfx::Context* context, int16_t x, int16_t y, uint32_t image, uint8_t palette_index);
    void drawImagePaletteSet(Gfx::Context* context, int16_t x, int16_t y, uint32_t image, uint8_t* palette);
    [[nodiscard]] uint32_t recolour(uint32_t image);
    [[nodiscard]] uint32_t recolour(uint32_t image, uint8_t colour);
    [[nodiscard]] uint32_t recolour2(uint32_t image, uint8_t colour1, uint8_t colour2);
    [[nodiscard]] uint32_t recolour2(uint32_t image, ColourScheme colourScheme);
    [[nodiscard]] uint32_t recolourTranslucent(uint32_t image, uint8_t colour);
    [[nodiscard]] uint32_t applyGhostToImage(uint32_t imageId);

    void invalidateScreen();
    void setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void drawDirtyBlocks();
    void render();

    void redrawScreenRect(Ui::Rect rect);
    void redrawScreenRect(int16_t left, int16_t top, int16_t right, int16_t bottom);

    std::optional<Gfx::Context> clipContext(const Gfx::Context& src, const Ui::Rect& newRect);

    G1Element* getG1Element(uint32_t id);

    void setCurrentFontSpriteBase(int16_t value);
}
