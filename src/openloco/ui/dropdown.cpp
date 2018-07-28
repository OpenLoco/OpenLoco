#include "dropdown.h"
#include "../interop/interop.hpp"

#include <cassert>
#include <cstdarg>

using namespace openloco::interop;

namespace openloco::ui::dropdown
{
    static constexpr int num_args_per_item = 4;

    static loco_global<int16_t, 0x0113D84E> _dropdownHighlightedIndex;
    static loco_global<string_id[40], 0x0113D850> _dropdownItemFormats;
    static loco_global<string_id[40][num_args_per_item], 0x0113D8A0> _dropdownItemArgs;

    void add(int16_t index, string_id title)
    {
        _dropdownItemFormats[index] = title;
    }

    void add(int16_t index, string_id title, int n_args, ...)
    {
        _dropdownItemFormats[index] = title;

        assert(n_args < num_args_per_item);

        va_list args;
        va_start(args, n_args);
        for (int arg_index = 0; arg_index < n_args; arg_index++)
        {
            int arg = va_arg(args, int);
            _dropdownItemArgs[index][arg_index] = static_cast<string_id>(arg);
        }
        va_end(args);
    }

    void set_selection(int16_t index)
    {
        _dropdownHighlightedIndex = index;
    }

    /**
     * 0x004CCA6D
     * x @<cx>
     * y @<dx>
     * width @<bp>
     * height @<di>
     * colour @<al>
     * count @<bl>
     * flags @<bh>
     */
    void show_text(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bl = count;
        regs.bh = flags;
        regs.bp = width;
        regs.di = height;

        call(0x4CCA6D, regs);
    }

    /**
     * 0x004CCC7C
     * x @<cx>
     * y @<dx>
     * width @<bp>
     * height @<di>
     * colour @<al>
     * count @<bl>
     * flags @<bh>
     */
    void show_text_2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bl = count;
        regs.bh = flags;
        regs.bp = width;
        regs.di = height;

        call(0x4CCC7C, regs);
    }
}
