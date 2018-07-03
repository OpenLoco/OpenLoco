#include "gfx.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../ui.h"
#include "colours.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

using namespace openloco::interop;

namespace openloco::gfx
{
    namespace g1_expected_count
    {
        constexpr uint32_t disc = 0x101A; // And GOG
        constexpr uint32_t steam = 0x0F38;
    }

    static loco_global<drawpixelinfo_t, 0x0050B884> _screen_dpi;
    static loco_global<g1_element[g1_expected_count::disc], 0x9E2424> _g1Elements;

    static std::unique_ptr<std::byte[]> _g1Buffer;

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

    template<typename T1, typename T2, typename T3>
    std::basic_istream<T1, T2>& read_data(std::basic_istream<T1, T2>& stream, T3* dst, size_t count)
    {
        return stream.read((char*)dst, count * sizeof(T3));
    }

    template<typename T1, typename T2, typename T3>
    std::basic_istream<T1, T2>& read_data(std::basic_istream<T1, T2>& stream, T3& dst)
    {
        return read_data(stream, &dst, 1);
    }

    // 0x0044733C
    void load_g1()
    {
        auto g1Path = environment::get_path(environment::path_id::g1);
#ifdef _OPENLOCO_USE_BOOST_FS_
        std::ifstream stream(g1Path.make_preferred().string(), std::ios::in | std::ios::binary);
#else
        std::ifstream stream(g1Path, std::ios::in | std::ios::binary);
#endif
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

    void draw_string(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        const char* string)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.edi = (int32_t)&dpi;
        regs.esi = (int32_t)string;
        call(0x00451025, regs);
    }

    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<int32_t, 0x112C824> _currentFontFlags;
    static loco_global<uint8_t[4], 0x1136594> windowColours;
    static loco_global<uint8_t[224 * 4], 0x112C884> characterWidths;

    /**
     * 0x00451025
     *
     * @param x  @<cx>
     * @param y @<dx>
     * @param context @<edi>
     * @param text @<esi>
     */
    void drawString(int16_t x, int16_t y, drawpixelinfo_t* context, uint8_t* str)
    {
        gfx::point_t origin = { x, y };
        gfx::point_t pos = origin;
        uint8_t colours[8] = { 0 };

        colours[0] = 0;
        colours[1] = 0;
        colours[2] = 0;
        colours[3] = 0;
        colours[4] = colour::get_shade(colour::bright_red, 4);

        // 4510FF
        colours[1] = colour::get_shade(colour::bright_red, 4);
        colours[3] = colour::get_shade(colour::bright_red, 6);

        // 451112
        colours[1] = colour::get_shade(colour::bright_red, 3);
        colours[3] = colour::get_shade(colour::bright_red, 5);

        // 0x451130
        colours[1] = colour::get_shade(colour::bright_red, 2);
        colours[3] = colour::get_shade(colour::bright_red, 4);

        while (true)
        {
            uint8_t chr = *str;
            str++;

            switch (chr)
            {
                case 0:
                    return;

                case 2:
                    assert(false);
                    str++;
                    break;

                case 6:
                    pos.x = origin.x;
                    if (_currentFontSpriteBase <= 224)
                    {
                        pos.y += 5;
                    }
                    else if (_currentFontSpriteBase == 448)
                    {
                        pos.y += 3;
                    }
                    else
                    {
                        pos.y += 9;
                    }
                    break;

                case 5:
                    pos.x = origin.x;
                    if (_currentFontSpriteBase <= 224)
                    {
                        pos.y += 10;
                    }
                    else if (_currentFontSpriteBase == 448)
                    {
                        pos.y += 6;
                    }
                    else
                    {
                        pos.y += 18;
                    }
                    break;

                case 1:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    break;
                }

                case 17:
                {
                    uint8_t offset = *str;
                    str++;
                    pos.x = origin.x + offset;

                    offset = *str;
                    str++;
                    pos.y = origin.y + offset;

                    break;
                }

                case 7:
                    _currentFontSpriteBase = 448;
                    break;
                case 8:
                    _currentFontSpriteBase = 672;
                    break;
                case 10:
                    _currentFontSpriteBase = 0;
                    break;
                case 9:
                    _currentFontSpriteBase = 224;
                    break;
                case 11:
                    _currentFontFlags |= ~(1 << 1);
                    break;
                case 12:
                    _currentFontFlags &= ~(1 << 1);
                    break;
                case 13:
                {
                    int hue = windowColours[0];

                    colours[1] = colour::get_shade(hue, 7);
                    if (_currentFontFlags & (1 << 1))
                    {
                        colours[2] = 0x0A;
                        colours[3] = 0x0A;
                    }
                    break;
                }
                case 14:
                {
                    int hue = windowColours[1];
                    if (_currentFontFlags & (1 << 0))
                    {
                        // 0x451189
                    }

                    colours[1] = colour::get_shade(hue, 9);
                    ;
                    if (_currentFontFlags & (1 << 1))
                    {
                        colours[2] = 0x0A;
                        colours[3] = 0x0A;
                    }
                    break;
                }
                case 15:
                {
                    int hue = windowColours[2];
                    if (_currentFontFlags & (1 << 0))
                    {
                        // 0x451189
                    }

                    colours[1] = colour::get_shade(hue, 9);
                    ;
                    if (_currentFontFlags & (1 << 1))
                    {
                        colours[2] = 0x0A;
                        colours[3] = 0x0A;
                    }
                    break;
                }

                case 144 + 0:
                    // black

                case 144 + 1:
                    // grey

                case 144 + 2:
                    // white

                case 144 + 3:
                    // red

                case 144 + 4:
                    // green

                case 144 + 5:
                    // yellow

                case 144 + 6:
                    // TOPAZ

                case 144 + 7:
                    // CELADON

                case 144 + 8:
                    // BABYBLUE

                case 144 + 9:
                    // PALELAVENDER

                case 144 + 10:
                    // PALEGOLD

                case 144 + 11:
                    // LIGHTPINK

                case 144 + 12:
                    // PEARLAQUA

                case 144 + 13:
                    // PALESILVER
                    break;

                default:
                    if (chr >= 32)
                    {

                        gfx::draw_sprite_palete_set(context, pos.x, pos.y, 1116 + chr - 32 + _currentFontSpriteBase, colours);
                        pos.x += characterWidths[chr - 32 + _currentFontSpriteBase];
                    }
                    break;
            }
        }
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

    void draw_string_centred(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        uint8_t colour,
        string_id stringId,
        const char* args)
    {
        registers regs;
        regs.al = colour;
        regs.bx = stringId;
        regs.cx = x;
        regs.dx = y;
        regs.edi = (int32_t)&dpi;
        call(0x00494DE8, regs);
    }

    void draw_string_centred_clipped(
        drawpixelinfo_t& dpi,
        int16_t x,
        int16_t y,
        int16_t width,
        uint8_t colour,
        string_id stringId,
        const char* args)
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
        const char* args)
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
    static void draw_rect_impl(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t color)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = color;
        regs.edi = (uint32_t)dpi;
        call(0x004474BA, regs);
    }

    void fill_rect(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t color)
    {
        draw_rect_impl(dpi, left, top, right, bottom, color);
    }

    void draw_rect(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint32_t color)
    {
        // This makes the function signature more like a drawing application
        draw_rect_impl(dpi, x, y, x + dx - 1, y + dy - 1, color);
    }

    void fill_rect_inset(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t color, uint8_t flags)
    {
        registers regs;
        regs.ax = left;
        regs.bx = right;
        regs.cx = top;
        regs.dx = bottom;
        regs.ebp = color;
        regs.edi = (uint32_t)dpi;
        regs.si = flags;
        call(0x004C58C7, regs);
    }

    // 0x004CD406
    void invalidate_screen()
    {
        set_dirty_blocks(0, 0, ui::width(), ui::height());
    }

    // 0x004C5C69
    void set_dirty_blocks(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        registers regs;
        regs.ax = left;
        regs.bx = top;
        regs.dx = right;
        regs.bp = bottom;
        call(0x004C5C69, regs);
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

    loco_global<uint8_t*, 0x0050B860> _50B860;
    loco_global<uint32_t, 0x00E04324> _E04324;

    void draw_image_solid(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image, uint8_t palette_index)
    {
        uint8_t palette[256];
        memset(palette, palette_index, 256);
        palette[0] = 0;

        draw_sprite_palete_set(dpi, x, y, image, palette);
    }

    void draw_sprite_palete_set(gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint32_t image, uint8_t* palette)
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
}
