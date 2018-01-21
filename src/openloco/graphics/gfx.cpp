#include "gfx.h"
#include "../interop/interop.hpp"
#include "../environment.h"
#include "../ui.h"

using namespace openloco::interop;

namespace openloco::gfx
{
    loco_global<drawpixelinfo_t, 0x0050B884> _screen_dpi;
    loco_global_array<g1_element_t, LOCO_G1_ELEMENT_COUNT, 0x9E2424> _g1Elements;

    drawpixelinfo_t& screen_dpi()
    {
        return _screen_dpi;
    }

    // 0x0044733C
    void load_g1()
    {
        auto g1Path = environment::get_path(environment::path_id::g1);

        FILE * file;
        gfx::g1_header_t header;
        void * g1Buffer;

        file = fopen(g1Path.make_preferred().u8string().c_str(), "rb");

        try
        {
            if (file == nullptr)
            {
                throw std::exception("Opening g1 file failed.");
            }

            if (fread(&header, 8, 1, file) != 1)
            {
                throw std::exception("Reading g1 file header failed.");
            }

            // Read element headers
            if (fread(_g1Elements, header.num_entries * sizeof(g1_element_t), 1, file) != 1)
            {
                throw std::exception("Reading g1 element headers failed.");
            }

            // Read element data
            g1Buffer = malloc(header.total_size);
            if (fread(g1Buffer, header.total_size, 1, file) != 1)
            {
                free(g1Buffer);
                throw std::exception("Reading g1 elements failed.");
            }

            // Adjust memory offsets
            for (uint32_t i = 0; i < header.num_entries; i++)
            {
                _g1Elements[i].offset += (int32_t)g1Buffer;
            }
            fclose(file);
        }
        catch (const std::exception &e)
        {
            if (file != nullptr)
            {
                fclose(file);
            }
            throw std::runtime_error(e.what());
        }
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
