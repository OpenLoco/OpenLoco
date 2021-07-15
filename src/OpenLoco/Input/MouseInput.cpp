#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Console.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Tutorial.h"
#include "../Ui/ScrollView.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"
#include <map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Ui::ScrollView;
using namespace OpenLoco::Ui::ViewportInteraction;

#define DROPDOWN_ITEM_UNDEFINED -1

namespace OpenLoco::Input
{
    static void stateScrollLeft(MouseButton cx, WidgetIndex_t edx, Ui::Window* window, Ui::Widget* widget, int16_t x, int16_t y);
    static void stateScrollRight(const MouseButton button, const int16_t x, const int16_t y);
    static void stateResizing(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateWidgetPressed(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormal(MouseButton state, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormalHover(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormalLeft(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void stateNormalRight(int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void statePositioningWindow(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex);
    static void windowPositionEnd();

    static void windowResizeBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widget_index);

    static void viewportDragBegin(Window* w);

    static void scrollDragBegin(int16_t x, int16_t y, Window* pWindow, WidgetIndex_t index);

    static void widgetOverFlatbuttonInvalidate();

#pragma mark - Input

    static loco_global<MouseButton, 0x001136FA0> _lastKnownButtonState;

    static loco_global<string_id, 0x0050A018> _mapTooltipFormatArguments;

    static loco_global<uint16_t, 0x0050C19C> time_since_last_tick;

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
    static loco_global<Ui::WindowNumber_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<int8_t, 0x00523393> _currentTool;
    static loco_global<int16_t, 0x00523394> _toolWidgetIndex;

    static loco_global<int16_t, 0x005233A4> _5233A4;
    static loco_global<int16_t, 0x005233A6> _5233A6;
    static loco_global<Ui::WindowType, 0x005233A8> _hoverWindowType;
    static uint8_t _5233A9;
    static loco_global<Ui::WindowNumber_t, 0x005233AA> _hoverWindowNumber;
    static loco_global<uint16_t, 0x005233AC> _hoverWidgetIdx;
    static loco_global<uint32_t, 0x005233AE> _5233AE;
    static loco_global<uint32_t, 0x005233B2> _5233B2;
    static loco_global<Ui::WindowType, 0x005233B6> _modalWindowType;

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

    static std::map<Ui::ScrollView::ScrollPart, string_id> scroll_widget_tooltips = {
        { Ui::ScrollView::ScrollPart::hscrollbarButtonLeft, StringIds::tooltip_scroll_left },
        { Ui::ScrollView::ScrollPart::hscrollbarButtonRight, StringIds::tooltip_scroll_right },
        { Ui::ScrollView::ScrollPart::hscrollbarTrackLeft, StringIds::tooltip_scroll_left_fast },
        { Ui::ScrollView::ScrollPart::hscrollbarTrackRight, StringIds::tooltip_scroll_right_fast },
        { Ui::ScrollView::ScrollPart::hscrollbarThumb, StringIds::tooltip_scroll_left_right },
        { Ui::ScrollView::ScrollPart::vscrollbarButtonTop, StringIds::tooltip_scroll_up },
        { Ui::ScrollView::ScrollPart::vscrollbarButtonBottom, StringIds::tooltip_scroll_down },
        { Ui::ScrollView::ScrollPart::vscrollbarTrackTop, StringIds::tooltip_scroll_up_fast },
        { Ui::ScrollView::ScrollPart::vscrollbarTrackBottom, StringIds::tooltip_scroll_down_fast },
        { Ui::ScrollView::ScrollPart::vscrollbarThumb, StringIds::tooltip_scroll_up_down },
    };

    void initMouse()
    {
        _pressedWindowType = Ui::WindowType::undefined;

        _tooltipNotShownTicks = -1;
        _hoverWindowType = Ui::WindowType::undefined;

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

    bool isDropdownActive(Ui::WindowType type, Ui::WidgetIndex_t index)
    {
        if (state() != State::dropdownActive)
            return false;

        if (*_pressedWindowType != type)
            return false;

        if (!hasFlag(Flags::widgetPressed))
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
                Ui::setCursorPos(*_cursorX2, *_cursorY2);
                break;
            }

            case Tutorial::State::recording:
            {
                call(0x004C6EC3);
                break;
            }
        }
    }

    Window* toolGetActiveWindow()
    {
        if (!hasFlag(Flags::toolActive))
        {
            return nullptr;
        }

        return WindowManager::find(_toolWindowType, _toolWindowNumber);
    }

    int16_t getToolWidgetIndex()
    {
        return _toolWidgetIndex;
    }

    bool isToolActive(Ui::WindowType type)
    {
        if (!hasFlag(Flags::toolActive))
            return false;

        return (*_toolWindowType == type);
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number)
    {
        if (!isToolActive(type))
            return false;

        return _toolWindowNumber == number;
    }

    bool isToolActive(Ui::WindowType type, Ui::WindowNumber_t number, int16_t widgetIndex)
    {
        if (!isToolActive(type, number))
            return false;
        return _toolWidgetIndex == widgetIndex;
    }

    // 0x004CE367
    // tool (al)
    // widgetIndex (dx)
    // w (esi)
    // TODO: Maybe create a an enum similar to TOOL_IDX in OpenRCT2 for tool (instead of uint8_t)
    bool toolSet(Ui::Window* w, int16_t widgetIndex, uint8_t tool)
    {
        if (Input::hasFlag(Input::Flags::toolActive))
        {
            if (w->type == *_toolWindowType && w->number == _toolWindowNumber
                && widgetIndex == _toolWidgetIndex)
            {
                toolCancel();
                return false;
            }
            else
            {
                toolCancel();
            }
        }

        Input::setFlag(Input::Flags::toolActive);
        Input::resetFlag(Input::Flags::flag6);
        _currentTool = tool;
        _toolWindowType = w->type;
        _toolWindowNumber = w->number;
        _toolWidgetIndex = widgetIndex;
        return true;
    }

    // 0x004CE3D6
    void toolCancel()
    {
        if (Input::hasFlag(Input::Flags::toolActive))
        {
            Input::resetFlag(Input::Flags::toolActive);

            Map::TileManager::mapInvalidateSelectionRect();
            Map::TileManager::mapInvalidateMapSelectionTiles();

            resetMapSelectionFlag(MapSelectionFlags::enable | MapSelectionFlags::enableConstruct | MapSelectionFlags::unk_02 | MapSelectionFlags::unk_03 | MapSelectionFlags::unk_04);

            if (_toolWidgetIndex >= 0)
            {
                // Invalidate tool widget
                Ui::WindowManager::invalidateWidget(_toolWindowType, _toolWindowNumber, _toolWidgetIndex);

                // Abort tool event
                Window* w = Ui::WindowManager::find(_toolWindowType, _toolWindowNumber);
                if (w != nullptr)
                    w->callToolAbort(_toolWidgetIndex);
            }
        }
    }

    void toolCancel(Ui::WindowType type, Ui::WindowNumber_t number)
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

        if (*_modalWindowType != Ui::WindowType::undefined)
        {
            if (window != nullptr)
            {
                if (window->type != *_modalWindowType)
                {
                    if (button == MouseButton::leftPressed)
                    {
                        WindowManager::bringToFront(_modalWindowType);
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
            case State::reset:
                _tooltipCursorX = x;
                _tooltipCursorY = y;
                _tooltipTimeout = 0;
                _tooltipWindowType = Ui::WindowType::undefined;
                state(State::normal);
                resetFlag(Flags::flag4);
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
                stateResizing(button, x, y, window, widget, widgetIndex);
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
                        auto tool = WindowManager::find(_toolWindowType, _toolWindowNumber);
                        if (tool != nullptr)
                        {
                            tool->callToolDragContinue(_toolWidgetIndex, x, y);
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
                    auto tool = WindowManager::find(_toolWindowType, _toolWindowNumber);
                    if (tool != nullptr)
                    {
                        tool->callToolDragEnd(_toolWidgetIndex);
                    }
                }
                else if (!hasFlag(Flags::flag4))
                {
                    auto interaction = ViewportInteraction::getItemLeft(x, y);
                    switch (interaction.type)
                    {
                        case InteractionItem::entity:
                        {
                            auto _thing = reinterpret_cast<EntityBase*>(interaction.object);
                            auto veh = _thing->asVehicle();
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
                            Ui::Windows::Station::open(interaction.value);
                            break;
                        }

                        case InteractionItem::headquarterBuilding:
                        {
                            auto building = ((Map::TileElement*)interaction.object)->asBuilding();
                            if (building != nullptr)
                            {
                                auto index = building->multiTileIndex();
                                const auto firstTile = interaction.pos - Map::offsets[index];
                                const Pos3 pos = { firstTile.x,
                                                   firstTile.y,
                                                   building->baseZ() };

                                for (auto& company : CompanyManager::companies())
                                {
                                    if (company.headquarters_x == pos.x
                                        && company.headquarters_y == pos.y
                                        && company.headquarters_z == pos.z)
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
                            Ui::Windows::Industry::open(interaction.value);
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
    static void signalInteract(Window* main, Map::SignalElement* signal, const uint8_t bh, const Map::Pos2 pos)
    {
        auto* track = reinterpret_cast<Map::TileElement*>(signal - 1)->asTrack();
        if (track == nullptr)
        {
            return;
        }

        uint16_t unkFlags = 1 << 15;
        if (bh != 0)
        {
            unkFlags = 1 << 14;
        }
        if (!Input::isToolActive(WindowType::construction, 11 /* Ui::Windows::Construction::Signal::widx::signal_direction */))
        {
            if (signal->hasLeftSignal() && signal->hasRightSignal())
            {
                unkFlags = (1 << 15) | (1 << 14);
            }
        }

        GameCommands::SignalRemovalArgs args;
        args.pos = { pos.x, pos.y, track->baseZ() * 4 };
        args.rotation = track->unkDirection();
        args.trackId = track->trackId();
        args.index = track->sequenceIndex();
        args.type = track->trackObjectId();
        args.flags = unkFlags;

        auto* window = WindowManager::find(WindowType::construction);
        if (window != nullptr)
        {
            Ui::Windows::Construction::sub_49FEC7();
        }

        GameCommands::setErrorTitle(StringIds::cant_remove_signal);
        if (GameCommands::do_14(GameCommands::Flags::apply, args))
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5B66 TODO: Move to a better file
    static void trackStationInteract(Window* main, Map::StationElement* station, const Map::Pos2 pos)
    {
        auto* track = reinterpret_cast<Map::TileElement*>(station - 1)->asTrack();
        if (track == nullptr)
        {
            return;
        }

        GameCommands::setErrorTitle(StringIds::cant_remove_station);
        GameCommands::TrackStationRemovalArgs args;
        args.pos = { pos.x, pos.y, track->baseZ() * 4 };
        args.rotation = track->unkDirection();
        args.trackId = track->trackId();
        args.index = track->sequenceIndex();
        args.type = track->trackObjectId();
        if (GameCommands::do_16(GameCommands::Flags::apply, args))
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5BDF TODO: Move to a better file
    static void roadStationInteract(Window* main, Map::StationElement* station, const Map::Pos2 pos)
    {
        auto* road = reinterpret_cast<Map::TileElement*>(station - 1)->asRoad();
        if (road == nullptr)
        {
            return;
        }

        GameCommands::setErrorTitle(StringIds::cant_remove_station);
        GameCommands::RoadStationRemovalArgs args;
        args.pos = { pos.x, pos.y, road->baseZ() * 4 };
        args.rotation = road->unkDirection();
        args.roadId = road->roadId();
        args.index = road->sequenceIndex();
        args.type = road->roadObjectId();
        if (GameCommands::do_43(GameCommands::Flags::apply, args))
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5C58 TODO: Move to a better file
    static void airportInteract(Window* main, Map::StationElement* station, const Map::Pos2 pos)
    {
        if (!Ui::Windows::Construction::isStationTabOpen())
        {
            Ui::Windows::Construction::openWithFlags(1ULL << 31);
            return;
        }
        GameCommands::setErrorTitle(StringIds::cant_remove_airport);
        GameCommands::AirportRemovalArgs args;
        args.pos = { pos.x, pos.y, station->baseZ() * 4 };
        if (GameCommands::do_57(GameCommands::Flags::apply, args))
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004A5CC5 TODO: Move to a better file
    static void dockInteract(Window* main, Map::StationElement* station, const Map::Pos2 pos)
    {
        if (!Ui::Windows::Construction::isStationTabOpen())
        {
            Ui::Windows::Construction::openWithFlags(1ULL << 30);
            return;
        }
        GameCommands::setErrorTitle(StringIds::cant_remove_ship_port);
        GameCommands::PortRemovalArgs args;
        Pos2 firstTile = pos - Map::offsets[station->multiTileIndex()];
        args.pos = Pos3(firstTile.x, firstTile.y, station->baseZ() * 4);
        if (GameCommands::do_61(GameCommands::Flags::apply, args))
        {
            Audio::playSound(Audio::SoundId::demolish, GameCommands::getPosition());
        }
    }

    // 0x004BB116 TODO: Move to a better file
    static void treeInteract(Map::TreeElement* tree, const Map::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::TreeRemovalArgs args;
        args.pos = pos;
        args.baseZ = tree->baseZ();
        args.elementType = tree->rawData()[0];
        args.type = tree->treeObjectId();
        GameCommands::do_22(GameCommands::Flags::apply, args);
    }

    // 0x0042D9BF TODO: Move to a better file
    static void buildingInteract(Map::BuildingElement* building, const Map::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::BuildingRemovalArgs args;
        Pos2 firstTile = pos - Map::offsets[building->multiTileIndex()];
        args.pos = Pos3(firstTile.x, firstTile.y, building->baseZ() * 4);
        GameCommands::do_45(GameCommands::Flags::apply, args);
    }

    // 0x004C4809 TODO: Move to a better file
    static void wallInteract(Map::WallElement* wall, const Map::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::WallRemovalArgs args;
        args.pos = pos;
        args.rotation = wall->rotation();
        args.baseZ = wall->baseZ();
        GameCommands::do_33(GameCommands::Flags::apply, args);
    }

    // 0x0042F007 TODO: Move to a better file
    static void headquarterInteract(Map::BuildingElement* building, const Map::Pos2 pos)
    {
        GameCommands::setErrorTitle(StringIds::error_cant_remove_this);
        GameCommands::HeadquarterRemovalArgs args;
        Pos2 firstTile = pos - Map::offsets[building->multiTileIndex()];
        args.pos = Pos3(firstTile.x, firstTile.y, building->baseZ() * 4);
        GameCommands::do_55(GameCommands::Flags::apply, args);
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
                _ticksSinceDragStart += time_since_last_tick;
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

                if (window->flags & WindowFlags::viewport_no_scrolling)
                {
                    return;
                }

                Gfx::point_t dragOffset = { x, y };
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
                        window->viewport_configurations[0].saved_view_x += dragOffset.x << (vp->zoom + 1);
                        window->viewport_configurations[0].saved_view_y += dragOffset.y << (vp->zoom + 1);
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
                auto interaction = ViewportInteraction::rightOver(_dragLastX, _dragLastY);

                switch (interaction.type)
                {
                    case InteractionItem::noInteraction:
                    default:
                    {
                        auto item2 = ViewportInteraction::getItemLeft(_dragLastX, _dragLastY);
                        switch (item2.type)
                        {
                            case InteractionItem::entity:
                            {
                                auto _thing = reinterpret_cast<EntityBase*>(item2.object);
                                auto veh = _thing->asVehicle();
                                if (veh != nullptr)
                                {
                                    auto head = EntityManager::get<Vehicles::VehicleHead>(veh->getHead());
                                    Ui::Windows::VehicleList::open(head->owner, head->vehicleType);
                                }
                                break;
                            }
                            case InteractionItem::townLabel:
                                Ui::Windows::TownList::open();
                                break;
                            case InteractionItem::stationLabel:
                            {
                                auto station = StationManager::get(item2.value);
                                Ui::Windows::StationList::open(station->owner);
                                break;
                            }
                            case InteractionItem::industry:
                                Ui::Windows::IndustryList::open();
                                break;
                            default:
                                break;
                        }

                        break;
                    }

                    case InteractionItem::track:
                    {
                        auto track = ((Map::TileElement*)interaction.object)->asTrack();
                        if (track != nullptr)
                        {
                            if (track->owner() == CompanyManager::getControllingId())
                            {
                                Ui::Windows::Construction::openAtTrack(window, track, interaction.pos);
                            }
                            else
                            {
                                Ui::Windows::CompanyWindow::open(track->owner());
                            }
                        }
                        break;
                    }
                    case InteractionItem::road:
                    {
                        auto road = reinterpret_cast<Map::TileElement*>(interaction.object)->asRoad();
                        if (road != nullptr)
                        {

                            auto owner = road->owner();

                            auto roadObject = ObjectManager::get<RoadObject>(road->roadObjectId());
                            if (owner == CompanyManager::getControllingId() || owner == CompanyId::neutral || (roadObject->flags & Flags12::unk_03))
                            {
                                Ui::Windows::Construction::openAtRoad(window, road, interaction.pos);
                            }
                            else
                            {
                                Ui::Windows::CompanyWindow::open(owner);
                            }
                        }
                        break;
                    }
                    case InteractionItem::trackExtra:
                    {
                        auto track = ((Map::TileElement*)interaction.object)->asTrack();
                        if (track != nullptr)
                        {
                            Ui::Windows::Construction::setToTrackExtra(window, track, interaction.unkBh, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::roadExtra:
                    {
                        auto road = ((Map::TileElement*)interaction.object)->asRoad();
                        if (road != nullptr)
                        {
                            Ui::Windows::Construction::setToRoadExtra(window, road, interaction.unkBh, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::signal:
                    {
                        auto signal = ((Map::TileElement*)interaction.object)->asSignal();
                        if (signal != nullptr)
                        {
                            signalInteract(window, signal, interaction.unkBh, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::trackStation:
                    {
                        auto station = ((Map::TileElement*)interaction.object)->asStation();
                        if (station != nullptr)
                        {
                            trackStationInteract(window, station, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::roadStation:
                    {
                        auto station = ((Map::TileElement*)interaction.object)->asStation();
                        if (station != nullptr)
                        {
                            roadStationInteract(window, station, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::airport:
                    {
                        auto station = ((Map::TileElement*)interaction.object)->asStation();
                        if (station != nullptr)
                        {
                            airportInteract(window, station, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::dock:
                    {
                        auto station = ((Map::TileElement*)interaction.object)->asStation();
                        if (station != nullptr)
                        {
                            dockInteract(window, station, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::tree:
                    {
                        auto tree = ((Map::TileElement*)interaction.object)->asTree();
                        if (tree != nullptr)
                        {
                            treeInteract(tree, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::building:
                    {
                        auto building = ((Map::TileElement*)interaction.object)->asBuilding();
                        if (building != nullptr)
                        {
                            buildingInteract(building, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::wall:
                    {
                        auto wall = ((Map::TileElement*)interaction.object)->asWall();
                        if (wall != nullptr)
                        {
                            wallInteract(wall, interaction.pos);
                        }
                        break;
                    }
                    case InteractionItem::headquarterBuilding:
                    {
                        auto building = ((Map::TileElement*)interaction.object)->asBuilding();
                        if (building != nullptr)
                        {
                            headquarterInteract(building, interaction.pos);
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
                _ticksSinceDragStart += time_since_last_tick;
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
    static void stateResizing(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
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

                if (w->flags & Ui::WindowFlags::flag_15)
                {
                    doDefault = true;
                    break;
                }

                if (w->flags & Ui::WindowFlags::flag_16)
                {
                    x = window->var_88A - window->width + _dragLastX;
                    y = window->var_88C - window->height + _dragLastY;
                    w->flags &= ~Ui::WindowFlags::flag_16;
                    doDefault = true;
                    break;
                }

                window->var_88A = window->width;
                window->var_88C = window->height;
                x = _dragLastX - window->x - window->width + Ui::width();
                y = _dragLastY - window->y - window->height + Ui::height() - 27;
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

        w->width = std::clamp<uint16_t>(w->width + dx, w->min_width, w->max_width);
        w->height = std::clamp<uint16_t>(w->height + dy, w->min_height, w->max_height);
        w->flags |= Ui::WindowFlags::flag_15;
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
    static void statePositioningWindow(MouseButton button, int16_t x, int16_t y, Ui::Window* window, Ui::Widget* widget, Ui::WidgetIndex_t widgetIndex)
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

                            Audio::playSound(Audio::SoundId::clickPress, dragWindow->x + pressedWidget->mid_x());
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

        if (*_modalWindowType == Ui::WindowType::undefined || *_modalWindowType == window->type)
        {
            window->callOnDropdown(_pressedWidgetIndex, item);
        }
    }

    static int dropdownIndexFromPoint(Ui::Window* window, int x, int y)
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
                        Audio::playSound(Audio::SoundId::clickUp, window->x + buttonWidget->mid_x());
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
                        if (item != DROPDOWN_ITEM_UNDEFINED)
                        {
                            dropdownRegisterSelection(item);
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

                            dropdownRegisterSelection(DROPDOWN_ITEM_UNDEFINED);
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
                Audio::playSound(Audio::SoundId::clickUp, window->x + widget->mid_x());
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
                if (item != DROPDOWN_ITEM_UNDEFINED)
                {
                    _dropdownHighlightedIndex = item;
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

        string_id tooltipStringId = StringIds::null;
        if (window != nullptr && widgetIndex != -1)
        {
            if (widget->type == Ui::WidgetType::scrollview)
            {
                Ui::ScrollView::ScrollPart scrollArea;
                int16_t scrollX, scrollY;
                size_t scrollIndex;
                Ui::ScrollView::getPart(window, widget, x, y, &scrollX, &scrollY, &scrollArea, &scrollIndex);

                if (scrollArea == Ui::ScrollView::ScrollPart::none)
                {
                }
                else if (scrollArea == Ui::ScrollView::ScrollPart::view)
                {
                    window->callScrollMouseOver(scrollX, scrollY, static_cast<uint8_t>(scrollIndex));
                }
                else
                {
                    tooltipStringId = scroll_widget_tooltips[scrollArea];
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
            if (*_tooltipWindowType == window->type && _tooltipWindowNumber == window->number && _tooltipWidgetIndex == widgetIndex)
            {
                _tooltipTimeout += time_since_last_tick;
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

        window = WindowManager::bringToFront(window);
        if (widgetIndex == -1)
        {
            return;
        }

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
                    auto w = WindowManager::find(_toolWindowType, _toolWindowNumber);
                    if (w != nullptr)
                    {
                        w->callToolDown(_toolWidgetIndex, x, y);
                        setFlag(Flags::flag4);
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
                    Audio::playSound(Audio::SoundId::clickDown, window->x + widget->mid_x());

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

        window = WindowManager::bringToFront(window);

        if (widgetIndex == -1)
        {
            return;
        }

        if (*_modalWindowType != Ui::WindowType::undefined)
        {
            if (*_modalWindowType == window->type)
            {
                Ui::ScrollView::scrollModalRight(x, y, window, widget, widgetIndex);
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
                setFlag(Flags::flag5);
                break;

            case Ui::WidgetType::scrollview:
                scrollDragBegin(x, y, window, widgetIndex);

                _5233AE = 0;
                _5233B2 = 0;
                setFlag(Flags::flag5);
                break;
        }
    }

#pragma mark - Window positioning

    // 0x004C877D
    void windowPositionBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widget_index)
    {
        state(State::positioningWindow);
        _pressedWidgetIndex = widget_index;
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
    static void windowResizeBegin(int16_t x, int16_t y, Ui::Window* window, Ui::WidgetIndex_t widget_index)
    {
        state(State::resizing);
        _pressedWidgetIndex = widget_index;
        _dragLastX = x;
        _dragLastY = y;
        _dragWindowType = window->type;
        _dragWindowNumber = window->number;
        window->flags &= ~Ui::WindowFlags::flag_15;
    }

#pragma mark - Viewport dragging

    static void viewportDragBegin(Window* w)
    {
        w->flags &= ~Ui::WindowFlags::scrolling_to_location;
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
            if (oldWidget->type == Ui::WidgetType::wt_10 || oldWidget->type == Ui::WidgetType::wt_9)
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

        if (hasMapSelectionFlag(MapSelectionFlags::unk_6))
        {
            resetMapSelectionFlag(MapSelectionFlags::unk_6);
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
                        if (window->flags & Ui::WindowFlags::resizable)
                        {
                            if (window->min_width != window->max_width || window->min_height != window->max_height)
                            {
                                if (x >= window->x + window->width - 19 && y >= window->y + window->height - 19)
                                {
                                    cursorId = Ui::CursorId::diagonalArrows;
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

                    case Ui::WidgetType::scrollview:
                        _5233A4 = x;
                        _5233A6 = y;
                        Ui::ScrollView::ScrollPart output_scroll_area;
                        size_t scroll_id;
                        int16_t scroll_x, scroll_y;
                        Ui::ScrollView::getPart(
                            window,
                            &window->widgets[widgetIdx],
                            x,
                            y,
                            &scroll_x,
                            &scroll_y,
                            &output_scroll_area,
                            &scroll_id);

                        if (output_scroll_area == Ui::ScrollView::ScrollPart::view)
                        {

                            cursorId = window->callCursor(widgetIdx, scroll_x, scroll_y, cursorId);
                        }

                        break;

                    case Ui::WidgetType::viewport:
                        if (Input::hasFlag(Flags::toolActive))
                        {
                            // 3
                            cursorId = (Ui::CursorId)*_currentTool;
                            auto wnd = Ui::WindowManager::find(_toolWindowType, _toolWindowNumber);
                            if (wnd)
                            {
                                bool out = false;
                                cursorId = wnd->call_15(x, y, cursorId, &out);
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

    Gfx::point_t getMouseLocation()
    {
        return Gfx::point_t(_cursorX, _cursorY);
    }

    Gfx::point_t getMouseLocation2()
    {
        return Gfx::point_t(_cursorX2, _cursorY2);
    }

    Gfx::point_t getTooltipMouseLocation()
    {
        return Gfx::point_t(_tooltipCursorX, _tooltipCursorY);
    }

    Gfx::point_t getDragLastLocation()
    {
        return Gfx::point_t(_dragLastX, _dragLastY);
    }

    Gfx::point_t getScrollLastLocation()
    {
        return Gfx::point_t(_5233A4, _5233A6);
    }

    void setTooltipMouseLocation(const Gfx::point_t& loc)
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

    void setClickRepeatTicks(uint16_t ticks)
    {
        _clickRepeatTicks = ticks;
    }
}
