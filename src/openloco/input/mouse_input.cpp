#include "../audio/audio.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../stationmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"
#include "../window.h"
#include <map>

using namespace openloco::interop;
using namespace openloco::ui;

#define DROPDOWN_ITEM_UNDEFINED -1

namespace openloco::input
{
    static void state_resizing(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_widget_pressed(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal_hover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal_left(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_normal_right(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void state_positioning_window(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);

    static void window_position_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index);
    static void window_position_end();

    static void window_resize_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index);

    static void viewport_drag_begin(window* w);

    static void scroll_begin(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex);

    static void scroll_drag_begin(int16_t x, int16_t y, window* pWindow, widget_index index);

    static void widget_over_flatbutton_invalidate();

#pragma mark - Input

    static loco_global<mouse_button, 0x001136FA0> _lastKnownButtonState;

    static loco_global<string_id, 0x0050A018> _mapTooltipFormatArguments;

    static loco_global<int8_t, 0x0050A040> _50A040;

    static loco_global<uint16_t, 0x0050C19C> time_since_last_tick;

    static loco_global<uint16_t, 0x0052334A> _52334A;
    static loco_global<uint16_t, 0x0052334C> _52334C;

    static loco_global<int8_t, 0x0052336C> _52336C;

    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<int32_t, 0x0113E730> _cursorY;

    static loco_global<ui::WindowType, 0x0052336F> _pressedWindowType;
    static loco_global<ui::window_number, 0x00523370> _pressedWindowNumber;
    static loco_global<int32_t, 0x00523372> _pressedWidgetIndex;
    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;
    static loco_global<uint16_t, 0x00523378> _dragLastX;
    static loco_global<uint16_t, 0x0052337A> _dragLastY;
    static loco_global<ui::window_number, 0x0052337C> _dragWindowNumber;
    static loco_global<ui::WindowType, 0x0052337E> _dragWindowType;
    static loco_global<uint8_t, 0x0052337F> _dragWidgetIndex;
    static loco_global<uint8_t, 0x00523380> _dragScrollIndex;
    static loco_global<ui::WindowType, 0x00523381> _tooltipWindowType;
    static loco_global<int16_t, 0x00523382> _tooltipWindowNumber;
    static loco_global<int16_t, 0x00523384> _tooltipWidgetIndex;
    static loco_global<uint16_t, 0x00523386> _tooltipCursorX;
    static loco_global<uint16_t, 0x00523388> _tooltipCursorY;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x0052338E> _ticksSinceDragStart;
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<int8_t, 0x00523393> _currentTool;
    static loco_global<int16_t, 0x00523394> _toolWidgetIndex;

    static loco_global<uint16_t, 0x00523396> _currentScrollArea;
    static loco_global<uint32_t, 0x00523398> _currentScrollOffset;

    static loco_global<int16_t, 0x005233A4> _5233A4;
    static loco_global<int16_t, 0x005233A6> _5233A6;
    static loco_global<ui::WindowType, 0x005233A8> _hoverWindowType;
    static uint8_t _5233A9;
    static loco_global<ui::window_number, 0x005233AA> _hoverWindowNumber;
    static loco_global<uint16_t, 0x005233AC> _hoverWidgetIdx;
    static loco_global<uint32_t, 0x005233AE> _5233AE;
    static loco_global<uint32_t, 0x005233B2> _5233B2;
    static loco_global<ui::WindowType, 0x005233B6> _modalWindowType;

    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;

    static loco_global<uint16_t, 0x00F252A4> _F252A4;

    static loco_global<int32_t, 0x01136F98> _currentTooltipStringId;

    static loco_global<uint16_t, 0x0113D84C> _dropdownItemCount;
    static loco_global<uint16_t, 0x0113D84E> _dropdownHighlightedIndex;
    static loco_global<string_id[40], 0x0113D850> _dropdownItemFormats;

    static loco_global<uint32_t, 0x0113DC60> _dropdownDisabledItems;

    static loco_global<uint32_t, 0x0113DC68> _dropdownItemHeight;
    static loco_global<uint32_t, 0x0113DC6C> _dropdownItemWidth;
    static loco_global<uint32_t, 0x0113DC70> _dropdownColumnCount;
    static loco_global<uint32_t, 0x0113DC74> _dropdownRowCount;
    static loco_global<uint16_t, 0x0113DC78> _113DC78;

    static std::map<ui::scrollview::scroll_part, string_id> scroll_widget_tooltips = {
        { ui::scrollview::scroll_part::hscrollbar_button_left, string_ids::tooltip_scroll_left },
        { ui::scrollview::scroll_part::hscrollbar_button_right, string_ids::tooltip_scroll_right },
        { ui::scrollview::scroll_part::hscrollbar_track_left, string_ids::tooltip_scroll_left_fast },
        { ui::scrollview::scroll_part::hscrollbar_track_right, string_ids::tooltip_scroll_right_fast },
        { ui::scrollview::scroll_part::hscrollbar_thumb, string_ids::tooltip_scroll_left_right },
        { ui::scrollview::scroll_part::vscrollbar_button_top, string_ids::tooltip_scroll_up },
        { ui::scrollview::scroll_part::vscrollbar_button_bottom, string_ids::tooltip_scroll_down },
        { ui::scrollview::scroll_part::vscrollbar_track_top, string_ids::tooltip_scroll_up_fast },
        { ui::scrollview::scroll_part::vscrollbar_track_bottom, string_ids::tooltip_scroll_down_fast },
        { ui::scrollview::scroll_part::vscrollbar_thumb, string_ids::tooltip_scroll_up_down },
    };

    void init_mouse()
    {
        _pressedWindowType = ui::WindowType::undefined;

        _tooltipNotShownTicks = -1;
        _hoverWindowType = ui::WindowType::undefined;

        _5233AE = 0;
        _5233B2 = 0;

        _mapSelectionFlags = 0;
    }

    void move_mouse(int32_t x, int32_t y, int32_t relX, int32_t relY)
    {
        _cursorX = x;
        _cursorY = y;
        addr<0x0114084C, int32_t>() = relX;
        addr<0x01140840, int32_t>() = relY;
    }

    bool is_hovering(ui::WindowType type)
    {
        return *_hoverWindowType == type;
    }

    bool is_hovering(ui::WindowType type, ui::window_number number)
    {
        return (*_hoverWindowType == type) && (_hoverWindowNumber == number);
    }

    bool is_hovering(ui::WindowType type, ui::window_number number, ui::widget_index widgetIndex)
    {
        return *_hoverWindowType == type && _hoverWindowNumber == number && _hoverWidgetIdx == widgetIndex;
    }

    ui::widget_index get_hovered_widget_index()
    {
        return _hoverWidgetIdx;
    }

    bool is_dropdown_active(ui::WindowType type, ui::widget_index index)
    {
        if (state() != input_state::dropdown_active)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (!has_flag(input_flags::widget_pressed))
            return false;

        return _pressedWidgetIndex == index;
    }

    bool is_pressed(ui::WindowType type, ui::window_number number)
    {
        if (state() != input_state::widget_pressed)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (_pressedWindowNumber != number)
            return false;

        if (!has_flag(input_flags::widget_pressed))
            return false;

        return true;
    }

    bool is_pressed(ui::WindowType type, ui::window_number number, ui::widget_index index)
    {
        return is_pressed(type, number) && _pressedWidgetIndex == index;
    }

    ui::widget_index get_pressed_widget_index()
    {
        return _pressedWidgetIndex;
    }

    bool is_tool_active(ui::WindowType type, ui::window_number number)
    {
        if (!has_flag(input_flags::tool_active))
            return false;

        return (*_toolWindowType == type && _toolWindowNumber == number);
    }

    // 0x004CE3D6
    void cancel_tool()
    {
        call(0x004CE3D6);
    }

    void cancel_tool(ui::WindowType type, ui::window_number number)
    {
        if (!is_tool_active(type, number))
            return;

        cancel_tool();
    }

#pragma mark - Mouse input

    // 0x004C7174
    void handle_mouse(int16_t x, int16_t y, mouse_button button)
    {
        _lastKnownButtonState = button;

        ui::window* window = WindowManager::findAt(x, y);

        ui::widget_index widgetIndex = -1;
        if (window != nullptr)
        {
            widgetIndex = window->find_widget_at(x, y);
        }

        if (*_modalWindowType != ui::WindowType::undefined)
        {
            if (window != nullptr)
            {
                if (window->type != *_modalWindowType)
                {
                    if (button == mouse_button::left_pressed)
                    {
                        WindowManager::bringToFront(_modalWindowType, 0);
                        audio::play_sound(audio::sound_id::error, x);
                        return;
                    }

                    if (button == mouse_button::right_pressed)
                    {
                        return;
                    }
                }
            }
        }

        ui::widget_t* widget = nullptr;
        if (widgetIndex != -1)
        {
            widget = &window->widgets[widgetIndex];
        }

        registers regs;
        regs.ebp = (int32_t)state();
        regs.esi = (uint32_t)window;
        regs.edx = widgetIndex;
        regs.edi = (uint32_t)widget;
        regs.cx = (uint16_t)button;
        regs.ax = x;
        regs.bx = y;
        switch (state())
        {
            case input_state::reset:
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                _tooltipTimeout = 0;
                _tooltipWindowType = ui::WindowType::undefined;
                state(input_state::normal);
                reset_flag(input_flags::flag4);
                state_normal(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::normal:
                state_normal(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::widget_pressed:
            case input_state::dropdown_active:
                state_widget_pressed(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::positioning_window:
                state_positioning_window(button, x, y, window, widget, widgetIndex);
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
                state_resizing(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::scroll_right:
                call(0x004C76A7, regs);
                break;
        }
    }

    mouse_button getLastKnownButtonState()
    {
        return _lastKnownButtonState;
    }

    // 0x004C7722
    static void state_resizing(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        auto w = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (w == nullptr)
        {
            state(input_state::reset);
            return;
        }

        bool doDefault = false;
        int dx = 0, dy = 0;
        switch (button)
        {
            case mouse_button::released:
                doDefault = true;
                break;

            case mouse_button::left_released:
                state(input_state::normal);
                _tooltipTimeout = 0;
                _tooltipWidgetIndex = _pressedWidgetIndex;
                _tooltipWindowType = _dragWindowType;
                _tooltipWindowNumber = _dragWindowNumber;

                if (w->flags & ui::window_flags::flag_15)
                {
                    doDefault = true;
                    break;
                }

                if (w->flags & ui::window_flags::flag_16)
                {
                    x = window->var_88A - window->width + _dragLastX;
                    y = window->var_88C - window->height + _dragLastY;
                    w->flags &= ~ui::window_flags::flag_16;
                    doDefault = true;
                    break;
                }

                window->var_88A = window->width;
                window->var_88C = window->height;
                x = _dragLastX - window->x - window->width + ui::width();
                y = _dragLastY - window->y - window->height + ui::height() - 27;
                w->flags |= ui::window_flags::flag_16;
                if (y >= ui::height() - 2)
                {
                    _dragLastX = x;
                    _dragLastY = y;
                    return;
                }

                dx = x - _dragLastX;
                dy = y - _dragLastY;

                if (dx == 0 && dy == 0)
                {
                    _dragLastX = x;
                    _dragLastY = y;
                    return;
                }

                break;

            default:
                return;
        }

        if (doDefault)
        {
            if (y >= ui::height() - 2)
            {
                _dragLastX = x;
                _dragLastY = y;
                return;
            }

            dx = x - _dragLastX;
            dy = y - _dragLastY;

            if (dx == 0 && dy == 0)
            {
                _dragLastX = x;
                _dragLastY = y;
                return;
            }

            w->flags &= ~ui::window_flags::flag_16;
        }

        w->invalidate();

        w->width = std::clamp<uint16_t>(w->width + dx, w->min_width, w->max_width);
        w->height = std::clamp<uint16_t>(w->height + dy, w->min_height, w->max_height);
        w->flags |= ui::window_flags::flag_15;
        w->call_on_resize();
        w->call_prepare_draw();
        w->scroll_areas[0].h_right = -1;
        w->scroll_areas[0].v_bottom = -1;
        w->scroll_areas[1].h_right = -1;
        w->scroll_areas[1].v_bottom = -1;
        window->update_scroll_widgets();
        w->invalidate();

        _dragLastX = x;
        _dragLastY = y;
    }

    // 0x004C7903
    static void state_positioning_window(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        auto w = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (w == nullptr)
        {
            state(input_state::reset);
            return;
        }

        switch (button)
        {
            case mouse_button::released:
            {
                y = std::clamp<int16_t>(y, 29, ui::height() - 29);

                int16_t dx = x - _dragLastX;
                int16_t dy = y - _dragLastY;

                if (w->move(dx, dy))
                {
                    _5233A9 = true;
                }

                _dragLastX = x;
                _dragLastY = y;
                break;
            }

            case mouse_button::left_released:
            {
                window_position_end();

                y = std::clamp<int16_t>(y, 29, ui::height() - 29);

                int dx = x - _dragLastX;
                int dy = y - _dragLastY;
                if (w->move(dx, dy))
                {
                    _5233A9 = true;
                }

                _dragLastX = x;
                _dragLastY = y;

                if (_5233A9 == false)
                {
                    auto dragWindow = WindowManager::find(_dragWindowType, _dragWindowNumber);
                    if (dragWindow != nullptr)
                    {
                        if (dragWindow->is_enabled(_pressedWidgetIndex))
                        {
                            auto pressedWidget = &dragWindow->widgets[_pressedWidgetIndex];

                            audio::play_sound(audio::sound_id::click_press, dragWindow->x + pressedWidget->mid_x());
                            dragWindow->call_on_mouse_up(_pressedWidgetIndex);
                        }
                    }
                }

                w->call_on_move(_dragLastX, _dragLastY);
            }
            break;

            default:
                break;
        }
    }

    static void dropdown_register_selection(int16_t item)
    {
        auto window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);
        if (window == nullptr)
            return;

        WindowManager::close(ui::WindowType::dropdown, 0);
        window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);

        bool flagSet = has_flag(input_flags::widget_pressed);
        reset_flag(input_flags::widget_pressed);
        if (flagSet)
        {
            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
        }

        input::state(input_state::normal);
        _tooltipTimeout = 0;
        _tooltipWidgetIndex = _pressedWidgetIndex;
        _tooltipWindowType = _pressedWindowType;
        _tooltipWindowNumber = _pressedWindowNumber;

        if (*_modalWindowType == ui::WindowType::undefined || *_modalWindowType == window->type)
        {
            window->call_on_dropdown(_pressedWidgetIndex, item);
        }
    }

    static int dropdown_index_from_point(ui::window* window, int x, int y)
    {
        // Check whether x and y are over a list item
        int left = x - window->x;
        if (left < 0)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }
        if (left >= window->width)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        // 2px of padding on the top of the list?
        int top = y - window->y - 2;
        if (top < 0)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        unsigned int itemY = top / _dropdownItemHeight;
        if (itemY >= _dropdownItemCount)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        left -= 2;
        if (left < 0)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        unsigned int itemX = left / _dropdownItemWidth;
        if (itemX >= _dropdownColumnCount)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }
        if (itemY >= _dropdownRowCount)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        int item = itemY * _dropdownColumnCount + itemX;
        if (item >= _dropdownItemCount)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        if (item < 32 && (_dropdownDisabledItems & (1ULL << item)) != 0)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        if (_dropdownItemFormats[item] == 0)
        {
            return DROPDOWN_ITEM_UNDEFINED;
        }

        return item;
    }

    // 0x004C7AE7
    static void state_widget_pressed(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        _52334A = x;
        _52334C = y;

        auto pressedWindow = WindowManager::find(_pressedWindowType, _pressedWindowNumber);
        if (pressedWindow == nullptr)
        {
            input::state(input_state::reset);
            return;
        }

        if (input::state() == input_state::dropdown_active)
        {
            if (_113DC78 & (1 << 0))
            {
                if (widgetIndex == -1 || *_pressedWindowType != window->type || _pressedWindowNumber != window->number || _pressedWidgetIndex != widgetIndex)
                {
                    if (widgetIndex == -1 || window->type != ui::WindowType::dropdown)
                    {
                        WindowManager::close(ui::WindowType::dropdown, 0);

                        if (*_pressedWindowType != ui::WindowType::undefined)
                        {
                            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                        }

                        _pressedWindowType = ui::WindowType::undefined;
                        input::reset_flag(input_flags::widget_pressed);
                        input::state(input_state::reset);
                        return;
                    }
                }
            }
        }

        bool doShared = false;
        switch (button)
        {
            case mouse_button::released: // 0
            {
                if (window == nullptr)
                    break;

                if (window->type == *_pressedWindowType && window->number == _pressedWindowNumber && widgetIndex == _pressedWidgetIndex)
                {
                    if (!window->is_disabled(widgetIndex))
                    {
                        if (_clickRepeatTicks != 0)
                        {
                            _clickRepeatTicks++;
                        }

                        // Handle click repeat
                        if (window->is_holdable(widgetIndex) && _clickRepeatTicks >= 16 && (_clickRepeatTicks % 4) == 0)
                        {
                            window->call_on_mouse_down(widgetIndex);
                        }

                        bool flagSet = input::has_flag(input_flags::widget_pressed);
                        input::set_flag(input_flags::widget_pressed);
                        if (!flagSet)
                        {
                            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, widgetIndex);
                        }

                        return;
                    }
                }

                break;
            }

            case mouse_button::left_pressed: // 1
                if (input::state() == input_state::dropdown_active)
                {
                    if (window != nullptr && widgetIndex != -1)
                    {
                        auto buttonWidget = &window->widgets[widgetIndex];
                        audio::play_sound(audio::sound_id::click_up, window->x + buttonWidget->mid_x());
                    }
                }
                return;

            case mouse_button::left_released: // 2
                doShared = true;
                break;

            case mouse_button::right_pressed: // 3
                if (input::state() == input_state::dropdown_active)
                {
                    doShared = true;
                }
                else
                {
                    return;
                }

                break;

            case mouse_button::right_released:
                return;
        }

        if (doShared)
        {
            // 0x4C7BC7
            if (input::state() == input_state::dropdown_active)
            {
                if (window != nullptr)
                {
                    if (window->type == ui::WindowType::dropdown)
                    {
                        auto item = dropdown_index_from_point(window, x, y);
                        if (item != DROPDOWN_ITEM_UNDEFINED)
                        {
                            dropdown_register_selection(item);
                        }
                    }
                    else
                    {
                        if (window->type == *_pressedWindowType && window->number == _pressedWindowNumber && widgetIndex == _pressedWidgetIndex)
                        {
                            if (has_flag(input_flags::flag1))
                            {
                                bool flagSet = has_flag(input_flags::flag2);
                                set_flag(input_flags::flag2);
                                if (!flagSet)
                                {
                                    return;
                                }
                            }

                            dropdown_register_selection(DROPDOWN_ITEM_UNDEFINED);
                        }
                    }
                }

                // 0x4C7DA0
                WindowManager::close(ui::WindowType::dropdown, 0);
                window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);
            }

            input::state(input_state::normal);
            _tooltipTimeout = 0;
            _tooltipWidgetIndex = _pressedWidgetIndex;
            _tooltipWindowType = _pressedWindowType;
            _tooltipWindowNumber = _pressedWindowNumber;
            if (window != nullptr)
            {
                audio::play_sound(audio::sound_id::click_up, window->x + widget->mid_x());
            }

            if (window != nullptr && window->type == *_pressedWindowType && window->number == _pressedWindowNumber && widgetIndex == _pressedWidgetIndex && !window->is_disabled(widgetIndex))
            {
                WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                window->call_on_mouse_up(widgetIndex);
                return;
            }
        }

        // 0x4C7F02
        _clickRepeatTicks = 0;
        if (input::state() != input_state::dropdown_active)
        {
            bool flagSet = has_flag(input_flags::widget_pressed);
            reset_flag(input_flags::widget_pressed);
            if (flagSet)
            {
                WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
            }
        }

        if (input::state() == input_state::dropdown_active)
        {
            if (window != nullptr && window->type == ui::WindowType::dropdown)
            {
                auto item = dropdown_index_from_point(window, x, y);
                if (item != DROPDOWN_ITEM_UNDEFINED)
                {
                    _dropdownHighlightedIndex = item;
                    WindowManager::invalidate(ui::WindowType::dropdown, 0);
                }
            }
        }
    }

    // 0x004C8048
    static void state_normal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        switch (state)
        {
            case mouse_button::left_pressed:
                state_normal_left(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::right_pressed:
                state_normal_right(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::released:
                state_normal_hover(x, y, window, widget, widgetIndex);
                break;

            default:
                break;
        }
    }

    // 0x004C8098
    static void state_normal_hover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        ui::WindowType windowType = ui::WindowType::undefined;
        ui::window_number windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        if (windowType != *_hoverWindowType || windowNumber != *_hoverWindowNumber || widgetIndex != *_hoverWidgetIdx)
        {
            widget_over_flatbutton_invalidate();
            _hoverWindowType = windowType;
            _hoverWindowNumber = windowNumber;
            _hoverWidgetIdx = widgetIndex;
            widget_over_flatbutton_invalidate();
        }

        if (window != nullptr && widgetIndex != -1)
        {
            if (!window->is_disabled(widgetIndex))
            {
                window->call_3(widgetIndex);
            }
        }

        string_id tooltipStringId = string_ids::null;
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
                    window->call_scroll_mouse_over(scrollX, scrollY, scrollIndex / sizeof(ui::scroll_area_t));
                }
                else
                {
                    tooltipStringId = scroll_widget_tooltips[scrollArea];
                    if (*_tooltipWindowType != ui::WindowType::undefined)
                    {
                        if (tooltipStringId != _currentTooltipStringId)
                        {
                            call(0x4C87B5);
                        }
                    }
                }
            }
        }

        if (*_tooltipWindowType != ui::WindowType::undefined)
        {
            if (*_tooltipWindowType == window->type && _tooltipWindowNumber == window->number && _tooltipWidgetIndex == widgetIndex)
            {
                _tooltipTimeout += time_since_last_tick;
                if (_tooltipTimeout >= 8000)
                {
                    WindowManager::close(ui::WindowType::tooltip);
                }
            }
            else
            {
                call(0x4C87B5);
            }

            return;
        }

        if (_tooltipNotShownTicks < 500 || (x == _tooltipCursorX && y == _tooltipCursorY))
        {
            _tooltipTimeout += time_since_last_tick;
            int bp = 2000;
            if (_tooltipNotShownTicks <= 1000)
            {
                bp = 0;
            }

            if (bp > _tooltipTimeout)
            {
                return;
            }

            if (tooltipStringId == string_ids::null)
            {
                ui::tooltip::open(window, widgetIndex, x, y);
            }
            else
            {
                ui::tooltip::update(window, widgetIndex, tooltipStringId, x, y);
            }
        }

        _tooltipTimeout = 0;
        _tooltipCursorX = x;
        _tooltipCursorY = y;
    }

    // 0x004C84BE
    static void state_normal_left(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        ui::WindowType windowType = ui::WindowType::undefined;
        ui::window_number windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        WindowManager::close(ui::WindowType::error);
        WindowManager::close(ui::WindowType::tooltip);

        // Window might have changed position in the list, therefore find it again
        window = WindowManager::find(windowType, windowNumber);
        if (window == nullptr)
        {
            return;
        }

        window = WindowManager::bringToFront(window);
        if (widgetIndex == -1)
        {
            return;
        }

        switch (widget->type)
        {
            case ui::widget_type::caption_22:
            case ui::widget_type::caption_23:
            case ui::widget_type::caption_24:
            case ui::widget_type::caption_25:
                window_position_begin(x, y, window, widgetIndex);
                break;

            case ui::widget_type::panel:
            case ui::widget_type::frame:
                if (window->can_resize() && (x >= (window->x + window->width - 19)) && (y >= (window->y + window->height - 19)))
                {
                    window_resize_begin(x, y, window, widgetIndex);
                }
                else
                {
                    window_position_begin(x, y, window, widgetIndex);
                }
                break;

            case ui::widget_type::viewport:
                state(input_state::viewport_left);
                _dragLastX = x;
                _dragLastY = y;
                _dragWindowType = window->type;
                _dragWindowNumber = window->number;
                if (has_flag(input_flags::tool_active))
                {
                    auto w = WindowManager::find(_toolWindowType, _toolWindowNumber);
                    if (w != nullptr)
                    {
                        w->call_tool_down(_toolWidgetIndex, x, y);
                        set_flag(input_flags::flag4);
                    }
                }
                break;

            case ui::widget_type::scrollview:
                state(input_state::scroll_left);
                _pressedWidgetIndex = widgetIndex;
                _pressedWindowType = window->type;
                _pressedWindowNumber = window->number;
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                scroll_begin(x, y, window, widget, widgetIndex);
                break;

            default:
                if (window->is_enabled(widgetIndex) && !window->is_disabled(widgetIndex))
                {
                    audio::play_sound(audio::sound_id::click_down, window->x + widget->mid_x());

                    // Set new cursor down widget
                    _pressedWidgetIndex = widgetIndex;
                    _pressedWindowType = window->type;
                    _pressedWindowNumber = window->number;
                    set_flag(input_flags::widget_pressed);
                    state(input_state::widget_pressed);
                    _clickRepeatTicks = 1;

                    WindowManager::invalidateWidget(window->type, window->number, widgetIndex);
                    window->call_on_mouse_down(widgetIndex);
                }

                break;
        }
    }

    // 0x004C834A
    static void state_normal_right(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        ui::WindowType windowType = ui::WindowType::undefined;
        ui::window_number windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        WindowManager::close(ui::WindowType::tooltip);

        // Window might have changed position in the list, therefore find it again
        window = WindowManager::find(windowType, windowNumber);
        if (window == nullptr)
        {
            return;
        }

        window = WindowManager::bringToFront(window);

        if (widgetIndex == -1)
        {
            return;
        }

        if (*_modalWindowType != ui::WindowType::undefined)
        {
            if (*_modalWindowType == window->type)
            {
                scroll_begin(x, y, window, widget, widgetIndex);
            }

            return;
        }

        if (is_title_mode())
        {
            return;
        }

        switch (widget->type)
        {
            default:
                break;

            case ui::widget_type::viewport:
                viewport_drag_begin(window);

                _dragLastX = x;
                _dragLastY = y;

                ui::hide_cursor();
                sub_407218();

                _5233AE = 0;
                _5233B2 = 0;
                set_flag(input_flags::flag5);
                break;

            case ui::widget_type::scrollview:
                scroll_drag_begin(x, y, window, widgetIndex);

                _5233AE = 0;
                _5233B2 = 0;
                set_flag(input_flags::flag5);
                break;
        }
    }

#pragma mark - Window positioning

    // 0x00C877D
    static void window_position_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index)
    {
        state(input_state::positioning_window);
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _5233A9 = false;
    }

    static void window_position_end()
    {
        state(input_state::normal);
        _tooltipTimeout = 0;
        _tooltipWidgetIndex = _pressedWidgetIndex;
        _tooltipWindowType = _dragWindowType;
        _tooltipWindowNumber = _dragWindowNumber;
    }

#pragma mark - Window resizing

    // 0x004C85D1
    static void window_resize_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index)
    {
        state(input_state::resizing);
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        window->flags &= ~ui::window_flags::flag_15;
    }

#pragma mark - Viewport dragging

    static void viewport_drag_begin(window* w)
    {
        w->flags &= ~ui::window_flags::scrolling_to_location;
        state(input_state::viewport_right);
        _dragWindowType = w->type;
        _dragWindowNumber = w->number;
        _ticksSinceDragStart = 0;
    }

#pragma mark - Scroll bars

    // 0x004C8689
    void scroll_begin(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, int8_t widgetIndex)
    {
        ui::scrollview::scroll_part scrollArea;
        int16_t outX, outY;
        int32_t scrollAreaOffset;

        ui::scrollview::get_part(window, widget, x, y, &outX, &outY, &scrollArea, &scrollAreaOffset);

        _currentScrollArea = (uint16_t)scrollArea;
        _currentScrollOffset = scrollAreaOffset;

        if (scrollArea == ui::scrollview::scroll_part::view)
        {
            window->call_scroll_mouse_down(outX, outY, scrollAreaOffset / sizeof(ui::scroll_area_t));
            return;
        }

        // Not implemented for any window
        // window->call_22()

        registers regs;
        regs.eax = outX;
        regs.ebx = outY;
        regs.cx = (int16_t)scrollArea;
        regs.edx = scrollAreaOffset;

        switch (scrollArea)
        {
            case ui::scrollview::scroll_part::hscrollbar_button_left:
                call(0x4c894f, regs);
                break;
            case ui::scrollview::scroll_part::hscrollbar_button_right:
                call(0x4c89ae, regs);
                break;
            case ui::scrollview::scroll_part::hscrollbar_track_left:
                call(0x4c8a36, regs);
                break;
            case ui::scrollview::scroll_part::hscrollbar_track_right:
                call(0x4c8aa6, regs);
                break;

            case ui::scrollview::scroll_part::vscrollbar_button_top:
                call(0x4c8b26, regs);
                break;
            case ui::scrollview::scroll_part::vscrollbar_button_bottom:
                call(0x4c8b85, regs);
                break;
            case ui::scrollview::scroll_part::vscrollbar_track_top:
                call(0x4c8c0d, regs);
                break;
            case ui::scrollview::scroll_part::vscrollbar_track_bottom:
                call(0x4c8c7d, regs);
                break;

            default:
                break;
        }
    }

#pragma mark - Scrollview dragging

    static void scroll_drag_begin(int16_t x, int16_t y, ui::window* window, ui::widget_index widgetIndex)
    {
        state(input_state::scroll_right);
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _dragWidgetIndex = widgetIndex;
        _ticksSinceDragStart = 0;

        _dragScrollIndex = window->get_scroll_data_index(widgetIndex);

        ui::hide_cursor();
        sub_407218();
    }

#pragma mark - Widgets

    static void widget_over_flatbutton_invalidate()
    {
        ui::WindowType windowType = _hoverWindowType;
        uint16_t widgetIdx = _hoverWidgetIdx;
        uint16_t windowNumber = _hoverWindowNumber;

        if (windowType == ui::WindowType::undefined)
        {
            WindowManager::invalidateWidget(windowType, windowNumber, widgetIdx);
            return;
        }

        ui::window* oldWindow = WindowManager::find(windowType, windowNumber);

        if (oldWindow != nullptr)
        {
            oldWindow->call_prepare_draw();

            ui::widget_t* oldWidget = &oldWindow->widgets[widgetIdx];
            if (
                oldWidget->type == ui::widget_type::wt_16 || oldWidget->type == ui::widget_type::wt_10 || oldWidget->type == ui::widget_type::wt_9)
            {

                WindowManager::invalidateWidget(windowType, windowNumber, widgetIdx);
            }
        }
    }

#pragma mark -

    // 0x004CD47A
    void process_mouse_over(int16_t x, int16_t y)
    {
        bool skipItem = false;
        ui::cursor_id cursorId = ui::cursor_id::pointer;

        _mapTooltipFormatArguments = string_ids::null;
        _50A040 = -1;

        if (_mapSelectionFlags & (1 << 6))
        {
            _mapSelectionFlags &= (uint16_t) ~(1 << 6);
            auto station = stationmgr::get(_F252A4);
            if (!station->empty())
            {
                station->invalidate();
            }
        }

        ui::window* window = ui::WindowManager::findAt(x, y);

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
                            cursorId = (ui::cursor_id)*_currentTool;
                            auto wnd = ui::WindowManager::find(_toolWindowType, _toolWindowNumber);
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
                            switch (viewport_interaction::get_item_left(x, y, nullptr))
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
            viewport_interaction::right_over(x, y);
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

    gfx::point_t getMouseLocation()
    {
        return gfx::point_t(_cursorX, _cursorY);
    }
}
