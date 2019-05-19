#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/land_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../objects/water_object.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top
{
    namespace widx
    {
        enum
        {
            loadsave_menu = 0,
            audio_menu = 1,
            w2 = 2,
            w3 = 3,
            w4 = 4,

            w5 = 5,
            railroad_menu = 6,
            w7 = 7,
            w8,
            w9,
            w10,
            w11,
            w12 = 12,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 30, 28 }, widget_type::wt_7, 0),           // 0
        make_widget({ 30, 0 }, { 30, 28 }, widget_type::wt_7, 0),          // 1
        make_widget({ 74, 0 }, { 30, 28 }, widget_type::wt_7, 1),          // 2
        make_widget({ 104, 0 }, { 30, 28 }, widget_type::wt_7, 1),         // 3
        make_widget({ 134, 0 }, { 30, 28 }, widget_type::wt_7, 1),         // 4
        make_widget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2),         // 5
        make_widget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2),         // 6
        make_widget({ 357, 0 }, { 30, 28 }, widget_type::wt_7, 2),         // 7
        make_widget({ 0, 0 }, { 1, 1 }, widget_type::none, 0, 0xFFFFFFFF), // 8
        make_widget({ 0, 0 }, { 1, 1 }, widget_type::none, 0, 0xFFFFFFFF), // 9
        make_widget({ 0, 0 }, { 1, 1 }, widget_type::none, 0, 0xFFFFFFFF), // 10
        make_widget({ 0, 0 }, { 1, 1 }, widget_type::none, 0, 0xFFFFFFFF), // 11
        make_widget({ 460, 0 }, { 30, 28 }, widget_type::wt_7, 3),         // 12
        widget_end(),
    };

    static window_event_list _events;

    static void on_resize(window* window);
    static void on_mouse_down(window* window, widget_index widgetIndex);
    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    static void on_update(window* window);
    static void prepare_draw(window* window);
    static void draw(window* window, gfx::drawpixelinfo_t* dpi);

    void open()
    {
        _events.on_resize = on_resize;
        _events.event_03 = on_mouse_down;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = on_update;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            gfx::ui_size_t(ui::width(), 28),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::loadsave_menu) | (1 << widx::audio_menu) | (1 << widx::w2) | (1 << widx::w3) | (1 << widx::w4) | (1 << widx::w5) | (1 << widx::railroad_menu) | (1 << widx::w7) | (1 << widx::w12);
        window->init_scroll_widgets();
        window->colours[0] = 1;
        window->colours[1] = 1;
        window->colours[2] = 1;
        window->colours[3] = 1;

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = skin->colour_12;
            window->colours[1] = skin->colour_13;
            window->colours[2] = skin->colour_14;
            window->colours[3] = skin->colour_15;
        }
    }

    // 0x0043D638
    static void loc_43D638(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::str_1581);
        dropdown::add(1, string_ids::str_1582);
        dropdown::add(2, 0);
        dropdown::add(3, string_ids::menu_about);
        dropdown::add(4, string_ids::options);
        dropdown::add(5, string_ids::menu_screenshot);
        dropdown::add(6, 0);
        dropdown::add(7, string_ids::menu_quit_scenario_editor);
        dropdown::show_below(window, widgetIndex, 8);
        dropdown::set_highlighted_item(1);
    }

    // 0x0043D695
    static void loc_43D695(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                // Load Landscape
                game_commands::do_21(1, 0, 0);
                break;

            case 1:
                // Save Landscape
                call(0x0043D705);
                break;

            case 3:
                about::open();
                break;

            case 4:
                options::open();
                break;

            case 5:
            {
                loco_global<uint8_t, 0x00508F16> screenshot_countdown;
                screenshot_countdown = 10;
                break;
            }

            case 7:
                // Return to title screen
                game_commands::do_21(1, 0, 1);
                break;
        }
    }

    // 0x0043D789
    static void loc_43D789(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::dropdown_without_checkmark, string_ids::menu_mute);
        dropdown::show_below(window, widgetIndex, 1);

        if (audio::isAllAudioDisabled())
            dropdown::set_item_selected(0);

        dropdown::set_highlighted_item(0);
    }

    // 0x0043D7C1
    static void loc_43D7C1(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                audio::toggle_sound();
                break;
        }
    }

    // 0x004402BC
    static void loc_4402BC(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::str_229);
        dropdown::show_below(window, widgetIndex, 1);
        dropdown::set_highlighted_item(0);
    }

    static void loc_4402DA(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                call(0x0043DA43);
                break;
        }
    }

    // 0x0043D541
    static void on_mouse_down(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case 0:
                loc_43D638(window, widgetIndex);
                break;

            case 1:
                loc_43D789(window, widgetIndex);
                break;

            case widx::w2:
                zoom_menu_mouse_down(window, widgetIndex);
                break;

            case widx::w3:
                rotate_menu_mouse_down(window, widgetIndex);
                break;

            case widx::w4:
                view_menu_mouse_down(window, widgetIndex);
                break;

            case widx::w5:
                terraform_menu_mouse_down(window, widgetIndex);
                break;

            case 6:
                loc_4402BC(window, widgetIndex);
                break;

            case widx::w7:
                road_menu_mouse_down(window, widgetIndex);
                break;

            case widx::w12:
                towns_menu_mouse_down(window, widgetIndex);
                break;
        }
    }

    // 0x0043D5A6
    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case 0:
                loc_43D695(window, widgetIndex, itemIndex);
                break;

            case 1:
                loc_43D7C1(window, widgetIndex, itemIndex);
                break;
            case widx::w2:
                zoom_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case widx::w3:
                rotate_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case widx::w4:
                view_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case widx::w5:
                terraform_menu_dropdown(window, widgetIndex, itemIndex);
                break;
            case 6:
                loc_4402DA(window, widgetIndex, itemIndex);
                break;
            case widx::w7:
                road_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case widx::w12:
                towns_menu_dropdown(window, widgetIndex, itemIndex); // checled
                break;
        }
    }

    // 0x0043D60B
    static void on_update(window* window)
    {
        loco_global<int32_t, 0x9C86F8> _9C86F8;
        _9C86F8++;
    }

    // 0x0043D612
    static void on_resize(window* window)
    {
        auto main = WindowManager::getMainWindow();
        if (main != nullptr)
        {
            window->set_disabled_widgets_and_invalidate(0);
        }
        else
        {
            window->set_disabled_widgets_and_invalidate(1 << 2 | 1 << 3);
        }
    }

}
