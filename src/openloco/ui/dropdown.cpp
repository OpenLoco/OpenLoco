#include "dropdown.h"
#include "../companymgr.h"
#include "../console.h"
#include "../interop/interop.hpp"
#include "../window.h"

#include <cassert>
#include <cstdarg>
#include <limits>

using namespace openloco::interop;

namespace openloco::ui::dropdown
{
    static constexpr int bytes_per_item = 8;

    static loco_global<uint32_t, 0x0113DC60> _dropdownDisabledItems;
    static loco_global<int16_t, 0x0113D84E> _dropdownHighlightedIndex;
    static loco_global<uint32_t, 0x0113DC64> _dropdownSelection;

    static loco_global<string_id[40], 0x0113D850> _dropdownItemFormats;
    static loco_global<std::byte[40][bytes_per_item], 0x0113D8A0> _dropdownItemArgs;

    void add(size_t index, string_id title)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownItemFormats[index] = title;
    }

    void add(size_t index, string_id title, std::initializer_list<format_arg> l)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownItemFormats[index] = title;

        std::byte* args = _dropdownItemArgs[index];

        for (auto arg : l)
        {
            switch (arg.type)
            {
                case format_arg_type::u16:
                {
                    uint16_t* ptr = (uint16_t*)args;
                    *ptr = arg.u16;
                    args += 2;
                    break;
                }

                case format_arg_type::u32:
                {
                    uint32_t* ptr = (uint32_t*)args;
                    *ptr = arg.u32;
                    args += 4;
                    break;
                }

                case format_arg_type::ptr:
                {
                    uintptr_t* ptr = (uintptr_t*)args;
                    *ptr = arg.ptr;
                    args += 4;
                    break;
                }

                default:
                    console::error("Unknown format: %d", arg.type);
                    break;
            }
        }
    }

    void add(size_t index, string_id title, format_arg l)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        add(static_cast<uint8_t>(index), title, { l });
    }

    int16_t get_highlighted_item()
    {
        return _dropdownHighlightedIndex;
    }

    void set_item_disabled(size_t index)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownDisabledItems |= (1U << static_cast<uint8_t>(index));
    }

    void set_highlighted_item(size_t index)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownHighlightedIndex = static_cast<uint8_t>(index);
    }

    void set_item_selected(size_t index)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownSelection |= (1U << static_cast<uint8_t>(index));
    }

    /**
     * 0x004CC807
     *
     * @param x
     * @param y
     * @param width
     * @param height
     * @param colour
     * @param count
     * @param flags
     */
    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bl = static_cast<uint8_t>(count);
        regs.bh = flags;
        regs.bp = width;
        regs.di = height;

        call(0x004CC807, regs);
    }

    // 0x004CC989
    void show_below(window* window, widget_index widgetIndex, size_t count, int8_t height)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.ah = height;
        regs.bx = static_cast<uint8_t>(count) + (1 << 14);
        regs.esi = (loco_ptr)window;
        regs.edx = widgetIndex;
        regs.edi = (loco_ptr)window->getWidget(widgetIndex);

        call(0x4CC989, regs);
    }

    // 0x004CC989
    void show_below(window* window, widget_index widgetIndex, size_t count)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.bx = static_cast<uint8_t>(count);
        regs.esi = (loco_ptr)window;
        regs.edx = widgetIndex;
        regs.edi = (loco_ptr)window->getWidget(widgetIndex);

        call(0x4CC989, regs);
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
    void show_text(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bl = static_cast<uint8_t>(count);
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
    void show_text_2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.al = colour;
        regs.bl = static_cast<uint8_t>(count);
        regs.bh = flags;
        regs.bp = width;
        regs.di = height;

        call(0x4CCC7C, regs);
    }

    // 0x004CF2B3
    void populateCompanySelect(window* window, widget_t* widget)
    {
        registers regs;
        // regs.edx = widgetIndex;
        regs.edi = (loco_ptr)widget;
        regs.esi = (loco_ptr)window;

        call(0x004CF2B3, regs);
    }

    // 0x004CF284
    company_id_t getCompanyIdFromSelection(int16_t itemIndex)
    {
        registers regs;
        // regs.edx = (int32_t)widgetIndex;
        // regs.esi = (int32_t)window;
        regs.ax = itemIndex;

        call(0x004CF284, regs);
        return regs.eax;
    }
}
