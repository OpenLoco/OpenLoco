#include "../audio/audio.h"
#include "../CompanyManager.h"
#include "../Console.h"
#include "../Input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../map/tilemgr.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../StationManager.h"
#include "../things/thingmgr.h"
#include "../townmgr.h"
#include "../tutorial.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"
#include "../window.h"
#include <map>

using namespace openloco::interop;
using namespace openloco::ui;
using namespace openloco::ui::scrollview;
using namespace openloco::ui::viewport_interaction;

#define DROPDOWN_ITEM_UNDEFINED -1

namespace openloco::input
{
    static void stateScrollLeft(mouse_button cx, widget_index edx, ui::window* window, ui::widget_t* widget, int16_t x, int16_t y);
    static void stateScrollRight(const mouse_button button, const int16_t x, const int16_t y);
    static void stateResizing(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void stateWidgetPressed(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void stateNormal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void stateNormalHover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void stateNormalLeft(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void stateNormalRight(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);
    static void statePositioningWindow(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex);

    static void windowPositionBegin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index);
    static void windowPositionEnd();

    static void windowResizeBegin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index);

    static void viewportDragBegin(window* w);

    static void scrollDragBegin(int16_t x, int16_t y, window* pWindow, widget_index index);

    static void widgetOverFlatbuttonInvalidate();

#pragma mark - Input

    static loco_global<mouse_button, 0x001136FA0> _lastKnownButtonState;

    static loco_global<map::map_pos[4], 0x004F9296> _4F9296;

    static loco_global<string_id, 0x0050A018> _mapTooltipFormatArguments;

    static loco_global<company_id_t, 0x0050A040> _mapTooltipOwner;

    static loco_global<uint16_t, 0x0050C19C> time_since_last_tick;

    static loco_global<uint16_t, 0x0052334A> _52334A;
    static loco_global<uint16_t, 0x0052334C> _52334C;

    static loco_global<int8_t, 0x0052336C> _52336C;

    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<int32_t, 0x0113E730> _cursorY;

    // TODO: name?
    static loco_global<int32_t, 0x00523338> _cursorX2;
    static loco_global<int32_t, 0x0052333C> _cursorY2;

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

    static loco_global<uint16_t, 0x00F252A4> _hoveredStationId;

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

    void initMouse()
    {
        _pressedWindowType = ui::WindowType::undefined;

        _tooltipNotShownTicks = -1;
        _hoverWindowType = ui::WindowType::undefined;

        _5233AE = 0;
        _5233B2 = 0;

        _mapSelectionFlags = 0;
    }

    void moveMouse(int32_t x, int32_t y, int32_t relX, int32_t relY)
    {
        _cursorX = x;
        _cursorY = y;
        addr<0x0114084C, int32_t>() = relX;
        addr<0x01140840, int32_t>() = relY;
    }

    bool isHovering(ui::WindowType type)
    {
        return *_hoverWindowType == type;
    }

    bool isHovering(ui::WindowType type, ui::window_number number)
    {
        return (*_hoverWindowType == type) && (_hoverWindowNumber == number);
    }

    bool isHovering(ui::WindowType type, ui::window_number number, ui::widget_index widgetIndex)
    {
        return *_hoverWindowType == type && _hoverWindowNumber == number && _hoverWidgetIdx == widgetIndex;
    }

    ui::widget_index getHoveredWidgetIndex()
    {
        return _hoverWidgetIdx;
    }

    bool isDropdownActive(ui::WindowType type, ui::widget_index index)
    {
        if (state() != input_state::dropdown_active)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (!hasFlag(input_flags::widget_pressed))
            return false;

        return _pressedWidgetIndex == index;
    }

    bool isPressed(ui::WindowType type, ui::window_number number)
    {
        if (state() != input_state::widget_pressed)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (_pressedWindowNumber != number)
            return false;

        if (!hasFlag(input_flags::widget_pressed))
            return false;

        return true;
    }

    bool isPressed(ui::WindowType type, ui::window_number number, ui::widget_index index)
    {
        return isPressed(type, number) && _pressedWidgetIndex == index;
    }

    ui::widget_index getPressedWidgetIndex()
    {
        return _pressedWidgetIndex;
    }

    // 0x004C6E65
    void updateCursorPosition()
    {
        switch (tutorial::state())
        {
            case tutorial::tutorial_state::none:
            {
                _cursorX2 = _cursorX;
                _cursorY2 = _cursorY;
                break;
            }

            case tutorial::tutorial_state::playing:
            {
                _cursorX2 = tutorial::nextInput();
                _cursorY2 = tutorial::nextInput();
                ui::setCursorPos(*_cursorX2, *_cursorY2);
                break;
            }

            case tutorial::tutorial_state::recording:
            {
                call(0x004C6EC3);
                break;
            }
        }
    }

    bool isToolActive(ui::WindowType type, ui::window_number number)
    {
        if (!hasFlag(input_flags::tool_active))
            return false;

        return (*_toolWindowType == type && _toolWindowNumber == number);
    }

    // 0x004CE367
    // tool (al)
    // widgetIndex (dx)
    // w (esi)
    // TODO: Maybe create a an enum similar to TOOL_IDX in OpenRCT2 for tool (instead of uint8_t)
    bool toolSet(ui::window* w, int16_t widgetIndex, uint8_t tool)
    {
        if (input::hasFlag(input::input_flags::tool_active))
        {
            if (w->type == *_toolWindowType && w->number == _toolWindowNumber
                && widgetIndex == _toolWidgetIndex)
            {
                toolCancel();
                return true;
            }
            else
            {
                toolCancel();
            }
        }

        input::setFlag(input::input_flags::tool_active);
        input::resetFlag(input::input_flags::flag6);
        _currentTool = tool;
        _toolWindowType = w->type;
        _toolWindowNumber = w->number;
        _toolWidgetIndex = widgetIndex;
        return false;
    }

    // 0x004CE3D6
    void toolCancel()
    {
        if (input::hasFlag(input::input_flags::tool_active))
        {
            input::resetFlag(input::input_flags::tool_active);

            map::tilemgr::mapInvalidateSelectionRect();
            map::tilemgr::mapInvalidateMapSelectionTiles();

            // Reset map selection
            _mapSelectionFlags = _mapSelectionFlags & 0xFFE0;

            if (_toolWidgetIndex >= 0)
            {
                // Invalidate tool widget
                ui::WindowManager::invalidateWidget(_toolWindowType, _toolWindowNumber, _toolWidgetIndex);

                // Abort tool event
                window* w = ui::WindowManager::find(_toolWindowType, _toolWindowNumber);
                if (w != nullptr)
                    w->callToolAbort(_toolWidgetIndex);
            }
        }
    }

    void toolCancel(ui::WindowType type, ui::window_number number)
    {
        if (!isToolActive(type, number))
            return;

        toolCancel();
    }

    uint16_t getMapSelectionFlags()
    {
        return _mapSelectionFlags;
    }

    bool hasMapSelectionFlag(uint8_t flags)
    {
        return (_mapSelectionFlags & flags) != 0;
    }

    void setMapSelectionFlags(uint8_t flags)
    {
        _mapSelectionFlags = _mapSelectionFlags | flags;
    }

    void resetMapSelectionFlag(uint8_t flags)
    {
        _mapSelectionFlags = _mapSelectionFlags & ~flags;
    }

#pragma mark - Mouse input
    static void stateViewportLeft(const mouse_button cx, const int16_t x, const int16_t y);
    static void stateViewportRight(const mouse_button cx, const int16_t x, const int16_t y);

    // 0x004C7174
    void handleMouse(int16_t x, int16_t y, mouse_button button)
    {
        _lastKnownButtonState = button;

        ui::window* window = WindowManager::findAt(x, y);

        ui::widget_index widgetIndex = -1;
        if (window != nullptr)
        {
            widgetIndex = window->findWidgetAt(x, y);
        }

        if (*_modalWindowType != ui::WindowType::undefined)
        {
            if (window != nullptr)
            {
                if (window->type != *_modalWindowType)
                {
                    if (button == mouse_button::left_pressed)
                    {
                        WindowManager::bringToFront(_modalWindowType);
                        audio::playSound(audio::sound_id::error, x);
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
                resetFlag(input_flags::flag4);
                stateNormal(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::normal:
                stateNormal(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::widget_pressed:
            case input_state::dropdown_active:
                stateWidgetPressed(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::positioning_window:
                statePositioningWindow(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::viewport_right:
                stateViewportRight(button, x, y);
                break;

            case input_state::viewport_left:
                stateViewportLeft(button, x, y);
                break;

            case input_state::scroll_left:
                stateScrollLeft(button, widgetIndex, window, widget, x, y);
                break;

            case input_state::resizing:
                stateResizing(button, x, y, window, widget, widgetIndex);
                break;

            case input_state::scroll_right:
                stateScrollRight(button, x, y);
                break;
        }
    }

    // 0x004C7334
    static void stateViewportLeft(const mouse_button button, const int16_t x, const int16_t y)
    {
        auto window = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (window == nullptr)
        {
            input::state(input_state::reset);
            return;
        }

        switch (button)
        {
            case mouse_button::released:
            {
                // 0x4C735D
                auto viewport = window->viewports[0];
                if (viewport == nullptr)
                {
                    viewport = window->viewports[1];
                }
                if (viewport == nullptr)
                {
                    input::state(input_state::reset);
                    return;
                }

                if (window->type == _dragWindowType && window->number == _dragWindowNumber)
                {

                    if (input::hasFlag(input_flags::tool_active))
                    {
                        auto tool = WindowManager::find(_toolWindowType, _toolWindowNumber);
                        if (tool != nullptr)
                        {
                            tool->call_12(_toolWidgetIndex);
                        }
                    }
                }
                break;
            }

            case mouse_button::left_released:
            {
                // 0x4C73C2
                input::state(input_state::reset);
                if (window->type != _dragWindowType || window->number != _dragWindowNumber)
                    return;

                if (hasFlag(input_flags::tool_active))
                {
                    auto tool = WindowManager::find(_toolWindowType, _toolWindowNumber);
                    if (tool != nullptr)
                    {
                        tool->call_13(_toolWidgetIndex);
                    }
                }
                else if (!hasFlag(input_flags::flag4))
                {
                    viewport_interaction::InteractionArg ptr{};

                    auto interactionItem = viewport_interaction::getItemLeft(x, y, &ptr);
                    switch (interactionItem)
                    {
                        case InteractionItem::thing:
                        {
                            auto _thing = reinterpret_cast<Thing*>(ptr.object);
                            auto veh = _thing->asVehicle();
                            if (veh != nullptr)
                            {
                                ui::vehicle::main::open(reinterpret_cast<openloco::vehicle*>(veh));
                            }
                            break;
                        }

                        case InteractionItem::town:
                        {
                            ui::windows::town::open(ptr.value);
                            break;
                        }

                        case InteractionItem::station:
                        {
                            ui::windows::station::open(ptr.value);
                            break;
                        }

                        case InteractionItem::headquarterBuilding:
                        {
                            auto building = ((map::tile_element*)ptr.object)->asBuilding();
                            if (building != nullptr)
                            {
                                auto index = building->multiTileIndex();
                                map::map_pos pos{ ptr.x, ptr.y };
                                pos.x -= _4F9296[index].x;
                                pos.y -= _4F9296[index].y;

                                auto z = building->baseZ();
                                for (auto& company : companymgr::companies())
                                {
                                    if (company.empty())
                                        continue;

                                    if (company.headquarters_x == pos.x
                                        && company.headquarters_y == pos.y
                                        && company.headquarters_z == z)
                                    {
                                        ui::windows::CompanyWindow::open(company.id());
                                        break;
                                    }
                                }
                            }
                            break;
                        }

                        case InteractionItem::industry:
                        {
                            ui::windows::industry::open(ptr.value);
                            break;
                        }

                        default:
                            break;
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // 0x004A5AA1 TODO: Move to a better file
    static void signalInteract(window* main, map::signal_element* signal, const uint8_t bh, const map::map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(signal);
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A5AA1, regs);
    }

    // 0x004A5B66 TODO: Move to a better file
    static void trackStationInteract(window* main, map::station_element* station, const map::map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(station);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A5B66, regs);
    }

    // 0x004A5BDF TODO: Move to a better file
    static void roadStationInteract(window* main, map::station_element* station, const map::map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(station);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A5BDF, regs);
    }

    // 0x004A5C58 TODO: Move to a better file
    static void airportInteract(window* main, map::station_element* station, const map::map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(station);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A5C58, regs);
    }

    // 0x004A5CC5 TODO: Move to a better file
    static void dockInteract(window* main, map::station_element* station, const map::map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(station);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A5CC5, regs);
    }

    // 0x004BB116 TODO: Move to a better file
    static void treeInteract(map::tree_element* tree, const map::map_pos pos)
    {
        registers regs{};
        regs.edx = reinterpret_cast<uint32_t>(tree);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004BB116, regs);
    }

    // 0x0042D9BF TODO: Move to a better file
    static void buildingInteract(map::building_element* building, const map::map_pos pos)
    {
        registers regs{};
        regs.edx = reinterpret_cast<uint32_t>(building);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x0042D9BF, regs);
    }

    // 0x004C4809 TODO: Move to a better file
    static void wallInteract(map::wall_element* wall, const map::map_pos pos)
    {
        registers regs{};
        regs.edx = reinterpret_cast<uint32_t>(wall);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004C4809, regs);
    }

    // 0x0042F007 TODO: Move to a better file
    static void headquarterInteract(map::building_element* building, const map::map_pos pos)
    {
        registers regs{};
        regs.edx = reinterpret_cast<uint32_t>(building);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x0042F007, regs);
    }

    // 0x004C74BB
    static void stateViewportRight(const mouse_button button, const int16_t x, const int16_t y)
    {
        auto window = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (window == nullptr)
        {
            input::state(input_state::reset);
            return;
        }

        switch (button)
        {

            case mouse_button::released:
            {
                // 4C74E4
                _ticksSinceDragStart += time_since_last_tick;
                auto vp = window->viewports[0];
                if (vp == nullptr)
                {
                    vp = window->viewports[1];
                }
                if (vp == nullptr)
                {
                    input::state(input_state::reset);
                    return;
                }

                if (window->flags & window_flags::viewport_no_scrolling)
                {
                    return;
                }

                if (x != 0 || y != 0)
                {
                    _ticksSinceDragStart = 1000;
                    window->viewport_configurations[0].saved_view_x += x << (vp->zoom + 1);
                    window->viewport_configurations[0].saved_view_y += y << (vp->zoom + 1);
                }

                break;
            }

            case mouse_button::right_released:
            {
                if (_ticksSinceDragStart > 500)
                {
                    input::state(input_state::reset);
                    return;
                }

                input::state(input_state::reset);
                viewport_interaction::InteractionArg ptr{};
                auto item = viewport_interaction::rightOver(_dragLastX, _dragLastY, &ptr);

                switch (item)
                {
                    case InteractionItem::t_0:
                    default:
                    {
                        auto item2 = viewport_interaction::getItemLeft(_dragLastX, _dragLastY, &ptr);
                        switch (item2)
                        {
                            case InteractionItem::thing:
                            {
                                auto _thing = reinterpret_cast<Thing*>(ptr.object);
                                auto veh = _thing->asVehicle();
                                if (veh != nullptr)
                                {
                                    auto head = thingmgr::get<vehicle>(reinterpret_cast<openloco::vehicle*>(veh)->head);
                                    ui::windows::vehicle_list::open(head->owner, static_cast<uint8_t>(head->vehicleType));
                                }
                                break;
                            }
                            case InteractionItem::town:
                                ui::windows::town_list::open();
                                break;
                            case InteractionItem::station:
                            {
                                auto station = stationmgr::get(ptr.value);
                                ui::windows::station_list::open(station->owner);
                                break;
                            }
                            case InteractionItem::industry:
                                ui::windows::industry_list::open();
                                break;
                            default:
                                break;
                        }

                        break;
                    }

                    case InteractionItem::track:
                    {
                        auto track = ((map::tile_element*)ptr.object)->asTrack();
                        if (track != nullptr)
                        {
                            if (track->owner() == companymgr::getControllingId())
                            {
                                ui::windows::construction::openAtTrack(window, track, { ptr.x, ptr.y });
                            }
                            else
                            {
                                ui::windows::CompanyWindow::open(track->owner());
                            }
                        }
                        break;
                    }
                    case InteractionItem::road:
                    {
                        auto road = reinterpret_cast<map::tile_element*>(ptr.object)->asRoad();
                        if (road != nullptr)
                        {

                            auto owner = road->owner();

                            auto roadObject = objectmgr::get<road_object>(road->roadObjectId());
                            if (owner == companymgr::getControllingId() || owner == company_id::neutral || (roadObject->flags & flags_12::unk_03))
                            {
                                ui::windows::construction::openAtRoad(window, road, { ptr.x, ptr.y });
                            }
                            else
                            {
                                ui::windows::CompanyWindow::open(owner);
                            }
                        }
                        break;
                    }
                    case InteractionItem::trackExtra:
                    {
                        auto track = ((map::tile_element*)ptr.object)->asTrack();
                        if (track != nullptr)
                        {
                            ui::windows::construction::setToTrackExtra(window, track, ptr.unkBh, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::roadExtra:
                    {
                        auto road = ((map::tile_element*)ptr.object)->asRoad();
                        if (road != nullptr)
                        {
                            ui::windows::construction::setToRoadExtra(window, road, ptr.unkBh, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::signal:
                    {
                        auto signal = ((map::tile_element*)ptr.object)->asSignal();
                        if (signal != nullptr)
                        {
                            signalInteract(window, signal, ptr.unkBh, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::trackStation:
                    {
                        auto station = ((map::tile_element*)ptr.object)->asStation();
                        if (station != nullptr)
                        {
                            trackStationInteract(window, station, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::roadStation:
                    {
                        auto station = ((map::tile_element*)ptr.object)->asStation();
                        if (station != nullptr)
                        {
                            roadStationInteract(window, station, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::airport:
                    {
                        auto station = ((map::tile_element*)ptr.object)->asStation();
                        if (station != nullptr)
                        {
                            airportInteract(window, station, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::dock:
                    {
                        auto station = ((map::tile_element*)ptr.object)->asStation();
                        if (station != nullptr)
                        {
                            dockInteract(window, station, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::tree:
                    {
                        auto tree = ((map::tile_element*)ptr.object)->asTree();
                        if (tree != nullptr)
                        {
                            treeInteract(tree, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::building:
                    {
                        auto building = ((map::tile_element*)ptr.object)->asBuilding();
                        if (building != nullptr)
                        {
                            buildingInteract(building, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::wall:
                    {
                        auto wall = ((map::tile_element*)ptr.object)->asWall();
                        if (wall != nullptr)
                        {
                            wallInteract(wall, { ptr.x, ptr.y });
                        }
                        break;
                    }
                    case InteractionItem::headquarterBuilding:
                    {
                        auto building = ((map::tile_element*)ptr.object)->asBuilding();
                        if (building != nullptr)
                        {
                            headquarterInteract(building, { ptr.x, ptr.y });
                        }
                        break;
                    }
                }

                break;
            }

            default:
                break;
        }
    }

    // 0x004C71F6
    static void stateScrollLeft(const mouse_button button, const widget_index widgetIndex, ui::window* const window, ui::widget_t* const widget, const int16_t x, const int16_t y)
    {
        switch (button)
        {
            case mouse_button::released:
            {
                if (widgetIndex != _pressedWidgetIndex || window->type != _pressedWindowType || window->number != _pressedWindowNumber)
                {
                    scrollview::clearPressedButtons(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                    return;
                }
                scrollview::scrollLeftContinue(x, y, window, widget, widgetIndex);

                break;
            }

            case mouse_button::left_released:
            {
                input::state(input_state::reset);
                scrollview::clearPressedButtons(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                break;
            }

            default:
                break;
        }
    }

    // 0x004C76A7
    // regs.cx = (uint16_t)button;
    // regs.ax = x;
    // regs.bx = y;
    static void stateScrollRight(const mouse_button button, const int16_t x, const int16_t y)
    {
        auto window = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (window == nullptr)
        {
            input::state(input_state::reset);
            return;
        }

        switch (button)
        {
            case mouse_button::released:
            {
                _ticksSinceDragStart += time_since_last_tick;
                if (x != 0 || y != 0)
                {
                    _ticksSinceDragStart = 1000;
                    ui::widget_t* widget = &window->widgets[_dragWidgetIndex];
                    ui::scrollview::horizontalDragFollow(window, widget, _dragWidgetIndex, _dragScrollIndex, x);
                    ui::scrollview::verticalDragFollow(window, widget, _dragWidgetIndex, _dragScrollIndex, y);
                }

                break;
            }

            case mouse_button::right_released:
            {
                input::state(input_state::reset);
                // in the original assembly code we load into registers values from _dragLastX, _dragLastY
                // if _ticksSinceDragStart <= 500, however the result was unused
                break;
            }

            default:
                break;
        }
    }

    mouse_button getLastKnownButtonState()
    {
        return _lastKnownButtonState;
    }

    // 0x004C7722
    static void stateResizing(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
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
        w->callOnResize();
        w->callPrepareDraw();
        w->scroll_areas[0].contentWidth = -1;
        w->scroll_areas[0].contentHeight = -1;
        w->scroll_areas[1].contentWidth = -1;
        w->scroll_areas[1].contentHeight = -1;
        window->updateScrollWidgets();
        w->invalidate();

        _dragLastX = x;
        _dragLastY = y;
    }

    // 0x004C7903
    static void statePositioningWindow(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
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
                windowPositionEnd();

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
                        if (dragWindow->isEnabled(_pressedWidgetIndex))
                        {
                            auto pressedWidget = &dragWindow->widgets[_pressedWidgetIndex];

                            audio::playSound(audio::sound_id::click_press, dragWindow->x + pressedWidget->mid_x());
                            dragWindow->callOnMouseUp(_pressedWidgetIndex);
                        }
                    }
                }

                w->callOnMove(_dragLastX, _dragLastY);
            }
            break;

            default:
                break;
        }
    }

    static void dropdownRegisterSelection(int16_t item)
    {
        auto window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);
        if (window == nullptr)
            return;

        WindowManager::close(ui::WindowType::dropdown, 0);
        window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);

        bool flagSet = hasFlag(input_flags::widget_pressed);
        resetFlag(input_flags::widget_pressed);
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
            window->callOnDropdown(_pressedWidgetIndex, item);
        }
    }

    static int dropdownIndexFromPoint(ui::window* window, int x, int y)
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
    static void stateWidgetPressed(mouse_button button, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
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
                        input::resetFlag(input_flags::widget_pressed);
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
                    if (!window->isDisabled(widgetIndex))
                    {
                        if (_clickRepeatTicks != 0)
                        {
                            _clickRepeatTicks++;
                        }

                        // Handle click repeat
                        if (window->isHoldable(widgetIndex) && _clickRepeatTicks >= 16 && (_clickRepeatTicks % 4) == 0)
                        {
                            window->callOnMouseDown(widgetIndex);
                        }

                        bool flagSet = input::hasFlag(input_flags::widget_pressed);
                        input::setFlag(input_flags::widget_pressed);
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
                        audio::playSound(audio::sound_id::click_up, window->x + buttonWidget->mid_x());
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
                        auto item = dropdownIndexFromPoint(window, x, y);
                        if (item != DROPDOWN_ITEM_UNDEFINED)
                        {
                            dropdownRegisterSelection(item);
                        }
                    }
                    else
                    {
                        if (window->type == *_pressedWindowType && window->number == _pressedWindowNumber && widgetIndex == _pressedWidgetIndex)
                        {
                            if (hasFlag(input_flags::flag1))
                            {
                                bool flagSet = hasFlag(input_flags::flag2);
                                setFlag(input_flags::flag2);
                                if (!flagSet)
                                {
                                    return;
                                }
                            }

                            dropdownRegisterSelection(DROPDOWN_ITEM_UNDEFINED);
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
                audio::playSound(audio::sound_id::click_up, window->x + widget->mid_x());
            }

            if (window != nullptr && window->type == *_pressedWindowType && window->number == _pressedWindowNumber && widgetIndex == _pressedWidgetIndex && !window->isDisabled(widgetIndex))
            {
                WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                window->callOnMouseUp(widgetIndex);
                return;
            }
        }

        // 0x4C7F02
        _clickRepeatTicks = 0;
        if (input::state() != input_state::dropdown_active)
        {
            bool flagSet = hasFlag(input_flags::widget_pressed);
            resetFlag(input_flags::widget_pressed);
            if (flagSet)
            {
                WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
            }
        }

        if (input::state() == input_state::dropdown_active)
        {
            if (window != nullptr && window->type == ui::WindowType::dropdown)
            {
                auto item = dropdownIndexFromPoint(window, x, y);
                if (item != DROPDOWN_ITEM_UNDEFINED)
                {
                    _dropdownHighlightedIndex = item;
                    WindowManager::invalidate(ui::WindowType::dropdown, 0);
                }
            }
        }
    }

    // 0x004C8048
    static void stateNormal(mouse_button state, int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
    {
        switch (state)
        {
            case mouse_button::left_pressed:
                stateNormalLeft(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::right_pressed:
                stateNormalRight(x, y, window, widget, widgetIndex);
                break;
            case mouse_button::released:
                stateNormalHover(x, y, window, widget, widgetIndex);
                break;

            default:
                break;
        }
    }

    // 0x004C8098
    static void stateNormalHover(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
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
            widgetOverFlatbuttonInvalidate();
            _hoverWindowType = windowType;
            _hoverWindowNumber = windowNumber;
            _hoverWidgetIdx = widgetIndex;
            widgetOverFlatbuttonInvalidate();
        }

        if (window != nullptr && widgetIndex != -1)
        {
            if (!window->isDisabled(widgetIndex))
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
                size_t scrollIndex;
                ui::scrollview::getPart(window, widget, x, y, &scrollX, &scrollY, &scrollArea, &scrollIndex);

                if (scrollArea == ui::scrollview::scroll_part::none)
                {
                }
                else if (scrollArea == ui::scrollview::scroll_part::view)
                {
                    window->callScrollMouseOver(scrollX, scrollY, static_cast<uint8_t>(scrollIndex));
                }
                else
                {
                    tooltipStringId = scroll_widget_tooltips[scrollArea];
                    if (*_tooltipWindowType != ui::WindowType::undefined)
                    {
                        if (tooltipStringId != _currentTooltipStringId)
                        {
                            ui::tooltip::closeAndReset();
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
                ui::tooltip::closeAndReset();
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
    static void stateNormalLeft(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
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
                windowPositionBegin(x, y, window, widgetIndex);
                break;

            case ui::widget_type::panel:
            case ui::widget_type::frame:
                if (window->canResize() && (x >= (window->x + window->width - 19)) && (y >= (window->y + window->height - 19)))
                {
                    windowResizeBegin(x, y, window, widgetIndex);
                }
                else
                {
                    windowPositionBegin(x, y, window, widgetIndex);
                }
                break;

            case ui::widget_type::viewport:
                state(input_state::viewport_left);
                _dragLastX = x;
                _dragLastY = y;
                _dragWindowType = window->type;
                _dragWindowNumber = window->number;
                if (hasFlag(input_flags::tool_active))
                {
                    auto w = WindowManager::find(_toolWindowType, _toolWindowNumber);
                    if (w != nullptr)
                    {
                        w->callToolDown(_toolWidgetIndex, x, y);
                        setFlag(input_flags::flag4);
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
                ui::scrollview::scrollLeftBegin(x, y, window, widget, widgetIndex);
                break;

            default:
                if (window->isEnabled(widgetIndex) && !window->isDisabled(widgetIndex))
                {
                    audio::playSound(audio::sound_id::click_down, window->x + widget->mid_x());

                    // Set new cursor down widget
                    _pressedWidgetIndex = widgetIndex;
                    _pressedWindowType = window->type;
                    _pressedWindowNumber = window->number;
                    setFlag(input_flags::widget_pressed);
                    state(input_state::widget_pressed);
                    _clickRepeatTicks = 1;

                    WindowManager::invalidateWidget(window->type, window->number, widgetIndex);
                    window->callOnMouseDown(widgetIndex);
                }

                break;
        }
    }

    // 0x004C834A
    static void stateNormalRight(int16_t x, int16_t y, ui::window* window, ui::widget_t* widget, ui::widget_index widgetIndex)
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
                ui::scrollview::scrollModalRight(x, y, window, widget, widgetIndex);
            }

            return;
        }

        if (isTitleMode())
        {
            return;
        }

        switch (widget->type)
        {
            default:
                break;

            case ui::widget_type::viewport:
                viewportDragBegin(window);

                _dragLastX = x;
                _dragLastY = y;

                ui::hideCursor();
                sub_407218();

                _5233AE = 0;
                _5233B2 = 0;
                setFlag(input_flags::flag5);
                break;

            case ui::widget_type::scrollview:
                scrollDragBegin(x, y, window, widgetIndex);

                _5233AE = 0;
                _5233B2 = 0;
                setFlag(input_flags::flag5);
                break;
        }
    }

#pragma mark - Window positioning

    // 0x004C877D
    static void windowPositionBegin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index)
    {
        state(input_state::positioning_window);
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _5233A9 = false;
    }

    static void windowPositionEnd()
    {
        state(input_state::normal);
        _tooltipTimeout = 0;
        _tooltipWidgetIndex = _pressedWidgetIndex;
        _tooltipWindowType = _dragWindowType;
        _tooltipWindowNumber = _dragWindowNumber;
    }

#pragma mark - Window resizing

    // 0x004C85D1
    static void windowResizeBegin(int16_t x, int16_t y, ui::window* window, ui::widget_index widget_index)
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

    static void viewportDragBegin(window* w)
    {
        w->flags &= ~ui::window_flags::scrolling_to_location;
        state(input_state::viewport_right);
        _dragWindowType = w->type;
        _dragWindowNumber = w->number;
        _ticksSinceDragStart = 0;
    }

#pragma mark - Scrollview dragging

    static void scrollDragBegin(int16_t x, int16_t y, ui::window* window, ui::widget_index widgetIndex)
    {
        state(input_state::scroll_right);
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _dragWidgetIndex = widgetIndex;
        _ticksSinceDragStart = 0;

        _dragScrollIndex = window->getScrollDataIndex(widgetIndex);

        ui::hideCursor();
        sub_407218();
    }

#pragma mark - Widgets

    static void widgetOverFlatbuttonInvalidate()
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
            oldWindow->callPrepareDraw();

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
    void processMouseOver(int16_t x, int16_t y)
    {
        bool skipItem = false;
        ui::cursor_id cursorId = ui::cursor_id::pointer;

        _mapTooltipFormatArguments = string_ids::null;
        _mapTooltipOwner = company_id::null;

        if (_mapSelectionFlags & (1 << 6))
        {
            _mapSelectionFlags &= (uint16_t) ~(1 << 6);
            auto station = stationmgr::get(_hoveredStationId);
            if (!station->empty())
            {
                station->invalidate();
            }
        }

        ui::window* window = ui::WindowManager::findAt(x, y);

        if (window != nullptr)
        {
            int16_t widgetIdx = window->findWidgetAt(x, y);

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
                        cursorId = window->callCursor(widgetIdx, x, y, cursorId);
                        break;

                    case ui::widget_type::scrollview:
                        _5233A4 = x;
                        _5233A6 = y;
                        ui::scrollview::scroll_part output_scroll_area;
                        size_t scroll_id;
                        int16_t scroll_x, scroll_y;
                        ui::scrollview::getPart(
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

                            cursorId = window->callCursor(widgetIdx, scroll_x, scroll_y, cursorId);
                        }

                        break;

                    case ui::widget_type::viewport:
                        if (input::hasFlag(input_flags::tool_active))
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
                            switch (viewport_interaction::getItemLeft(x, y, nullptr))
                            {
                                case InteractionItem::thing:
                                case InteractionItem::town:
                                case InteractionItem::station:
                                case InteractionItem::industry:
                                case InteractionItem::headquarterBuilding:
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
            viewport_interaction::rightOver(x, y, nullptr);
        }

        if (input::state() == input::input_state::resizing)
        {
            cursorId = ui::cursor_id::diagonal_arrows;
        }

        if (cursorId != (ui::cursor_id)*_52336C)
        {
            _52336C = (int8_t)cursorId;
            ui::setCursor(cursorId);
        }
    }

    gfx::point_t getMouseLocation()
    {
        return gfx::point_t(_cursorX, _cursorY);
    }

    gfx::point_t getTooltipMouseLocation()
    {
        return gfx::point_t(_tooltipCursorX, _tooltipCursorY);
    }

    void setTooltipMouseLocation(const gfx::point_t& loc)
    {
        _tooltipCursorX = loc.x;
        _tooltipCursorY = loc.y;
    }

    uint16_t getTooltipTimeout()
    {
        return _tooltipTimeout;
    }
    void setTooltipTimeout(uint16_t tooltipTimeout)
    {
        _tooltipTimeout = tooltipTimeout;
    }
}
