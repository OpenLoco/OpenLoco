#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input/ShortcutManager.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::KeyboardShortcuts
{

    static const int rowHeight = 10; // CJK: 13

    static window_event_list _events;
    static loco_global<string_id[8], 0x0112C826> _commonFormatArgs;

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 358, 13 }, widget_type::caption_25, 0, StringIds::keyboard_shortcuts),
        makeWidget({ 345, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, widget_type::panel, 1),
        makeWidget({ 4, 19 }, { 352, 202 }, widget_type::scrollview, 1, vertical, StringIds::keyboard_shortcut_list_tip),
        makeWidget({ 4, 223 }, { 150, 12 }, widget_type::wt_11, 1, StringIds::reset_keys, StringIds::reset_keys_tip),
        widgetEnd(),
    };

    namespace Widx
    {
        enum
        {
            frame,
            caption,
            close_button,
            panel,
            list,
            reset_keys_btn,
        };
    }

    static void draw(Ui::window* self, Gfx::drawpixelinfo_t* dpi);
    static void drawScroll(Ui::window* self, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void onMouseUp(window* self, widget_index widgetIndex);
    static void loc_4BE832(window* self);
    static std::optional<FormatArguments> tooltip(window*, widget_index);
    static void getScrollSize(Ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onScrollMouseOver(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseDown(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index);

    static void initEvents()
    {
        _events.on_mouse_up = onMouseUp;
        _events.get_scroll_size = getScrollSize;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
    }

    // 0x004BE6C7
    window* open()
    {
        window* window;

        window = WindowManager::bringToFront(WindowType::keyboardShortcuts, 0);
        if (window != nullptr)
            return window;

        initEvents();

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(WindowType::keyboardShortcuts, { 360, 238 }, 0, &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << Widx::close_button) | (1 << Widx::reset_keys_btn);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_10;

        window->row_count = static_cast<uint16_t>(ShortcutManager::count());
        window->row_hover = -1;

        return window;
    }

    // 0x004BE726
    static void draw(Ui::window* self, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        self->draw(dpi);
    }

    // 0x004BE72C
    static void drawScroll(Ui::window* self, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        auto colour = self->colours[1];
        auto shade = Colour::getShade(colour, 4);
        Gfx::clearSingle(*dpi, shade);

        auto yPos = 0;
        for (auto i = 0; i < self->row_count; i++)
        {
            string_id format = StringIds::black_stringid;
            if (i == self->row_hover)
            {
                Gfx::drawRect(dpi, 0, yPos, 800, rowHeight, 0x2000030);
                format = StringIds::wcolour2_stringid;
            }

            _commonFormatArgs[1] = ShortcutManager::getName(static_cast<Shortcut>(i));
            _commonFormatArgs[2] = StringIds::empty;
            _commonFormatArgs[3] = StringIds::empty;

            if (Config::get().keyboard_shortcuts[i].var_0 != 0xFF)
            {
                _commonFormatArgs[3] = StringIds::shortcut_key_base + Config::get().keyboard_shortcuts[i].var_0;

                if (Config::get().keyboard_shortcuts[i].var_1 != 0)
                {
                    _commonFormatArgs[2] = StringIds::keyboard_shortcut_modifier_shift;
                    if (Config::get().keyboard_shortcuts[i].var_1 != 1)
                    {
                        _commonFormatArgs[2] = StringIds::keyboard_shortcut_modifier_ctrl;
                    }
                }
            }

            _commonFormatArgs[0] = StringIds::keyboard_shortcut_list_format;

            Gfx::drawString_494B3F(*dpi, 0, yPos - 1, Colour::black, format, _commonFormatArgs);
            yPos += rowHeight;
        }
    }

    // 0x004BE821
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close_button:
                WindowManager::close(self);
                return;

            case Widx::reset_keys_btn:
                loc_4BE832(self);
                return;
        }
    }

    // 0x004BE832
    static void loc_4BE832(window* self)
    {
        call(0x004BE3F3);
        OpenLoco::Config::write();
        self->invalidate();
    }

    // 0x004BE844
    static std::optional<FormatArguments> tooltip(window*, widget_index)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x004BE84E
    static void getScrollSize(Ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = self->row_count * rowHeight;
    }

    // 0x004BE853
    static void onScrollMouseOver(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->row_count)
            return;

        if (row != self->row_hover)
        {
            self->row_hover = row;
            self->invalidate();
        }
    }

    // 0x004BE87B
    static void onScrollMouseDown(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->row_count)
            return;

        EditKeyboardShortcut::open(row);
    }
}
