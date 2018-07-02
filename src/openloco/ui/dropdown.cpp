#include "dropdown.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco::ui::dropdown
{
    static loco_global<string_id[40], 0x0113D850> _dropdownItemFormats;

    void add(int16_t index, string_id title)
    {
        _dropdownItemFormats[index] = title;
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
}
