#include "Gfx.h"
#include "../Console.h"
#include "../Drawing/SoftwareDrawingEngine.h"
#include "../Environment.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/LanguageFiles.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Stream.hpp"
#include "Colour.h"
#include "ImageIds.h"
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
    namespace g1_expected_count
    {
        constexpr uint32_t disc = 0x101A; // And GOG
        constexpr uint32_t steam = 0x0F38;
    }

    namespace text_draw_flags
    {
        constexpr uint8_t inset = (1ULL << 0);
        constexpr uint8_t outline = (1ULL << 1);
        constexpr uint8_t dark = (1ULL << 2);
        constexpr uint8_t extra_dark = (1ULL << 3);
    }

    constexpr uint32_t g1_count_objects = 0x40000;
    constexpr uint32_t g1_count_temporary = 0x1000;

    static loco_global<Context, 0x0050B884> _screenContext;

    static loco_global<G1Element[g1_expected_count::disc + g1_count_temporary + g1_count_objects], 0x9E2424> _g1Elements;

    static std::unique_ptr<std::byte[]> _g1Buffer;
    static loco_global<uint16_t[147], 0x050B8C8> _paletteToG1Offset;

    static loco_global<uint16_t, 0x112C824> _currentFontFlags;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;
    static loco_global<uint8_t[4], 0x1136594> _windowColours;

    static PaletteIndex_t _textColours[8] = { 0 };

    Ui::Rect Context::getDrawableRect() const
    {
        auto zoom = zoom_level;
        auto left = x >> zoom;
        auto top = y >> zoom;
        auto right = (width >> zoom) + left;
        auto bottom = (height >> zoom) + top;
        return Ui::Rect::fromLTRB(left, top, right, bottom);
    }

    Ui::Rect Context::getUiRect() const
    {
        return Ui::Rect::fromLTRB(x, y, x + width, y + height);
    }

    Context& screenContext()
    {
        return _screenContext;
    }

    const PaletteMap& PaletteMap::getDefault()
    {
        static bool initialised = false;
        static uint8_t data[256];
        static PaletteMap defaultMap(data);
        if (!initialised)
        {
            std::iota(std::begin(data), std::end(data), 0);
        }
        return defaultMap;
    }

    uint8_t& PaletteMap::operator[](size_t index)
    {
        assert(index < _dataLength);

        // Provide safety in release builds
        if (index >= _dataLength)
        {
            static uint8_t dummy;
            return dummy;
        }

        return _data[index];
    }

    uint8_t PaletteMap::operator[](size_t index) const
    {
        assert(index < _dataLength);

        // Provide safety in release builds
        if (index >= _dataLength)
        {
            return 0;
        }

        return _data[index];
    }

    uint8_t PaletteMap::blend(uint8_t src, uint8_t dst) const
    {
        // src = 0 would be transparent so there is no blend palette for that, hence (src - 1)
        assert(src != 0 && (src - 1) < _numMaps);
        assert(dst < _mapLength);
        auto idx = ((src - 1) * 256) + dst;
        return (*this)[idx];
    }

    void PaletteMap::copy(size_t dstIndex, const PaletteMap& src, size_t srcIndex, size_t length)
    {
        auto maxLength = std::min(_mapLength - srcIndex, _mapLength - dstIndex);
        assert(length <= maxLength);
        auto copyLength = std::min(length, maxLength);
        std::memcpy(&_data[dstIndex], &src._data[srcIndex], copyLength);
    }

    std::optional<uint32_t> getPaletteG1Index(Colour_t paletteId)
    {
        if (paletteId < std::size(_paletteToG1Offset))
        {
            return _paletteToG1Offset[paletteId];
        }
        return std::nullopt;
    }

    std::optional<PaletteMap> getPaletteMapForColour(Colour_t paletteId)
    {
        auto g1Index = getPaletteG1Index(paletteId);
        if (g1Index)
        {
            auto g1 = getG1Element(*g1Index);
            if (g1 != nullptr)
            {
                return PaletteMap(g1->offset, g1->height, g1->width);
            }
        }
        return std::nullopt;
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
        auto g1Path = Environment::getPath(Environment::path_id::g1);
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

        if (header.num_entries != g1_expected_count::disc)
        {
            std::cout << "G1 element count doesn't match expected value: ";
            std::cout << "Expected " << g1_expected_count::disc << "; Got " << header.num_entries << std::endl;
            if (header.num_entries == g1_expected_count::steam)
            {
                std::cout << "Got Steam G1.DAT variant, will fix elements automatically." << std::endl;
            }
        }

        // Read element headers
        auto elements32 = std::vector<G1Element32>(header.num_entries);
        if (!readData(stream, elements32.data(), header.num_entries))
        {
            throw std::runtime_error("Reading g1 element headers failed.");
        }
        auto elements = convertElements(elements32);

        // Read element data
        auto elementData = std::make_unique<std::byte[]>(header.total_size);
        if (!readData(stream, elementData.get(), header.total_size))
        {
            throw std::runtime_error("Reading g1 elements failed.");
        }
        stream.close();

        // The steam G1.DAT is missing two localised tutorial icons, and a smaller font variant
        // This code copies the closest variants into their place, and moves other elements accordingly
        if (header.num_entries == g1_expected_count::steam)
        {
            elements.resize(g1_expected_count::disc);

            // Extra two tutorial images
            std::copy_n(&elements[3549], header.num_entries - 3549, &elements[3551]);
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

    // 0x00447485
    // edi: context
    // ebp: fill
    void clear(Context& context, uint32_t fill)
    {
        int32_t w = context.width / (1 << context.zoom_level);
        int32_t h = context.height / (1 << context.zoom_level);
        uint8_t* ptr = context.bits;

        for (int32_t y = 0; y < h; y++)
        {
            std::fill_n(ptr, w, fill);
            ptr += w + context.pitch;
        }
    }

    void clearSingle(Context& context, uint8_t paletteId)
    {
        auto fill = (paletteId << 24) | (paletteId << 16) | (paletteId << 8) | paletteId;
        clear(context, fill);
    }

    // 0x004957C4
    int16_t clipString(int16_t width, char* string)
    {

        registers regs;
        regs.di = width;
        regs.esi = (int32_t)string;
        call(0x004957C4, regs);
        return regs.cx;
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
        int16_t fontSpriteBase = _currentFontSpriteBase;

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
                case ControlCodes::move_x:
                    width = *str;
                    str++;
                    break;

                case ControlCodes::adjust_palette:
                case 3:
                case 4:
                    str++;
                    break;

                case ControlCodes::newline:
                case ControlCodes::newline_smaller:
                    continue;

                case ControlCodes::font_small:
                    fontSpriteBase = Font::small;
                    break;

                case ControlCodes::font_large:
                    fontSpriteBase = Font::large;
                    break;

                case ControlCodes::font_bold:
                    fontSpriteBase = Font::medium_bold;
                    break;

                case ControlCodes::font_regular:
                    fontSpriteBase = Font::medium_normal;
                    break;

                case ControlCodes::outline:
                case ControlCodes::outline_off:
                case ControlCodes::window_colour_1:
                case ControlCodes::window_colour_2:
                case ControlCodes::window_colour_3:
                case ControlCodes::window_colour_4:
                    break;

                case ControlCodes::inline_sprite_str:
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

    static void setTextColours(PaletteIndex_t pal1, PaletteIndex_t pal2, PaletteIndex_t pal3)
    {
        if ((_currentFontFlags & text_draw_flags::inset) != 0)
            return;

        _textColours[1] = pal1;
        _textColours[2] = PaletteIndex::transparent;
        _textColours[3] = PaletteIndex::transparent;
        if ((_currentFontFlags & text_draw_flags::outline) != 0)
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
    static Gfx::point_t loopNewline(Context* context, Gfx::point_t origin, uint8_t* str)
    {
        Gfx::point_t pos = origin;
        while (true)
        {
            // When offscreen in y dimension don't draw text
            // In original this check only performed if pos.y updated instead of every loop
            bool offscreen = true;
            if (pos.y + 19 > context->y)
            {
                if (context->y + context->height > pos.y)
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

                case ControlCodes::adjust_palette:
                    // This control character does not appear in the localisation files
                    assert(false);
                    str++;
                    break;

                case ControlCodes::newline_smaller:
                    pos.x = origin.x;
                    if (_currentFontSpriteBase == Font::medium_normal || _currentFontSpriteBase == Font::medium_bold)
                    {
                        pos.y += 5;
                    }
                    else if (_currentFontSpriteBase == Font::small)
                    {
                        pos.y += 3;
                    }
                    else if (_currentFontSpriteBase == Font::large)
                    {
                        pos.y += 9;
                    }
                    break;

                case ControlCodes::newline:
                    pos.x = origin.x;
                    if (_currentFontSpriteBase == Font::medium_normal || _currentFontSpriteBase == Font::medium_bold)
                    {
                        pos.y += 10;
                    }
                    else if (_currentFontSpriteBase == Font::small)
                    {
                        pos.y += 6;
                    }
                    else if (_currentFontSpriteBase == Font::large)
                    {
                        pos.y += 18;
                    }
                    break;

                case ControlCodes::move_x:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    break;
                }

                case ControlCodes::newline_x_y:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    offset = *str;
                    str++;
                    pos.y = origin.y + offset;

                    break;
                }

                case ControlCodes::font_small:
                    _currentFontSpriteBase = Font::small;
                    break;
                case ControlCodes::font_large:
                    _currentFontSpriteBase = Font::large;
                    break;
                case ControlCodes::font_regular:
                    _currentFontSpriteBase = Font::medium_normal;
                    break;
                case ControlCodes::font_bold:
                    _currentFontSpriteBase = Font::medium_bold;
                    break;
                case ControlCodes::outline:
                    _currentFontFlags = _currentFontFlags | text_draw_flags::outline;
                    break;
                case ControlCodes::outline_off:
                    _currentFontFlags = _currentFontFlags & ~text_draw_flags::outline;
                    break;
                case ControlCodes::window_colour_1:
                {
                    int hue = _windowColours[0];
                    setTextColours(Colour::getShade(hue, 7), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }
                case ControlCodes::window_colour_2:
                {
                    int hue = _windowColours[1];
                    setTextColours(Colour::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }
                case ControlCodes::window_colour_3:
                {
                    int hue = _windowColours[2];
                    setTextColours(Colour::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }
                case ControlCodes::window_colour_4:
                {
                    int hue = _windowColours[3];
                    setTextColours(Colour::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                    break;
                }

                case ControlCodes::inline_sprite_str:
                {
                    uint32_t image = ((uint32_t*)str)[0];
                    uint32_t imageId = image & 0x7FFFF;
                    str += 4;

                    if ((_currentFontFlags & text_draw_flags::inset) != 0)
                    {
                        Gfx::drawImageSolid(context, pos.x, pos.y, imageId, _textColours[3]);
                        Gfx::drawImageSolid(context, pos.x + 1, pos.y + 1, imageId, _textColours[1]);
                    }
                    else
                    {
                        Gfx::drawImage(context, pos.x, pos.y, image);
                    }

                    pos.x += _g1Elements[imageId].width;
                    break;
                }

                case ControlCodes::colour_black:
                    setTextColour(0);
                    break;

                case ControlCodes::colour_grey:
                    setTextColour(1);
                    break;

                case ControlCodes::colour_white:
                    setTextColour(2);
                    break;

                case ControlCodes::colour_red:
                    setTextColour(3);
                    break;

                case ControlCodes::colour_green:
                    setTextColour(4);
                    break;

                case ControlCodes::colour_yellow:
                    setTextColour(5);
                    break;

                case ControlCodes::colour_topaz:
                    setTextColour(6);
                    break;

                case ControlCodes::colour_celadon:
                    setTextColour(7);
                    break;

                case ControlCodes::colour_babyblue:
                    setTextColour(8);
                    break;

                case ControlCodes::colour_palelavender:
                    setTextColour(9);
                    break;

                case ControlCodes::colour_palegold:
                    setTextColour(10);
                    break;

                case ControlCodes::colour_lightpink:
                    setTextColour(11);
                    break;

                case ControlCodes::colour_pearlaqua:
                    setTextColour(12);
                    break;

                case ControlCodes::colour_palesilver:
                    setTextColour(13);
                    break;

                default:
                    if (!offscreen)
                    {
                        // When offscreen in the y dimension there is no requirement to keep pos.x correct
                        if (chr >= 32)
                        {
                            Gfx::drawImagePaletteSet(context, pos.x, pos.y, 1116 + chr - 32 + _currentFontSpriteBase, _textColours);
                            pos.x += _characterWidths[chr - 32 + _currentFontSpriteBase];
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
     * @param context @<edi>
     * @param text @<esi>
     */
    Gfx::point_t drawString(Context* context, int16_t x, int16_t y, uint8_t colour, void* str)
    {
        // 0x00E04348, 0x00E0434A
        Gfx::point_t origin = { x, y };

        if (colour == FormatFlags::fe)
        {
            return loopNewline(context, origin, (uint8_t*)str);
        }

        if (colour == FormatFlags::fd)
        {
            _currentFontFlags = 0;
            setTextColour(0);
            return loopNewline(context, origin, (uint8_t*)str);
        }

        if (x >= context->x + context->width)
            return origin;

        if (x < context->x - 1280)
            return origin;

        if (y >= context->y + context->height)
            return origin;

        if (y < context->y - 90)
            return origin;

        if (colour == FormatFlags::ff)
        {
            return loopNewline(context, origin, (uint8_t*)str);
        }

        _currentFontFlags = 0;
        if (_currentFontSpriteBase == Font::m1)
        {
            _currentFontSpriteBase = Font::medium_bold;
            _currentFontFlags = _currentFontFlags | text_draw_flags::dark;
        }
        else if (_currentFontSpriteBase == Font::m2)
        {
            _currentFontSpriteBase = Font::medium_bold;
            _currentFontFlags = _currentFontFlags | text_draw_flags::dark;
            _currentFontFlags = _currentFontFlags | text_draw_flags::extra_dark;
        }

        _textColours[0] = PaletteIndex::transparent;
        _textColours[1] = Colour::getShade(Colour::dark_purple, 5);
        _textColours[2] = Colour::getShade(Colour::bright_pink, 5);
        _textColours[3] = Colour::getShade(Colour::light_blue, 5);

        if (colour & FormatFlags::textflag_5)
        {
            colour &= ~FormatFlags::textflag_5;
            _currentFontFlags = _currentFontFlags | text_draw_flags::outline;
        }

        if (colour & FormatFlags::textflag_6)
        {
            colour &= ~FormatFlags::textflag_6;
            _currentFontFlags = _currentFontFlags | text_draw_flags::inset;
        }

        if ((_currentFontFlags & text_draw_flags::inset) != 0)
        {
            if ((_currentFontFlags & text_draw_flags::dark) != 0 && (_currentFontFlags & text_draw_flags::extra_dark) != 0)
            {
                _textColours[1] = Colour::getShade(colour, 2);
                _textColours[2] = PaletteIndex::transparent;
                _textColours[3] = Colour::getShade(colour, 4);
            }
            else if ((_currentFontFlags & text_draw_flags::dark) != 0)
            {
                _textColours[1] = Colour::getShade(colour, 3);
                _textColours[2] = PaletteIndex::transparent;
                _textColours[3] = Colour::getShade(colour, 5);
            }
            else
            {
                _textColours[1] = Colour::getShade(colour, 4);
                _textColours[2] = PaletteIndex::transparent;
                _textColours[3] = Colour::getShade(colour, 6);
            }
        }
        else
        {
            setTextColours(Colour::getShade(colour, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
        }

        return loopNewline(context, origin, (uint8_t*)str);
    }

    // 0x00495224
    // al: colour
    // bp: width
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    int16_t drawString_495224(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.bp = width;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00495224, regs);

        return regs.dx;
    }

    // 0x00494B3F
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawString_494B3F(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00494B3F, regs);
    }

    /**
     *
     * @param context @<edi>
     * @param origin {x @<cx>, y @<dx>}
     * @param colour @<al>
     * @param stringId  @<bx>
     * @param args @<edi>
     */
    void drawString_494B3F(
        Context& context,
        point_t* origin,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = origin->x;
        regs.dx = origin->y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00494B3F, regs);

        origin->x = regs.cx;
        origin->y = regs.dx;
    }

    // 0x00494BBF
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    // bp: width
    void drawString_494BBF(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        regs.bp = width;
        call(0x00494BBF, regs);
    }

    // 0x00494C78
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawString_494C78(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00494C78, regs);
    }

    // 0x00494CB2
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawStringUnderline(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00494CB2, regs);
    }

    // 0x00494D78
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawStringLeftUnderline(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00494D78, regs);
    }

    // 0x00494DE8
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawStringCentred(
        Context& context,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)args;
        regs.edi = (int32_t)&context;
        call(0x00494DE8, regs);
    }

    // 0x00494C36
    // al: colour
    // bx: string id
    // bp: width
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawStringCentredClipped(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.edi = (int32_t)&context;
        regs.esi = (int32_t)args;
        regs.ebx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bp = width;
        call(0x00494C36, regs);
    }

    /**
     * 0x00494ECF
     *
     * @param context @<edi>
     * @param origin {x @<cx>, y @<dx>}
     * @param width @<bp>
     * @param colour @<al>
     * @param stringId @<bx>
     * @param args @<esi>
     * returns width @<ax>
     */
    uint16_t drawStringCentredWrapped(
        Context* context,
        point_t* origin,
        uint16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.edi = (uintptr_t)context;
        regs.esi = (uintptr_t)args;
        regs.cx = origin->x;
        regs.dx = origin->y;
        regs.bp = width;
        regs.al = colour;
        regs.bx = stringId;
        call(0x00494ECF, regs);

        origin->x = regs.cx;
        origin->y = regs.dx;
        return regs.ax;
    }

    // 0x00494E33
    // al: colour
    // bx: string id
    // bp: width
    // cx: x
    // dx: y
    // esi: args
    // edi: context
    void drawStringCentredRaw(
        Context& context,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        const void* args)
    {
        registers regs;
        regs.edi = (int32_t)&context;
        regs.esi = (int32_t)args;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bp = width;
        call(0x00494E33, regs);
    }

    // 0x00495715
    // @param buffer @<esi>
    // @return width @<cx>
    uint16_t getStringWidthNewLined(const char* buffer)
    {
        registers regs;
        regs.esi = (uintptr_t)buffer;
        call(0x00495715, regs);
        return regs.cx;
    }

    std::pair<uint16_t, uint16_t> wrapString(const char* buffer, uint16_t stringWidth)
    {
        // gfx_wrap_string
        registers regs;
        regs.esi = (uintptr_t)buffer;
        regs.di = stringWidth;
        call(0x00495301, regs);

        return std::make_pair(regs.cx, regs.di);
    }

    // 0x004474BA
    static void drawRectImpl(Gfx::Context* context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = (uint32_t)context;
        call(0x004474BA, regs);
    }

    void fillRect(Gfx::Context* context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        drawRectImpl(context, left, top, right, bottom, colour);
    }

    void drawRect(Gfx::Context* context, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour)
    {
        // This makes the function signature more like a drawing application
        drawRectImpl(context, x, y, x + dx - 1, y + dy - 1, colour);
    }

    void fillRectInset(Gfx::Context* context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = (uint32_t)context;
        regs.si = flags;
        call(0x004C58C7, regs);
    }

    void drawRectInset(Gfx::Context* context, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags)
    {
        // This makes the function signature more like a drawing application
        fillRectInset(context, x, y, x + dx - 1, y + dy - 1, colour, flags);
    }

    // 0x00452DA4
    void drawLine(Gfx::Context* context, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        registers regs;
        regs.ax = left;
        regs.bx = top;
        regs.cx = right;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = (uint32_t)context;
        call(0x00452DA4, regs);
    }

    // 0x004CD406
    void invalidateScreen()
    {
        setDirtyBlocks(0, 0, Ui::width(), Ui::height());
    }

    Drawing::SoftwareDrawingEngine* engine;

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
        if (engine == nullptr)
            engine = new Drawing::SoftwareDrawingEngine();

        engine->setDirtyBlocks(left, top, right, bottom);
    }

    // 0x004C5CFA
    void drawDirtyBlocks()
    {
        if (engine == nullptr)
            engine = new Drawing::SoftwareDrawingEngine();

        engine->drawDirtyBlocks();
    }

    loco_global<char[512], 0x0112CC04> byte_112CC04;
    loco_global<char[512], 0x0112CE04> byte_112CE04;

    // 0x004CF63B
    void render()
    {
        if (engine == nullptr)
            engine = new Drawing::SoftwareDrawingEngine();

        char backup1[512] = { 0 };
        char backup2[512] = { 0 };

        std::memcpy(backup1, byte_112CC04, 512);
        std::memcpy(backup2, byte_112CE04, 512);

        if (Ui::dirtyBlocksInitialised())
        {
            engine->drawDirtyBlocks();
        }

        if (Input::hasFlag(Input::Flags::flag5))
        {
            call(0x004072EC); // NOP on _NO_LOCO_WIN32_
        }
        else
        {
            Ui::processMessages();
        }

        if (addr<0x005252AC, uint32_t>() != 0)
        {
            //            sub_4058F5();
        }

        std::memcpy(byte_112CC04, backup1, 512);
        std::memcpy(byte_112CE04, backup2, 512);
    }

    void redrawScreenRect(Rect rect)
    {
        engine->drawRect(rect);
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

    void drawImage(Gfx::Context* context, int16_t x, int16_t y, uint32_t image)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.ebx = image;
        regs.edi = (uint32_t)context;
        call(0x00448C79, regs);
    }

    uint32_t recolour(uint32_t image)
    {
        return ImageIdFlags::remap | image;
    }

    uint32_t recolour(uint32_t image, uint8_t colour)
    {
        return ImageIdFlags::remap | (colour << 19) | image;
    }

    uint32_t recolour2(uint32_t image, uint8_t colour1, uint8_t colour2)
    {
        return ImageIdFlags::remap | ImageIdFlags::remap2 | (colour1 << 19) | (colour2 << 24) | image;
    }

    uint32_t recolour2(uint32_t image, ColourScheme colourScheme)
    {
        return recolour2(image, colourScheme.primary, colourScheme.secondary);
    }

    uint32_t recolourTranslucent(uint32_t image, uint8_t colour)
    {
        return ImageIdFlags::translucent | (colour << 19) | image;
    }

    loco_global<uint8_t*, 0x0050B860> _50B860;
    loco_global<uint32_t, 0x00E04324> _E04324;

    void drawImageSolid(Gfx::Context* context, int16_t x, int16_t y, uint32_t image, uint8_t palette_index)
    {
        uint8_t palette[256];
        memset(palette, palette_index, 256);
        palette[0] = 0;

        drawImagePaletteSet(context, x, y, image, palette);
    }

    void drawImagePaletteSet(Gfx::Context* context, int16_t x, int16_t y, uint32_t image, uint8_t* palette)
    {
        _50B860 = palette;
        _E04324 = 0x20000000;
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.ebx = image;
        regs.edi = (uint32_t)context;
        call(0x00448D90, regs);
    }

    bool clipContext(Gfx::Context** dst, Gfx::Context* src, int16_t x, int16_t y, int16_t width, int16_t height)
    {
        registers regs;
        regs.ax = x;
        regs.bx = width;
        regs.edi = (int32_t)src;
        regs.dx = height;
        regs.cx = y;
        call(0x4cec50, regs);
        *dst = (Gfx::Context*)regs.edi;

        return *dst != nullptr;
    }

    bool clipContext(Gfx::Context** dst, Gfx::Context* src, point_t pos, Gfx::ui_size_t size)
    {
        return clipContext(dst, src, pos.x, pos.y, size.width, size.height);
    }

    G1Element* getG1Element(uint32_t id)
    {
        if (id < _g1Elements.size())
        {
            return &_g1Elements[id];
        }
        return nullptr;
    }
}
