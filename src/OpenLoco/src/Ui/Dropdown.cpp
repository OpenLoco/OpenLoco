#include "Dropdown.h"
#include "Engine/Limits.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Objects/CompetitorObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Widget.h"
#include "Window.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <cassert>
#include <cstdarg>
#include <limits>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Ui::Dropdown
{
    static constexpr int kBytesPerItem = 8;

    // 0x00504619
    // Translucent colour to base colour mapping table
    // Maps each colour index to its base colour when handling translucent colours
    static constexpr std::array<Colour, 31> kDropdownColourTable = { {
        Colour::grey,              // black -> grey
        Colour::grey,              // grey -> grey
        Colour::white,             // white -> white
        Colour::mutedPurple,       // mutedDarkPurple -> mutedPurple
        Colour::mutedPurple,       // mutedPurple -> mutedPurple
        Colour::purple,            // purple -> purple
        Colour::blue,              // darkBlue -> blue
        Colour::blue,              // blue -> blue
        Colour::mutedDarkTeal,     // mutedDarkTeal -> mutedDarkTeal
        Colour::mutedDarkTeal,     // mutedTeal -> mutedDarkTeal
        Colour::green,             // darkGreen -> green
        Colour::mutedSeaGreen,     // mutedSeaGreen -> mutedSeaGreen
        Colour::mutedGrassGreen,   // mutedGrassGreen -> mutedGrassGreen
        Colour::green,             // green -> green
        Colour::mutedAvocadoGreen, // mutedAvocadoGreen -> mutedAvocadoGreen
        Colour::mutedOliveGreen,   // mutedOliveGreen -> mutedOliveGreen
        Colour::yellow,            // yellow -> yellow
        Colour::yellow,            // darkYellow -> yellow
        Colour::orange,            // orange -> orange
        Colour::amber,             // amber -> amber
        Colour::orange,            // darkOrange -> orange
        Colour::mutedDarkYellow,   // mutedDarkYellow -> mutedDarkYellow
        Colour::mutedDarkYellow,   // mutedYellow -> mutedDarkYellow
        Colour::brown,             // brown -> brown
        Colour::mutedOrange,       // mutedOrange -> mutedOrange
        Colour::mutedDarkRed,      // mutedDarkRed -> mutedDarkRed
        Colour::red,               // darkRed -> red
        Colour::red,               // red -> red
        Colour::pink,              // darkPink -> pink
        Colour::pink,              // pink -> pink
        Colour::mutedRed,          // mutedRed -> mutedRed
    } };

    // 0x005046FA
    // Optimal items per row for image dropdown layouts, indexed by total item count
    // Determines grid layout to best fit items in a dropdown (balances width vs height)
    static constexpr std::array<uint8_t, 33> kAppropriateImageDropdownItemsPerRow = { {
        // clang-format off
        1, 1, 1, 1,    // 0-3 items
        2, 2,          // 4-5 items
        3, 3,          // 6-7 items
        4,             // 8 items
        3,             // 9 items
        5,             // 10 items
        4, 4,          // 11-12 items
        5, 5, 5,       // 13-15 items
        4,             // 16 items
        5,             // 17 items
        6,             // 18 items
        5, 5,          // 19-20 items
        7,             // 21 items
        4,             // 22 items
        5,             // 23 items
        6,             // 24 items
        5,             // 25 items
        6, 6, 6, 6, 6, // 26-30 items
        8, 8,          // 31-32 items
        // clang-format on
    } };

    static char _byte_112CC04[512];                         // 0x0112CC04
    static uint8_t _windowDropdownOnpaintCellX;             // 0x01136F94
    static uint8_t _windowDropdownOnpaintCellY;             // 0x01136F96
    static uint16_t _dropdownItemCount;                     // 0x0113D84C
    static uint32_t _dropdownDisabledItems;                 // 0x0113DC60
    static uint32_t _dropdownItemHeight;                    // 0x0113DC68
    static uint32_t _dropdownItemWidth;                     // 0x0113DC6C
    static uint32_t _dropdownColumnCount;                   // 0x0113DC70
    static uint32_t _dropdownRowCount;                      // 0x0113DC74
    static Flags _dropdownFlags;                            // 0x0113DC78
    static int16_t _dropdownHighlightedIndex;               // 0x0113D84E
    static uint32_t _dropdownSelection;                     // 0x0113DC64
    static StringId _dropdownItemFormats[40];               // 0x0113D850
    static std::byte _dropdownItemArgs[40][kBytesPerItem];  // 0x0113D8A0
    static std::byte _dropdownItemArgs2[40][kBytesPerItem]; // 0x0113D9E0
    static uint8_t _menuOptions[40];                        // 0x0113DB20

    static std::vector<std::optional<DropdownItemId>> _dropdownIds;
    static bool _dropdownUseDefault;

    void addSeparator(size_t index)
    {
        add(index, 0);
    }

    void add(size_t index, StringId title)
    {
        assert(index < std::numeric_limits<uint8_t>::max());

        _dropdownItemFormats[index] = title;
    }

    void add(size_t index, StringId title, std::initializer_list<format_arg> l)
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
                    Logging::error("Unknown format: {}", static_cast<int>(arg.type));
                    break;
            }
        }
    }

    void add(size_t index, StringId title, const FormatArguments& fArgs)
    {
        add(index, title);
        std::byte* args = _dropdownItemArgs[index];

        int32_t copyLength = static_cast<int32_t>(std::min(fArgs.getLength(), sizeof(_dropdownItemArgs[index])));

        memcpy(args, fArgs.getBufferStart(), copyLength);
        copyLength = std::min(static_cast<int32_t>(fArgs.getLength() - sizeof(_dropdownItemArgs[index])), static_cast<int32_t>(sizeof(_dropdownItemArgs2[index])));
        if (copyLength > 0)
        {
            args = _dropdownItemArgs2[index];
            memcpy(args, reinterpret_cast<const std::byte*>(&fArgs) + sizeof(_dropdownItemArgs[index]), copyLength);
        }
    }

    void add(size_t index, StringId title, format_arg l)
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
        // Ensure that a valid item index is passed, or -1 to disable.
        assert(index < std::numeric_limits<uint8_t>::max() || index == std::numeric_limits<size_t>::max());

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

        static auto widgets = makeWidgets(
            Widgets::Wt3Widget({ 0, 0 }, { 1, 1 }, WindowColour::primary)

        );

        static WindowEventList events;

        // 0x004CD015
        static void onUpdate(Window& self)
        {
            self.invalidate();
        }

        static void dropdownFormatArgsToFormatArgs(uint8_t itemIndex, FormatArguments& args)
        {
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs[itemIndex][0]));
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs[itemIndex][4]));
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs2[itemIndex][0]));
            args.push(*reinterpret_cast<uint32_t*>(&_dropdownItemArgs2[itemIndex][4]));
        }

        // 0x00494BF6
        static void sub_494BF6([[maybe_unused]] Window* self, Gfx::DrawingContext& drawingCtx, StringId stringId, int16_t x, int16_t y, int16_t width, AdvancedColour colour, FormatArguments args)
        {
            StringManager::formatString(_byte_112CC04, stringId, args);

            auto tr = Gfx::TextRenderer(drawingCtx);

            tr.setCurrentFont(Gfx::Font::medium_bold);

            tr.clipString(width, _byte_112CC04);

            tr.setCurrentFont(Gfx::Font::m1);

            tr.drawString(Point(x, y), colour, _byte_112CC04);
        }

        // 0x004CD00E
        static void draw([[maybe_unused]] Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            _windowDropdownOnpaintCellX = 0;
            _windowDropdownOnpaintCellY = 0;

            for (auto itemCount = 0; itemCount < _dropdownItemCount; itemCount++)
            {
                if (_dropdownItemFormats[itemCount] != StringIds::empty)
                {
                    if (itemCount == _dropdownHighlightedIndex)
                    {
                        auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self.x + 2;
                        auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self.y + 2;
                        drawingCtx.drawRect(x, y, _dropdownItemWidth, _dropdownItemHeight, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
                    }

                    auto args = FormatArguments();

                    dropdownFormatArgsToFormatArgs(itemCount, args);

                    auto dropdownItemFormat = _dropdownItemFormats[itemCount];

                    if (dropdownItemFormat != (StringId)-2)
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

                            auto colour = self.getColour(WindowColour::primary).opaque();

                            if (itemCount == _dropdownHighlightedIndex)
                            {
                                colour = Colour::white;
                            }

                            if ((_dropdownDisabledItems & (1 << itemCount)))
                            {
                                if (itemCount < 32)
                                {
                                    colour = self.getColour(WindowColour::primary).opaque().inset();
                                }
                            }

                            auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self.x + 2;
                            auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self.y + 1;
                            auto width = self.width - 5;
                            sub_494BF6(&self, drawingCtx, dropdownItemFormat, x, y, width, colour, args);
                        }
                    }

                    if (dropdownItemFormat == (StringId)-2 || dropdownItemFormat == StringIds::null)
                    {
                        auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self.x + 2;
                        auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self.y + 2;

                        auto imageId = *(uint32_t*)&args;
                        if (dropdownItemFormat == (StringId)-2 && itemCount == _dropdownHighlightedIndex)
                        {
                            imageId++;
                        }
                        drawingCtx.drawImage(x, y, imageId);
                    }
                }
                else
                {
                    auto x = _windowDropdownOnpaintCellX * _dropdownItemWidth + self.x + 2;
                    auto y = _windowDropdownOnpaintCellY * _dropdownItemHeight + self.y + 1 + _dropdownItemHeight / 2;

                    if (!self.getColour(WindowColour::primary).isTranslucent())
                    {
                        drawingCtx.drawRect(x, y, _dropdownItemWidth - 1, 1, Colours::getShade(self.getColour(WindowColour::primary).c(), 3), Gfx::RectFlags::none);
                        drawingCtx.drawRect(x, y + 1, _dropdownItemWidth - 1, 1, Colours::getShade(self.getColour(WindowColour::primary).c(), 7), Gfx::RectFlags::none);
                    }
                    else
                    {
                        uint32_t colour = enumValue(Colours::getTranslucent(self.getColour(WindowColour::primary).c()));
                        colour++; // Gets ExtColour::translucentXXX2 highlight
                        drawingCtx.drawRect(x, y, _dropdownItemWidth - 1, 1, colour, Gfx::RectFlags::transparent);
                        colour++; // Gets ExtColour::translucentXXX0 shadow
                        drawingCtx.drawRect(x, y + 1, _dropdownItemWidth - 1, 1, colour, Gfx::RectFlags::transparent);
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
            events.onUpdate = onUpdate;
            events.draw = draw;
        }

        // 0x004CCF1E
        static void open(Ui::Point32 origin, Ui::Size32 size, AdvancedColour colour)
        {
            auto window = WindowManager::createWindow(WindowType::dropdown, origin, size, WindowFlags::stickToFront, common::events);

            window->setWidgets(common::widgets);

            if (colour.isTranslucent())
            {
                window->flags |= WindowFlags::transparent;
            }

            common::initEvents();

            window->widgets[0].windowColour = WindowColour::primary;
            window->setColour(WindowColour::primary, colour);

            _dropdownHighlightedIndex = -1;
            _dropdownDisabledItems = 0;
            _dropdownSelection = 0;
            Input::state(Input::State::dropdownActive);
        }

        // 0x004CC807 based on
        static void setColourAndInputFlags(AdvancedColour& colour, uint8_t& flags)
        {
            if (colour.isTranslucent())
            {
                colour = kDropdownColourTable[enumValue(colour.c())];
                colour = colour.translucent();
            }

            Input::resetFlag(Input::Flags::flag1);
            Input::resetFlag(Input::Flags::flag2);

            if (flags & (1 << 7))
            {
                Input::setFlag(Input::Flags::flag1);
            }

            flags &= ~(1 << 7);
        }

        // 0x004CCAB2
        static void showText(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, AdvancedColour colour, size_t count, uint8_t flags)
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

                StringManager::formatString(_byte_112CC04, _dropdownItemFormats[itemCount], args);

                auto stringWidth = Gfx::TextRenderer::getMaxStringWidth(Gfx::Font::medium_bold, _byte_112CC04);

                maxStringWidth = std::max(maxStringWidth, stringWidth);
            }

            maxStringWidth += 3;
            _dropdownItemWidth = maxStringWidth;
            _dropdownItemCount = static_cast<uint16_t>(count);
            _dropdownRowCount = static_cast<uint32_t>(count);
            uint16_t dropdownHeight = _dropdownItemHeight * static_cast<uint16_t>(count) + 3;
            widgets[0].bottom = dropdownHeight;
            dropdownHeight++;

            Ui::Size32 size = { static_cast<int32_t>(_dropdownItemWidth), dropdownHeight };
            Ui::Point32 origin = { x, y };
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
    void show(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t itemHeight, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        common::setColourAndInputFlags(colour, flags);

        WindowManager::close(WindowType::dropdown, 0);
        _dropdownFlags = Flags::none;

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
        _dropdownRowCount = static_cast<uint32_t>(count);

        int16_t dropdownHeight = (static_cast<int16_t>(count) * _dropdownItemHeight) + 3;
        common::widgets[0].bottom = dropdownHeight;
        dropdownHeight++;
        Ui::Size32 size = { width, height };
        Ui::Point32 origin = { x, y };
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
    void show(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t flags)
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
     * @param heightOffset
     * @param colour
     * @param columnCount
     * @param count
     * @param flags
     */
    void showImage(int16_t x, int16_t y, int16_t width, int16_t height, int16_t heightOffset, AdvancedColour colour, uint8_t columnCount, uint8_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());
        assert(count < std::size(kAppropriateImageDropdownItemsPerRow));

        common::setColourAndInputFlags(colour, flags);

        WindowManager::close(WindowType::dropdown, 0);
        _dropdownFlags = Flags::none;

        WindowManager::close(WindowType::dropdown, 0);
        _dropdownFlags = Flags::none;
        _dropdownItemHeight = height;
        _dropdownItemWidth = width;
        _dropdownItemCount = count;
        _dropdownColumnCount = columnCount;

        _dropdownRowCount = _dropdownItemCount / _dropdownColumnCount + ((_dropdownItemCount % _dropdownColumnCount) ? 1 : 0);
        uint16_t dropdownWidth = _dropdownItemWidth * _dropdownColumnCount + 3;
        common::widgets[0].right = dropdownWidth;
        uint16_t dropdownHeight = _dropdownItemHeight * _dropdownRowCount + 3;
        common::widgets[0].bottom = dropdownHeight;
        dropdownHeight++;

        Ui::Size32 size = { dropdownWidth, dropdownHeight };
        Ui::Point32 origin = { x, y };
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
    void showBelow(Window* window, WidgetIndex_t widgetIndex, size_t count, int8_t itemHeight, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        WindowManager::close(WindowType::dropdown, 0);
        _dropdownFlags = Flags::none;

        if (Input::state() != Input::State::widgetPressed || Input::hasFlag(Input::Flags::widgetPressed))
        {
            _dropdownFlags = _dropdownFlags | Flags::unk1;
        }

        if (Input::getPressedWindowType() != WindowType::undefined)
        {
            WindowManager::invalidateWidget(Input::getPressedWindowType(), Input::getPressedWindowNumber(), Input::getPressedWidgetIndex());
        }

        Input::setPressedWidgetIndex(widgetIndex);
        Input::setPressedWindowType(window->type);
        Input::setPressedWindowNumber(window->number);
        WindowManager::invalidateWidget(window->type, window->number, widgetIndex);

        auto widget = window->widgets[widgetIndex];
        auto colour = window->getColour(widget.windowColour).translucent();

        auto x = widget.left + window->x;
        auto y = widget.top + window->y;

        if (colour.isTranslucent())
        {
            colour = kDropdownColourTable[enumValue(colour.c())];
            colour = colour.translucent();
        }

        Input::resetFlag(Input::Flags::flag1);
        Input::resetFlag(Input::Flags::flag2);

        if (flags & (1 << 7))
        {
            Input::setFlag(Input::Flags::flag1);
        }

        flags &= ~(1 << 7);

        common::showText(x, y, widget.width(), widget.height(), itemHeight, colour, count, flags);
    }

    // 0x004CC989
    void showBelow(Window* window, WidgetIndex_t widgetIndex, size_t count, uint8_t flags)
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
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, AdvancedColour colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        common::setColourAndInputFlags(colour, flags);

        WindowManager::close(WindowType::dropdown, 0);
        _dropdownFlags = Flags::none;

        common::showText(x, y, width, height, itemHeight, colour, count, flags);
    }

    // 0x004CCA6D
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t flags)
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
    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, AdvancedColour colour, size_t count, uint8_t flags)
    {
        assert(count < std::numeric_limits<uint8_t>::max());

        common::setColourAndInputFlags(colour, flags);

        WindowManager::close(WindowType::dropdown, 0);
        _dropdownFlags = Flags::none;

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
        uint16_t dropdownHeight = static_cast<uint16_t>(count) * _dropdownItemHeight + 3;
        common::widgets[0].bottom = dropdownHeight;
        dropdownHeight++;

        Ui::Size32 size = { static_cast<uint16_t>(width), static_cast<uint16_t>(height) };
        Ui::Point32 origin = { x, y };
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

    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t flags)
    {
        showText2(x, y, width, height, 0, colour, count, flags & ~(1 << 6));
    }

    /**
     * 0x004CCF8C
     * window @ <esi>
     * widget @ <edi>
     * availableColours @<ebp>
     * dropdownColour @<al>
     * selectedColour @<ah>
     */
    void showColour(const Window* window, const Widget* widget, uint32_t availableColours, Colour selectedColour, AdvancedColour dropdownColour)
    {
        uint8_t count = 0;
        for (uint8_t i = 0; i < 32; i++)
        {
            if (availableColours & (1 << i))
            {
                count++;
            }
        }

        const uint8_t columnCount = getItemsPerRow(count);
        const uint8_t flags = 0x80;
        const uint8_t itemWidth = 16;
        const uint8_t itemHeight = 16;
        const int16_t x = window->x + widget->left;
        const int16_t y = window->y + widget->top;
        const int16_t heightOffset = widget->height() + 1;

        showImage(x, y, itemWidth, itemHeight, heightOffset, dropdownColour, columnCount, count, flags);

        uint8_t currentIndex = 0;
        for (uint8_t i = 0; i < 32; i++)
        {
            if (!(availableColours & (1 << i)))
            {
                continue;
            }

            const auto colour = static_cast<Colour>(i);
            if (colour == selectedColour)
            {
                Dropdown::setHighlightedItem(currentIndex);
            }

            auto args = FormatArguments();
            args.push(Gfx::recolour(ImageIds::colour_swatch_recolourable_raised, colour));
            args.push<uint16_t>(i);

            Dropdown::add(currentIndex, 0xFFFE, args);

            currentIndex++;
        }
    }

    // 0x004CF3CC
    void forceCloseCompanySelect()
    {
        if (hasFlags(Flags::unk1))
        {
            WindowManager::close(WindowType::dropdown);
        }
    }

    // 0x004CF2B3
    void populateCompanySelect(Window* window, Widget* widget)
    {
        std::array<bool, 16> companyOrdered = {};

        CompanyId companyId = CompanyId::null;

        size_t index = 0;
        for (; index < Limits::kMaxCompanies; index++)
        {
            int16_t maxPerformanceIndex = -1;
            for (const auto& company : CompanyManager::companies())
            {
                if (companyOrdered[enumValue(company.id())] & 1)
                {
                    continue;
                }

                if (maxPerformanceIndex < company.performanceIndex)
                {
                    maxPerformanceIndex = company.performanceIndex;
                    companyId = company.id();
                }
            }

            if (maxPerformanceIndex == -1)
            {
                break;
            }

            companyOrdered[enumValue(companyId)] |= 1;
            _dropdownItemFormats[index] = StringIds::dropdown_company_select;
            _menuOptions[index] = enumValue(companyId);

            auto company = CompanyManager::get(companyId);
            auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);
            auto ownerEmotion = company->ownerEmotion;
            auto imageId = ImageId(competitorObj->images[enumValue(ownerEmotion)], company->getColourScheme(CompanyColourType::company)).toUInt32();

            add(index, StringIds::dropdown_company_select, { imageId, company->name });
        }
        auto x = widget->left + window->x;
        auto y = widget->top + window->y;
        auto colour = window->getColour(widget->windowColour).translucent();

        showText(x, y, widget->width(), widget->height(), 25, colour, index, (1 << 6));

        size_t highlightedIndex = 0;

        while (enumValue(window->owner) != _menuOptions[highlightedIndex])
        {
            highlightedIndex++;

            if (highlightedIndex > Limits::kMaxCompanies)
            {
                highlightedIndex = std::numeric_limits<size_t>::max();
                break;
            }
        }

        setHighlightedItem(highlightedIndex);
        _dropdownFlags = _dropdownFlags | Flags::unk2;
    }

    // 0x004CF284
    CompanyId getCompanyIdFromSelection(int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = _dropdownHighlightedIndex;
        }

        auto companyId = static_cast<CompanyId>(_menuOptions[itemIndex]);
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
        return kAppropriateImageDropdownItemsPerRow[itemCount];
    }

    Builder& Builder::below(Window& window, WidgetIndex_t widgetIndex)
    {
        _window = &window;
        _widgetIndex = widgetIndex;
        return *this;
    }

    Builder& Builder::item(DropdownItemId id, StringId text)
    {
        _items.emplace_back(id, text);
        return *this;
    }

    Builder& Builder::separator()
    {
        _items.emplace_back(std::nullopt, StringIds::empty);
        return *this;
    }

    Builder& Builder::highlight(DropdownItemId id)
    {
        _highlightedId = id;
        return *this;
    }

    void Builder::show()
    {
        if (_window == nullptr)
        {
            throw Exception::InvalidArgument("Window and widget index not set");
        }

        _dropdownIds.clear();

        size_t index{};
        std::optional<size_t> highlightedIndex;
        for (const auto& item : _items)
        {
            auto& id = std::get<0>(item);
            if (id)
            {
                auto& text = std::get<1>(item);
                Dropdown::add(index, text);

                if (id == _highlightedId)
                {
                    highlightedIndex = index;
                }
            }
            else
            {
                Dropdown::addSeparator(index);
            }
            _dropdownIds.push_back(id);
            index++;
        }

        showBelow(_window, _widgetIndex, _items.size(), 0);
        if (highlightedIndex)
        {
            Dropdown::setHighlightedItem(*highlightedIndex);
            _dropdownUseDefault = true;
        }
        else
        {
            _dropdownUseDefault = false;
        }
    }

    Builder create()
    {
        return Builder();
    }

    std::optional<DropdownItemId> getSelectedItem(int32_t index)
    {
        if (index == -1 && _dropdownUseDefault)
        {
            index = Dropdown::getHighlightedItem();
        }
        if (index >= 0 && static_cast<size_t>(index) < _dropdownIds.size())
        {
            return _dropdownIds[index];
        }
        return std::nullopt;
    }

    std::optional<int> dropdownIndexFromPoint(Ui::Window* window, int x, int y)
    {
        // Check whether x and y are over a list item
        int left = x - window->x;
        if (left < 0)
        {
            return std::nullopt;
        }
        if (left >= window->width)
        {
            return std::nullopt;
        }

        // 2px of padding on the top of the list?
        int top = y - window->y - 2;
        if (top < 0)
        {
            return std::nullopt;
        }

        unsigned int itemY = top / _dropdownItemHeight;
        if (itemY >= _dropdownItemCount)
        {
            return std::nullopt;
        }

        left -= 2;
        if (left < 0)
        {
            return std::nullopt;
        }

        unsigned int itemX = left / _dropdownItemWidth;
        if (itemX >= _dropdownColumnCount)
        {
            return std::nullopt;
        }
        if (itemY >= _dropdownRowCount)
        {
            return std::nullopt;
        }

        int item = itemY * _dropdownColumnCount + itemX;
        if (item >= _dropdownItemCount)
        {
            return std::nullopt;
        }

        if (item < 32 && (_dropdownDisabledItems & (1ULL << item)) != 0)
        {
            return std::nullopt;
        }

        if (_dropdownItemFormats[item] == 0)
        {
            return std::nullopt;
        }

        return item;
    }

    bool hasFlags(Flags flags)
    {
        return (_dropdownFlags & flags) != Flags::none;
    }

    void setFlags(Flags flags)
    {
        _dropdownFlags |= flags;
    }

    void setMenuOption(size_t index, uint8_t value)
    {
        assert(index < std::size(_menuOptions));

        _menuOptions[index] = value;
    }

    uint8_t getMenuOption(size_t index)
    {
        assert(index < std::size(_menuOptions));

        return _menuOptions[index];
    }

}
