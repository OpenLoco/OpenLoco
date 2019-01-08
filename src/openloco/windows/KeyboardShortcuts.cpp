#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input/ShortcutManager.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;
using namespace openloco::input;

namespace openloco::ui::KeyboardShortcuts
{

    static const int rowHeight = 10; // CJK: 13

    static window_event_list _events;
    static loco_global<string_id[8], 0x0112C826> _commonFormatArgs;

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 360, 238 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 358, 13 }, widget_type::caption_25, 0, string_ids::keyboard_shortcuts),
        make_widget({ 345, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 360, 223 }, widget_type::panel, 1),
        make_widget({ 4, 19 }, { 352, 202 }, widget_type::scrollview, 1, vertical, string_ids::keyboard_shortcut_list_tip),
        make_widget({ 4, 223 }, { 150, 12 }, widget_type::wt_11, 1, string_ids::reset_keys, string_ids::reset_keys_tip),
        widget_end(),
    };

    namespace widx
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

    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void on_mouse_up(window* self, widget_index widgetIndex);
    static void loc_4BE832(window* self);
    static void tooltip(window*, widget_index);
    static void get_scroll_size(ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index);

    static void init_events()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.get_scroll_size = get_scroll_size;
        _events.scroll_mouse_down = on_scroll_mouse_down;
        _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;
    }

    // 0x004BE6C7
    window* open()
    {
        window* window;

        window = WindowManager::bringToFront(WindowType::keyboardShortcuts, 0);
        if (window != nullptr)
            return window;

        init_events();

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(WindowType::keyboardShortcuts, { 360, 238 }, 0, &_events);

        window->widgets = (loco_ptr) _widgets;
        window->enabled_widgets = (1 << widx::close_button) | (1 << widx::reset_keys_btn);
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_10;

        window->row_count = static_cast<uint16_t>(ShortcutManager::count());
        window->row_hover = -1;

        return window;
    }

    // 0x004BE726
    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        self->draw(dpi);
    }

    // 0x004BE72C
    static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        auto colour = self->colours[1];
        auto shade = colour::get_shade(colour, 4);
        gfx::clear_single(*dpi, shade);

        auto yPos = 0;
        for (auto i = 0; i < self->row_count; i++)
        {
            string_id format = string_ids::white_stringid2;
            if (i == self->row_hover)
            {
                gfx::draw_rect(dpi, 0, yPos, 800, rowHeight, 0x2000030);
                format = string_ids::wcolour2_stringid2;
            }

            _commonFormatArgs[1] = ShortcutManager::getName(static_cast<Shortcut>(i));
            _commonFormatArgs[2] = string_ids::empty;
            _commonFormatArgs[3] = string_ids::empty;

            if (config::get().keyboard_shortcuts[i].var_0 != 0xFF)
            {
                _commonFormatArgs[3] = string_ids::shortcut_key_base + config::get().keyboard_shortcuts[i].var_0;

                if (config::get().keyboard_shortcuts[i].var_1 != 0)
                {
                    _commonFormatArgs[2] = string_ids::keyboard_shortcut_modifier_shift;
                    if (config::get().keyboard_shortcuts[i].var_1 != 1)
                    {
                        _commonFormatArgs[2] = string_ids::keyboard_shortcut_modifier_ctrl;
                    }
                }
            }

            _commonFormatArgs[0] = string_ids::keyboard_shortcut_list_format;

            gfx::draw_string_494B3F(*dpi, 0, yPos - 1, colour::black, format, _commonFormatArgs);
            yPos += rowHeight;
        }
    }

    // 0x004BE821
    static void on_mouse_up(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(self);
                return;

            case widx::reset_keys_btn:
                loc_4BE832(self);
                return;
        }
    }

    // 0x004BE832
    static void loc_4BE832(window* self)
    {
        call(0x004BE3F3);
        openloco::config::write();
        self->invalidate();
    }

    // 0x004BE844
    static void tooltip(window*, widget_index)
    {
        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = string_ids::tooltip_scroll_list;
    }

    // 0x004BE84E
    static void get_scroll_size(ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = self->row_count * rowHeight;
    }

    // 0x004BE853
    static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
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
    static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->row_count)
            return;

        EditKeyboardShortcut::open(row);
    }
}
