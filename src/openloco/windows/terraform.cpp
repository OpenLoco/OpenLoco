#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../map/tilemgr.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;
using namespace openloco::map;

namespace openloco::ui::windows::terraform
{
    static loco_global<company_id_t, 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x01136496> _byte_1136496;
    static loco_global<uint8_t, 0x0113649A> _byte_113649A;
    static loco_global<uint8_t, 0x0113649E> _byte_113649E;
    static loco_global<uint32_t, 0x01136484> _dword_1136484;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    static loco_global<int8_t, 0x00523393> _currentTool;

    namespace common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_clear_area,
            tab_adjust_land,
            tab_adjust_water,
            tab_plant_trees,
            tab_build_walls,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_adjust_land) | (1 << widx::tab_adjust_water) | (1 << widx::tab_build_walls) | (1 << widx::tab_clear_area) | (1 << widx::tab_plant_trees);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { 129, 49 }, widget_type::panel, 1),                                                                       \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_clear_land),                   \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_adjust_land),                  \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_adjust_water),                 \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_plant_trees),                  \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_build_walls)

        static window_event_list _events;

        static void init_events();
        static void sub_4BD297();
    }

    namespace plant_trees
    {
        static const gfx::ui_size_t windowSize = { 634, 162 };

        static const uint8_t rowHeight = 102;

        enum widx
        {
            scrollview = 9,
            rotate_object,
            object_colour,
            plant_cluster_selected,
            plant_cluster_random,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour) | (1 << plant_cluster_selected) | (1 << plant_cluster_random);

        widget_t widgets[] = {
            commonWidgets(634, 162, string_ids::title_plant_trees),
            make_widget({ 3, 45 }, { 605, 101 }, widget_type::scrollview, 1, scrollbars::vertical),
            make_widget({ 609, 23 }, { 46, 23 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_object_90),
            make_widget({ 609, 23 }, { 70, 23 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_object_colour),
            make_widget({ 609, 23 }, { 94, 23 }, widget_type::wt_9, 1, image_ids::plant_cluster_selected_tree, string_ids::plant_cluster_selected_tree),
            make_widget({ 609, 23 }, { 118, 23 }, widget_type::wt_9, 1, image_ids::plant_cluster_random_tree, string_ids::plant_cluster_random_tree),
            widget_end(),
        };

        static window_event_list events;

        // 0x004BB63F
        static void refreshTreeList(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BB63F, regs);
        }

        // 0x004BBB0A
        static void on_close(window* self)
        {
            common::sub_4BD297();
            ui::windows::hideGridlines();
        }

        // 0x004BBAB5
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BBAB5, regs);
        }

        // 0x004BBFBD
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BBFBD, regs);
        }

        // 0x004BBAEA
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BBAEA, regs);
        }

        // 0x004BBAF5
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BBAF5, regs);
        }

        // 0x004BBDA5
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BBDA5, regs);
        }

        // 0x004BBEDF
        static void event_08(window* self)
        {
            if (self->var_846 != -1)
            {
                self->var_846 = -1;
                self->invalidate();
            }
        }

        // 0x004BBB15
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BBB15, regs);
        }

        // 0x004BBB20
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BBB20, regs);
        }

        // 0x004BBEC1
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (self->var_83C + 8) / 9;
            if (*scrollHeight == 0)
                *scrollHeight++;
            *scrollHeight *= rowHeight;
        }

        // 0x004BBF3B
        static void scroll_mouse_down(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = int32_t(self);
            regs.cx = x;
            regs.dx = y;
            call(0x004BBF3B, regs);
        }

        // 0x004BBEF8
        static void scroll_mouse_over(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = int32_t(self);
            regs.cx = x;
            regs.dx = y;
            call(0x004BBEF8, regs);
        }

        // 0x004BBB00
        static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_trees_list);
        }

        // 0x004BB756
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BB756, regs);
        }

        // 0x004BB8C9
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BB8C9, regs);
        }

        // 0x004BB982
        static void draw_scroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            registers regs;
            regs.ax = scrollIndex;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BB982, regs);
        }

        static void init_events()
        {
            events.on_close = on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.event_08 = event_08;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.get_scroll_size = get_scroll_size;
            events.scroll_mouse_down = scroll_mouse_down;
            events.scroll_mouse_over = scroll_mouse_over;
            events.tooltip = tooltip;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.draw_scroll = draw_scroll;
        }
    }

    // 0x004BB4A3
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::terraform, 0);
        if (window != nullptr)
        {
            window->call_on_mouse_up(common::widx::tab_plant_trees);
        }
        else
        {
            // 0x004BB586
            auto origin = gfx::point_t(ui::width() - plant_trees::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::terraform,
                origin,
                plant_trees::windowSize,
                window_flags::flag_8,
                &plant_trees::events);

            window->number = 0;
            window->current_tab = common::widx::tab_plant_trees - common::widx::tab_clear_area;
            window->frame_no = 0;
            _byte_113649A = 0;
            _dword_1136484 = 0x80000000;
            window->owner = _player_company;
            window->var_846 = 0xFFFF;
            window->saved_view.mapX = 0;
            _byte_113649E = 0;

            WindowManager::sub_4CEE0B(window);

            window->min_width = plant_trees::windowSize.width;
            window->min_height = plant_trees::windowSize.height;
            window->max_width = plant_trees::windowSize.width;
            window->max_height = plant_trees::windowSize.height;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0E;

            // End of 0x004BB586

            ui::windows::showGridlines();
            _byte_1136496 = 2;

            common::init_events();

            window->invalidate();

            window->widgets = plant_trees::widgets;
            window->enabled_widgets = plant_trees::enabledWidgets;
            window->holdable_widgets = 0;
            window->activated_widgets = 0;

            auto disabledWidgets = 0;
            if (!is_editor_mode())
                disabledWidgets |= common::widx::tab_build_walls;
            window->disabled_widgets = disabledWidgets;

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();

            window->var_83C = 0;
            window->row_hover = -1;

            plant_trees::refreshTreeList(window);

            input::toolSet(window, common::widx::panel, 18);

            input::set_flag(input::input_flags::flag6);
        }
        return window;
    }

    namespace clear_area
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area);

        widget_t widgets[] = {
            commonWidgets(129, 104, string_ids::clear_area),
            make_widget({ 33, 45 }, { 63, 43 }, widget_type::wt_3, 1, image_ids::tool_area, string_ids::tooltip_clear_area),
            make_widget({ 34, 46 }, { 15, 15 }, widget_type::wt_7, 1, image_ids::decrease_tool_area, string_ids::tooltip_decrease_clear_area),
            make_widget({ 80, 72 }, { 15, 15 }, widget_type::wt_7, 1, image_ids::increase_tool_area, string_ids::tooltip_increase_clear_area),
            widget_end(),
        };

        static window_event_list events;

        // 0x004BC671
        static void on_close(window* self)
        {
            ui::windows::hideGridlines();
        }

        // 0x004BC641
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BC641, regs);
        }

        // 0x004BC7C6
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC7C6, regs);
        }

        // 0x004BC65C
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BC65C, regs);
        }

        // 0x004BC78A
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC78A, regs);
        }

        // 0x004BC677
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC677, regs);
        }

        // 0x004BC689
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC689, regs);
        }

        // 0x004BC682
        static void event_12(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC682, regs);
        }

        // 0x004BC701
        static void event_13(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::panel)
            {
                tilemgr::map_invalidate_selection_rect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & 0xFFE0;
            }
        }

        // 0x004BC555
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC555, regs);
        }

        // 0x004BC5E7
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BC5E7, regs);
        }

        static void init_events()
        {
            events.on_close = on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_mouse_down = on_mouse_down;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.event_12 = event_12;
            events.event_13 = event_13;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
    }

    namespace adjust_land
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
            land_material,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area) | (1 << land_material);

        widget_t widgets[] = {
            commonWidgets(129, 104, string_ids::title_adjust_land),
            make_widget({ 33, 45 }, { 63, 43 }, widget_type::wt_3, 1, image_ids::tool_area, string_ids::tooltip_adjust_land_tool),
            make_widget({ 34, 46 }, { 15, 15 }, widget_type::wt_7, 1, image_ids::decrease_tool_area, string_ids::tooltip_decrease_adjust_land_area),
            make_widget({ 80, 72 }, { 15, 15 }, widget_type::wt_7, 1, image_ids::increase_tool_area, string_ids::tooltip_increase_adjust_land_area),
            make_widget({ 55, 92 }, { 13, 13 }, widget_type::wt_6, 0),
            widget_end(),
        };

        static window_event_list events;

        // 0x004BC9D1
        static void on_close(window* self)
        {
            ui::windows::hideGridlines();
        }

        // 0x004BC98C
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BC98C, regs);
        }

        // 0x004BCBF8
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BCBF8, regs);
        }

        // 0x004BC9A7
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BC9A7, regs);
        }

        // 0x004BC9C6
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BC9C6, regs);
        }

        // 0x004BCB0B
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BCB0B, regs);
        }

        // 0x004BC9D7
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC9D7, regs);
        }

        // 0x004BC9ED
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC9ED, regs);
        }

        // 0x004BC9E2
        static void event_12(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC9E2, regs);
        }

        // 0x004BCA5D
        static void event_13(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::panel)
            {
                tilemgr::map_invalidate_selection_rect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & 0xFFE0;
                _currentTool = 18;
            }
        }

        // 0x004BC83B
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC83B, regs);
        }

        // 0x004BC909
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BC909, regs);
        }
        static void init_events()
        {
            events.on_close = on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.event_12 = event_12;
            events.event_13 = event_13;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
    }

    namespace adjust_water
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area);

        widget_t widgets[] = {
            commonWidgets(129, 104, string_ids::title_adjust_water),
            make_widget({ 33, 45 }, { 63, 43 }, widget_type::wt_3, 1, image_ids::tool_area, string_ids::tooltip_adjust_water_tool),
            make_widget({ 34, 46 }, { 15, 15 }, widget_type::wt_7, 1, image_ids::decrease_tool_area, string_ids::tooltip_decrease_adjust_water_area),
            make_widget({ 80, 72 }, { 15, 15 }, widget_type::wt_7, 1, image_ids::increase_tool_area, string_ids::tooltip_increase_adjust_water_area),
            widget_end(),
        };

        static window_event_list events;

        // 0x004BCDAE
        static void on_close(window* self)
        {
            ui::windows::hideGridlines();
        }

        // 0x004BCD82
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BCD82, regs);
        }

        // 0x004BCEB4
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BCEB4, regs);
        }

        // 0x004BCD9D
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BCD9D, regs);
        }

        // 0x004BCE78
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BCE78, regs);
        }

        // 0x004BCDB4
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BCDB4, regs);
        }

        // 0x004BCDCA
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BCDCA, regs);
        }

        // 0x004BCDBF
        static void event_12(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BCDBF, regs);
        }

        // 0x004BCDE8
        static void event_13(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::panel)
            {
                tilemgr::map_invalidate_selection_rect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & 0xFFE0;
                _currentTool = 19;
            }
        }

        // 0x004BCC6D
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BCC6D, regs);
        }

        // 0x004BCCFF
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BCCFF, regs);
        }
        static void init_events()
        {
            events.on_close = on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_mouse_down = on_mouse_down;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.event_12 = event_12;
            events.event_13 = event_13;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
    }

    namespace build_walls
    {
        static const uint8_t rowHeight = 48;

        enum widx
        {
            scrollview = 9,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(417, 107, string_ids::title_build_walls),
            make_widget({ 3, 45 }, { 390, 47 }, widget_type::scrollview, 1, scrollbars::vertical),
            widget_end(),
        };

        static window_event_list events;

        // 0x004BC21C
        static void on_close(window* self)
        {
            common::sub_4BD297();
            ui::windows::hideGridlines();
        }

        // 0x004BC1F7
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = int32_t(self);
            call(0x004BC1F7, regs);
        }

        // 0x004BC44B
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC44B, regs);
        }

        // 0x004BC23D
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC23D, regs);
        }

        // 0x004BC377
        static void event_08(window* self)
        {
            if (self->var_846 != -1)
            {
                self->var_846 = -1;
                self->invalidate();
            }
        }

        // 0x004BC227
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC227, regs);
        }

        // 0x004BC232
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC232, regs);
        }

        // 0x004BC359
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (self->var_83C + 9) / 10;
            if (*scrollHeight == 0)
                *scrollHeight++;
            *scrollHeight *= rowHeight;
        }

        // 0x004BC3D3
        static void scroll_mouse_down(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = int32_t(self);
            regs.cx = x;
            regs.dx = y;
            call(0x004BC3D3, regs);
        }

        // 0x004BC390
        static void scroll_mouse_over(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = int32_t(self);
            regs.cx = x;
            regs.dx = y;
            call(0x004BC390, regs);
        }

        // 0x004BC212
        static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_walls_list);
        }

        // 0x004BC029
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = int32_t(self);
            call(0x004BC029, regs);
        }

        // 0x004BC0C2
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BC0C2, regs);
        }

        // 0x004BC11C
        static void draw_scroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            registers regs;
            regs.ax = scrollIndex;
            regs.esi = int32_t(self);
            regs.edi = int32_t(dpi);
            call(0x004BC11C, regs);
        }

        static void init_events()
        {
            events.on_close = on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = on_update;
            events.event_08 = event_08;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.get_scroll_size = get_scroll_size;
            events.scroll_mouse_down = scroll_mouse_down;
            events.scroll_mouse_over = scroll_mouse_over;
            events.tooltip = tooltip;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.draw_scroll = draw_scroll;
        }
    }

    namespace common
    {
        // 0x004BD297
        static void sub_4BD297()
        {
            registers regs;
            call(0x004BD297, regs);
        }
        static void init_events()
        {
            plant_trees::init_events();
            clear_area::init_events();
            adjust_land::init_events();
            adjust_water::init_events();
            build_walls::init_events();
        }
    }

    // 0x004BB566
    void open_clear_area()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_clear_area);
    }

    // 0x004BB546
    void open_adjust_land()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_adjust_land);
    }

    // 0x004BB556
    void open_adjust_water()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_adjust_water);
    }

    // 0x004BB4A3
    void open_plant_trees()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_plant_trees);
    }

    // 0x004BB576
    void open_build_walls()
    {
        auto terraform_window = open();
        terraform_window->call_on_mouse_up(common::widx::tab_build_walls);
    }
}
