#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input/Shortcut.h"
#include "../input/ShortcutManager.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;
using namespace openloco::input;

namespace openloco::ui::EditKeyboardShortcut
{

    static window_event_list _events;
    static loco_global<uint8_t, 0x011364A4> _11364A4;
    static loco_global<string_id[8], 0x0112C826> _commonFormatArgs;

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 280, 72 }, widget_type::frame, 0, 0xFFFFFFFF),                                                // 0,
        make_widget({ 1, 1 }, { 278, 13 }, widget_type::caption_25, 0, string_ids::change_keyboard_shortcut),                 // 1,
        make_widget({ 265, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), // 2,
        make_widget({ 0, 15 }, { 280, 57 }, widget_type::panel, 1, 0xFFFFFFFF),                                               // 3,
        widget_end(),
    };

    static void on_mouse_up(window* self, widget_index widgetIndex);
    static void draw(ui::window* self, gfx::drawpixelinfo_t* ctx);

    static void init_events()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.draw = draw;
    }

    namespace widx
    {
        enum
        {
            frame,
            caption,
            close,
            panel,
        };
    }

    // 0x004BF7B9
    window* open(uint8_t shortcutIndex)
    {
        WindowManager::close(WindowType::editKeyboardShortcut);
        _11364A4 = shortcutIndex;

        // TODO: only needs to be called once
        init_events();

        auto window = WindowManager::createWindow(WindowType::editKeyboardShortcut, { 280, 72 }, 0, &_events);

        window->widgets = (loco_ptr)_widgets;
        window->enabled_widgets = 1 << widx::close;
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_10;

        return window;
    }

    // 0x004BE8DF
    static void draw(ui::window* self, gfx::drawpixelinfo_t* ctx)
    {
        self->draw(ctx);

        _commonFormatArgs[0] = ShortcutManager::getName(static_cast<Shortcut>(*_11364A4));
        auto point = gfx::point_t(self->x + 140, self->y + 32);
        gfx::draw_string_centred_wrapped(ctx, &point, 272, 0, string_ids::change_keyboard_shortcut_desc, &*_commonFormatArgs);
    }

    // 0x004BE821
    static void on_mouse_up(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self);
                return;
        }
    }
}
