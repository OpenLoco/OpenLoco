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
#include "../s5/s5.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "toolbar_top_common.h"

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top::editor
{
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x009C870C> last_town_option;

    namespace widx
    {
        enum
        {
            map_generation_menu = common::widx::w6,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 30, 28 }, widget_type::wt_7, 0),   // 0
        make_widget({ 30, 0 }, { 30, 28 }, widget_type::wt_7, 0),  // 1
        make_widget({ 74, 0 }, { 30, 28 }, widget_type::wt_7, 1),  // 2
        make_widget({ 104, 0 }, { 30, 28 }, widget_type::wt_7, 1), // 3
        make_widget({ 134, 0 }, { 30, 28 }, widget_type::wt_7, 1), // 4

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
    static void prepare_draw(window* window);
    static void draw(window* window, gfx::drawpixelinfo_t* dpi);

    static void initEvents()
    {
        _events.on_resize = on_resize;
        _events.event_03 = on_mouse_down;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = common::on_update;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;
    }

    // 0x0043CC2C
    void open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            gfx::ui_size_t(ui::width(), 28),
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << common::widx::loadsave_menu) | (1 << common::widx::audio_menu) | (1 << common::widx::zoom_menu) | (1 << common::widx::rotate_menu) | (1 << common::widx::view_menu) | (1 << common::widx::terraform_menu) | (1 << widx::map_generation_menu) | (1 << common::widx::road_menu) | (1 << common::widx::towns_menu);
        window->init_scroll_widgets();
        window->colours[0] = colour::grey;
        window->colours[1] = colour::grey;
        window->colours[2] = colour::grey;
        window->colours[3] = colour::grey;

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
    static void loadsaveMenuMouseDown(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::load_landscape);
        dropdown::add(1, string_ids::save_landscape);
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
    static void loadsaveMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                // Load Landscape
                game_commands::do_21(0, 0);
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
                game_commands::do_21(0, 1);
                break;
        }
    }

    // 0x0043D789
    static void audioMenuMouseDown(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::dropdown_without_checkmark, string_ids::menu_mute);
        dropdown::show_below(window, widgetIndex, 1);

        if (!audio::isAudioEnabled())
            dropdown::set_item_selected(0);

        dropdown::set_highlighted_item(0);
    }

    // 0x0043D7C1
    static void audioMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
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
    static void mapGenerationMenuMouseDown(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::landscape_generation_options);
        dropdown::show_below(window, widgetIndex, 1);
        dropdown::set_highlighted_item(0);
    }

    // 0x004402DA
    static void mapGenerationMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
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
            case common::widx::loadsave_menu:
                loadsaveMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::audio_menu:
                audioMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::zoom_menu:
                common::zoom_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::rotate_menu:
                common::rotate_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::view_menu:
                common::view_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::terraform_menu:
                common::terraform_menu_mouse_down(window, widgetIndex);
                break;

            case widx::map_generation_menu:
                mapGenerationMenuMouseDown(window, widgetIndex);
                break;

            case common::widx::road_menu:
                common::road_menu_mouse_down(window, widgetIndex);
                break;

            case common::widx::towns_menu:
                common::towns_menu_mouse_down(window, widgetIndex);
                break;
        }
    }

    // 0x0043D5A6
    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case common::widx::loadsave_menu:
                loadsaveMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case common::widx::audio_menu:
                audioMenuDropdown(window, widgetIndex, itemIndex);
                break;
            case common::widx::zoom_menu:
                common::zoom_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case common::widx::rotate_menu:
                common::rotate_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case common::widx::view_menu:
                common::view_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case common::widx::terraform_menu:
                common::terraform_menu_dropdown(window, widgetIndex, itemIndex);
                break;
            case widx::map_generation_menu:
                mapGenerationMenuDropdown(window, widgetIndex, itemIndex);
                break;
            case common::widx::road_menu:
                common::road_menu_dropdown(window, widgetIndex, itemIndex); // checked
                break;

            case common::widx::towns_menu:
                common::towns_menu_dropdown(window, widgetIndex, itemIndex); // checled
                break;
        }
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
            window->set_disabled_widgets_and_invalidate((1 << common::widx::zoom_menu) | (1 << common::widx::rotate_menu));
        }
    }

    // 0x0043D2F3
    static void prepare_draw(window* window)
    {
        uint32_t x = std::max(640, ui::width()) - 1;

        _widgets[common::widx::towns_menu].right = x;
        _widgets[common::widx::towns_menu].left = _widgets[common::widx::towns_menu].right - 29;
        _widgets[common::widx::road_menu].right = _widgets[common::widx::towns_menu].left - 11;
        _widgets[common::widx::road_menu].left = _widgets[common::widx::road_menu].right - 29;
        _widgets[widx::map_generation_menu].right = _widgets[common::widx::road_menu].left - 1;
        _widgets[widx::map_generation_menu].left = _widgets[widx::map_generation_menu].right - 29;
        _widgets[common::widx::terraform_menu].right = _widgets[widx::map_generation_menu].left - 1;
        _widgets[common::widx::terraform_menu].left = _widgets[common::widx::terraform_menu].right - 29;

        if (s5::getOptions().editorStep == 1)
        {
            _widgets[common::widx::zoom_menu].type = widget_type::wt_7;
            _widgets[common::widx::rotate_menu].type = widget_type::wt_7;
            _widgets[common::widx::view_menu].type = widget_type::wt_7;
            _widgets[common::widx::terraform_menu].type = widget_type::wt_7;
            _widgets[widx::map_generation_menu].type = widget_type::wt_7;
            _widgets[common::widx::towns_menu].type = widget_type::wt_7;
            if (last_road_option != 0xFF)
            {
                _widgets[common::widx::road_menu].type = widget_type::wt_7;
            }
            else
            {
                _widgets[common::widx::road_menu].type = widget_type::none;
            }
        }
        else
        {
            _widgets[common::widx::zoom_menu].type = widget_type::none;
            _widgets[common::widx::rotate_menu].type = widget_type::none;
            _widgets[common::widx::view_menu].type = widget_type::none;
            _widgets[common::widx::terraform_menu].type = widget_type::none;
            _widgets[widx::map_generation_menu].type = widget_type::none;
            _widgets[common::widx::road_menu].type = widget_type::none;
            _widgets[common::widx::towns_menu].type = widget_type::none;
        }

        auto interface = objectmgr::get<interface_skin_object>();
        if (!audio::isAudioEnabled())
        {
            window->activated_widgets |= (1 << common::widx::audio_menu);
            _widgets[common::widx::audio_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_audio_inactive, window->colours[0]);
        }
        else
        {
            window->activated_widgets &= ~(1 << common::widx::audio_menu);
            _widgets[common::widx::audio_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_audio_active, window->colours[0]);
        }

        _widgets[common::widx::loadsave_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_loadsave, 0);
        _widgets[common::widx::zoom_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_zoom, 0);
        _widgets[common::widx::rotate_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_rotate, 0);
        _widgets[common::widx::view_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_view, 0);

        _widgets[common::widx::terraform_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_terraform, 0);
        _widgets[widx::map_generation_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_map_generation, 0);
        _widgets[common::widx::road_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_opaque, 0);

        if (last_town_option == 0)
            _widgets[common::widx::towns_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_towns, 0);
        else
            _widgets[common::widx::towns_menu].image = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_industries, 0);
    }

    // 0x00439DE4
    static void draw(window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        uint32_t company_colour = companymgr::get_player_company_colour();

        if (_widgets[common::widx::road_menu].type != widget_type::none && last_road_option != 0xFF)
        {
            uint32_t x = _widgets[common::widx::road_menu].left + window->x;
            uint32_t y = _widgets[common::widx::road_menu].top + window->y;
            uint32_t fgImage = 0;

            // Figure out what icon to show on the button face.
            bool isRoad = last_road_option & (1 << 7);
            if (isRoad)
            {
                auto obj = objectmgr::get<road_object>(last_road_option & ~(1 << 7));
                fgImage = gfx::recolour(obj->var_0E, company_colour);
            }
            else
            {
                auto obj = objectmgr::get<track_object>(last_road_option);
                fgImage = gfx::recolour(obj->var_1E, company_colour);
            }

            y--;
            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bgImage = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_transparent, window->colours[2]);

            if (input::is_dropdown_active(ui::WindowType::topToolbar, common::widx::road_menu))
            {
                y++;
                bgImage++;
            }

            gfx::draw_image(dpi, x, y, fgImage);

            y = _widgets[common::widx::road_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bgImage);
        }
    }
}
