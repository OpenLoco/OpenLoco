#include "Gfx.h"
#include "../Config.h"
#include "../Console.h"
#include "../Drawing/DrawSprite.h"
#include "../Drawing/SoftwareDrawingEngine.h"
#include "../Environment.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/LanguageFiles.h"
#include "../Localisation/StringManager.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Stream.hpp"
#include "Colour.h"
#include "ImageIds.h"
#include "PaletteMap.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Utility;
using namespace OpenLoco::Drawing;
using namespace OpenLoco::Ui;

namespace OpenLoco::Gfx
{
    namespace TextDrawFlags
    {
        constexpr uint8_t inset = (1ULL << 0);
        constexpr uint8_t outline = (1ULL << 1);
        constexpr uint8_t dark = (1ULL << 2);
        constexpr uint8_t extraDark = (1ULL << 3);
    }

    constexpr uint32_t kG1CountTemporary = 0x1000;

    static loco_global<G1Element[G1ExpectedCount::kDisc + kG1CountTemporary + G1ExpectedCount::kObjects], 0x9E2424> _g1Elements;
    // 0x009DA3E0
    // Originally 0x009DA3E0 was an array of the image data pointers setup within 0x00452336
    // We have removed that step and instead work directly on the images.
    static constexpr std::array<uint32_t, 8> _noiseMaskImages = {
        ImageIds::null,
        ImageIds::noise_mask_1,
        ImageIds::noise_mask_2,
        ImageIds::noise_mask_3,
        ImageIds::noise_mask_4,
        ImageIds::noise_mask_5,
        ImageIds::noise_mask_6,
        ImageIds::noise_mask_7,
    };

    static std::unique_ptr<std::byte[]> _g1Buffer;

    static loco_global<uint16_t, 0x112C824> _currentFontFlags;
    static loco_global<int16_t, 0x0112C876> _currentFontSpriteBase;
    static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;
    static loco_global<AdvancedColour[4], 0x1136594> _windowColours;
    loco_global<uint32_t, 0x00E04324> _E04324;
    loco_global<const uint8_t*, 0x009DA3D8> _noiseMaskImageData;

    static PaletteIndex_t _textColours[8] = { 0 };

    // 0x004FFAE8
    ImageId applyGhostToImage(uint32_t imageIndex)
    {
        if (Config::get().constructionMarker)
        {
            return ImageId(imageIndex).withTranslucency(ExtColour::unk31);
        }
        else
        {
            return ImageId(imageIndex, ExtColour::unk2C);
        }
    }

    static std::vector<G1Element> convertElements(const std::vector<G1Element32>& elements32)
    {
        auto elements = std::vector<G1Element>();
        elements.reserve(elements32.size());
        std::transform(
            elements32.begin(),
            elements32.end(),
            std::back_inserter(elements),
            [](G1Element32 src) { return G1Element(src); });
        return elements;
    }

    // 0x0044733C
    void loadG1()
    {
        auto g1Path = Environment::getPath(Environment::PathId::g1);
        std::ifstream stream(g1Path, std::ios::in | std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("Opening g1 file failed.");
        }

        G1Header header;
        if (!readData(stream, header))
        {
            throw std::runtime_error("Reading g1 file header failed.");
        }

        if (header.numEntries != G1ExpectedCount::kDisc)
        {
            std::cout << "G1 element count doesn't match expected value: ";
            std::cout << "Expected " << G1ExpectedCount::kDisc << "; Got " << header.numEntries << std::endl;
            if (header.numEntries == G1ExpectedCount::kSteam)
            {
                std::cout << "Got Steam G1.DAT variant, will fix elements automatically." << std::endl;
            }
        }

        // Read element headers
        auto elements32 = std::vector<G1Element32>(header.numEntries);
        if (!readData(stream, elements32.data(), header.numEntries))
        {
            throw std::runtime_error("Reading g1 element headers failed.");
        }
        auto elements = convertElements(elements32);

        // Read element data
        auto elementData = std::make_unique<std::byte[]>(header.totalSize);
        if (!readData(stream, elementData.get(), header.totalSize))
        {
            throw std::runtime_error("Reading g1 elements failed.");
        }
        stream.close();

        // The steam G1.DAT is missing two localised tutorial icons, and a smaller font variant
        // This code copies the closest variants into their place, and moves other elements accordingly
        if (header.numEntries == G1ExpectedCount::kSteam)
        {
            elements.resize(G1ExpectedCount::kDisc);

            // Extra two tutorial images
            std::copy_n(&elements[3549], header.numEntries - 3549, &elements[3551]);
            std::copy_n(&elements[3551], 1, &elements[3549]);
            std::copy_n(&elements[3551], 1, &elements[3550]);

            // Extra font variant
            std::copy_n(&elements[1788], 223, &elements[3898]);
        }

        // Adjust memory offsets
        for (auto& element : elements)
        {
            element.offset += (uintptr_t)elementData.get();
        }

        _g1Buffer = std::move(elementData);
        std::copy(elements.begin(), elements.end(), _g1Elements.get());
    }

    // 0x004949BC
    void initialiseCharacterWidths()
    {
        struct FontEntry
        {
            int16_t offset;
            int8_t widthFudge;
        };
        constexpr std::array<FontEntry, 4> fonts = {
            FontEntry{ Font::medium_normal, -1 },
            FontEntry{ Font::medium_bold, -1 },
            FontEntry{ Font::small, -1 },
            FontEntry{ Font::large, 1 },
        };

        for (const auto& font : fonts)
        {
            // Supported character range is from 32 -> 255
            for (auto i = 0; i < 224; ++i)
            {
                auto c = i + 32;
                // TODO: Use an indirection to convert from character to imageId
                auto* element = getG1Element(ImageIds::characters_medium_normal_space + i + font.offset);
                if (element == nullptr)
                {
                    continue;
                }
                auto width = element->width + font.widthFudge;
                // Characters from 123 to 150 are unused
                // Unsure why this zeros it out though since a negative width isn't an issue
                if (c >= '{' && c <= '\x96')
                {
                    width = 0;
                }
                _characterWidths[font.offset + i] = width;
            }
        }
        // Vanilla setup scrolling text related globals here (unused)
    }

    // 0x00452336
    void initialiseNoiseMaskMap()
    {
        call(0x00452336);
    }

    // 0x00447485
    // edi: rt
    // ebp: fill
    void clear(RenderTarget& rt, uint32_t fill)
    {
        int32_t w = rt.width / (1 << rt.zoomLevel);
        int32_t h = rt.height / (1 << rt.zoomLevel);
        uint8_t* ptr = rt.bits;

        for (int32_t y = 0; y < h; y++)
        {
            std::fill_n(ptr, w, fill);
            ptr += w + rt.pitch;
        }
    }

    void clearSingle(RenderTarget& rt, uint8_t paletteId)
    {
        auto fill = (paletteId << 24) | (paletteId << 16) | (paletteId << 8) | paletteId;
        clear(rt, fill);
    }

    // 0x004957C4
    int16_t clipString(int16_t width, char* string)
    {
        if (width < 6)
        {
            *string = '\0';
            return 0;
        }

        // If width of the full string is less than allowed width then we don't need to clip
        auto clippedWidth = getStringWidth(string);
        if (clippedWidth <= width)
        {
            return clippedWidth;
        }

        // Append each character 1 by 1 with an ellipsis on the end until width is exceeded
        std::string bestString;
        std::string curString;

        for (const auto* chr = string; *chr != '\0'; ++chr)
        {
            curString.push_back(*chr);
            switch (*chr)
            {
                case ControlCodes::moveX:
                    curString.push_back(*++chr);
                    break;

                case ControlCodes::adjustPalette:
                case 3:
                case 4:
                    curString.push_back(*++chr);
                    break;

                case ControlCodes::newline:
                case ControlCodes::newlineSmaller:
                case ControlCodes::Font::small:
                case ControlCodes::Font::large:
                case ControlCodes::Font::bold:
                case ControlCodes::Font::regular:
                case ControlCodes::Font::outline:
                case ControlCodes::Font::outlineOff:
                case ControlCodes::windowColour1:
                case ControlCodes::windowColour2:
                case ControlCodes::windowColour3:
                case ControlCodes::windowColour4:
                    break;

                case ControlCodes::inlineSpriteStr:
                    curString.push_back(*++chr);
                    curString.push_back(*++chr);
                    curString.push_back(*++chr);
                    curString.push_back(*++chr);
                    break;

                case ControlCodes::newlineXY:
                    curString.push_back(*++chr);
                    curString.push_back(*++chr);
                    break;

                default:
                    if (*chr <= 0x16)
                    {
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                    }
                    else if (*chr < 32)
                    {
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                    }
                    break;
            }

            auto ellipseString = curString;
            ellipseString.append("...");

            auto ellipsedWidth = getStringWidth(ellipseString.c_str());
            if (ellipsedWidth < width)
            {
                // Keep best string with ellipse
                bestString = ellipseString;
            }
            else
            {
                strcpy(string, bestString.c_str());
                return getStringWidth(string);
            }
        }
        return getStringWidth(string);
    }

    /**
     * 0x00495685
     *
     * @param buffer @<esi>
     * @return width @<cx>
     */
    uint16_t getStringWidth(const char* buffer)
    {
        uint16_t width = 0;
        const uint8_t* str = reinterpret_cast<const uint8_t*>(buffer);
        auto fontSpriteBase = getCurrentFontSpriteBase();

        while (*str != (uint8_t)0)
        {
            const uint8_t chr = *str;
            str++;

            if (chr >= 32)
            {
                width += _characterWidths[chr - 32 + fontSpriteBase];
                continue;
            }

            switch (chr)
            {
                case ControlCodes::moveX:
                    width = *str;
                    str++;
                    break;

                case ControlCodes::adjustPalette:
                case 3:
                case 4:
                    str++;
                    break;

                case ControlCodes::newline:
                case ControlCodes::newlineSmaller:
                    continue;

                case ControlCodes::Font::small:
                    fontSpriteBase = Font::small;
                    break;

                case ControlCodes::Font::large:
                    fontSpriteBase = Font::large;
                    break;

                case ControlCodes::Font::bold:
                    fontSpriteBase = Font::medium_bold;
                    break;

                case ControlCodes::Font::regular:
                    fontSpriteBase = Font::medium_normal;
                    break;

                case ControlCodes::Font::outline:
                case ControlCodes::Font::outlineOff:
                case ControlCodes::windowColour1:
                case ControlCodes::windowColour2:
                case ControlCodes::windowColour3:
                case ControlCodes::windowColour4:
                    break;

                case ControlCodes::inlineSpriteStr:
                {
                    const uint32_t image = reinterpret_cast<const uint32_t*>(str)[0];
                    const uint32_t imageId = image & 0x7FFFF;
                    str += 4;
                    width += _g1Elements[imageId].width;
                    break;
                }

                default:
                    if (chr <= 0x16)
                    {
                        str += 2;
                    }
                    else
                    {
                        str += 4;
                    }
                    break;
            }
        }

        return width;
    }

    /**
     * 0x004955BC
     *
     * @param buffer @<esi>
     * @return width @<cx>
     */
    uint16_t getMaxStringWidth(const char* buffer)
    {
        uint16_t width = 0;
        uint16_t maxWidth = 0;
        const uint8_t* str = reinterpret_cast<const uint8_t*>(buffer);
        auto fontSpriteBase = getCurrentFontSpriteBase();

        while (*str != (uint8_t)0)
        {
            const uint8_t chr = *str;
            str++;

            if (chr >= 32)
            {
                width += _characterWidths[chr - 32 + fontSpriteBase];
                continue;
            }

            switch (chr)
            {
                case ControlCodes::moveX:
                    maxWidth = std::max(width, maxWidth);
                    width = *str;
                    str++;
                    break;

                case ControlCodes::adjustPalette:
                case 3:
                case 4:
                    str++;
                    break;

                case ControlCodes::newline:
                case ControlCodes::newlineSmaller:
                    continue;

                case ControlCodes::Font::small:
                    fontSpriteBase = Font::small;
                    break;

                case ControlCodes::Font::large:
                    fontSpriteBase = Font::large;
                    break;

                case ControlCodes::Font::bold:
                    fontSpriteBase = Font::medium_bold;
                    break;

                case ControlCodes::Font::regular:
                    fontSpriteBase = Font::medium_normal;
                    break;

                case ControlCodes::Font::outline:
                case ControlCodes::Font::outlineOff:
                case ControlCodes::windowColour1:
                case ControlCodes::windowColour2:
                case ControlCodes::windowColour3:
                case ControlCodes::windowColour4:
                    break;

                case ControlCodes::newlineXY:
                    maxWidth = std::max(width, maxWidth);
                    width = *str;
                    str += 2;
                    break;

                case ControlCodes::inlineSpriteStr:
                {
                    const uint32_t image = reinterpret_cast<const uint32_t*>(str)[0];
                    const uint32_t imageId = image & 0x7FFFF;
                    str += 4;
                    width += _g1Elements[imageId].width;
                    break;
                }

                default:
                    if (chr <= 0x16)
                    {
                        str += 2;
                    }
                    else
                    {
                        str += 4;
                    }
                    break;
            }
        }
        maxWidth = std::max(width, maxWidth);
        return maxWidth;
    }

    static void setTextColours(PaletteIndex_t pal1, PaletteIndex_t pal2, PaletteIndex_t pal3)
    {
        if ((_currentFontFlags & TextDrawFlags::inset) != 0)
            return;

        _textColours[1] = pal1;
        _textColours[2] = PaletteIndex::transparent;
        _textColours[3] = PaletteIndex::transparent;
        if ((_currentFontFlags & TextDrawFlags::outline) != 0)
        {
            _textColours[2] = pal2;
            _textColours[3] = pal3;
        }
    }

    static void setTextColour(int colour)
    {
        auto el = &_g1Elements[ImageIds::text_palette];
        setTextColours(el->offset[colour * 4 + 0], el->offset[colour * 4 + 1], el->offset[colour * 4 + 2]);
    }

    // 0x00451189
    static Ui::Point loopNewline(RenderTarget* rt, Ui::Point origin, uint8_t* str)
    {
        Ui::Point pos = origin;
        while (true)
        {
            // When offscreen in y dimension don't draw text
            // In original this check only performed if pos.y updated instead of every loop
            bool offscreen = true;
            if (pos.y + 19 > rt->y)
            {
                if (rt->y + rt->height > pos.y)
                {
                    offscreen = false;
                }
            }
            uint8_t chr = *str;
            str++;

            switch (chr)
            {
                case '\0':
                    return pos;

                case ControlCodes::adjustPalette:
                    // This control character does not appear in the localisation files
                    assert(false);
                    str++;
                    break;

                case ControlCodes::newlineSmaller:
                    pos.x = origin.x;
                    if (getCurrentFontSpriteBase() == Font::medium_normal || getCurrentFontSpriteBase() == Font::medium_bold)
                    {
                        pos.y += 5;
                    }
                    else if (getCurrentFontSpriteBase() == Font::small)
                    {
                        pos.y += 3;
                    }
                    else if (getCurrentFontSpriteBase() == Font::large)
                    {
                        pos.y += 9;
                    }
                    break;

                case ControlCodes::newline:
                    pos.x = origin.x;
                    if (getCurrentFontSpriteBase() == Font::medium_normal || getCurrentFontSpriteBase() == Font::medium_bold)
                    {
                        pos.y += 10;
                    }
                    else if (getCurrentFontSpriteBase() == Font::small)
                    {
                        pos.y += 6;
                    }
                    else if (getCurrentFontSpriteBase() == Font::large)
                    {
                        pos.y += 18;
                    }
                    break;

                case ControlCodes::moveX:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    break;
                }

                case ControlCodes::newlineXY:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    offset = *str;
                    str++;
                    pos.y = origin.y + offset;

                    break;
                }

                case ControlCodes::Font::small:
                    setCurrentFontSpriteBase(Font::small);
                    break;
                case ControlCodes::Font::large:
                    setCurrentFontSpriteBase(Font::large);
                    break;
                case ControlCodes::Font::regular:
                    setCurrentFontSpriteBase(Font::medium_normal);
                    break;
                case ControlCodes::Font::bold:
                    setCurrentFontSpriteBase(Font::medium_bold);
                    break;
                case ControlCodes::Font::outline:
                    _currentFontFlags = _currentFontFlags | TextDrawFlags::outline;
                    break;
                case ControlCodes::Font::outlineOff:
                    _currentFontFlags = _currentFontFlags & ~TextDrawFlags::outline;
                    break;
                case ControlCodes::windowColour1:
                {
                    auto hue = _windowColours[0].c();
                    setTextColours(Colours::getShade(hue, 7), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }
                case ControlCodes::windowColour2:
                {
                    auto hue = _windowColours[1].c();
                    setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }
                case ControlCodes::windowColour3:
                {
                    auto hue = _windowColours[2].c();
                    setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }
                case ControlCodes::windowColour4:
                {
                    auto hue = _windowColours[3].c();
                    setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }

                case ControlCodes::inlineSpriteStr:
                {
                    uint32_t image = ((uint32_t*)str)[0];
                    ImageId imageId{ image & 0x7FFFF };
                    str += 4;

                    if ((_currentFontFlags & TextDrawFlags::inset) != 0)
                    {
                        Gfx::drawImageSolid(*rt, pos, imageId, _textColours[3]);
                        Gfx::drawImageSolid(*rt, pos + Ui::Point{ 1, 1 }, imageId, _textColours[1]);
                    }
                    else
                    {
                        Gfx::drawImage(rt, pos.x, pos.y, image);
                    }

                    pos.x += _g1Elements[imageId.getIndex()].width;
                    break;
                }

                case ControlCodes::Colour::black:
                    setTextColour(0);
                    break;

                case ControlCodes::Colour::grey:
                    setTextColour(1);
                    break;

                case ControlCodes::Colour::white:
                    setTextColour(2);
                    break;

                case ControlCodes::Colour::red:
                    setTextColour(3);
                    break;

                case ControlCodes::Colour::green:
                    setTextColour(4);
                    break;

                case ControlCodes::Colour::yellow:
                    setTextColour(5);
                    break;

                case ControlCodes::Colour::topaz:
                    setTextColour(6);
                    break;

                case ControlCodes::Colour::celadon:
                    setTextColour(7);
                    break;

                case ControlCodes::Colour::babyBlue:
                    setTextColour(8);
                    break;

                case ControlCodes::Colour::paleLavender:
                    setTextColour(9);
                    break;

                case ControlCodes::Colour::paleGold:
                    setTextColour(10);
                    break;

                case ControlCodes::Colour::lightPink:
                    setTextColour(11);
                    break;

                case ControlCodes::Colour::pearlAqua:
                    setTextColour(12);
                    break;

                case ControlCodes::Colour::paleSilver:
                    setTextColour(13);
                    break;

                default:
                    if (!offscreen)
                    {
                        // When offscreen in the y dimension there is no requirement to keep pos.x correct
                        if (chr >= 32)
                        {
                            // Use withPrimary to set imageId flag to use the correct palette code (Colour::black is not actually used)
                            Gfx::drawImagePaletteSet(*rt, pos, ImageId(1116 + chr - 32 + getCurrentFontSpriteBase()).withPrimary(Colour::black), PaletteMapView{ _textColours }, {});
                            pos.x += _characterWidths[chr - 32 + getCurrentFontSpriteBase()];
                        }
                        else
                        {
                            // Unhandled control code
                            assert(false);
                        }
                    }
                    break;
            }
        }

        return pos;
    }

    /**
     * 0x00451025
     *
     * @param x  @<cx>
     * @param y @<dx>
     * @param colour @<al>
     * @param rt @<edi>
     * @param text @<esi>
     */
    Ui::Point drawString(RenderTarget& rt, int16_t x, int16_t y, AdvancedColour colour, void* str)
    {
        // 0x00E04348, 0x00E0434A
        Ui::Point origin = { x, y };

        if (colour.isFE())
        {
            return loopNewline(&rt, origin, (uint8_t*)str);
        }

        if (colour.isFD())
        {
            _currentFontFlags = 0;
            setTextColour(0);
            return loopNewline(&rt, origin, (uint8_t*)str);
        }

        if (x >= rt.x + rt.width)
            return origin;

        if (x < rt.x - 1280)
            return origin;

        if (y >= rt.y + rt.height)
            return origin;

        if (y < rt.y - 90)
            return origin;

        if (colour.isFF())
        {
            return loopNewline(&rt, origin, (uint8_t*)str);
        }

        _currentFontFlags = 0;
        if (getCurrentFontSpriteBase() == Font::m1)
        {
            setCurrentFontSpriteBase(Font::medium_bold);
            _currentFontFlags = _currentFontFlags | TextDrawFlags::dark;
        }
        else if (getCurrentFontSpriteBase() == Font::m2)
        {
            setCurrentFontSpriteBase(Font::medium_bold);
            _currentFontFlags = _currentFontFlags | TextDrawFlags::dark;
            _currentFontFlags = _currentFontFlags | TextDrawFlags::extraDark;
        }

        _textColours[0] = PaletteIndex::transparent;
        _textColours[1] = Colours::getShade(Colour::mutedDarkPurple, 5);
        _textColours[2] = Colours::getShade(Colour::mutedRed, 5);
        _textColours[3] = Colours::getShade(Colour::blue, 5);

        if (colour.isOutline())
        {
            colour = colour.clearOutline();
            _currentFontFlags = _currentFontFlags | TextDrawFlags::outline;
        }

        if (colour.isInset())
        {
            colour = colour.clearInset();
            _currentFontFlags = _currentFontFlags | TextDrawFlags::inset;
        }

        if ((_currentFontFlags & TextDrawFlags::inset) != 0)
        {
            if ((_currentFontFlags & TextDrawFlags::dark) != 0 && (_currentFontFlags & TextDrawFlags::extraDark) != 0)
            {
                _textColours[1] = Colours::getShade(colour.c(), 2);
                _textColours[2] = PaletteIndex::transparent;
                _textColours[3] = Colours::getShade(colour.c(), 4);
            }
            else if ((_currentFontFlags & TextDrawFlags::dark) != 0)
            {
                _textColours[1] = Colours::getShade(colour.c(), 3);
                _textColours[2] = PaletteIndex::transparent;
                _textColours[3] = Colours::getShade(colour.c(), 5);
            }
            else
            {
                _textColours[1] = Colours::getShade(colour.c(), 4);
                _textColours[2] = PaletteIndex::transparent;
                _textColours[3] = Colours::getShade(colour.c(), 6);
            }
        }
        else
        {
            setTextColours(Colours::getShade(colour.c(), 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
        }

        return loopNewline(&rt, origin, (uint8_t*)str);
    }

    static const char* advanceToNextLine(const char* buffer)
    {
        // Traverse the buffer for the next line
        const char* ptr = buffer;
        while (true)
        {
            const auto chr = *ptr++;
            if (chr == '\0')
                return ptr;

            if (chr >= ControlCodes::oneArgBegin && chr < ControlCodes::oneArgEnd)
            {
                // Skip argument
                ptr++;
            }
            else if (chr >= ControlCodes::twoArgBegin && chr < ControlCodes::twoArgEnd)
            {
                // Skip argument
                ptr += 2;
            }
            else if (chr >= ControlCodes::fourArgBegin && chr < ControlCodes::fourArgEnd)
            {
                // Skip argument
                ptr += 4;
            }
        }

        return nullptr;
    }

    // 0x00495224
    // al: colour
    // bp: width
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    int16_t drawStringLeftWrapped(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        // Setup the text colours (FIXME: This should be a separate function)
        char empty[1] = "";
        Gfx::drawString(rt, rt.x, rt.y, colour, empty);

        _currentFontSpriteBase = Font::medium_bold;
        auto wrapResult = wrapString(buffer, width);
        auto breakCount = wrapResult.second + 1;

        // wrapString might change the font due to formatting codes
        uint16_t lineHeight = 0; // _112D404
        if (_currentFontSpriteBase <= Font::medium_bold)
            lineHeight = 10;
        else if (_currentFontSpriteBase == Font::small)
            lineHeight = 6;
        else if (_currentFontSpriteBase == Font::large)
            lineHeight = 18;

        _currentFontFlags = 0;
        Ui::Point point = { x, y };

        const char* ptr = buffer;

        for (auto i = 0; ptr != nullptr && i < breakCount; i++)
        {
            Gfx::drawString(rt, point.x, point.y, AdvancedColour::FE(), const_cast<char*>(ptr));
            ptr = advanceToNextLine(ptr);
            point.y += lineHeight;
        }

        return point.y;
    }

    // 0x00494B3F
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringLeft(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        Point origin = { x, y };
        drawStringLeft(rt, &origin, colour, stringId, args);
    }

    /**
     *
     * @param rt @<edi>
     * @param origin {x @<cx>, y @<dx>}
     * @param colour @<al>
     * @param stringId  @<bx>
     * @param args @<edi>
     */
    void drawStringLeft(
        RenderTarget& rt,
        Point* origin,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        auto point = Gfx::drawString(rt, origin->x, origin->y, colour, buffer);

        origin->x = point.x;
        origin->y = point.y;
    }

    // 0x00494BBF
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    // bp: width
    void drawStringLeftClipped(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        clipString(width, buffer);

        Gfx::drawString(rt, x, y, colour, buffer);
    }

    // 0x00494C78
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringRight(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        uint16_t width = getStringWidth(buffer);

        Gfx::drawString(rt, x - width, y, colour, buffer);
    }

    // 0x00494CB2
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringRightUnderline(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        uint16_t width = getStringWidth(buffer);

        drawString(rt, x - width, y, colour, buffer);

        // Draw underline
        drawRect(rt, x - width, y + 11, width, 1, _textColours[1]);
        if (_textColours[2] != 0)
            drawRect(rt, x - width, y + 12, width, 1, _textColours[2]);
    }

    // 0x00494D78
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringLeftUnderline(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        uint16_t width = getStringWidth(buffer);

        drawString(rt, x, y, colour, buffer);

        // Draw underline
        drawRect(rt, x, y + 11, width, 1, _textColours[1]);
        if (_textColours[2] != 0)
            drawRect(rt, x, y + 12, width, 1, _textColours[2]);
    }

    // 0x00494DE8
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringCentred(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        uint16_t width = getStringWidth(buffer);

        if (x - (width / 2) < 0)
            return;

        Gfx::drawString(rt, x - (width / 2), y, colour, buffer);
    }

    // 0x00494C36
    // al: colour
    // bx: string id
    // bp: width
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringCentredClipped(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        width = clipString(width, buffer);

        Gfx::drawString(rt, x - (width / 2), y, colour, buffer);
    }

    /**
     * 0x00494ECF
     *
     * @param rt @<edi>
     * @param origin {x @<cx>, y @<dx>}
     * @param width @<bp>
     * @param colour @<al>
     * @param stringId @<bx>
     * @param args @<esi>
     * returns width @<ax>
     */
    uint16_t drawStringCentredWrapped(
        RenderTarget& rt,
        Point& origin,
        uint16_t width,
        AdvancedColour colour,
        string_id stringId,
        const void* args)
    {
        _currentFontSpriteBase = Font::medium_bold;
        // Setup the text colours (FIXME: This should be a separate function)
        char empty[1] = "";
        Gfx::drawString(rt, rt.x, rt.y, colour, empty);

        char buffer[512];
        StringManager::formatString(buffer, std::size(buffer), stringId, args);

        _currentFontSpriteBase = Font::medium_bold;
        auto wrapResult = wrapString(buffer, width);
        auto breakCount = wrapResult.second + 1;

        // wrapString might change the font due to formatting codes
        uint16_t lineHeight = 0; // _112D404
        if (_currentFontSpriteBase <= Font::medium_bold)
            lineHeight = 10;
        else if (_currentFontSpriteBase == Font::small)
            lineHeight = 6;
        else if (_currentFontSpriteBase == Font::large)
            lineHeight = 18;

        _currentFontFlags = 0;
        Ui::Point point = origin;
        point.y -= (lineHeight / 2) * (breakCount - 1);

        const char* ptr = buffer;

        for (auto i = 0; ptr != nullptr && i < breakCount; i++)
        {
            uint16_t lineWidth = getStringWidth(ptr);

            Gfx::drawString(rt, point.x - (lineWidth / 2), point.y, AdvancedColour::FE(), const_cast<char*>(ptr));
            ptr = advanceToNextLine(ptr);
            point.y += lineHeight;
        }

        return point.y;
    }

    // 0x00494E33
    // al: colour
    // bx: string id
    // bp: width
    // cx: x
    // dx: y
    // esi: args
    // edi: rt
    void drawStringCentredRaw(
        RenderTarget& rt,
        int16_t x,
        int16_t y,
        int16_t width,
        AdvancedColour colour,
        const void* args)
    {
        registers regs;
        regs.edi = X86Pointer(&rt);
        regs.esi = X86Pointer(args);
        regs.cx = x;
        regs.dx = y;
        regs.al = colour.u8();
        regs.bp = width;
        call(0x00494E33, regs);
    }

    // 0x00495715
    // @param buffer @<esi>
    // @return width @<cx>
    uint16_t getStringWidthNewLined(const char* buffer)
    {
        registers regs;
        regs.esi = X86Pointer(buffer);
        call(0x00495715, regs);
        return regs.cx;
    }

    // 0x00495301
    // Note: Returned break count is -1. TODO: Refactor out this -1.
    // @return maxWidth @<cx> (breakCount-1) @<di>
    std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth)
    {
        // std::vector<const char*> wrap; TODO: refactor to return pointers to line starts
        uint16_t wrapCount = 0;
        auto font = *_currentFontSpriteBase;
        uint16_t maxWidth = 0;

        for (auto* ptr = buffer; *ptr != '\0';)
        {
            auto* startLine = ptr;
            uint16_t lineWidth = 0;
            auto lastWordLineWith = lineWidth;
            auto* wordStart = ptr;
            for (; *ptr != '\0' && lineWidth < stringWidth; ++ptr)
            {
                if (*ptr >= ControlCodes::noArgBegin && *ptr < ControlCodes::noArgEnd)
                {
                    bool forceEndl = false;
                    switch (*ptr)
                    {
                        case ControlCodes::newline:
                        {
                            *ptr = '\0';
                            forceEndl = true;
                            ++ptr; // Skip over '\0' when forcing a new line
                            wrapCount++;
                            // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                            maxWidth = std::max(maxWidth, lineWidth);
                            break;
                        }
                        case ControlCodes::Font::small:
                            font = Font::small;
                            break;
                        case ControlCodes::Font::large:
                            font = Font::large;
                            break;
                        case ControlCodes::Font::bold:
                            font = Font::medium_bold;
                            break;
                        case ControlCodes::Font::regular:
                            font = Font::medium_normal;
                            break;
                    }
                    if (forceEndl)
                    {
                        break;
                    }
                }
                else if (*ptr >= ControlCodes::oneArgBegin && *ptr < ControlCodes::oneArgEnd)
                {
                    switch (*ptr)
                    {
                        case ControlCodes::moveX:
                            lineWidth = static_cast<uint8_t>(ptr[1]);
                            break;
                    }
                    ptr += 1;
                }
                else if (*ptr >= ControlCodes::twoArgBegin && *ptr < ControlCodes::twoArgEnd)
                {
                    ptr += 2;
                }
                else if (*ptr >= ControlCodes::fourArgBegin && *ptr < ControlCodes::fourArgEnd)
                {
                    switch (*ptr)
                    {
                        case ControlCodes::inlineSpriteStr:
                        {
                            uint32_t image = *reinterpret_cast<const uint32_t*>(ptr);
                            ImageId imageId{ image & 0x7FFFF };
                            auto* el = Gfx::getG1Element(imageId.getIndex());
                            if (el != nullptr)
                            {
                                lineWidth += el->width;
                            }
                            break;
                        }
                    }
                    ptr += 4;
                }
                else
                {
                    if (*ptr == ' ')
                    {
                        wordStart = ptr;
                        lastWordLineWith = lineWidth;
                    }
                    lineWidth += _characterWidths[font + (static_cast<uint8_t>(*ptr) - 32)];
                }
            }
            if (lineWidth >= stringWidth || *ptr == '\0')
            {
                if (startLine == wordStart || (*ptr == '\0' && lineWidth < stringWidth))
                {
                    // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                    maxWidth = std::max(maxWidth, lineWidth);
                    if (startLine == wordStart && *ptr != '\0')
                    {
                        // Warning! Not loco string argument safe assumes no one/two/four argument strings.
                        // TODO: Implement loco string strlen!
                        memmove(ptr + 1, ptr, strlen(ptr) + 1);
                        *ptr++ = '\0';
                    }
                }
                else
                {
                    // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                    maxWidth = std::max(maxWidth, lastWordLineWith);
                    *wordStart = '\0';
                    ptr = wordStart + 1;
                }
                wrapCount++;
            }
        }

        // Note that this is always the font used in the last line.
        // TODO: refactor to pair up with each line, and to not use a global.
        _currentFontSpriteBase = font;
        return std::make_pair(maxWidth, std::max(static_cast<uint16_t>(wrapCount) - 1, 0));
    }

    // 0x004474BA
    static void drawRectImpl(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = X86Pointer(&rt);
        call(0x004474BA, regs);
    }

    void fillRect(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        drawRectImpl(rt, left, top, right, bottom, colour);
    }

    void drawRect(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour)
    {
        // This makes the function signature more like a drawing application
        drawRectImpl(rt, x, y, x + dx - 1, y + dy - 1, colour);
    }

    void fillRectInset(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = X86Pointer(&rt);
        regs.si = flags;
        call(0x004C58C7, regs);
    }

    void drawRectInset(Gfx::RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags)
    {
        // This makes the function signature more like a drawing application
        fillRectInset(rt, x, y, x + dx - 1, y + dy - 1, colour, flags);
    }

    // 0x00452DA4
    void drawLine(Gfx::RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        registers regs;
        regs.ax = left;
        regs.bx = top;
        regs.cx = right;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = X86Pointer(&rt);
        call(0x00452DA4, regs);
    }

    // 0x004CD406
    void invalidateScreen()
    {
        setDirtyBlocks(0, 0, Ui::width(), Ui::height());
    }

    static std::unique_ptr<Drawing::SoftwareDrawingEngine> engine;

    Drawing::SoftwareDrawingEngine& getDrawingEngine()
    {
        if (!engine)
        {
            engine = std::make_unique<Drawing::SoftwareDrawingEngine>();
        }
        return *engine;
    }

    /**
     * 0x004C5C69
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        getDrawingEngine().setDirtyBlocks(left, top, right, bottom);
    }

    // 0x004C5CFA
    void drawDirtyBlocks()
    {
        getDrawingEngine().drawDirtyBlocks();
    }

    loco_global<char[512], 0x0112CC04> _byte_112CC04;
    loco_global<char[512], 0x0112CE04> _byte_112CE04;

    // 0x004CF63B
    void render()
    {
        char backup1[512] = { 0 };
        char backup2[512] = { 0 };

        std::memcpy(backup1, _byte_112CC04, 512);
        std::memcpy(backup2, _byte_112CE04, 512);

        if (Ui::dirtyBlocksInitialised())
        {
            getDrawingEngine().drawDirtyBlocks();
        }

        if (Input::hasFlag(Input::Flags::flag5))
        {
            Ui::processMessagesMini();
        }
        else
        {
            Ui::processMessages();
        }

        if (addr<0x005252AC, uint32_t>() != 0)
        {
            //            sub_4058F5();
        }

        std::memcpy(_byte_112CC04, backup1, 512);
        std::memcpy(_byte_112CE04, backup2, 512);
    }

    void redrawScreenRect(Rect rect)
    {
        getDrawingEngine().drawRect(rect);
    }

    /**
     * 0x004C5DD5
     * rct2: window_draw_all
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void redrawScreenRect(int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        redrawScreenRect(Rect::fromLTRB(left, top, right, bottom));
    }

    static const G1Element* getNoiseMaskImageFromImage(const ImageId image)
    {
        if (image.hasNoiseMask())
        {
            const auto noise = image.getNoiseMask();
            const auto* noiseImage = getG1Element(_noiseMaskImages[noise]);
            if (noiseImage == nullptr)
            {
                return nullptr;
            }
            return noiseImage;
        }
        else
        {
            return nullptr;
        }
    }

    // 0x00448C79
    void drawImage(Gfx::RenderTarget* rt, int16_t x, int16_t y, uint32_t image)
    {
        drawImage(*rt, { x, y }, ImageId::fromUInt32(image));
    }

    // 0x00448C79
    void drawImage(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image)
    {
        const auto* noiseImage = getNoiseMaskImageFromImage(image);
        const auto palette = getPaletteMapFromImage(image);

        if (!palette.has_value())
        {
            drawImagePaletteSet(rt, pos, image, getDefaultPaletteMap(), noiseImage);
        }
        else
        {
            drawImagePaletteSet(rt, pos, image, *palette, noiseImage);
        }
    }

    uint32_t recolour(uint32_t image)
    {
        return ImageIdFlags::remap | image;
    }

    uint32_t recolour(uint32_t image, ExtColour colour)
    {
        return ImageIdFlags::remap | (enumValue(colour) << 19) | image;
    }
    uint32_t recolour(uint32_t image, Colour colour)
    {
        return recolour(image, Colours::toExt(colour));
    }

    uint32_t recolour2(uint32_t image, Colour colour1, Colour colour2)
    {
        return ImageIdFlags::remap | ImageIdFlags::remap2 | (enumValue(colour1) << 19) | (enumValue(colour2) << 24) | image;
    }

    uint32_t recolour2(uint32_t image, ColourScheme colourScheme)
    {
        return recolour2(image, colourScheme.primary, colourScheme.secondary);
    }

    uint32_t recolourTranslucent(uint32_t image, ExtColour colour)
    {
        return ImageIdFlags::translucent | (enumValue(colour) << 19) | image;
    }

    loco_global<uint8_t*, 0x0050B860> _50B860;

    void drawImageSolid(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex)
    {
        uint8_t palette[256];
        memset(palette, paletteIndex, 256);
        palette[0] = 0;

        // Set the image primary flag to tell drawImagePaletteSet to recolour with the palette (Colour::black is not actually used)
        drawImagePaletteSet(rt, pos, image.withPrimary(Colour::black), PaletteMapView{ palette }, {});
    }

    template<uint8_t TZoomLevel, bool TIsRLE>
    static std::optional<DrawSpritePosArgs> getDrawImagePosArgs(Gfx::RenderTarget& rt, const Ui::Point& pos, const G1Element& element)
    {
        if constexpr (TZoomLevel > 0)
        {
            if (element.flags & G1ElementFlags::noZoomDraw)
            {
                return std::nullopt;
            }
        }

        auto dispPos{ pos };
        // Its used super often so we will define it to a separate variable.
        constexpr auto zoomMask = static_cast<uint32_t>(0xFFFFFFFFULL << TZoomLevel);

        if constexpr (TZoomLevel > 0 && TIsRLE)
        {
            dispPos.x -= ~zoomMask;
            dispPos.y -= ~zoomMask;
        }

        // This will be the height of the drawn image
        auto height = element.height;

        // This is the start y coordinate on the destination
        auto dstTop = dispPos.y + element.yOffset;

        // For whatever reason the RLE version does not use
        // the zoom mask on the y coordinate but does on x.
        if constexpr (TIsRLE)
        {
            dstTop -= rt.y;
        }
        else
        {
            dstTop = (dstTop & zoomMask) - rt.y;
        }
        // This is the start y coordinate on the source
        auto srcY = 0;

        if (dstTop < 0)
        {
            // If the destination y is negative reduce the height of the
            // image as we will cut off the bottom
            height += dstTop;
            // If the image is no longer visible nothing to draw
            if (height <= 0)
            {
                return std::nullopt;
            }
            // The source image will start a further up the image
            srcY -= dstTop;
            // The destination start is now reset to 0
            dstTop = 0;
        }
        else
        {
            if constexpr (TZoomLevel > 0 && TIsRLE)
            {
                srcY -= dstTop & ~zoomMask;
                height += dstTop & ~zoomMask;
            }
        }

        auto dstBottom = dstTop + height;

        if (dstBottom > rt.height)
        {
            // If the destination y is outside of the drawing
            // image reduce the height of the image
            height -= dstBottom - rt.height;
        }
        // If the image no longer has anything to draw
        if (height <= 0)
            return std::nullopt;

        dstTop = dstTop >> TZoomLevel;

        // This will be the width of the drawn image
        auto width = element.width;

        // This is the source start x coordinate
        auto srcX = 0;
        // This is the destination start x coordinate
        int32_t dstLeft = ((dispPos.x + element.xOffset + ~zoomMask) & zoomMask) - rt.x;

        if (dstLeft < 0)
        {
            // If the destination is negative reduce the width
            // image will cut off the side
            width += dstLeft;
            // If there is no image to draw
            if (width <= 0)
            {
                return std::nullopt;
            }
            // The source start will also need to cut off the side
            srcX -= dstLeft;
            // Reset the destination to 0
            dstLeft = 0;
        }
        else
        {
            if constexpr (TZoomLevel > 0 && TIsRLE)
            {
                srcX -= dstLeft & ~zoomMask;
            }
        }

        const auto dstRight = dstLeft + width;

        if (dstRight > rt.width)
        {
            // If the destination x is outside of the drawing area
            // reduce the image width.
            width -= dstRight - rt.width;
            // If there is no image to draw.
            if (width <= 0)
                return std::nullopt;
        }

        dstLeft = dstLeft >> TZoomLevel;

        return DrawSpritePosArgs{ Ui::Point32{ srcX, srcY }, Ui::Point32{ dstLeft, dstTop }, Ui::Size(width, height) };
    }

    template<uint8_t TZoomLevel, bool TIsRLE>
    static void drawImagePaletteSet(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const G1Element& element, const PaletteMapView& palette, const G1Element* noiseImage)
    {
        auto args = getDrawImagePosArgs<TZoomLevel, TIsRLE>(rt, pos, element);
        if (args.has_value())
        {
            const DrawSpriteArgs fullArgs{ palette, element, args->srcPos, args->dstPos, args->size, noiseImage };
            const auto op = Drawing::getDrawBlendOp(image, fullArgs);
            Drawing::drawSpriteToBuffer<TZoomLevel, TIsRLE>(rt, fullArgs, op);
        }
    }

    // 0x00448D90
    void drawImagePaletteSet(Gfx::RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const PaletteMapView& palette, const G1Element* noiseImage)
    {
        const auto* element = getG1Element(image.getIndex());
        if (element == nullptr)
        {
            return;
        }

        if (rt.zoomLevel > 0 && (element->flags & G1ElementFlags::hasZoomSprites))
        {
            auto zoomedrt{ rt };
            zoomedrt.bits = rt.bits;
            zoomedrt.x = rt.x >> 1;
            zoomedrt.y = rt.y >> 1;
            zoomedrt.height = rt.height >> 1;
            zoomedrt.width = rt.width >> 1;
            zoomedrt.pitch = rt.pitch;
            zoomedrt.zoomLevel = rt.zoomLevel - 1;

            const auto zoomCoords = Ui::Point(pos.x >> 1, pos.y >> 1);
            drawImagePaletteSet(
                zoomedrt, zoomCoords, image.withIndexOffset(-element->zoomOffset), palette, noiseImage);
            return;
        }

        const bool isRLE = element->flags & G1ElementFlags::isRLECompressed;
        if (isRLE)
        {
            switch (rt.zoomLevel)
            {
                default:
                    drawImagePaletteSet<0, true>(rt, pos, image, *element, palette, noiseImage);
                    break;
                case 1:
                    drawImagePaletteSet<1, true>(rt, pos, image, *element, palette, noiseImage);
                    break;
                case 2:
                    drawImagePaletteSet<2, true>(rt, pos, image, *element, palette, noiseImage);
                    break;
                case 3:
                    drawImagePaletteSet<3, true>(rt, pos, image, *element, palette, noiseImage);
                    break;
            }
        }
        else
        {
            switch (rt.zoomLevel)
            {
                default:
                    drawImagePaletteSet<0, false>(rt, pos, image, *element, palette, noiseImage);
                    break;
                case 1:
                    drawImagePaletteSet<1, false>(rt, pos, image, *element, palette, noiseImage);
                    break;
                case 2:
                    drawImagePaletteSet<2, false>(rt, pos, image, *element, palette, noiseImage);
                    break;
                case 3:
                    drawImagePaletteSet<3, false>(rt, pos, image, *element, palette, noiseImage);
                    break;
            }
        }
    }

    G1Element* getG1Element(uint32_t imageId)
    {
        const auto id = getImageIndex(imageId);
        if (id < _g1Elements.size())
        {
            return &_g1Elements[id];
        }
        return nullptr;
    }

    // 0x0112C876
    int16_t getCurrentFontSpriteBase()
    {
        return _currentFontSpriteBase;
    }
    void setCurrentFontSpriteBase(int16_t base)
    {
        _currentFontSpriteBase = base;
    }

    static loco_global<Drawing::PaletteEntry[256], 0x0113ED20> _113ED20;

    // 0x004523F4
    void loadPalette()
    {
        auto* g1Palette = getG1Element(ImageIds::default_palette);
        uint8_t* colourData = g1Palette->offset;
        for (auto i = g1Palette->xOffset; i < g1Palette->width; ++i)
        {
            _113ED20[i].b = *colourData++;
            _113ED20[i].g = *colourData++;
            _113ED20[i].r = *colourData++;
        }
        getDrawingEngine().updatePalette(_113ED20, 10, 236);
    }
}
