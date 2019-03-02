#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top
{
    namespace widx
    {
        enum
        {
            loadsave_menu,
            audio_menu,
            zoom_menu,
            rotate_menu,
            view_menu,

            terraform_menu,
            railroad_menu,
            road_menu,
            port_menu,
            build_vehicles_menu,

            vehicles_menu,
            stations_menu,
            towns_menu,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 30, 28 }, widget_type::wt_7, 0),
        make_widget({ 30, 0 }, { 30, 28 }, widget_type::wt_7, 0),
        make_widget({ 74, 0 }, { 30, 28 }, widget_type::wt_7, 1),
        make_widget({ 104, 0 }, { 30, 28 }, widget_type::wt_7, 1),
        make_widget({ 134, 0 }, { 30, 28 }, widget_type::wt_7, 1),

        make_widget({ 267, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 387, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 357, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 417, 0 }, { 30, 28 }, widget_type::wt_7, 2),
        make_widget({ 417, 0 }, { 30, 28 }, widget_type::wt_7, 2),

        make_widget({ 490, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        make_widget({ 520, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        make_widget({ 460, 0 }, { 30, 28 }, widget_type::wt_7, 3),
        widget_end(),
    };

    static window_event_list _events;

    static void on_resize(window* window);
    static void on_mouse_down(window* window, widget_index widgetIndex);
    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    static void on_update(window* window);
    static void prepare_draw(window* window);
    static void draw(window* window, gfx::drawpixelinfo_t* dpi);

    // 0x00438B26
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
        window->enabled_widgets = (1 << widx::loadsave_menu) | (1 << widx::audio_menu) | (1 << widx::zoom_menu) | (1 << widx::rotate_menu) | (1 << widx::view_menu) | (1 << widx::terraform_menu) | (1 << widx::railroad_menu) | (1 << widx::road_menu) | (1 << widx::port_menu) | (1 << widx::build_vehicles_menu) | (1 << widx::vehicles_menu) | (1 << widx::stations_menu) | (1 << widx::towns_menu);
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = skin->colour_12;
            window->colours[1] = skin->colour_13;
            window->colours[2] = skin->colour_14;
            window->colours[3] = skin->colour_15;
        }
    }

    static void on_resize(window* window)
    {
        registers regs;
        regs.esi = (int32_t)window;
        call(0x43A17E, regs);
    }

    static void on_mouse_down(window* window, widget_index widgetIndex)
    {
        registers regs;
        regs.esi = (int32_t)window;
        regs.edx = widgetIndex;
        regs.edi = (int32_t)&window->widgets[widgetIndex];
        call(0x43A071, regs);
    }

    static void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        registers regs;
        regs.esi = (int32_t)window;
        regs.edx = widgetIndex;
        regs.ax = itemIndex;
        call(0x43A0F4, regs);
    }

    static void on_update(window* window)
    {
        loco_global<int32_t, 0x9C86F8> _9C86F8;
        _9C86F8++;
    }

    // 0x00439DE4
    static void draw(window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        if (_widgets[widx::road_menu].type != widget_type::none)
        {
            uint32_t x = _widgets[widx::road_menu].left + window->x;
            uint32_t y = _widgets[widx::road_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = addr<0x00525FAB, uint8_t>();
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = objectmgr::get<road_object>(ebx);
                fg_image = (1 << 29) | obj->var_0E;
            }
            else
            {
                auto obj = objectmgr::get<track_object>(ebx);
                fg_image = (1 << 29) | obj->var_1E;
            }

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            y--;
            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[2] << 19;

            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::road_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::road_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        if (_widgets[widx::railroad_menu].type != widget_type::none)
        {
            uint32_t x = _widgets[widx::railroad_menu].left + window->x;
            uint32_t y = _widgets[widx::railroad_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = addr<0x00525FAA, uint8_t>();
            if ((ebx & (1 << 7)) != 0)
            {
                ebx = ebx & ~(1 << 7);
                auto obj = objectmgr::get<road_object>(ebx);
                fg_image = (1 << 29) | obj->var_0E;
            }
            else
            {
                auto obj = objectmgr::get<track_object>(ebx);
                fg_image = (1 << 29) | obj->var_1E;
            }

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[2] << 19;

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::railroad_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::railroad_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        {
            uint32_t x = _widgets[widx::vehicles_menu].left + window->x;
            uint32_t y = _widgets[widx::vehicles_menu].top + window->y;
            uint32_t fg_image = 0;

            // Figure out what icon to show on the button face.
            uint8_t ebx = addr<0x00525FAF, uint8_t>();
            switch (ebx)
            {
                case 0:
                    fg_image = 0x1A2;
                    break;
                case 1:
                    fg_image = 0x1B2;
                    break;
                case 2:
                    fg_image = 0x1C2;
                    break;
                case 3:
                    fg_image = 0x1BA;
                    break;
                case 4:
                    fg_image = 0x1AA;
                    break;
                case 5:
                    fg_image = 0x1CA;
            }

            auto interface = objectmgr::get<interface_skin_object>();
            fg_image += (1 << 29) | interface->img;

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[3] << 19;

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::vehicles_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::vehicles_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }

        {
            uint32_t x = _widgets[widx::build_vehicles_menu].left + window->x;
            uint32_t y = _widgets[widx::build_vehicles_menu].top + window->y;

            // Figure out what icon to show on the button face.
            uint32_t fg_image = addr<0x0052622C, uint8_t>();
            fg_image <<= 1;
            auto interface = objectmgr::get<interface_skin_object>();
            fg_image += (1 << 29) | (interface->img + 0x1F);

            // Apply company colours.
            loco_global<uint8_t[16], 0x009C645C> company_colours;
            uint32_t colour = company_colours[addr<0x00525E3C, uint8_t>()];
            fg_image |= colour << 19;

            uint32_t bg_image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_transparent);
            bg_image |= window->colours[2] << 19;

            y--;
            if (input::is_dropdown_active(ui::WindowType::topToolbar, widx::build_vehicles_menu))
            {
                y++;
                bg_image++;
            }

            gfx::draw_image(dpi, x, y, fg_image);

            y = _widgets[widx::build_vehicles_menu].top + window->y;
            gfx::draw_image(dpi, x, y, bg_image);
        }
    }

    // 0x00439BCB
    static void prepare_draw(window* window)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        uint32_t colour = window->colours[0] << 19;

        loco_global<bool, 0x0050D555> enable_sound;
        if (enable_sound)
        {
            window->activated_widgets |= (1 << widx::audio_menu);
            _widgets[widx::audio_menu].image = (1 << 29) | colour | (interface->img + interface_skin::image_ids::toolbar_audio_inactive);
        }
        else
        {
            window->activated_widgets &= ~(1 << widx::audio_menu);
            _widgets[widx::audio_menu].image = (1 << 29) | colour | (interface->img + interface_skin::image_ids::toolbar_audio_active);
        }

        loco_global<bool, 0x009C870D> _9C870D;
        if (!_9C870D && addr<0x00525FAC, int8_t>() != -1 && addr<0x00525FAD, int8_t>() == -1)
            _9C870D = true;

        _widgets[widx::loadsave_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_loadsave);
        _widgets[widx::zoom_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_zoom);
        _widgets[widx::rotate_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_rotate);
        _widgets[widx::view_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_view);

        _widgets[widx::terraform_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_terraform);
        _widgets[widx::railroad_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::road_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::port_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::build_vehicles_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);

        _widgets[widx::vehicles_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_empty_opaque);
        _widgets[widx::stations_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_stations);

        if (addr<0x009C870C, int8_t>() == 0)
            _widgets[widx::towns_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_towns);
        else
            _widgets[widx::towns_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_industries);

        if (!_9C870D)
            _widgets[widx::port_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_airports);
        else
            _widgets[widx::port_menu].image = (1 << 29) | (interface->img + interface_skin::image_ids::toolbar_ports);

        if (addr<0x00525FAB, int8_t>() != -1)
            _widgets[widx::road_menu].type = widget_type::wt_7;
        else
            _widgets[widx::road_menu].type = widget_type::none;

        if (addr<0x00525FAA, int8_t>() != -1)
            _widgets[widx::railroad_menu].type = widget_type::wt_7;
        else
            _widgets[widx::railroad_menu].type = widget_type::none;

        if (addr<0x00525FAC, int8_t>() != -1 || addr<0x00525FAD, int8_t>() != -1)
            _widgets[widx::port_menu].type = widget_type::wt_7;
        else
            _widgets[widx::port_menu].type = widget_type::none;

        uint32_t x = std::max(640, ui::width()) - 1;
        _widgets[widx::towns_menu].right = x;
        x -= 29;
        _widgets[widx::towns_menu].left = x;
        x -= 1;

        _widgets[widx::stations_menu].right = x;
        x -= 29;
        _widgets[widx::stations_menu].left = x;
        x -= 1;

        _widgets[widx::vehicles_menu].right = x;
        x -= 29;
        _widgets[widx::vehicles_menu].left = x;
        x -= 11;

        _widgets[widx::build_vehicles_menu].right = x;
        x -= 29;
        _widgets[widx::build_vehicles_menu].left = x;
        x -= 1;

        if (_widgets[widx::port_menu].type != widget_type::none)
        {
            _widgets[widx::port_menu].right = x;
            x -= 29;
            _widgets[widx::port_menu].left = x;
            x -= 1;
        }

        if (_widgets[widx::road_menu].type != widget_type::none)
        {
            _widgets[widx::road_menu].right = x;
            x -= 29;
            _widgets[widx::road_menu].left = x;
            x -= 1;
        }

        if (_widgets[widx::railroad_menu].type != widget_type::none)
        {
            _widgets[widx::railroad_menu].right = x;
            x -= 29;
            _widgets[widx::railroad_menu].left = x;
            x -= 1;
        }

        _widgets[widx::terraform_menu].right = x;
        x -= 29;
        _widgets[widx::terraform_menu].left = x;
        x -= 1;
    }
}
