#include "input.h"
#include "audio/audio.h"
#include "interop/interop.hpp"
#include "ui.h"
#include "ui/scrollview.h"
#include "window.h"
#include "windowmgr.h"

using namespace openloco::interop;

namespace openloco::input
{
    loco_global<uint8_t, 0x00508F18> _keyModifier;
    loco_global<uint32_t, 0x00523368> _flags;
    loco_global<uint8_t, 0x0052336D> _state;
    static int32_t _cursor_drag_start_x;
    static int32_t _cursor_drag_start_y;
    loco_global<uint32_t, 0x00525374> _cursor_drag_state;

    bool has_flag(input_flags value)
    {
        return (_flags & (uint32_t)value) != 0;
    }

    void set_flag(input_flags value)
    {
        _flags |= (uint32_t)value;
    }

    void reset_flag(input_flags value)
    {
        _flags &= ~(uint32_t)value;
    }

    input_state state()
    {
        return (input_state)*_state;
    }

    bool has_key_modifier(key_modifier modifier)
    {
        return _keyModifier;
    }

    // 0x004BE92A
    void handle_keyboard()
    {
        call(0x004BE92A);
    }

    // 0x00406FEC
    void enqueue_mouse_button(mouse_button button)
    {
        ((void (*)(int))0x00406FEC)((int32_t)button);
    }

    void move_mouse(int32_t x, int32_t y, int32_t relX, int32_t relY)
    {
        addr<0x0113E72C, int32_t>() = x;
        addr<0x0113E730, int32_t>() = y;
        addr<0x0114084C, int32_t>() = relX;
        addr<0x01140840, int32_t>() = relY;
    }

    void sub_407218()
    {
        if (_cursor_drag_state == 0)
        {
            _cursor_drag_state = 1;
            ui::get_cursor_pos(_cursor_drag_start_x, _cursor_drag_start_y);
            ui::hide_cursor();
        }
    }

    void sub_407231()
    {
        if (_cursor_drag_state != 0)
        {
            _cursor_drag_state = 0;
            ui::set_cursor_pos(_cursor_drag_start_x, _cursor_drag_start_y);
            ui::show_cursor();
        }
    }

    loco_global<uint16_t, 0x0050C19C> time_since_last_tick;

    static loco_global<ui::window_type, 0x0052336F> _pressedWindowType;
    static loco_global<uint16_t, 0x00523370> _pressedWindowNumber;
    static loco_global<uint32_t, 0x00523372> _pressedWidgetIndex;
    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;
    static loco_global<uint16_t, 0x00523378> _dragLastX;
    static loco_global<uint16_t, 0x0052337A> _dragLastY;
    static loco_global<uint16_t, 0x0052337C> _dragWindowNumber;
    static loco_global<ui::window_type, 0x0052337E> _dragWindowType;
    static loco_global<uint8_t, 0x0052337F> _dragWidgetIndex;
    static loco_global<uint8_t, 0x00523380> _dragScrollIndex;
    static loco_global<int8_t, 0x00523381> _523381;
    static loco_global<uint16_t, 0x00523386> _523386;
    static loco_global<uint16_t, 0x00523388> _523388;
    static loco_global<uint16_t, 0x0052338A> _52338A;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x0052338E> _ticksSinceDragStart;

    static loco_global<uint16_t, 0x00523396> _523396;
    static loco_global<uint32_t, 0x00523398> _523398;

    static loco_global<ui::window_type, 0x005233A8> _hoverWindowType;

    static loco_global<uint16_t, 0x005233AA> _hoverWindowNumber;
    static loco_global<uint16_t, 0x005233AC> _hoverWidgetIdx;
    static loco_global<uint32_t, 0x005233AE> _5233AE;
    static loco_global<uint32_t, 0x005233B2> _5233B2;
    static loco_global<ui::window_type, 0x005233B6> modalWindowType;

    static void state_normal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);
    static void state_normal_hover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);
    static void state_normal_left(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);
    static void state_normal_right(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);
    static void loc_4C8689(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);
    static void input_window_resize(int16_t x, int16_t y, ui::window* window, int32_t widget_index);
    static void window_drag(int16_t x, int16_t y, ui::window* window, int32_t widget_index);

    void handle_mouse(int16_t x, int16_t y, mouse_button button)
    {
        loco_global<uint16_t, 0x001136fa0>() = (uint16_t)button;

        ui::window* window = ui::windowmgr::find_at(x, y);

        int widget_index = -1;
        if (window != nullptr)
        {
            widget_index = window->find_widget_at(x, y);
        }

        if (modalWindowType != ui::window_type::undefined)
        {
            if (window != nullptr)
            {
                if (window->type != modalWindowType)
                {
                    if (button == mouse_button::left_down)
                    {

                        { // window_bring_to_front_by_id
                            registers r1;
                            r1.cx = (int8_t)(ui::window_type)modalWindowType;
                            r1.dx = 0;
                            call(0x004CD3A9, r1);
                        }

                        { // audio_play_sound_panned
                            registers r1;
                            r1.eax = 14;
                            r1.ebx = x;
                            call(0x489cb5, r1);
                        }
                    }

                    if (button == mouse_button::left_up)
                    {
                        return;
                    }
                }
            }
        }

        ui::widget_t* widget = nullptr;
        if (widget_index != -1)
        {
            widget = &window->widgets[widget_index];
        }

        registers regs;
        regs.ebp = _state;
        regs.esi = (uint32_t)window;
        regs.edx = widget_index;
        regs.edi = (uint32_t)widget;
        regs.cx = (uint16_t)button;
        regs.ax = x;
        regs.bx = y;
        switch (input::state())
        {
            case input_state::reset:
                _523386 = x;
                _523388 = y;
                _52338A = 0;
                _523381 = -1;
                _state = (uint8_t)input_state::reset;
                reset_flag(input_flags::flag4);
                state_normal(button, x, y, window, widget, widget_index);
                break;

            case input_state::normal:
                state_normal(button, x, y, window, widget, widget_index);
                break;

            case input_state::widget_pressed:
            case input_state::dropdown_active:
                call(0x004C7AE7, regs);
                break;

            case input_state::positioning_window:
                call(0x004C7903, regs);
                break;

            case input_state::viewport_right:
                call(0x004C74BB, regs);
                break;

            case input_state::viewport_left:
                call(0x004C7334, regs);
                break;

            case input_state::scroll_left:
                call(0x004C71F6, regs);
                break;

            case input_state::resizing:
                call(0x004C7722, regs);
                break;

            case input_state::scroll_right:
                call(0x004C76A7, regs);
                break;
        }
    }

    // 0x004C8048
    static void state_normal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {
        switch (state)
        {
            case mouse_button::left_down:
                state_normal_left(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::left_up:
                state_normal_right(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::released:
                state_normal_hover(x, y, window, widget, widgetIndex);
                break;

            default:
                break;
        }
    }

    static void input_widget_over_flatbutton_invalidate()
    {
        ui::window_type windowType = _hoverWindowType;
        uint16_t widgetIdx = _hoverWidgetIdx;
        uint16_t windowNumber = _hoverWindowNumber;

        if (windowType == ui::window_type::undefined)
        {
            ui::windowmgr::invalidate_widget((ui::window_type)windowType, windowNumber, widgetIdx);
            return;
        }

        ui::window* oldWindow = ui::windowmgr::find((ui::window_type)windowType, windowNumber);

        if (oldWindow != nullptr)
        {
            oldWindow->call_prepare_draw();

            ui::widget_t* oldWidget = &oldWindow->widgets[widgetIdx];
            if (
                oldWidget->type == ui::widget_type::wt_16 || oldWidget->type == ui::widget_type::wt_10 || oldWidget->type == ui::widget_type::wt_9)
            {

                ui::windowmgr::invalidate_widget((ui::window_type)windowType, windowNumber, widgetIdx);
            }
        }
    }

    // 0x004C8098
    static void state_normal_hover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {
        if (window->type != _hoverWindowType || window->number != _hoverWindowNumber || widgetIndex != _hoverWidgetIdx)
        {
            input_widget_over_flatbutton_invalidate();
            _hoverWindowType = window->type;
            _hoverWindowNumber = window->number;
            _hoverWidgetIdx = widgetIndex;
            input_widget_over_flatbutton_invalidate();
        }

        if (window != nullptr && widgetIndex != -1)
        {
            if ((window->disabled_widgets & (1ULL << widgetIndex)))
            {
                window->call_3();
            }
        }

        bool open = true;
        if (window != nullptr && widgetIndex != -1)
        {
            if (widget->type == ui::widget_type::scrollview)
            {
                ui::scrollview::scroll_part scrollArea;
                int16_t scrollX, scrollY;
                int32_t scrollIndex;
                ui::scrollview::get_part(window, widget, x, y, &scrollX, &scrollY, &scrollArea, &scrollIndex);

                if (scrollArea == ui::scrollview::scroll_part::none)
                {
                }
                else if (scrollArea == ui::scrollview::scroll_part::view)
                {
                    window->call_scroll_mouse_over(scrollX, scrollY, scrollIndex / 0x12);
                }
                else
                {
                    call(0x4C87B5);
                    open = false;
                }
            }
        }

        if (_523381 != -1)
        {
            {
                registers regs;
                regs.esi = (uint32_t)window;

                call(0x4C82FC, regs);
            }
            return;
        }

        if (_tooltipNotShownTicks < 500 || (x == _523386 && y == _523388))
        {
            _52338A += time_since_last_tick;
            int bp = 2000;
            if (_tooltipNotShownTicks <= 1000)
            {
                bp = 0;
            }

            if (bp > _52338A)
            {
                return;
            }

            if (open)
            {
                ui::tooltip::open(window, widgetIndex, x, y);
            }
            else
            {
                ui::tooltip::update(window, widgetIndex, x, y);
            }
        }

        _52338A = 0;
        _523386 = x;
        _523388 = y;
    }

    // 0x004C84BE
    static void state_normal_left(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {

        ui::windowmgr::close(ui::window_type::wt_12);
        ui::windowmgr::close(ui::window_type::tooltip);

        if (window == nullptr)
        {
            return;
        }

        // ui::windowmgr::bring_to_front(window);

        if (widget == nullptr)
        {
            return;
        }

        switch (widget->type)
        {
            case ui::widget_type::caption_22:
            case ui::widget_type::caption_23:
            case ui::widget_type::caption_24:
            case ui::widget_type::caption_25:
                window_drag(x, y, window, widgetIndex);
                break;

            case ui::widget_type::panel:
            case ui::widget_type::frame:
                input_window_resize(x, y, window, widgetIndex);
                break;

            case ui::widget_type::wt_19:
                //input_drag_viewport
                break;

            case ui::widget_type::scrollview:
                // 0x4c8658
                _state = (uint8_t)input_state::scroll_left;
                _pressedWidgetIndex = widgetIndex;
                _pressedWindowType = window->type;
                _pressedWindowNumber = window->number;
                _523386 = x;
                _523388 = y;
                loc_4C8689(x, y, window, widget, widgetIndex);
                break;

            default:
                if (!(window->enabled_widgets & (1ULL << widgetIndex)))
                {
                    break;
                }
                if ((window->disabled_widgets & (1ULL << widgetIndex)))
                {
                    break;
                }

                // Not sure if there is NO location, or a location with only a Z
                audio::play_sound((audio::sound_id)0, window->x + ((widget->left + widget->right) / 2));

                _pressedWidgetIndex = widgetIndex;
                _pressedWindowType = window->type;
                _pressedWindowNumber = window->number;
                set_flag(input_flags::flag0);
                _state = (uint8_t)input_state::widget_pressed;
                _clickRepeatTicks = 1;
                ui::windowmgr::invalidate_widget((ui::window_type)window->type, window->number, widgetIndex);
                window->call_on_mouse_down(widgetIndex);

                break;
        }
    }

    // 0x004C85D1
    static void input_window_resize(int16_t x, int16_t y, ui::window* window, int32_t widget_index)
    {
        bool resize = true;

        // TEST + JZ is hard to understand
        if (!(window->flags & (1u << 9)))
        {
            resize = false;
        }

        if (window->min_width == window->max_width && window->min_height == window->max_height)
        {
            resize = false;
        }

        if (x < (window->x + window->width - 19))
        {
            resize = false;
        }

        if (y < (window->y + window->height - 19))
        {
            resize = false;
        }

        if (resize)
        {
            _state = (uint8_t)input_state::resizing;
        }
        else
        {
            _state = (uint8_t)input_state::positioning_window;
        }

        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
    }

    // 0x00C877D
    static void
    window_drag(int16_t x, int16_t y, ui::window* window, int32_t widget_index)
    {
        _state = (uint8_t)input_state::positioning_window;
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
    }

    static void state_normal_right(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {
        ui::windowmgr::close(ui::window_type::tooltip);

        if (!window)
        {
            return;
        }
        //  ui::windowmgr::bring_to_front(window);

        if (modalWindowType == window->type)
        {
            loc_4C8689(x, y, window, widget, widgetIndex);
            return;
        }

        if (widget)
        {
            switch (widget->type)
            {
                default:
                    break;

                case ui::widget_type::wt_19:
                    window->flags &= ~(1u << 3);
                    _state = (uint8_t)input_state::viewport_right;
                    _dragLastX = x;
                    _dragLastY = y;
                    _dragWindowType = window->type;
                    _dragWindowNumber = window->number;
                    _ticksSinceDragStart = 0;
                    // set cursor to 1
                    sub_407218();
                    _5233AE = 0;
                    _5233B2 = 0;
                    set_flag(input_flags::flag5);
                    break;

                case ui::widget_type::scrollview:
                    _state = (uint8_t)input_state::scroll_right;
                    _dragLastX = x;
                    _dragLastY = y;
                    _dragWindowType = window->type;
                    _dragWindowNumber = window->number;
                    _dragWidgetIndex = widgetIndex;
                    _ticksSinceDragStart = 0;

                    int scrollIndex = 0;
                    for (int i = 0;; i++)
                    {
                        if (window->widgets[i].type == ui::widget_type::scrollview)
                        {
                            scrollIndex++;
                        }
                        if (&window->widgets[i] == widget)
                        {
                            break;
                        }
                    }

                    _dragScrollIndex = scrollIndex;
                    // set cursor to 1
                    sub_407218();
                    _5233AE = 0;
                    _5233B2 = 0;
                    set_flag(input_flags::flag5);
                    break;
            }
        }
    }

    void loc_4C8689(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {
        ui::scrollview::scroll_part outArea;
        int16_t outX, outY;
        int32_t scrollAreaOffset;
        ui::scrollview::get_part(window, widget, x, y, &outX, &outY, &outArea, &scrollAreaOffset);
        _523396 = (uint16_t)outArea;
        _523398 = scrollAreaOffset;

        if (outArea == ui::scrollview::scroll_part::view)
        {
            window->call_scroll_mouse_down(outX, outY, scrollAreaOffset / 0x12);
            return;
        }

        // Not implemented for any window
        // window->call_22()

        switch (outArea)
        {
            case ui::scrollview::scroll_part::hscrollbar_left:
                call(0x4c894f);
                break;
            case ui::scrollview::scroll_part::hscrollbar_right:
                call(0x4c89ae);
                break;
            case ui::scrollview::scroll_part::hscrollbar_left_trough:
                call(0x4c8a36);
                break;
            case ui::scrollview::scroll_part::hscrollbar_right_trough:
                call(0x4c8aa6);
                break;

            case ui::scrollview::scroll_part::vscrollbar_top:
                call(0x4c8b26);
                break;
            case ui::scrollview::scroll_part::vscrollbar_bottom:
                call(0x4c8b85);
                break;
            case ui::scrollview::scroll_part::vscrollbar_top_trough:
                call(0x4c8c0d);
                break;
            case ui::scrollview::scroll_part::vscrollbar_bottom_trough:
                call(0x4c8c7d);
                break;

            default:
                break;
        }
    }
}
