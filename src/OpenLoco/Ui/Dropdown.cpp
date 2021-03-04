#include "Dropdown.h"
#include "../CompanyManager.h"
#include "../Console.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/CompetitorObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ptr.h"
#include "../Window.h"

#include <cassert>
#include <cstdarg>
#include <limits>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Dropdown
{
    static constexpr int bytes_per_item = 8;

    static loco_global<uint8_t[31], 0x005045FA> _byte_5045FA;
    static loco_global<uint8_t[31], 0x00504619> _byte_504619;
    static loco_global<std::uint8_t[33], 0x005046FA> _appropriateImageDropdownItemsPerRow;
    static loco_global<Ui::WindowType, 0x0052336F> _pressedWindowType;
    static loco_global<Ui::window_number, 0x00523370> _pressedWindowNumber;
    static loco_global<int32_t, 0x00523372> _pressedWidgetIndex;
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
    static loco_global<string_id[40], 0x0113D850> _dropdownItemFormats;
    static loco_global<std::byte[40][bytes_per_item], 0x0113D8A0> _dropdownItemArgs;
    static loco_global<std::byte[40][bytes_per_item], 0x0113D9E0> _dropdownItemArgs2;
    static loco_global<uint8_t[40], 0x00113DB20> _menuOptions;

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
                    Console::error("Unknown format: %d", arg.type);
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

    int16_t getHighlightedItem()
    {
        return _dropdownHighlightedIndex;
    }

    void setItemDisabled(size_t index)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownDisabledItems |= (1U << static_cast<uint8_t>(index));
    }

    void setHighlightedItem(size_t index)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownHighlightedIndex = static_cast<uint8_t>(index);
    }

    void setItemSelected(size_t index)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownSelection |= (1U << static_cast<uint8_t>(index));
    }

    namespace common
    {
        enum widx
        {
            frame = 0,
        };

        widget_t widgets[] = {
            makeWidget({ 0, 0 }, { 1, 1 }, widget_type::wt_3, 0),
            widgetEnd()
        };

        static window_event_list events;

        // 0x004CD015
        static void onUpdate(window* self)
        {
            self->invalidate();
        }

        static void dropdownFormatArgsToFormatArgs(uint8_t itemIndex, FormatArguments args)
        {
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs[itemIndex][0]));
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs[itemIndex][4]));
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs2[itemIndex][0]));
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs2[itemIndex][4]));
        }

        // 0x00494BF6
        static void sub_494BF6(window* self, Gfx::drawpixelinfo_t* dpi, string_id stringId, int16_t x, int16_t y, int16_t width, colour_t colour, FormatArguments args)
        {
            StringManager::formatString(_byte_112CC04, stringId, &args);

            _currentFontSpriteBase = Font::medium_bold;

            Gfx::clipString(width, _byte_112CC04);

            _currentFontSpriteBase = Font::m1;

            Gfx::drawString(dpi, x, y, colour, _byte_112CC04);
        }

        // 0x004CD00E
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            _windowDropdownOnpaintCellX = 0;
            _windowDropdownOnpaintCellY = 0;

            for (auto itemCount = 0; itemCount < _dropdownItemCount; itemCount++)
            {
                if (_dropdownItemFormats[itemCount] != StringIds::empty)
                {
                    if (itemCount == _dropdownHighlightedIndex)
                    {
                        auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                        auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 2;
                        Gfx::drawRect(dpi, x, y, _dropdownItemWidth, _dropdownItemHeight, (1 << 25) | PaletteIndex::index_2E);
                    }

                    auto args = FormatArguments();

                    dropdownFormatArgsToFormatArgs(itemCount, args);

                    auto dropdownItemFormat = _dropdownItemFormats[itemCount];

                    if (dropdownItemFormat != (string_id)-2)
                    {
                        if (dropdownItemFormat != StringIds::null)
                        {
                            if (itemCount < 32)
                            {
                                if (_dropdownSelection & (1 << itemCount))
                                {
                                    dropdownItemFormat++;
                                }
                            }

                            auto colour = Colour::opaque(self->colours[0]);

                            if (itemCount == _dropdownHighlightedIndex)
                            {
                                colour = Colour::white;
                            }

                            if ((_dropdownDisabledItems & (1 << itemCount)))
                            {
                                if (itemCount < 32)
                                {
                                    colour = Colour::inset(Colour::opaque(self->colours[0]));
                                }
                            }

                            auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                            auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 1;
                            auto width = self->width - 5;
                            sub_494BF6(self, dpi, dropdownItemFormat, x, y, width, colour, args);
                        }
                    }

                    if (dropdownItemFormat == (string_id)-2 || dropdownItemFormat == StringIds::null)
                    {
                        auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                        auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 2;

                        auto imageId = *(uint32_t*)&args;
                        if (dropdownItemFormat == (string_id)-2 && itemCount == _dropdownHighlightedIndex)
                        {
                            imageId++;
                        }
                        Gfx::drawImage(dpi, x, y, imageId);
                    }
                }
                else
                {
                    auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self->x + 2;
                    auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self->y + 1 + _dropdownItemHeight / 2;

                    if (!(self->colours[0] & Colour::translucent_flag))
                    {
                        Gfx::drawRect(dpi, x, y, _dropdownItemWidth - 1, 1, Colour::getShade(self->colours[0], 3));
                        Gfx::drawRect(dpi, x, y + 1, _dropdownItemWidth - 1, 1, Colour::getShade(self->colours[0], 7));
                    }
                    else
                    {
                        uint32_t colour = _byte_5045FA[Colour::opaque(self->colours[0])] | (1 << 25);
                        colour++;
                        Gfx::drawRect(dpi, x, y, _dropdownItemWidth - 1, 1, colour);
                        colour++;
                        Gfx::drawRect(dpi, x, y + 1, _dropdownItemWidth - 1, 1, colour);
                    }
                }

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
        static void open(Gfx::point_t origin, Gfx::ui_size_t size, colour_t colour)
        {
            auto window = WindowManager::createWindow(WindowType::dropdown, origin, size, WindowFlags::stick_to_front, &common::events);

            window->widgets = common::widgets;

            if (colour & Colour::translucent_flag)
            {
                window->flags |= WindowFlags::transparent;
            }

            common::initEvents();

            common::widgets[0].colour = Colour::black;
            window->colours[0] = colour;

            _dropdownHighlightedIndex = -1;
            _dropdownDisabledItems = 0;
            _dropdownSelection = 0;
            Input::state(Input::input_state::dropdown_active);
        }

        // 0x004CC807 based on
        static void setColourAndInputFlags(colour_t& colour, uint8_t& flags)
        {
            if (colour & Colour::translucent_flag)
            {
                colour = _byte_504619[Colour::opaque(colour)];
                colour = Colour::translucent(colour);
            }

            Input::resetFlag(Input::input_flags::flag1);
            Input::resetFlag(Input::input_flags::flag2);

            if (flags & (1 << 7))
            {
                Input::setFlag(Input::input_flags::flag1);
            }

            flags &= ~(1 << 7);
        }

        // 0x004955BC
        static uint16_t getStringWidth(char* buffer)
        {
            registers regs;
            regs.esi = ToInt(buffer);
            call(0x004955BC, regs);

            return regs.cx;
        }

        // 0x004CCAB2
        static void showText(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, colour_t colour, size_t count, uint8_t flags)
        {
            _dropdownColumnCount = 1;
            _dropdownItemWidth = 0;
            _dropdownItemHeight = 10;

            if (flags & (1 << 6))
            {
                _dropdownItemHeight = itemHeight;
            }

            flags &= ~(1 << 6);

            uint16_t maxStringWidth = 0;
            for (uint8_t itemCount = 0; itemCount < count; itemCount++)
            {
                auto args = FormatArguments();

                dropdownFormatArgsToFormatArgs(itemCount, args);

                StringManager::formatString(_byte_112CC04, _dropdownItemFormats[itemCount], &args);

                _currentFontSpriteBase = Font::medium_bold;

                auto stringWidth = getStringWidth(_byte_112CC04);

                maxStringWidth = std::max(maxStringWidth, stringWidth);
            }

            maxStringWidth += 3;
            _dropdownItemWidth = maxStringWidth;
            _dropdownItemCount = static_cast<uint16_t>(count);
            _dropdownRowCount = static_cast<uint32_t>(count);
            widgets[0].colour = colour;
            uint16_t dropdownHeight = _dropdownItemHeight * static_cast<uint16_t>(count) + 3;
            widgets[0].bottom = dropdownHeight;
            dropdownHeight++;

            Gfx::ui_size_t size = { static_cast<uint16_t>(_dropdownItemWidth), dropdownHeight };
            Gfx::point_t origin = { x, y };
            origin.y += height;

            if ((size.height + origin.y) > Ui::height() || origin.y < 0)
            {
                origin.y -= (height + size.height);
                auto dropdownBottom = origin.y;

                if (origin.y >= 0)
                {
                    dropdownBottom = origin.y + size.height;
                }

                if (origin.y < 0 || dropdownBottom > Ui::height())
                {
                    origin.x += width;
                    origin.x += maxStringWidth;

                    if (origin.x > Ui::width())
                    {
                        origin.x = x;
                        origin.x -= (maxStringWidth + 4);
                    }

                    origin.y = 0;
                }
            }

            size.width = maxStringWidth + 3;
            widgets[0].right = size.width;
            size.width++;

            if (origin.x < 0)
            {
                origin.x = 0;
            }

            origin.x += size.width;

            if (origin.x > Ui::width())
            {
                origin.x = Ui::width();
            }

            origin.x -= size.width;

            open(origin, size, colour);
        }
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
     * Custom Dropdown height if flags & (1<<6) is true
     */
    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t itemHeight, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        common::setColourAndInputFlags(colour, flags);

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

        _dropdownItemCount = static_cast<uint16_t>(count);
        _dropdownRowCount = 0;
        _dropdownRowCount = count;

        common::widgets[0].colour = colour;
        int16_t dropdownHeight = (static_cast<int16_t>(count) * _dropdownItemHeight) + 3;
        common::widgets[0].bottom = dropdownHeight;
        dropdownHeight++;
        Gfx::ui_size_t size = { static_cast<uint16_t>(width), static_cast<uint16_t>(height) };
        Gfx::point_t origin = { x, y };
        origin.y += height;

        size.height = dropdownHeight;
        if ((size.height + origin.y) > Ui::height() || origin.y < 0)
        {
            origin.y -= (height + dropdownHeight);
            auto dropdownBottom = origin.y;

            if (origin.y >= 0)
            {
                dropdownBottom = origin.y + dropdownHeight;
            }

            if (origin.y < 0 || dropdownBottom > Ui::height())
            {
                origin.x += width + 3;
                origin.y = 0;
            }
        }

        size.width = width + 3;
        common::widgets[0].right = size.width;
        size.width++;

        if (origin.x < 0)
        {
            origin.x = 0;
        }

        origin.x += size.width;

        if (origin.x > Ui::width())
        {
            origin.x = Ui::width();
        }

        origin.x -= size.width;

        common::open(origin, size, colour);

        for (auto i = 0; i < _dropdownItemCount; i++)
        {
            _dropdownItemFormats[i] = StringIds::empty;
        }
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

    void showImage(int16_t x, int16_t y, int16_t width, int16_t height, int16_t heightOffset, colour_t colour, uint8_t columnCount, uint8_t count)
    {
        assert(count < std::numeric_limits<uint8_t>::max());
        assert(count < std::size(_appropriateImageDropdownItemsPerRow));

        common::setColourAndInputFlags(colour, count);

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;
        _dropdownItemHeight = height;
        _dropdownItemWidth = width;
        _dropdownItemCount = count;
        _dropdownColumnCount = columnCount;

        common::widgets[0].colour = colour;
        _dropdownRowCount = _dropdownItemCount / _dropdownColumnCount + 1 * (_dropdownItemCount % _dropdownColumnCount);
        uint16_t dropdownWidth = _dropdownItemWidth * _dropdownColumnCount + 3;
        common::widgets[0].right = dropdownWidth;
        uint16_t dropdownHeight = _dropdownItemHeight * _dropdownRowCount + 3;
        common::widgets[0].bottom = dropdownHeight;
        dropdownHeight++;

        Gfx::ui_size_t size = { dropdownWidth, dropdownHeight };
        Gfx::point_t origin = { x, y };
        origin.y += heightOffset;

        size.height = dropdownHeight;
        if ((size.height + origin.y) > Ui::height() || origin.y < 0)
        {
            origin.y -= (heightOffset + dropdownHeight);
            auto dropdownBottom = origin.y;

            if (origin.y >= 0)
            {
                dropdownBottom = origin.y + dropdownHeight;
            }

            if (origin.y < 0 || dropdownBottom > Ui::height())
            {
                origin.x += common::widgets[0].right;
                origin.y = 0;
            }
        }

        size.width = common::widgets[0].right + 1;

        if (origin.x < 0)
        {
            origin.x = 0;
        }

        origin.x += size.width;

        if (origin.x > Ui::width())
        {
            origin.x = Ui::width();
        }

        origin.x -= size.width;

        common::open(origin, size, colour);

        for (auto i = 0; i < _dropdownItemCount; i++)
        {
            _dropdownItemFormats[i] = StringIds::empty;
        }
    }

    // 0x004CC989
    void showBelow(window* window, widget_index widgetIndex, size_t count, int8_t itemHeight, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;

        if (Input::state() != Input::input_state::widget_pressed || Input::hasFlag(Input::input_flags::widget_pressed))
        {
            _word_113DC78 = _word_113DC78 | 1;
        }

        if (_pressedWindowType != WindowType::undefined)
        {
            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
        }

        _pressedWidgetIndex = widgetIndex;
        _pressedWindowType = window->type;
        _pressedWindowNumber = window->number;
        WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);

        auto widget = window->widgets[widgetIndex];
        auto colour = window->colours[widget.colour];
        colour = Colour::translucent(colour);

        auto x = widget.left + window->x;
        auto y = widget.top + window->y;

        if (colour & Colour::translucent_flag)
        {
            colour = _byte_504619[Colour::opaque(colour)];
            colour = Colour::translucent(colour);
        }

        Input::resetFlag(Input::input_flags::flag1);
        Input::resetFlag(Input::input_flags::flag2);

        if (flags & (1 << 7))
        {
            Input::setFlag(Input::input_flags::flag1);
        }

        flags &= ~(1 << 7);

        common::showText(x, y, widget.width(), widget.height(), itemHeight, colour, count, flags);
    }

    // 0x004CC989
    void showBelow(window* window, widget_index widgetIndex, size_t count, uint8_t flags)
    {
        showBelow(window, widgetIndex, count, 0, flags & ~(1 << 6));
    }

    /**
     * 0x004CCA6D
     * x @<cx>
     * y @<dx>
     * width @<bp>
     * height @<di>
     * colour @<al>
     * itemHeight @ <ah>
     * count @<bl>
     * flags @<bh>
     * Custom Dropdown height if flags & (1<<6) is true
     */
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, colour_t colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        common::setColourAndInputFlags(colour, flags);

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;

        common::showText(x, y, width, height, itemHeight, colour, count, flags);
    }

    // 0x004CCA6D
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags)
    {
        showText(x, y, width, height, 0, colour, count, flags & ~(1 << 6));
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
    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, colour_t colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        common::setColourAndInputFlags(colour, flags);

        WindowManager::close(WindowType::dropdown, 0);
        _word_113DC78 = 0;

        _dropdownColumnCount = 1;
        _dropdownItemWidth = width;
        _dropdownItemHeight = 10;

        if (flags & (1 << 6))
        {
            _dropdownItemHeight = itemHeight;
        }

        flags &= ~(1 << 6);

        _dropdownItemCount = static_cast<uint16_t>(count);
        _dropdownRowCount = static_cast<uint32_t>(count);
        common::widgets[0].colour = colour;
        uint16_t dropdownHeight = static_cast<uint16_t>(count) * _dropdownItemHeight + 3;
        common::widgets[0].bottom = dropdownHeight;
        dropdownHeight++;

        Gfx::ui_size_t size = { static_cast<uint16_t>(width), static_cast<uint16_t>(height) };
        Gfx::point_t origin = { x, y };
        origin.y += height;

        size.height = dropdownHeight;
        if ((size.height + origin.y) > Ui::height() || origin.y < 0)
        {
            origin.y -= (height + dropdownHeight);
            auto dropdownBottom = origin.y;

            if (origin.y >= 0)
            {
                dropdownBottom = origin.y + dropdownHeight;
            }

            if (origin.y < 0 || dropdownBottom > Ui::height())
            {
                origin.x += width + 3;
                origin.y = 0;
            }
        }

        size.width = width + 3;
        common::widgets[0].right = size.width;
        size.width++;

        if (origin.x < 0)
        {
            origin.x = 0;
        }

        origin.x += size.width;

        if (origin.x > Ui::width())
        {
            origin.x = Ui::width();
        }

        origin.x -= size.width;

        common::open(origin, size, colour);
    }

    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags)
    {
        showText2(x, y, width, height, 0, colour, count, flags & ~(1 << 6));
    }

    // 0x004CF2B3
    void populateCompanySelect(window* window, widget_t* widget)
    {
        std::array<bool, 16> companyOrdered = {};

        company_id_t companyId = CompanyId::null;

        size_t index = 0;
        for (; index < CompanyManager::max_companies; index++)
        {
            int16_t maxPerformanceIndex = -1;
            for (const auto& company : CompanyManager::companies())
            {
                if (company.empty())
                    continue;

                if (companyOrdered[company.id()] & 1)
                    continue;

                if (maxPerformanceIndex < company.performance_index)
                {
                    maxPerformanceIndex = company.performance_index;
                    companyId = company.id();
                }
            }

            if (maxPerformanceIndex == -1)
                break;

            companyOrdered[companyId] |= 1;
            _dropdownItemFormats[index] = StringIds::dropdown_company_select;
            _menuOptions[index] = companyId;

            auto company = CompanyManager::get(companyId);
            auto competitorObj = ObjectManager::get<competitor_object>(company->competitor_id);
            auto ownerEmotion = company->owner_emotion;
            auto imageId = competitorObj->images[ownerEmotion];
            imageId = Gfx::recolour(imageId, company->mainColours.primary);

            add(index, StringIds::dropdown_company_select, { imageId, company->name });
        }
        auto x = widget->left + window->x;
        auto y = widget->top + window->y;
        auto colour = Colour::translucent(window->colours[widget->colour]);

        showText(x, y, widget->width(), widget->height(), 25, colour, index, (1 << 6));

        size_t highlightedIndex = 0;

        while (window->owner != _menuOptions[highlightedIndex])
        {
            highlightedIndex++;

            if (highlightedIndex > CompanyManager::max_companies)
            {
                highlightedIndex = -1;
                break;
            }
        }

        setHighlightedItem(highlightedIndex);
        _word_113DC78 = _word_113DC78 | (1 << 1);
    }

    // 0x004CF284
    company_id_t getCompanyIdFromSelection(int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = _dropdownHighlightedIndex;
        }

        auto companyId = _menuOptions[itemIndex];
        auto company = CompanyManager::get(companyId);

        if (company->empty())
        {
            companyId = CompanyId::null;
        }

        return companyId;
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
