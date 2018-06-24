#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::about
{
    constexpr uint16_t ww = 400;
    constexpr uint16_t wh = 328;

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            music_acknowledgements_btn,
            atari_inc_credits_btn,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 },       { ww - 1, wh - 1 }, widget_type::frame,      0),
        make_widget({ 1, 1 },       { ww - 2, 12 },     widget_type::caption_25, 0, string_ids::about_locomotion_caption),
        make_widget({ ww - 15, 2 }, { 12,     12 },     widget_type::wt_9,       0, 2321, string_ids::tooltip_close_window),
        make_widget({ 0,   15  },   { ww - 1, 75 },     widget_type::panel,      1),
        make_widget({ 100, 228 },   { 199,    11 },     widget_type::wt_11,      1, string_ids::music_acknowledgements_btn),
        make_widget({ 157, 305 },   { 199,    11 },     widget_type::wt_11,      1, string_ids::atari_inc_credits_btn),
        widget_end(),
    };

    static window_event_list _events;

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    // 0x0043B26C
    void open()
    {
        if (windowmgr::bring_to_front(window_type::about, 0) != nullptr)
            return;

        _events.draw = draw;

        auto window = windowmgr::create_window_centred(
            window_type::about,
            ww,
            wh,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::close) | (1 << widx::music_acknowledgements_btn) | (1 << widx::atari_inc_credits_btn);
        window->init_scroll_widgets();

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;
    }

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {

    }
}
