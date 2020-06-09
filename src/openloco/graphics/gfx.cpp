#include "gfx.h"
#include "../console.h"
#include "../drawing/SoftwareDrawingEngine.h"
#include "../environment.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/languagefiles.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../utility/stream.hpp"
#include "colours.h"
#include "image_ids.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

using namespace openloco::interop;
using namespace openloco::utility;
using namespace openloco::drawing;
using namespace openloco::ui;

namespace openloco::gfx
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

    static loco_global<drawpixelinfo_t, 0x0050B884> _screen_dpi;
    static loco_global<drawpixelinfo_t, 0x005233B8> _windowDPI;

    static loco_global<g1_element[g1_expected_count::disc + g1_count_temporary + g1_count_objects], 0x9E2424> _g1Elements;

    static std::unique_ptr<std::byte[]> _g1Buffer;

    static loco_global<uint16_t, 0x112C824> _currentFontFlags;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;
    static loco_global<uint8_t[4], 0x1136594> _windowColours;

    static palette_index_t _textColours[8] = { 0 };

    drawpixelinfo_t& screen_dpi()
    {
        return _screen_dpi;
    }

    static std::vector<g1_element> convert_elements(const std::vector<g1_element32_t>& elements32)
    {
        auto elements = std::vector<g1_element>();
        elements.reserve(elements32.size());
        std::transform(
            elements32.begin(),
            elements32.end(),
            std::back_inserter(elements),
            [](g1_element32_t src) { return g1_element(src); });
        return elements;
    }

    // 0x0044733C
    void load_g1()
    {
        auto g1Path = environment::get_path(environment::path_id::g1);
        std::ifstream stream(g1Path, std::ios::in | std::ios::binary);
        if (!stream)
        {
            throw std::runtime_error("Opening g1 file failed.");
        }

        g1_header_t header;
        if (!read_data(stream, header))
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
        auto elements32 = std::vector<g1_element32_t>(header.num_entries);
        if (!read_data(stream, elements32.data(), header.num_entries))
        {
            throw std::runtime_error("Reading g1 element headers failed.");
        }
        auto elements = convert_elements(elements32);

        // Read element data
        auto elementData = std::make_unique<std::byte[]>(header.total_size);
        if (!read_data(stream, elementData.get(), header.total_size))
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

    g1_element* get_g1_element(uint32_t image)
    {
        return &_g1Elements[image];
    }

    // 0x00447485
    // edi: dpi
    // ebp: fill
    void clear(drawpixelinfo_t& dpi, uint32_t fill)
    {
        registers regs;
        regs.edi = (int32_t)&dpi;
        regs.ebp = (int32_t)fill;
        call(0x00447485, regs);
    }

    void clear_single(drawpixelinfo_t& dpi, uint8_t paletteId)
    {
        auto fill = (paletteId << 24) | (paletteId << 16) | (paletteId << 8) | paletteId;
        clear(dpi, fill);
    }

    // 0x004957C4
    int16_t clip_string(int16_t width, char* string)
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
    uint16_t get_string_width(const char* buffer)
    {
        registers regs;
        regs.esi = (uintptr_t)buffer;
        call(0x495685, regs);
        return regs.cx;
    }

    static void setTextColours(palette_index_t pal1, palette_index_t pal2, palette_index_t pal3)
    {
        if ((_currentFontFlags & text_draw_flags::inset) != 0)
            return;

        _textColours[1] = pal1;
        _textColours[2] = palette_index::transparent;
        _textColours[3] = palette_index::transparent;
        if ((_currentFontFlags & text_draw_flags::outline) != 0)
        {
            _textColours[2] = pal2;
            _textColours[3] = pal3;
        }
    }

    static void setTextColour(int colour)
    {
        auto el = &_g1Elements[image_ids::text_palette];
        setTextColours(el->offset[colour * 4 + 0], el->offset[colour * 4 + 1], el->offset[colour * 4 + 2]);
    }

    // 0x00451189
    static gfx::point_t loop_newline(drawpixelinfo_t* context, gfx::point_t origin, uint8_t* str)
    {
        gfx::point_t pos = origin;
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

                case control_codes::adjust_palette:
                    // This control character does not appear in the localisation files
                    assert(false);
                    str++;
                    break;

                case control_codes::newline_smaller:
                    pos.x = origin.x;
                    if (_currentFontSpriteBase == font::medium_normal || _currentFontSpriteBase == font::medium_bold)
                    {
                        pos.y += 5;
                    }
                    else if (_currentFontSpriteBase == font::small)
                    {
                        pos.y += 3;
                    }
                    else if (_currentFontSpriteBase == font::large)
                    {
                        pos.y += 9;
                    }
                    break;

                case control_codes::newline:
                    pos.x = origin.x;
                    if (_currentFontSpriteBase == font::medium_normal || _currentFontSpriteBase == font::medium_bold)
                    {
                        pos.y += 10;
                    }
                    else if (_currentFontSpriteBase == font::small)
                    {
                        pos.y += 6;
                    }
                    else if (_currentFontSpriteBase == font::large)
                    {
                        pos.y += 18;
                    }
                    break;

                case control_codes::move_x:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    break;
                }

                case control_codes::newline_x_y:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    offset = *str;
                    str++;
                    pos.y = origin.y + offset;

                    break;
                }

                case control_codes::font_small:
                    _currentFontSpriteBase = font::small;
                    break;
                case control_codes::font_large:
                    _currentFontSpriteBase = font::large;
                    break;
                case control_codes::font_regular:
                    _currentFontSpriteBase = font::medium_normal;
                    break;
                case control_codes::font_bold:
                    _currentFontSpriteBase = font::medium_bold;
                    break;
                case control_codes::outline:
                    _currentFontFlags = _currentFontFlags | text_draw_flags::outline;
                    break;
                case control_codes::outline_off:
                    _currentFontFlags = _currentFontFlags & ~text_draw_flags::outline;
                    break;
                case control_codes::window_colour_1:
                {
                    int hue = _windowColours[0];
                    setTextColours(colour::get_shade(hue, 7), palette_index::index_0A, palette_index::index_0A);
                    break;
                }
                case control_codes::window_colour_2:
                {
                    int hue = _windowColours[1];
                    setTextColours(colour::get_shade(hue, 9), palette_index::index_0A, palette_index::index_0A);
                    break;
                }
                case control_codes::window_colour_3:
                {
                    int hue = _windowColours[2];
                    setTextColours(colour::get_shade(hue, 9), palette_index::index_0A, palette_index::index_0A);
                    break;
                }

                case control_codes::inline_sprite_str:
                {
                    uint32_t image = ((uint32_t*)str)[0];
                    uint32_t imageId = image & 0x7FFFF;
                    str += 4;

                    if ((_currentFontFlags & text_draw_flags::inset) != 0)
                    {
                        gfx::draw_image_solid(context, pos.x, pos.y, imageId, _textColours[3]);
                        gfx::draw_image_solid(context, pos.x + 1, pos.y + 1, imageId, _textColours[1]);
                    }
                    else
                    {
                        gfx::draw_image(context, pos.x, pos.y, image);
                    }

                    pos.x += _g1Elements[imageId].width;
                    break;
                }

                case control_codes::colour_black:
                    setTextColour(0);
                    break;

                case control_codes::colour_grey:
                    setTextColour(1);
                    break;

                case control_codes::colour_white:
                    setTextColour(2);
                    break;

                case control_codes::colour_red:
                    setTextColour(3);
                    break;

                case control_codes::colour_green:
                    setTextColour(4);
                    break;

                case control_codes::colour_yellow:
                    setTextColour(5);
                    break;

                case control_codes::colour_topaz:
                    setTextColour(6);
                    break;

                case control_codes::colour_celadon:
                    setTextColour(7);
                    break;

                case control_codes::colour_babyblue:
                    setTextColour(8);
                    break;

                case control_codes::colour_palelavender:
                    setTextColour(9);
                    break;

                case control_codes::colour_palegold:
                    setTextColour(10);
                    break;

                case control_codes::colour_lightpink:
                    setTextColour(11);
                    break;

                case control_codes::colour_pearlaqua:
                    setTextColour(12);
                    break;

                case control_codes::colour_palesilver:
                    setTextColour(13);
                    break;

                default:
                    if (!offscreen)
                    {
                        // When offscreen in the y dimension there is no requirement to keep pos.x correct
                        if (chr >= 32)
                        {
                            gfx::draw_image_palette_set(context, pos.x, pos.y, 1116 + chr - 32 + _currentFontSpriteBase, _textColours);
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
    gfx::point_t draw_string(drawpixelinfo_t* context, int16_t x, int16_t y, uint8_t colour, void* str)
    {
        // 0x00E04348, 0x00E0434A
        gfx::point_t origin = { x, y };

        if (colour == format_flags::fe)
        {
            return loop_newline(context, origin, (uint8_t*)str);
        }

        if (colour == format_flags::fd)
        {
            _currentFontFlags = 0;
            setTextColour(0);
            return loop_newline(context, origin, (uint8_t*)str);
        }

        if (x >= context->x + context->width)
            return origin;

        if (x < context->x - 1280)
            return origin;

        if (y >= context->y + context->height)
            return origin;

        if (y < context->y - 90)
            return origin;

        if (colour == format_flags::ff)
        {
            return loop_newline(context, origin, (uint8_t*)str);
        }

        _currentFontFlags = 0;
        if (_currentFontSpriteBase == font::m1)
        {
            _currentFontSpriteBase = font::medium_bold;
            _currentFontFlags = _currentFontFlags | text_draw_flags::dark;
        }
        else if (_currentFontSpriteBase == font::m2)
        {
            _currentFontSpriteBase = font::medium_bold;
            _currentFontFlags = _currentFontFlags | text_draw_flags::dark;
            _currentFontFlags = _currentFontFlags | text_draw_flags::extra_dark;
        }

        _textColours[0] = palette_index::transparent;
        _textColours[1] = colour::get_shade(colour::dark_purple, 5);
        _textColours[2] = colour::get_shade(colour::bright_pink, 5);
        _textColours[3] = colour::get_shade(colour::light_blue, 5);

        if (colour & format_flags::textflag_5)
        {
            colour &= ~format_flags::textflag_5;
            _currentFontFlags = _currentFontFlags | text_draw_flags::outline;
        }

        if (colour & format_flags::textflag_6)
        {
            colour &= ~format_flags::textflag_6;
            _currentFontFlags = _currentFontFlags | text_draw_flags::inset;
        }

        if ((_currentFontFlags & text_draw_flags::inset) != 0)
        {
            if ((_currentFontFlags & text_draw_flags::dark) != 0 && (_currentFontFlags & text_draw_flags::extra_dark) != 0)
            {
                _textColours[1] = colour::get_shade(colour, 2);
                _textColours[2] = palette_index::transparent;
                _textColours[3] = colour::get_shade(colour, 4);
            }
            else if ((_currentFontFlags & text_draw_flags::dark) != 0)
            {
                _textColours[1] = colour::get_shade(colour, 3);
                _textColours[2] = palette_index::transparent;
                _textColours[3] = colour::get_shade(colour, 5);
            }
            else
            {
                _textColours[1] = colour::get_shade(colour, 4);
                _textColours[2] = palette_index::transparent;
                _textColours[3] = colour::get_shade(colour, 6);
            }
        }
        else
        {
            setTextColours(colour::get_shade(colour, 9), palette_index::index_0A, palette_index::index_0A);
        }

        return loop_newline(context, origin, (uint8_t*)str);
    }

    // 0x00495224
    // al: colour
    // bp: width
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    int16_t draw_string_495224(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        call(0x00495224, regs);

        return regs.dx;
    }

    // 0x00494B3F
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    void draw_string_494B3F(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        call(0x00494B3F, regs);
    }

    /**
     *
     * @param dpi @<edi>
     * @param origin {x @<cx>, y @<dx>}
     * @param colour @<al>
     * @param stringId  @<bx>
     * @param args @<edi>
     */
    void draw_string_494B3F(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
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
    // edi: dpi
    // bp: width
    void draw_string_494BBF(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        regs.bp = width;
        call(0x00494BBF, regs);
    }

    // 0x00494C78
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    void draw_string_494C78(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        call(0x00494C78, regs);
    }

    // 0x00494CB2
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    void draw_string_underline(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        call(0x00494CB2, regs);
    }

    // 0x00494D78
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    void draw_string_left_underline(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        call(0x00494D78, regs);
    }

    // 0x00494DE8
    // al: colour
    // bx: string id
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    void draw_string_centred(
        drawpixelinfo_t& dpi,
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
        regs.edi = (int32_t)&dpi;
        call(0x00494DE8, regs);
    }

    // 0x00494C36
    // al: colour
    // bx: string id
    // bp: width
    // cx: x
    // dx: y
    // esi: args
    // edi: dpi
    void draw_string_centred_clipped(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const void* args)
    {
        registers regs;
        regs.edi = (int32_t)&dpi;
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
     */
    void draw_string_centred_wrapped(
        drawpixelinfo_t* context,
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
    }

    // 0x004474BA
    static void draw_rect_impl(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = (uint32_t)dpi;
        call(0x004474BA, regs);
    }

    void fill_rect(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        draw_rect_impl(dpi, left, top, right, bottom, colour);
    }

    void draw_rect(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour)
    {
        // This makes the function signature more like a drawing application
        draw_rect_impl(dpi, x, y, x + dx - 1, y + dy - 1, colour);
    }

    void fill_rect_inset(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour, uint8_t flags)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = (uint32_t)dpi;
        regs.si = flags;
        call(0x004C58C7, regs);
    }

    void draw_rect_inset(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t colour, uint8_t flags)
    {
        // This makes the function signature more like a drawing application
        fill_rect_inset(dpi, x, y, x + dx - 1, y + dy - 1, colour, flags);
    }

    // 0x00452DA4
    void draw_line(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        registers regs;
        regs.ax = left;
        regs.bx = top;
        regs.cx = right;
        regs.dx = bottom;
        regs.ebp = colour;
        regs.edi = (uint32_t)dpi;
        call(0x00452DA4, regs);
    }

    // 0x004CD406
    void invalidate_screen()
    {
        set_dirty_blocks(0, 0, ui::width(), ui::height());
    }

    drawing::SoftwareDrawingEngine* engine;

    /**
     * 0x004C5C69
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void set_dirty_blocks(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        if (engine == nullptr)
            engine = new drawing::SoftwareDrawingEngine();

        engine->setDirtyBlocks(left, top, right, bottom);
    }

    // 0x004C5CFA
    void draw_dirty_blocks()
    {
        if (engine == nullptr)
            engine = new drawing::SoftwareDrawingEngine();

        engine->drawDirtyBlocks();
    }

    loco_global<char[512], 0x0112CC04> byte_112CC04;
    loco_global<char[512], 0x0112CE04> byte_112CE04;

    // 0x004CF63B
    void render()
    {
        if (engine == nullptr)
            engine = new drawing::SoftwareDrawingEngine();

        char backup1[512] = { 0 };
        char backup2[512] = { 0 };

        std::memcpy(backup1, byte_112CC04, 512);
        std::memcpy(backup2, byte_112CE04, 512);

        if (ui::dirty_blocks_initialised())
        {
            engine->drawDirtyBlocks();
        }

        if (input::has_flag(input::input_flags::flag5))
        {
            call(0x004072EC); // NOP on _NO_LOCO_WIN32_
        }
        else
        {
            ui::process_messages();
        }

        if (addr<0x005252AC, uint32_t>() != 0)
        {
            //            sub_4058F5();
        }

        std::memcpy(byte_112CC04, backup1, 512);
        std::memcpy(byte_112CE04, backup2, 512);
    }

    void redraw_screen_rect(Rect rect)
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
    void redraw_screen_rect(int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        redraw_screen_rect(Rect::fromLTRB(left, top, right, bottom));
    }

    void draw_image(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.ebx = image;
        regs.edi = (uint32_t)dpi;
        call(0x00448C79, regs);
    }

    uint32_t recolour(uint32_t image)
    {
        return (1 << 29) | image;
    }

    uint32_t recolour(uint32_t image, uint8_t colour)
    {
        return (1 << 29) | (colour << 19) | image;
    }

    loco_global<uint8_t*, 0x0050B860> _50B860;
    loco_global<uint32_t, 0x00E04324> _E04324;

    void draw_image_solid(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image, uint8_t palette_index)
    {
        uint8_t palette[256];
        memset(palette, palette_index, 256);
        palette[0] = 0;

        draw_image_palette_set(dpi, x, y, image, palette);
    }

    void draw_image_palette_set(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image, uint8_t* palette)
    {
        _50B860 = palette;
        _E04324 = 0x20000000;
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.ebx = image;
        regs.edi = (uint32_t)dpi;
        call(0x00448D90, regs);
    }

    bool clip_drawpixelinfo(gfx::drawpixelinfo_t** dst, gfx::drawpixelinfo_t* src, int16_t x, int16_t y, int16_t width, int16_t height)
    {
        registers regs;
        regs.ax = x;
        regs.bx = width;
        regs.edi = (int32_t)src;
        regs.dx = height;
        regs.cx = y;
        call(0x4cec50, regs);
        *dst = (gfx::drawpixelinfo_t*)regs.edi;

        return *dst != nullptr;
    }

    g1_element* get_g1element(uint32_t id)
    {
        if (id < _g1Elements.size())
        {
            return &_g1Elements[id];
        }
        return nullptr;
    }
}
