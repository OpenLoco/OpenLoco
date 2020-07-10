#include "dropdown.h"
#include "../companymgr.h"
#include "../console.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../window.h"

#include <cassert>
#include <cstdarg>
#include <limits>

using namespace openloco::interop;

namespace openloco::ui::dropdown
{
    static constexpr int bytes_per_item = 8;

    static loco_global<uint8_t[31], 0x0050457A> _byte_50457A;
    static loco_global<uint8_t[31], 0x00504619> _byte_504619;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> _byte_112CC04;
    static loco_global<uint8_t, 0x01136F94> _windowDropdownOnpaintCellX;
    static loco_global<uint8_t, 0x01136F96> _windowDropdownOnpaintCellY;
    static loco_global<uint16_t, 0x0113D84C> _dropdownItemCount;
    static loco_global<uint32_t, 0x0113DC60> _dropdownDisabledItems;
    static loco_global<uint32_t, 0x0113DC68> _dropdownItemHeight;
    static loco_global<uint32_t, 0x0113DC6C> _dropdownItemWidth;
    static loco_global<uint32_t, 0x0113DC70> _dropdownColumnCount;
    static loco_global<uint32_t, 0x0113DC74> _dropdownRowCount;
    static loco_global<uint16_t, 0x0113DC78> _word_113DC78;
    static loco_global<int16_t, 0x0113D84E> _dropdownHighlightedIndex;
    static loco_global<uint32_t, 0x0113DC64> _dropdownSelection;
    static loco_global<std::uint8_t[33], 0x005046FA> _appropriateImageDropdownItemsPerRow;

    static loco_global<string_id[40], 0x0113D850> _dropdownItemFormats;
    static loco_global<std::byte[40][bytes_per_item], 0x0113D8A0> _dropdownItemArgs;
    static loco_global<std::byte[40][bytes_per_item], 0x0113D9E0> _dropdownItemArgs2;

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

    void add(size_t index, string_id title, FormatArguments& fArgs)
    {
        add(index, title);
        std::byte* args = _dropdownItemArgs[index];

        int32_t copyLength = std::min(fArgs.getLength(), sizeof(_dropdownItemArgs[index]));

        memcpy(args, &fArgs, copyLength);
        copyLength = std::min(fArgs.getLength() - sizeof(_dropdownItemArgs[index]), sizeof(_dropdownItemArgs2[index]));
        if (copyLength > 0)
        {
            args = _dropdownItemArgs2[index];
            memcpy(args, reinterpret_cast<const std::byte*>(&fArgs) + sizeof(_dropdownItemArgs[index]), copyLength);
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

    enum widx
    {
        frame = 0,
    };

    widget_t widgets[] = {
        make_widget({ 0, 0 }, { 1, 1 }, widget_type::wt_3, 0),
        widget_end()
    };

    static window_event_list events;

    // 0x004CD015
    static void onUpdate(window* self)
    {
        self->invalidate();
    }

    // 0x00494BF6
    static void sub_494BF6(window* self, gfx::drawpixelinfo_t* dpi, string_id stringId, int16_t x, int16_t y, int16_t width, colour_t colour, FormatArguments args)
    {
        stringmgr::format_string(_byte_112CC04, stringId, &args);

        _currentFontSpriteBase = font::medium_bold;

        gfx::clip_string(width, _byte_112CC04);

        _currentFontSpriteBase = font::m1;

        gfx::draw_string(dpi, x, y, colour, _byte_112CC04);
    }

    // 0x004CD00E
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        _windowDropdownOnpaintCellX = 0;
        _windowDropdownOnpaintCellY = 0;

        for (auto itemCount = 0; itemCount < _dropdownItemCount; itemCount++)
        {
            if (_dropdownItemFormats[itemCount] != string_ids::empty)
            {
                if (itemCount == _dropdownHighlightedIndex)
                {
                    auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                    auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 2;
                    gfx::draw_rect(dpi, x, y, _dropdownItemWidth, _dropdownItemHeight, (1 << 25) | palette_index::index_2E);
                }

                auto args = FormatArguments();
                std::byte* dropdownArgs = &_dropdownItemArgs[itemCount][0];

                uint32_t* ptr = (uint32_t*)dropdownArgs;
                dropdownArgs = &_dropdownItemArgs[itemCount][4];
                args.push(*ptr);

                ptr = (uint32_t*)dropdownArgs;
                args.push(*ptr);

                dropdownArgs = &_dropdownItemArgs2[itemCount][0];

                ptr = (uint32_t*)dropdownArgs;
                dropdownArgs = &_dropdownItemArgs2[itemCount][4];
                args.push(*ptr);

                ptr = (uint32_t*)dropdownArgs;
                args.push(*ptr);

                auto dropdownItemFormat = _dropdownItemFormats[itemCount];

                if (dropdownItemFormat != (string_id)-2)
                {
                    if (dropdownItemFormat != string_ids::null)
                    {
                        if (itemCount < 32)
                        {
                            if (_dropdownSelection & (1 << itemCount))
                            {
                                dropdownItemFormat++;
                            }
                        }

                        auto colour = colour::opaque(self->colours[0]);

                        if (itemCount == _dropdownHighlightedIndex)
                        {
                            colour = colour::white;
                        }

                        if ((_dropdownDisabledItems & (1 << itemCount)))
                        {
                            if (itemCount < 32)
                            {
                                colour = colour::inset(colour::opaque(self->colours[0]));
                            }
                        }

                        auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                        auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 1;
                        auto width = self->width - 5;
                        sub_494BF6(self, dpi, dropdownItemFormat, x, y, width, colour, args);
                    }
                }

                if (dropdownItemFormat == (string_id)-2 || dropdownItemFormat == string_ids::null)
                {
                    auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                    auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 2;

                    auto imageId = *(uint32_t*)&args;
                    if (dropdownItemFormat == (string_id)-2 && itemCount == _dropdownHighlightedIndex)
                    {
                        imageId++;
                    }
                    gfx::draw_image(dpi, x, y, imageId);
                }
            }
            else
            {
                auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                auto y = _windowDropdownOnpaintCellY * 3 * (_dropdownItemHeight / 2) + self->y + 1;

                if (!(self->colours[0] & colour::translucent_flag))
                {
                    gfx::draw_rect(dpi, x, y, _dropdownItemWidth, _dropdownItemHeight, colour::get_shade(self->colours[0], 3));
                    gfx::draw_rect(dpi, x, y + 1, _dropdownItemWidth, 0, colour::get_shade(self->colours[0], 7));
                }
                else
                {
                    auto colour = _byte_50457A[self->colours[0]] | (1 << 25);
                    colour++;
                    gfx::draw_rect(dpi, x, y, _dropdownItemWidth, _dropdownItemHeight, colour);
                    colour++;
                    gfx::draw_rect(dpi, x, y + 1, _dropdownItemWidth, 0, colour);
                }
            }

            if (itemCount + 1 >= _dropdownItemCount)
                return;

            _windowDropdownOnpaintCellX++;
            if (_windowDropdownOnpaintCellX >= _dropdownColumnCount)
            {
                _windowDropdownOnpaintCellX = 0;
                _windowDropdownOnpaintCellY++;
            }
        }
    }

    static void initEvents()
    {
        events.on_update = onUpdate;
        events.draw = draw;
    }

    // 0x004CCF1E
    static void open(gfx::point_t origin, gfx::ui_size_t size, colour_t colour)
    {
        auto window = WindowManager::createWindow(WindowType::dropdown, origin, size, window_flags::stick_to_front, &events);

        window->widgets = widgets;

        if (colour & colour::translucent_flag)
        {
            window->flags |= window_flags::transparent;
        }

        initEvents();

        widgets[0].colour = colour::black;
        window->colours[0] = colour;

        for (auto i = 0; i < _dropdownItemCount; i++)
        {
            _dropdownItemFormats[i] = string_ids::empty;
        }

        _dropdownHighlightedIndex = -1;
        _dropdownDisabledItems = 0;
        _dropdownSelection = 0;
        input::state(input::input_state::dropdown_active);
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
     * @param itemHeight
     * @param flags
     */
    // Custom dropdown height if flags & (1<<6) is true
    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t itemHeight, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        if (colour & colour::translucent_flag)
        {
            colour = _byte_504619[colour::opaque(colour)];
            colour = colour::translucent(colour);
        }

        input::reset_flag(input::input_flags::flag1);
        input::reset_flag(input::input_flags::flag2);

        if (flags & (1 << 7))
        {
            input::set_flag(input::input_flags::flag1);
        }

        flags &= ~(1 << 7);

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;
        _dropdownColumnCount = 1;
        _dropdownItemWidth = 0;
        _dropdownItemWidth = width;
        _dropdownItemHeight = 10;

        if (flags & (1 << 6))
        {
            _dropdownItemHeight = itemHeight;
        }

        flags &= ~(1 << 6);

        _dropdownItemCount = (uint16_t)count;
        _dropdownRowCount = 0;
        _dropdownRowCount = count;

        widgets[0].colour = colour;
        int16_t dropdownHeight = ((int16_t)count * _dropdownItemHeight) + 3;
        widgets[0].bottom = dropdownHeight;
        gfx::ui_size_t size = { (uint16_t)width, (uint16_t)height };
        gfx::point_t origin = { x, y };
        origin.y += height;

        size.height = dropdownHeight + 1;
        if (size.height > ui::height() || origin.y < 0)
        {
            origin.y -= (height + dropdownHeight + 1);

            if (origin.y >= 0)
            {
                size.height = origin.y + dropdownHeight + 1;
            }

            if (origin.y < 0 || size.height > ui::height())
            {
                origin.x += width + 3;
                origin.y = 0;
            }
        }

        widgets[0].right = width + 3;

        if (origin.x < 0)
        {
            origin.x = 0;
        }

        origin.x += width + 4;

        if (origin.x > ui::width())
        {
            origin.x = ui::width();
        }

        origin.x -= width + 4;

        open(origin, size, colour);
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
        show(x, y, width, height, colour, count, 0, flags & ~(1 << 6));
    }

    /**
     * 0x004CCDE7
     *
     * @param x
     * @param y
     * @param width
     * @param height
     * @param colour
     * @param count
     * @param columnCount
     * @param heightOffset
     */

    void show_image(int16_t x, int16_t y, int16_t width, int16_t height, int16_t heightOffset, colour_t colour, uint8_t columnCount, uint8_t count)
    {
        assert(count < std::numeric_limits<uint8_t>::max());
        assert(count < std::size(_appropriateImageDropdownItemsPerRow));

        //registers regs;
        //regs.cx = x;
        //regs.dx = y;
        //regs.al = colour;
        //regs.bl = columnCount;
        //regs.bh = count;
        //regs.bp = width;
        //regs.ah = height;
        //regs.di = heightOffset;

        //call(0x004CCDE7, regs);

        if (colour & colour::translucent_flag)
        {
            colour = _byte_504619[colour::opaque(colour)];
            colour = colour::translucent(colour);
        }

        input::reset_flag(input::input_flags::flag1);
        input::reset_flag(input::input_flags::flag2);

        if (count & (1 << 7))
        {
            input::set_flag(input::input_flags::flag1);
        }

        count &= ~(1 << 7);

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;
        _dropdownItemHeight = height;
        _dropdownItemWidth = width;
        _dropdownItemCount = count;
        _dropdownColumnCount = columnCount;
        widgets[0].colour = colour;
        _dropdownRowCount = _dropdownItemCount / _dropdownColumnCount + 1 * (_dropdownItemCount % _dropdownColumnCount);
        widgets[0].right = _dropdownItemWidth * _dropdownColumnCount + 3;
        uint16_t dropdownHeight = _dropdownItemHeight * _dropdownRowCount + 3;
        widgets[0].bottom = dropdownHeight;

        gfx::ui_size_t size = { (uint16_t)width, (uint16_t)height };
        gfx::point_t origin = { x, y };
        origin.y += heightOffset;

        size.height = dropdownHeight + 1;
        if (size.height > ui::height() || origin.y < 0)
        {
            origin.y -= (heightOffset + dropdownHeight + 1);

            if (origin.y >= 0)
            {
                size.height = origin.y + dropdownHeight + 1;
            }

            if (origin.y < 0 || size.height > ui::height())
            {
                origin.x += widgets[0].right;
                origin.y = 0;
            }
        }

        if (origin.x < 0)
        {
            origin.x = 0;
        }

        origin.x += widgets[0].right + 1;

        if (origin.x > ui::width())
        {
            origin.x = ui::width();
        }

        origin.x -= widgets[0].right + 1;

        open(origin, size, colour);
    }

    // 0x004CC989
    void show_below(window* window, widget_index widgetIndex, size_t count, int8_t height)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.ah = height;
        regs.bx = static_cast<uint8_t>(count) + (1 << 14);
        regs.esi = (int32_t)window;
        regs.edx = widgetIndex;
        regs.edi = (int32_t)&window->widgets[widgetIndex];

        call(0x4CC989, regs);
    }

    // 0x004CC989
    void show_below(window* window, widget_index widgetIndex, size_t count)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        registers regs;
        regs.bx = static_cast<uint8_t>(count);
        regs.esi = (int32_t)window;
        regs.edx = widgetIndex;
        regs.edi = (int32_t)&window->widgets[widgetIndex];

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
        regs.edi = (int32_t)widget;
        regs.esi = (int32_t)window;

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

    uint16_t getItemArgument(const uint8_t index, const uint8_t argument)
    {
        return reinterpret_cast<uint16_t*>(_dropdownItemArgs[index])[argument];
    }

    uint16_t getItemsPerRow(uint8_t itemCount)
    {
        return _appropriateImageDropdownItemsPerRow[itemCount];
    }
}
