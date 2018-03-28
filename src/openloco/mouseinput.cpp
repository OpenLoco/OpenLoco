#include "input.h"
#include "interop/interop.hpp"
#include "localisation/stringmgr.h"
#include "stationmgr.h"
#include "ui.h"
#include "ui/scrollview.h"
#include "window.h"
#include "windowmgr.h"

using namespace openloco::interop;

namespace openloco::input
{
    loco_global<int16_t, 0x00523390> tool_window_number;
    loco_global<ui::window_type, 0x00523392> tool_window_class;
    loco_global<int8_t, 0x00523393> current_tool;
    loco_global<int8_t, 0x0052336C> _52336C;

    loco_global<int16_t, 0x005233A4> _5233A4;
    loco_global<int16_t, 0x005233A6> _5233A6;

    loco_global<uint16_t, 0x0050A018> _map_tooltip_format;
    loco_global<int8_t, 0x0050A040> _50A040;

    loco_global<uint16_t, 0x00F24484> mapSelectionFlags;
    loco_global<uint16_t, 0x00F252A4> _F252A4;

    // 0x004CD658
    static uint8_t viewport_interaction_get_item_left(int16_t x, int16_t y, void* arg)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CD658, regs);
        return regs.bl;
    }

    // 0x004CDB2B
    static void viewport_interaction_right_over(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;

        call(0x004CDB2B, regs);
    }

    // 0x004CD47A
    void process_mouse_over(int16_t x, int16_t y)
    {
        bool skipItem = false;
        ui::cursor_id cursorId = ui::cursor_id::pointer;

        _map_tooltip_format = string_ids::null;
        _50A040 = -1;

        if (mapSelectionFlags & (1 << 6))
        {
            mapSelectionFlags &= (uint16_t) ~(1 << 6);
            auto station = stationmgr::get(_F252A4);
            if (station->name != string_ids::null)
            {
                station->invalidate();
            }
        }

        ui::window* window = ui::windowmgr::find_at(x, y);

        if (window != nullptr)
        {
            int16_t widgetIdx = window->find_widget_at(x, y);

            if (widgetIdx != -1)
            {
                ui::widget_t& widget = window->widgets[widgetIdx];
                switch (widget.type)
                {
                    case ui::widget_type::panel:
                    case ui::widget_type::frame:
                        if (window->flags & ui::window_flags::resizable)
                        {
                            if (window->min_width != window->max_width || window->min_height != window->max_height)
                            {
                                if (x >= window->x + window->width - 19 && y >= window->y + window->height - 19)
                                {
                                    cursorId = ui::cursor_id::diagonal_arrows;
                                    break;
                                }
                            }
                        }
                        //fall-through

                    default:
                        _5233A4 = x;
                        _5233A6 = y;
                        cursorId = window->call_cursor(widgetIdx, x, y, cursorId);
                        break;

                    case ui::widget_type::scrollview:
                        _5233A4 = x;
                        _5233A6 = y;
                        ui::scrollview::scroll_part output_scroll_area;
                        int32_t scroll_id;
                        int16_t scroll_x, scroll_y;
                        ui::scrollview::get_part(
                            window,
                            &window->widgets[widgetIdx],
                            x,
                            y,
                            &scroll_x,
                            &scroll_y,
                            &output_scroll_area,
                            &scroll_id);

                        if (output_scroll_area == ui::scrollview::scroll_part::view)
                        {

                            cursorId = window->call_cursor(widgetIdx, scroll_x, scroll_y, cursorId);
                        }

                        break;

                    case ui::widget_type::viewport:
                        if (input::has_flag(input_flags::tool_active))
                        {
                            // 3
                            cursorId = (ui::cursor_id)*current_tool;
                            auto wnd = ui::windowmgr::find(tool_window_class, tool_window_number);
                            if (wnd)
                            {
                                bool out = false;
                                wnd->call_15(x, y, cursorId, &out);
                                if (out)
                                {
                                    skipItem = true;
                                }
                            }
                        }
                        else
                        {
                            switch (viewport_interaction_get_item_left(x, y, nullptr))
                            {
                                case 3:
                                case 14:
                                case 15:
                                case 20:
                                case 21:
                                    skipItem = true;
                                    cursorId = ui::cursor_id::hand_pointer;
                                    break;
                                default:
                                    break;
                            }
                        }

                        break;
                }
            }
        }

        if (!skipItem)
        {
            viewport_interaction_right_over(x, y);
        }

        if (input::state() == input::input_state::resizing)
        {
            cursorId = ui::cursor_id::diagonal_arrows;
        }

        if (cursorId != (ui::cursor_id)*_52336C)
        {
            _52336C = (int8_t)cursorId;
            ui::set_cursor(cursorId);
        }
    }
}
