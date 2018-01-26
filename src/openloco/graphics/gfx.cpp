#include "gfx.h"
#include "../environment.h"
#include "../interop/interop.hpp"
#include "../ui.h"
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
}
