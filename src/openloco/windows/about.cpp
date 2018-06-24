#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
// #include "../openloco.h"
#include "../windowmgr.h"

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
        make_widget({ 0, 0 }, { ww, wh }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { ww - 2, 13 }, widget_type::caption_25, 0, string_ids::about_locomotion_caption),
        make_widget({ ww - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, 2321, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { ww, 313 }, widget_type::panel, 1),
        make_widget({ 100, 228 }, { 200, 12 }, widget_type::wt_11, 1, string_ids::music_acknowledgements_btn),
        make_widget({ 157, 305 }, { 200, 12 }, widget_type::wt_11, 1, string_ids::atari_inc_credits_btn),
        widget_end(),
    };

    static window_event_list _events;

    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    // 0x0043B26C
    void open()
    {
        if (windowmgr::bring_to_front(window_type::about, 0) != nullptr)
            return;

        _events.on_mouse_up = on_mouse_up;
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

    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                windowmgr::close(window->type);
                break;

            case widx::music_acknowledgements_btn:
                // Unimplemented.
                break;

            case widx::atari_inc_credits_btn:
                // Unimplemented.
                break;
        }
    }

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        int16_t x = window->x + 200;
        int16_t y = window->y + 17;

        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_69, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_70, nullptr);

        y += 79;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_71, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_72, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_73, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_74, nullptr);

        y += 13;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_75, nullptr);

        y += 25;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_76, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_77, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_78, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_79, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_80, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_81, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_82, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_83, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_84, nullptr);

        // Chris Sawyer logo
        draw_image(dpi, window->x + 92, window->y + 40, 3616);

        // Atari logo
        draw_image(dpi, window->x + 16, window->y + 252, 3623);

        // Licenced to Atari
        draw_string_494B3F(*dpi, window->x + 157, window->y + 255, colour::black, string_ids::licenced_to_atari_inc, nullptr);
    }
}
