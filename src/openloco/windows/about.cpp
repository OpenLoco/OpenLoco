#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

namespace openloco::ui::about
{
    constexpr gfx::ui_size_t windowSize = { 400, 260 };

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            music_acknowledgements_btn,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, windowSize, widget_type::frame, 0),
        make_widget({ 1, 1 }, { windowSize.width - 2, 13 }, widget_type::caption_25, 0, string_ids::about_locomotion_caption),
        make_widget({ windowSize.width - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { windowSize.width, 245 }, widget_type::panel, 1),
        make_widget({ 100, 234 }, { windowSize.width / 2, 12 }, widget_type::wt_11, 1, string_ids::music_acknowledgements_btn),
        widget_end(),
    };

    static window_event_list _events;

    static void initEvents();

    // 0x0043B26C
    void open()
    {
        if (WindowManager::bringToFront(WindowType::about, 0) != nullptr)
            return;

        initEvents();

        auto window = WindowManager::createWindowCentred(
            WindowType::about,
            windowSize,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::close) | (1 << widx::music_acknowledgements_btn);
        window->init_scroll_widgets();

        const auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;
    }

    // 0x0043B4AF
    static void on_mouse_up(ui::window* const window, const widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window->type);
                break;

            case widx::music_acknowledgements_btn:
                about_music::open();
                break;
        }
    }

    // 0x0043B2E4
    static void draw(ui::window* const window, gfx::drawpixelinfo_t* const dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        const int16_t x = window->x + windowSize.width / 2;
        int16_t y = window->y + 25;

        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_69, nullptr);

        y += 10;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::about_locomotion_70, nullptr);

        // Chris Sawyer logo
        draw_image(dpi, window->x + 92, window->y + 52, image_ids::chris_sawyer_logo_small);

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

        // Licenced to Atari
        y += 25;
        draw_string_centred(*dpi, x, y, colour::black, string_ids::licenced_to_atari_inc, nullptr);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.on_mouse_up = on_mouse_up;
    }
}
