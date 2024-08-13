#include "Audio/Audio.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/BuildingElement.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "Tutorial.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <map>
#include <queue>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Ui::ScrollView;
using namespace OpenLoco::Ui::ViewportInteraction;
using namespace OpenLoco::World;

namespace OpenLoco::Input
{
    static void stateScrollLeft(MouseButton cx, WidgetIndex_t edx, Ui::Window* window, Ui::Widget* widget, int16_t x, int16_t y);
    static void stateScrollRight(const MouseButton button, const int16_t x, const int16_t y);
    static void stateResizing(MouseButton button, int16_t x, int16_t y);
    static void stateWidgetPressed(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormal(MouseButton state, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormalHover(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormalLeft(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormalRight(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void statePositioningWindow(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void windowPositionEnd();

    static void windowResizeBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widgetIndex);

    static void viewportDragBegin(Window* w);

    static void scrollDragBegin(int16_t x, int16_t y, Window* pWindow, WidgetIndex_t index);

    static void widgetOverFlatbuttonInvalidate();

#pragma mark - Input

    static loco_global<MouseButton, 0x001136FA0> _lastKnownButtonState;

    static loco_global<StringId, 0x0050A018> _mapTooltipFormatArguments;

    static loco_global<uint16_t, 0x0050C19C> _timeSinceLastTick;

    static loco_global<uint16_t, 0x0052334A> _52334A;
    static loco_global<uint16_t, 0x0052334C> _52334C;

    static loco_global<int8_t, 0x0052336C> _52336C;

    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<int32_t, 0x0113E730> _cursorY;

    // TODO: name?
    static loco_global<int32_t, 0x00523338> _cursorX2;
    static loco_global<int32_t, 0x0052333C> _cursorY2;

    static loco_global<Ui::WindowType, 0x0052336F> _pressedWindowType;
    static loco_global<Ui::WindowNumber_t, 0x00523370> _pressedWindowNumber;
    static loco_global<int32_t, 0x00523372> _pressedWidgetIndex;
    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;
    static loco_global<uint16_t, 0x00523378> _dragLastX;
    static loco_global<uint16_t, 0x0052337A> _dragLastY;
    static loco_global<Ui::WindowNumber_t, 0x0052337C> _dragWindowNumber;
    static loco_global<Ui::WindowType, 0x0052337E> _dragWindowType;
    static loco_global<uint8_t, 0x0052337F> _dragWidgetIndex;
    static loco_global<uint8_t, 0x00523380> _dragScrollIndex;
    static loco_global<Ui::WindowType, 0x00523381> _tooltipWindowType;
    static loco_global<int16_t, 0x00523382> _tooltipWindowNumber;
    static loco_global<int16_t, 0x00523384> _tooltipWidgetIndex;
    static loco_global<uint16_t, 0x00523386> _tooltipCursorX;
    static loco_global<uint16_t, 0x00523388> _tooltipCursorY;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x0052338E> _ticksSinceDragStart;

    static loco_global<int16_t, 0x005233A4> _5233A4;
    static loco_global<int16_t, 0x005233A6> _5233A6;
    static loco_global<Ui::WindowType, 0x005233A8> _hoverWindowType;
    static uint8_t _5233A9;
    static loco_global<Ui::WindowNumber_t, 0x005233AA> _hoverWindowNumber;
    static loco_global<uint16_t, 0x005233AC> _hoverWidgetIdx;
    static loco_global<uint32_t, 0x005233AE> _5233AE;
    static loco_global<uint32_t, 0x005233B2> _5233B2;

    static Ui::WindowType _focusedWindowType;
    static Ui::WindowNumber_t _focusedWindowNumber;
    static Ui::WidgetIndex_t _focusedWidgetIndex;

    static loco_global<uint32_t, 0x005251C8> _rightMouseButtonStatus;

    static loco_global<StationId, 0x00F252A4> _hoveredStationId;

    static loco_global<int32_t, 0x01136F98> _currentTooltipStringId;

    static loco_global<uint16_t, 0x0113D84C> _dropdownItemCount;
    static loco_global<uint16_t, 0x0113D84E> _dropdownHighlightedIndex;
    static loco_global<StringId[40], 0x0113D850> _dropdownItemFormats;

    static loco_global<uint32_t, 0x0113DC60> _dropdownDisabledItems;

    static loco_global<uint32_t, 0x0113DC68> _dropdownItemHeight;
    static loco_global<uint32_t, 0x0113DC6C> _dropdownItemWidth;
    static loco_global<uint32_t, 0x0113DC70> _dropdownColumnCount;
    static loco_global<uint32_t, 0x0113DC74> _dropdownRowCount;
    static loco_global<uint16_t, 0x0113DC78> _113DC78;

    static loco_global<int32_t, 0x00525330> _cursorWheel;

    static const std::map<Ui::ScrollPart, StringId> kScrollWidgetTooltips = {
        { Ui::ScrollPart::hscrollbarButtonLeft, StringIds::tooltip_scroll_left },
        { Ui::ScrollPart::hscrollbarButtonRight, StringIds::tooltip_scroll_right },
        { Ui::ScrollPart::hscrollbarTrackLeft, StringIds::tooltip_scroll_left_fast },
        { Ui::ScrollPart::hscrollbarTrackRight, StringIds::tooltip_scroll_right_fast },
        { Ui::ScrollPart::hscrollbarThumb, StringIds::tooltip_scroll_left_right },
        { Ui::ScrollPart::vscrollbarButtonTop, StringIds::tooltip_scroll_up },
        { Ui::ScrollPart::vscrollbarButtonBottom, StringIds::tooltip_scroll_down },
        { Ui::ScrollPart::vscrollbarTrackTop, StringIds::tooltip_scroll_up_fast },
        { Ui::ScrollPart::vscrollbarTrackBottom, StringIds::tooltip_scroll_down_fast },
        { Ui::ScrollPart::vscrollbarThumb, StringIds::tooltip_scroll_up_down },
    };

    constexpr int32_t kDropdownItemUndefined = -1;

    void initMouse()
    {
        _pressedWindowType = Ui::WindowType::undefined;

        _tooltipNotShownTicks = 0xFFFFU;
        _hoverWindowType = Ui::WindowType::undefined;
        _focusedWindowType = Ui::WindowType::undefined;

        _5233AE = 0;
        _5233B2 = 0;

        World::resetMapSelectionFlags();
    }

    void moveMouse(int32_t x, int32_t y, int32_t relX, int32_t relY)
    {
        _cursorX = x;
        _cursorY = y;
        addr<0x0114084C, int32_t>() = relX;
        addr<0x01140840, int32_t>() = relY;
    }

    void mouseWheel(int wheel)
    {
        _cursorWheel += wheel;
    }

    bool isHovering(Ui::WindowType type)
    {
        return *_hoverWindowType == type;
    }

    bool isHovering(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        return (*_hoverWindowType == type) && (_hoverWindowNumber == number);
    }

    bool isHovering(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t widgetIndex)
    {
        return *_hoverWindowType == type && _hoverWindowNumber == number && _hoverWidgetIdx == widgetIndex;
    }

    Ui::WidgetIndex_t getHoveredWidgetIndex()
    {
        return _hoverWidgetIdx;
    }

    bool isDropdownActive(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (state() != State::dropdownActive)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (!hasFlag(Flags::widgetPressed))
            return false;

        if (_pressedWindowNumber != number)
            return false;

        return true;
    }

    bool isDropdownActive(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t index)
    {
        if (!isDropdownActive(type, number))
            return false;

        return _pressedWidgetIndex == index;
    }

    bool isPressed(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (state() != State::widgetPressed)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (_pressedWindowNumber != number)
            return false;

        if (!hasFlag(Flags::widgetPressed))
            return false;

        return true;
    }

    bool isPressed(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t index)
    {
        return isPressed(type, number) && _pressedWidgetIndex == index;
    }

    Ui::WidgetIndex_t getPressedWidgetIndex()
    {
        return _pressedWidgetIndex;
    }

    void setPressedWidgetIndex(Ui::WidgetIndex_t index)
    {
        _pressedWidgetIndex = index;
    }

    bool isFocused(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!hasFlag(Flags::widgetFocused))
            return false;

        if (_focusedWindowType != type)
            return false;

        if (_focusedWindowNumber != number)
            return false;

        return true;
    }

    bool isFocused(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t index)
    {
        return isFocused(type, number) && _focusedWidgetIndex == index;
    }

    void setFocus(Ui::WindowType type, Ui::WindowNumber_t number, Ui::WidgetIndex_t index)
    {
        _focusedWindowType = type;
        _focusedWindowNumber = number;
        _focusedWidgetIndex = index;

        setFlag(Flags::widgetFocused);
    }

    void resetFocus()
    {
        _focusedWindowType = WindowType::undefined;
        resetFlag(Flags::widgetFocused);
    }

    // 0x004C6E65
    void updateCursorPosition()
    {
        switch (Tutorial::state())
        {
            case Tutorial::State::none:
            {
                _cursorX2 = _cursorX;
                _cursorY2 = _cursorY;
                break;
            }

            case Tutorial::State::playing:
            {
                _cursorX2 = Tutorial::nextInput();
                _cursorY2 = Tutorial::nextInput();
                Ui::setCursorPosScaled(*_cursorX2, *_cursorY2);
                break;
            }

            case Tutorial::State::recording:
            {
                call(0x004C6EC3);
                break;
            }
        }
    }

    StationId getHoveredStationId()
    {
        return _hoveredStationId;
    }

#pragma mark - Mouse input
    static void stateViewportLeft(const MouseButton cx, const int16_t x, const int16_t y);
    static void stateViewportRight(const MouseButton cx, const int16_t x, const int16_t y);

    // 0x004C7174
    void handleMouse(int16_t x, int16_t y, MouseButton button)
    {
        _lastKnownButtonState = button;

        Ui::Window* window = WindowManager::findAt(x, y);

        Ui::WidgetIndex_t widgetIndex = -1;
        if (window != nullptr)
        {
            widgetIndex = window->findWidgetAt(x, y);
        }

        auto modalType = WindowManager::getCurrentModalType();
        if (modalType != WindowType::undefined)
        {
            if (window != nullptr)
            {
                if (window->type != modalType)
                {
                    if (button == MouseButton::leftPressed)
                    {
                        WindowManager::bringToFront(modalType);
                        Audio::playSound(Audio::SoundId::error, x);
                        return;
                    }

                    if (button == MouseButton::rightPressed)
                    {
                        return;
                    }
                }
            }
        }

        Ui::Widget* widget = nullptr;
        if (widgetIndex != -1)
        {
            widget = &window->widgets[widgetIndex];
        }

        switch (state())
        {
            case State::reset:
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                _tooltipTimeout = 0;
                _tooltipWindowType = Ui::WindowType::undefined;
                state(State::normal);
                resetFlag(Flags::leftMousePressed);
                stateNormal(button, x, y, window, widget, widgetIndex);
                break;

            case State::normal:
                stateNormal(button, x, y, window, widget, widgetIndex);
                break;

            case State::widgetPressed:
            case State::dropdownActive:
                stateWidgetPressed(button, x, y, window, widget, widgetIndex);
                break;

            case State::positioningWindow:
                statePositioningWindow(button, x, y, window, widget, widgetIndex);
                break;

            case State::viewportRight:
                stateViewportRight(button, x, y);
                break;

            case State::viewportLeft:
                stateViewportLeft(button, x, y);
                break;

            case State::scrollLeft:
                stateScrollLeft(button, widgetIndex, window, widget, x, y);
                break;

            case State::resizing:
                stateResizing(button, x, y);
                break;

            case State::scrollRight:
                stateScrollRight(button, x, y);
                break;
        }
    }

    // 0x004C7334
    static void stateViewportLeft(const MouseButton button, const int16_t x, const int16_t y)
    {
        auto window = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (window == nullptr)
        {
            Input::state(State::reset);
            return;
        }

        switch (button)
        {
            case MouseButton::released:
            {
                // 0x4C735D
                auto viewport = window->viewports[0];
                if (viewport == nullptr)
                {
                    viewport = window->viewports[1];
                }
                if (viewport == nullptr)
                {
                    Input::state(State::reset);
                    return;
                }

                if (window->type == _dragWindowType && window->number == _dragWindowNumber)
                {

                    if (Input::hasFlag(Flags::toolActive))
                    {
                        auto tool = WindowManager::find(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber());
                        if (tool != nullptr)
                        {
                            tool->callToolDragContinue(ToolManager::getToolWidgetIndex(), x, y);
                        }
                    }
                }
                break;
            }

            case MouseButton::leftReleased:
            {
                // 0x4C73C2
                Input::state(State::reset);
                if (window->type != _dragWindowType || window->number != _dragWindowNumber)
                    return;

                if (hasFlag(Flags::toolActive))
                {
                    auto tool = WindowManager::find(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber());
                    if (tool != nullptr)
                    {
                        tool->callToolDragEnd(ToolManager::getToolWidgetIndex());
                    }
                }
                else if (!hasFlag(Flags::leftMousePressed))
                {
                    auto interaction = ViewportInteraction::getItemLeft(x, y);
                    switch (interaction.type)
                    {
                        case InteractionItem::entity:
                        {
                            auto _entity = reinterpret_cast<EntityBase*>(interaction.object);
                            auto veh = _entity->asBase<Vehicles::VehicleBase>();
                            if (veh != nullptr)
                            {
                                Ui::Windows::Vehicle::Main::open(veh);
                            }
                            break;
                        }

                        case InteractionItem::townLabel:
                        {
                            Ui::Windows::Town::open(interaction.value);
                            break;
                        }

                        case InteractionItem::stationLabel:
                        {
                            Ui::Windows::Station::open(StationId(interaction.value));
                            break;
                        }

                        case InteractionItem::headquarterBuilding:
                        {
                            const auto* tileElement = reinterpret_cast<World::TileElement*>(interaction.object);
                            const auto* building = tileElement->as<World::BuildingElement>();
                            if (building != nullptr)
                            {
                                auto index = building->sequenceIndex();
                                const auto firstTile = interaction.pos - World::kOffsets[index];
                                const Pos3 pos = { firstTile.x,
                                                   firstTile.y,
                                                   building->baseZ() };

                                for (auto& company : CompanyManager::companies())
                                {
                                    if (company.headquartersX == pos.x
                                        && company.headquartersY == pos.y
                                        && company.headquartersZ == pos.z)
                                    {
                                        Ui::Windows::CompanyWindow::open(company.id());
                                        break;
                                    }
                                }
                            }
                            break;
                        }

                        case InteractionItem::industry:
                        {
                            Ui::Windows::Industry::open(IndustryId(interaction.value));
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

    // 0x004C74BB
    static void stateViewportRight(const MouseButton button, const int16_t x, const int16_t y)
    {
        auto window = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (window == nullptr)
        {
            Input::state(State::reset);
            return;
        }

        switch (button)
        {

            case MouseButton::released:
            {
                // 4C74E4
                _ticksSinceDragStart += _timeSinceLastTick;
                auto vp = window->viewports[0];
                if (vp == nullptr)
                {
                    vp = window->viewports[1];
                }
                if (vp == nullptr)
                {
                    Input::state(State::reset);
                    return;
                }

                if (window->hasFlags(WindowFlags::viewportNoScrolling))
                {
                    return;
                }

                Ui::Point dragOffset = { x, y };
                if (Tutorial::state() != Tutorial::State::playing)
                {
                    // Fix #151: use relative drag from one frame to the next rather than
                    //           using the relative position from the message loop
                    dragOffset = getNextDragOffset();
                }
                if (dragOffset.x != 0 || dragOffset.y != 0)
                {
                    _ticksSinceDragStart = 1000;

                    if (!window->viewportIsFocusedOnEntity())
                    {
                        auto invert = Config::get().invertRightMouseViewPan ? -1 : 1;
                        auto offsetX = dragOffset.x << (vp->zoom + 1);
                        auto offsetY = dragOffset.y << (vp->zoom + 1);

                        window->viewportConfigurations[0].savedViewX += offsetX * invert;
                        window->viewportConfigurations[0].savedViewY += offsetY * invert;
                    }
                    else
                    {
                        window->viewportUnfocusFromEntity();
                    }
                }

                break;
            }

            case MouseButton::rightReleased:
            {
                if (_ticksSinceDragStart > 500)
                {
                    Input::state(State::reset);
                    return;
                }

                Input::state(State::reset);
                ViewportInteraction::handleRightReleased(window, _dragLastX, _dragLastY);
                break;
            }

            default:
                break;
        }
    }

    // 0x004C71F6
    static void stateScrollLeft(const MouseButton button, const WidgetIndex_t widgetIndex, Ui::Window* const window, Ui::Widget* const widget, const int16_t x, const int16_t y)
    {
        switch (button)
        {
            case MouseButton::released:
            {
                if (widgetIndex != _pressedWidgetIndex || window->type != _pressedWindowType || window->number != _pressedWindowNumber)
                {
                    ScrollView::clearPressedButtons(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                    return;
                }
                ScrollView::scrollLeftContinue(x, y, window, widget, widgetIndex);

                break;
            }

            case MouseButton::leftReleased:
            {
                Input::state(State::reset);
                ScrollView::clearPressedButtons(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
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
    static void stateScrollRight(const MouseButton button, const int16_t x, const int16_t y)
    {
        auto window = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (window == nullptr)
        {
            Input::state(State::reset);
            return;
        }

        switch (button)
        {
            case MouseButton::released:
            {
                _ticksSinceDragStart += _timeSinceLastTick;
                if (x != 0 || y != 0)
                {
                    _ticksSinceDragStart = 1000;
                    Ui::Widget* widget = &window->widgets[_dragWidgetIndex];
                    Ui::ScrollView::horizontalDragFollow(window, widget, _dragWidgetIndex, _dragScrollIndex, x);
                    Ui::ScrollView::verticalDragFollow(window, widget, _dragWidgetIndex, _dragScrollIndex, y);
                }

                break;
            }

            case MouseButton::rightReleased:
            {
                Input::state(State::reset);
                // in the original assembly code we load into registers values from _dragLastX, _dragLastY
                // if _ticksSinceDragStart <= 500, however the result was unused
                break;
            }

            default:
                break;
        }
    }

    MouseButton getLastKnownButtonState()
    {
        return _lastKnownButtonState;
    }

    // 0x004C7722
    static void stateResizing(MouseButton button, int16_t x, int16_t y)
    {
        auto w = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (w == nullptr)
        {
            state(State::reset);
            return;
        }

        bool doDefault = false;
        int dx = 0, dy = 0;
        switch (button)
        {
            case MouseButton::released:
                doDefault = true;
                break;

            case MouseButton::leftReleased:
                state(State::normal);
                _tooltipTimeout = 0;
                _tooltipWidgetIndex = _pressedWidgetIndex;
                _tooltipWindowType = _dragWindowType;
                _tooltipWindowNumber = _dragWindowNumber;

                if (w->hasFlags(Ui::WindowFlags::flag_15))
                {
                    doDefault = true;
                    break;
                }

                if (w->hasFlags(Ui::WindowFlags::flag_16))
                {
                    x = w->var_88A - w->width + _dragLastX;
                    y = w->var_88C - w->height + _dragLastY;
                    w->flags &= ~Ui::WindowFlags::flag_16;
                    doDefault = true;
                    break;
                }

                w->var_88A = w->width;
                w->var_88C = w->height;
                x = _dragLastX - w->x - w->width + Ui::width();
                y = _dragLastY - w->y - w->height + Ui::height() - 27;
                w->flags |= Ui::WindowFlags::flag_16;
                if (y >= Ui::height() - 2)
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
            if (y >= Ui::height() - 2)
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

            w->flags &= ~Ui::WindowFlags::flag_16;
        }

        w->invalidate();

        w->width = std::clamp<uint16_t>(w->width + dx, w->minWidth, w->maxWidth);
        w->height = std::clamp<uint16_t>(w->height + dy, w->minHeight, w->maxHeight);
        w->flags |= Ui::WindowFlags::flag_15;
        w->callOnResize();
        w->callPrepareDraw();

        w->scrollAreas[0].contentWidth = -1;
        w->scrollAreas[0].contentHeight = -1;
        w->scrollAreas[1].contentWidth = -1;
        w->scrollAreas[1].contentHeight = -1;
        w->updateScrollWidgets();
        w->invalidate();

        _dragLastX = x;
        _dragLastY = y;
    }

    // 0x004C7903
    static void statePositioningWindow(MouseButton button, int16_t x, int16_t y, [[maybe_unused]] Ui::Window* window, [[maybe_unused]] Ui::Widget* widget, [[maybe_unused]] Ui::WidgetIndex_t widgetIndex)
    {
        auto w = WindowManager::find(_dragWindowType, _dragWindowNumber);
        if (w == nullptr)
        {
            state(State::reset);
            return;
        }

        switch (button)
        {
            case MouseButton::released:
            {
                y = std::clamp<int16_t>(y, 29, Ui::height() - 29);

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

            case MouseButton::leftReleased:
            {
                windowPositionEnd();

                y = std::clamp<int16_t>(y, 29, Ui::height() - 29);

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

                            Audio::playSound(Audio::SoundId::clickPress, dragWindow->x + pressedWidget->midX());
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

        WindowManager::close(Ui::WindowType::dropdown, 0);
        window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);

        bool flagSet = hasFlag(Flags::widgetPressed);
        resetFlag(Flags::widgetPressed);
        if (flagSet)
        {
            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
        }

        Input::state(State::normal);
        _tooltipTimeout = 0;
        _tooltipWidgetIndex = _pressedWidgetIndex;
        _tooltipWindowType = _pressedWindowType;
        _tooltipWindowNumber = _pressedWindowNumber;

        if (WindowManager::getCurrentModalType() == Ui::WindowType::undefined || WindowManager::getCurrentModalType() == window->type)
        {
            window->callOnDropdown(_pressedWidgetIndex, item);
        }
    }

    static std::optional<int> dropdownIndexFromPoint(Ui::Window* window, int x, int y)
    {
        // Check whether x and y are over a list item
        int left = x - window->x;
        if (left < 0)
        {
            return std::nullopt;
        }
        if (left >= window->width)
        {
            return std::nullopt;
        }

        // 2px of padding on the top of the list?
        int top = y - window->y - 2;
        if (top < 0)
        {
            return std::nullopt;
        }

        unsigned int itemY = top / _dropdownItemHeight;
        if (itemY >= _dropdownItemCount)
        {
            return std::nullopt;
        }

        left -= 2;
        if (left < 0)
        {
            return std::nullopt;
        }

        unsigned int itemX = left / _dropdownItemWidth;
        if (itemX >= _dropdownColumnCount)
        {
            return std::nullopt;
        }
        if (itemY >= _dropdownRowCount)
        {
            return std::nullopt;
        }

        int item = itemY * _dropdownColumnCount + itemX;
        if (item >= _dropdownItemCount)
        {
            return std::nullopt;
        }

        if (item < 32 && (_dropdownDisabledItems & (1ULL << item)) != 0)
        {
            return std::nullopt;
        }

        if (_dropdownItemFormats[item] == 0)
        {
            return std::nullopt;
        }

        return item;
    }

    // 0x004C7AE7
    static void stateWidgetPressed(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
    {
        _52334A = x;
        _52334C = y;

        auto pressedWindow = WindowManager::find(_pressedWindowType, _pressedWindowNumber);
        if (pressedWindow == nullptr)
        {
            Input::state(State::reset);
            return;
        }

        if (Input::state() == State::dropdownActive)
        {
            if (_113DC78 & (1 << 0))
            {
                if (widgetIndex == -1 || *_pressedWindowType != window->type || _pressedWindowNumber != window->number || _pressedWidgetIndex != widgetIndex)
                {
                    if (widgetIndex == -1 || window->type != Ui::WindowType::dropdown)
                    {
                        WindowManager::close(Ui::WindowType::dropdown, 0);

                        if (*_pressedWindowType != Ui::WindowType::undefined)
                        {
                            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
                        }

                        _pressedWindowType = Ui::WindowType::undefined;
                        Input::resetFlag(Flags::widgetPressed);
                        Input::state(State::reset);
                        return;
                    }
                }
            }
        }

        bool doShared = false;
        switch (button)
        {
            case MouseButton::released: // 0
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

                        bool flagSet = Input::hasFlag(Flags::widgetPressed);
                        Input::setFlag(Flags::widgetPressed);
                        if (!flagSet)
                        {
                            WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, widgetIndex);
                        }

                        return;
                    }
                }

                break;
            }

            case MouseButton::leftPressed: // 1
                if (Input::state() == State::dropdownActive)
                {
                    if (window != nullptr && widgetIndex != -1)
                    {
                        auto buttonWidget = &window->widgets[widgetIndex];
                        Audio::playSound(Audio::SoundId::clickUp, window->x + buttonWidget->midX());
                    }
                }
                return;

            case MouseButton::leftReleased: // 2
                doShared = true;
                break;

            case MouseButton::rightPressed: // 3
                if (Input::state() == State::dropdownActive)
                {
                    doShared = true;
                }
                else
                {
                    return;
                }

                break;

            case MouseButton::rightReleased:
                return;
        }

        if (doShared)
        {
            // 0x4C7BC7
            if (Input::state() == State::dropdownActive)
            {
                if (window != nullptr)
                {
                    if (window->type == Ui::WindowType::dropdown)
                    {
                        auto item = dropdownIndexFromPoint(window, x, y);
                        if (item.has_value())
                        {
                            dropdownRegisterSelection(*item);
                        }
                    }
                    else
                    {
                        if (window->type == *_pressedWindowType && window->number == _pressedWindowNumber && widgetIndex == _pressedWidgetIndex)
                        {
                            if (hasFlag(Flags::flag1))
                            {
                                bool flagSet = hasFlag(Flags::flag2);
                                setFlag(Flags::flag2);
                                if (!flagSet)
                                {
                                    return;
                                }
                            }

                            dropdownRegisterSelection(kDropdownItemUndefined);
                        }
                    }
                }

                // 0x4C7DA0
                WindowManager::close(Ui::WindowType::dropdown, 0);
                window = WindowManager::find(_pressedWindowType, _pressedWindowNumber);
            }

            Input::state(State::normal);
            _tooltipTimeout = 0;
            _tooltipWidgetIndex = _pressedWidgetIndex;
            _tooltipWindowType = _pressedWindowType;
            _tooltipWindowNumber = _pressedWindowNumber;
            if (window != nullptr)
            {
                Audio::playSound(Audio::SoundId::clickUp, window->x + widget->midX());
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
        if (Input::state() != State::dropdownActive)
        {
            bool flagSet = hasFlag(Flags::widgetPressed);
            resetFlag(Flags::widgetPressed);
            if (flagSet)
            {
                WindowManager::invalidateWidget(_pressedWindowType, _pressedWindowNumber, _pressedWidgetIndex);
            }
        }

        if (Input::state() == State::dropdownActive)
        {
            if (window != nullptr && window->type == Ui::WindowType::dropdown)
            {
                auto item = dropdownIndexFromPoint(window, x, y);
                if (item.has_value())
                {
                    _dropdownHighlightedIndex = *item;
                    WindowManager::invalidate(Ui::WindowType::dropdown, 0);
                }
            }
        }
    }

    // 0x004C8048
    static void stateNormal(MouseButton state, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
    {
        switch (state)
        {
            case MouseButton::leftPressed:
                stateNormalLeft(x, y, window, widget, widgetIndex);
                break;
            case MouseButton::rightPressed:
                stateNormalRight(x, y, window, widget, widgetIndex);
                break;
            case MouseButton::released:
                stateNormalHover(x, y, window, widget, widgetIndex);
                break;

            default:
                break;
        }
    }

    // 0x004C8098
    static void stateNormalHover(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
    {
        Ui::WindowType windowType = Ui::WindowType::undefined;
        Ui::WindowNumber_t windowNumber = 0;

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

        StringId tooltipStringId = StringIds::null;
        if (window != nullptr && widgetIndex != -1)
        {
            if (widget->type == Ui::WidgetType::scrollview)
            {
                auto res = Ui::ScrollView::getPart(window, widget, x, y);

                if (res.area == Ui::ScrollPart::none)
                {
                }
                else if (res.area == Ui::ScrollPart::view)
                {
                    window->callScrollMouseOver(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
                }
                else
                {
                    tooltipStringId = kScrollWidgetTooltips.at(res.area);
                    if (*_tooltipWindowType != Ui::WindowType::undefined)
                    {
                        if (tooltipStringId != _currentTooltipStringId)
                        {
                            Ui::Windows::ToolTip::closeAndReset();
                        }
                    }
                }
            }
        }

        if (*_tooltipWindowType != Ui::WindowType::undefined)
        {
            if (window != nullptr && *_tooltipWindowType == window->type && _tooltipWindowNumber == window->number && _tooltipWidgetIndex == widgetIndex)
            {
                _tooltipTimeout += _timeSinceLastTick;
                if (_tooltipTimeout >= 8000)
                {
                    WindowManager::close(Ui::WindowType::tooltip);
                }
            }
            else
            {
                Ui::Windows::ToolTip::closeAndReset();
            }

            return;
        }

        if (_tooltipNotShownTicks < 500 || (x == _tooltipCursorX && y == _tooltipCursorY))
        {
            _tooltipTimeout += _timeSinceLastTick;
            int bp = 2000;
            if (_tooltipNotShownTicks <= 1000)
            {
                bp = 0;
            }

            if (bp > _tooltipTimeout)
            {
                return;
            }

            if (tooltipStringId == StringIds::null)
            {
                Ui::Windows::ToolTip::open(window, widgetIndex, x, y);
            }
            else
            {
                Ui::Windows::ToolTip::update(window, widgetIndex, tooltipStringId, x, y);
            }
        }

        _tooltipTimeout = 0;
        _tooltipCursorX = x;
        _tooltipCursorY = y;
    }

    // 0x004C84BE
    static void stateNormalLeft(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
    {
        Ui::WindowType windowType = Ui::WindowType::undefined;
        Ui::WindowNumber_t windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        WindowManager::close(Ui::WindowType::error);
        WindowManager::close(Ui::WindowType::tooltip);

        // Window might have changed position in the list, therefore find it again
        window = WindowManager::find(windowType, windowNumber);
        if (window == nullptr)
        {
            return;
        }

        window = WindowManager::bringToFront(*window);
        if (widgetIndex == -1)
        {
            return;
        }

        // Handle text input focus
        if (widget->type == WidgetType::textbox)
        {
            setFocus(window->type, window->number, widgetIndex);
            WindowManager::invalidateWidget(_focusedWindowType, _focusedWindowNumber, _focusedWidgetIndex);
        }
        else
            resetFocus();

        // Regular widget handling
        switch (widget->type)
        {
            case Ui::WidgetType::caption_22:
            case Ui::WidgetType::caption_23:
            case Ui::WidgetType::caption_24:
            case Ui::WidgetType::caption_25:
                windowPositionBegin(x, y, window, widgetIndex);
                break;

            case Ui::WidgetType::panel:
            case Ui::WidgetType::frame:
                if (window->canResize() && (x >= (window->x + window->width - 19)) && (y >= (window->y + window->height - 19)))
                {
                    windowResizeBegin(x, y, window, widgetIndex);
                }
                else
                {
                    windowPositionBegin(x, y, window, widgetIndex);
                }
                break;

            case Ui::WidgetType::viewport:
                state(State::viewportLeft);
                _dragLastX = x;
                _dragLastY = y;
                _dragWindowType = window->type;
                _dragWindowNumber = window->number;
                if (hasFlag(Flags::toolActive))
                {
                    auto w = WindowManager::find(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber());
                    if (w != nullptr)
                    {
                        w->callToolDown(ToolManager::getToolWidgetIndex(), x, y);
                        setFlag(Flags::leftMousePressed);
                    }
                }
                break;

            case Ui::WidgetType::scrollview:
                state(State::scrollLeft);
                _pressedWidgetIndex = widgetIndex;
                _pressedWindowType = window->type;
                _pressedWindowNumber = window->number;
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                Ui::ScrollView::scrollLeftBegin(x, y, window, widget, widgetIndex);
                break;

            default:
                if (window->isEnabled(widgetIndex) && !window->isDisabled(widgetIndex))
                {
                    Audio::playSound(Audio::SoundId::clickDown, window->x + widget->midX());

                    // Set new cursor down widget
                    _pressedWidgetIndex = widgetIndex;
                    _pressedWindowType = window->type;
                    _pressedWindowNumber = window->number;
                    setFlag(Flags::widgetPressed);
                    state(State::widgetPressed);
                    _clickRepeatTicks = 1;

                    WindowManager::invalidateWidget(window->type, window->number, widgetIndex);
                    window->callOnMouseDown(widgetIndex);
                }

                break;
        }
    }

    // 0x004C834A
    static void stateNormalRight(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
    {
        Ui::WindowType windowType = Ui::WindowType::undefined;
        Ui::WindowNumber_t windowNumber = 0;

        if (window != nullptr)
        {
            windowType = window->type;
            windowNumber = window->number;
        }

        WindowManager::close(Ui::WindowType::tooltip);

        // Window might have changed position in the list, therefore find it again
        window = WindowManager::find(windowType, windowNumber);
        if (window == nullptr)
        {
            return;
        }

        window = WindowManager::bringToFront(*window);

        if (widgetIndex == -1)
        {
            return;
        }

        if (WindowManager::getCurrentModalType() != Ui::WindowType::undefined)
        {
            if (WindowManager::getCurrentModalType() == window->type)
            {
                ScrollView::scrollModalRight(x, y, window, widget, widgetIndex);
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

            case Ui::WidgetType::viewport:
                viewportDragBegin(window);

                _dragLastX = x;
                _dragLastY = y;

                Ui::hideCursor();
                sub_407218();

                _5233AE = 0;
                _5233B2 = 0;
                setFlag(Flags::rightMousePressed);
                break;

            case Ui::WidgetType::scrollview:
                scrollDragBegin(x, y, window, widgetIndex);

                _5233AE = 0;
                _5233B2 = 0;
                setFlag(Flags::rightMousePressed);
                break;
        }
    }

#pragma mark - Window positioning

    // 0x004C877D
    void windowPositionBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widgetIndex)
    {
        state(State::positioningWindow);
        _pressedWidgetIndex = widgetIndex;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _5233A9 = false;
    }

    static void windowPositionEnd()
    {
        state(State::normal);
        _tooltipTimeout = 0;
        _tooltipWidgetIndex = _pressedWidgetIndex;
        _tooltipWindowType = _dragWindowType;
        _tooltipWindowNumber = _dragWindowNumber;
    }

#pragma mark - Window resizing

    // 0x004C85D1
    static void windowResizeBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widgetIndex)
    {
        state(State::resizing);
        _pressedWidgetIndex = widgetIndex;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        window->flags &= ~Ui::WindowFlags::flag_15;
    }

#pragma mark - Viewport dragging

    static void viewportDragBegin(Window* w)
    {
        w->flags &= ~Ui::WindowFlags::scrollingToLocation;
        state(State::viewportRight);
        _dragWindowType = w->type;
        _dragWindowNumber = w->number;
        _ticksSinceDragStart = 0;
    }

#pragma mark - Scrollview dragging

    static void scrollDragBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widgetIndex)
    {
        state(State::scrollRight);
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        _dragWidgetIndex = widgetIndex;
        _ticksSinceDragStart = 0;

        _dragScrollIndex = window->getScrollDataIndex(widgetIndex);

        Ui::hideCursor();
        sub_407218();
    }

#pragma mark - Widgets

    static void widgetOverFlatbuttonInvalidate()
    {
        Ui::WindowType windowType = _hoverWindowType;
        uint16_t widgetIdx = _hoverWidgetIdx;
        uint16_t windowNumber = _hoverWindowNumber;

        if (windowType == Ui::WindowType::undefined)
        {
            WindowManager::invalidateWidget(windowType, windowNumber, widgetIdx);
            return;
        }

        Ui::Window* oldWindow = WindowManager::find(windowType, windowNumber);

        if (oldWindow != nullptr)
        {
            oldWindow->callPrepareDraw();

            Ui::Widget* oldWidget = &oldWindow->widgets[widgetIdx];
            if (oldWidget->type == Ui::WidgetType::buttonWithColour || oldWidget->type == Ui::WidgetType::buttonWithImage)
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
        Ui::CursorId cursorId = Ui::CursorId::pointer;

        Windows::MapToolTip::reset();

        if (hasMapSelectionFlag(MapSelectionFlags::hoveringOverStation))
        {
            resetMapSelectionFlag(MapSelectionFlags::hoveringOverStation);
            auto station = StationManager::get(_hoveredStationId);
            if (!station->empty())
            {
                station->invalidate();
            }
        }

        Ui::Window* window = Ui::WindowManager::findAt(x, y);

        if (window != nullptr)
        {
            int16_t widgetIdx = window->findWidgetAt(x, y);

            if (widgetIdx != -1)
            {
                Ui::Widget& widget = window->widgets[widgetIdx];
                switch (widget.type)
                {
                    case Ui::WidgetType::panel:
                    case Ui::WidgetType::frame:
                        if (window->hasFlags(Ui::WindowFlags::resizable))
                        {
                            if (window->minWidth != window->maxWidth || window->minHeight != window->maxHeight)
                            {
                                if (x >= window->x + window->width - 19 && y >= window->y + window->height - 19)
                                {
                                    cursorId = Ui::CursorId::diagonalArrows;
                                    break;
                                }
                            }
                        }
                        [[fallthrough]];
                    default:
                        _5233A4 = x;
                        _5233A6 = y;
                        cursorId = window->callCursor(widgetIdx, x, y, cursorId);
                        break;

                    case Ui::WidgetType::scrollview:
                    {
                        _5233A4 = x;
                        _5233A6 = y;

                        auto res = Ui::ScrollView::getPart(
                            window,
                            &window->widgets[widgetIdx],
                            x,
                            y);

                        if (res.area == Ui::ScrollPart::view)
                        {
                            cursorId = window->callCursor(widgetIdx, res.scrollviewLoc.x, res.scrollviewLoc.y, cursorId);
                        }
                        break;
                    }
                    case Ui::WidgetType::viewport:
                        if (Input::hasFlag(Flags::toolActive))
                        {
                            // 3
                            cursorId = ToolManager::getToolCursor();
                            auto wnd = Ui::WindowManager::find(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber());
                            if (wnd)
                            {
                                bool out = false;
                                cursorId = wnd->callToolCursor(x, y, cursorId, &out);
                                if (out)
                                {
                                    skipItem = true;
                                }
                            }
                        }
                        else
                        {
                            switch (ViewportInteraction::getItemLeft(x, y).type)
                            {
                                case InteractionItem::entity:
                                case InteractionItem::townLabel:
                                case InteractionItem::stationLabel:
                                case InteractionItem::industry:
                                case InteractionItem::headquarterBuilding:
                                    skipItem = true;
                                    cursorId = Ui::CursorId::handPointer;
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
            ViewportInteraction::rightOver(x, y);
        }

        if (Input::state() == Input::State::resizing)
        {
            cursorId = Ui::CursorId::diagonalArrows;
        }

        if (cursorId != (Ui::CursorId)*_52336C)
        {
            _52336C = (int8_t)cursorId;
            Ui::setCursor(cursorId);
        }
    }

    Ui::Point getMouseLocation()
    {
        return Ui::Point(_cursorX, _cursorY);
    }

    Ui::Point getMouseLocation2()
    {
        return Ui::Point(_cursorX2, _cursorY2);
    }

    Ui::Point getTooltipMouseLocation()
    {
        return Ui::Point(_tooltipCursorX, _tooltipCursorY);
    }

    Ui::Point getDragLastLocation()
    {
        return Ui::Point(_dragLastX, _dragLastY);
    }

    Ui::Point getScrollLastLocation()
    {
        return Ui::Point(_5233A4, _5233A6);
    }

    void setTooltipMouseLocation(const Ui::Point& loc)
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

    uint16_t getClickRepeatTicks()
    {
        return _clickRepeatTicks;
    }

    void setClickRepeatTicks(uint16_t ticks)
    {
        _clickRepeatTicks = ticks;
    }

    bool isRightMouseButtonDown()
    {
        return _rightMouseButtonStatus == 0;
    }

    void setRightMouseButtonDown(bool status)
    {
        _rightMouseButtonStatus = status;
    }

    // 0x00113E9E0
    static std::queue<QueuedMouseInput> _mouseQueue;

    // 0x00406FEC
    void enqueueMouseButton(const QueuedMouseInput& input)
    {
        constexpr uint32_t kMouseQueueSize = 64;
        if (_mouseQueue.size() >= kMouseQueueSize)
        {
            return;
        }
        _mouseQueue.push(input);
    }

    // 0x00407247
    static std::optional<QueuedMouseInput> dequeueMouseInput()
    {
        if (_mouseQueue.empty())
        {
            return std::nullopt;
        }
        std::optional<QueuedMouseInput> res = _mouseQueue.front();
        _mouseQueue.pop();
        return res;
    }

    // 0x004C6FCE
    static MouseButton loc_4C6FCE(uint32_t& x, int16_t& y)
    {
        x = _cursorX2;
        y = _cursorY2;
        return MouseButton::released;
    }

    // 0x004C70F1
    static MouseButton loc_4C70F1(uint32_t& x, int16_t& y)
    {
        sub_407231();
        resetFlag(Flags::rightMousePressed);
        Ui::setCursor(CursorId(*_52336C));

        if (Tutorial::state() == Tutorial::State::playing)
        {
            x = Tutorial::nextInput();
            y = Tutorial::nextInput();
        }
        else
        {
            x = _5233AE;
            y = _5233B2;
        }

        // 0x004C7136, 0x004C7165
        _cursorX2 = 0x80000000;
        return MouseButton::rightReleased;
    }

    // 0x004C6EE6
    MouseButton nextMouseInput(uint32_t& x, int16_t& y)
    {
        if (!hasFlag(Flags::rightMousePressed))
        {
            // Interrupt tutorial on mouse button input.
            auto input = dequeueMouseInput();
            if (Tutorial::state() == Tutorial::State::playing && input.has_value())
            {
                Tutorial::stop();
            }

            // If tutorial is playing, follow the recorded mouse coordinates.
            MouseButton button{};
            if (Tutorial::state() == Tutorial::State::playing)
            {
                button = MouseButton(Tutorial::nextInput());

                // 0x004C6F6C
                if (button != MouseButton::released)
                {
                    button = MouseButton(Tutorial::nextInput());
                    x = Tutorial::nextInput();
                    y = Tutorial::nextInput();
                }
                else
                {
                    button = loc_4C6FCE(x, y);
                    if (x == 0x80000000)
                        return button;
                }
            }
            // 0x004C6F5F
            else if (!input)
            {
                button = loc_4C6FCE(x, y);
                if (x == 0x80000000)
                    return button;
            }
            else if (input)
            {
                // 0x004C6F87
                switch (input->button)
                {
                    case 1:
                        button = MouseButton::leftPressed;
                        break;
                    case 2:
                        button = MouseButton::rightPressed;
                        break;
                    case 3:
                        button = MouseButton::leftReleased;
                        break;
                    default:
                        button = MouseButton::rightReleased;
                }
                x = input->pos.x;
                y = input->pos.y;
            }

            // 0x004C6FE4
            x = std::clamp<uint16_t>(x, 0, Ui::width() - 1);
            y = std::clamp<uint16_t>(y, 0, Ui::height() - 1);
            return button;
        }
        else
        {
            if (Tutorial::state() == Tutorial::State::playing)
            {
                auto button = MouseButton(Tutorial::nextInput());
                if (button == MouseButton::released)
                    return loc_4C70F1(x, y);
            }
            else if (isRightMouseButtonDown())
                return loc_4C70F1(x, y);

            // 0x004C704E
            if (Tutorial::state() == Tutorial::State::playing)
            {
                auto next = Tutorial::nextInput();
                if (!(next & 0x80))
                    return loc_4C70F1(x, y);
            }
            else if (!(addr<0x01140845, uint8_t>() & 0x80))
            {
                return loc_4C70F1(x, y);
            }

            // 0x004C7085
            if (Tutorial::state() == Tutorial::State::playing)
            {
                x = Tutorial::nextInput();
                y = Tutorial::nextInput();
            }
            else
            {
                x = _5233AE;
                y = _5233B2;
            }

            // 0x004C709F, 0x004C70D8
            _5233AE = 0;
            _5233B2 = 0;
            return MouseButton::released;
        }
    }

    // 0x004C6202
    void processMouseWheel()
    {
        int wheel = 0;

        for (; _cursorWheel > 0; _cursorWheel--)
        {
            wheel -= 17;
        }

        for (; _cursorWheel < 0; _cursorWheel++)
        {
            wheel += 17;
        }

        if (Tutorial::state() != Tutorial::State::none)
            return;

        if (Input::hasFlag(Input::Flags::rightMousePressed))
        {
            if (OpenLoco::isTitleMode())
                return;

            auto main = WindowManager::getMainWindow();
            if (main != nullptr && wheel != 0)
            {
                if (wheel > 0)
                {
                    main->viewportRotateRight();
                }
                else if (wheel < 0)
                {
                    main->viewportRotateLeft();
                }
                TownManager::updateLabels();
                StationManager::updateLabels();
                Windows::MapWindow::centerOnViewPoint();
            }

            return;
        }

        if (wheel == 0)
        {
            return;
        }

        WindowManager::wheelInput(wheel);
    }
}
