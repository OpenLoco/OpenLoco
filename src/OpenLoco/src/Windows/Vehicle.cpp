#include "Vehicles/Vehicle.h"
#include "Config.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Vehicles/RenameVehicle.h"
#include "GameCommands/Vehicles/VehicleChangeRunningMode.h"
#include "GameCommands/Vehicles/VehicleOrderDelete.h"
#include "GameCommands/Vehicles/VehicleOrderDown.h"
#include "GameCommands/Vehicles/VehicleOrderInsert.h"
#include "GameCommands/Vehicles/VehicleOrderReverse.h"
#include "GameCommands/Vehicles/VehicleOrderSkip.h"
#include "GameCommands/Vehicles/VehicleOrderUp.h"
#include "GameCommands/Vehicles/VehiclePassSignal.h"
#include "GameCommands/Vehicles/VehiclePickup.h"
#include "GameCommands/Vehicles/VehiclePickupAir.h"
#include "GameCommands/Vehicles/VehiclePickupWater.h"
#include "GameCommands/Vehicles/VehicleRefit.h"
#include "GameCommands/Vehicles/VehicleReverse.h"
#include "GameCommands/Vehicles/VehicleSell.h"
#include "GameCommands/Vehicles/VehicleSpeedControl.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "LabelFrame.h"
#include "LastGameOptionManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/RoadElement.h"
#include "Map/StationElement.h"
#include "Map/TileManager.h"
#include "Map/Track/SubpositionData.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/AirportObject.h"
#include "Objects/CargoObject.h"
#include "Objects/DockObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "SceneManager.h"
#include "Ui/Dropdown.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include "Widget.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <map>
#include <sstream>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::Literals;

using OpenLoco::GameCommands::VehicleChangeRunningModeArgs;

namespace OpenLoco::Ui::Windows::Vehicle
{
    namespace Common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            closeButton = 2,
            panel = 3,
            tabMain = 4,
            tabDetails = 5,
            tabCargo = 6,
            tabFinances = 7,
            tabRoute = 8,
        };

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { (frameWidth), (frameHeight) }, WidgetType::frame, WindowColour::primary),                                                                 \
        makeWidget({ 1, 1 }, { (frameWidth)-2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ (frameWidth)-15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { 265, 136 }, WidgetType::panel, WindowColour::secondary),                                                                             \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicle_tab_main),                        \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicle_tab_details),                    \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicle_tab_cargo),                      \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicle_tab_finance),                    \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicle_tab_route)

        constexpr uint64_t enabledWidgets = (1 << closeButton) | (1 << tabMain) | (1 << tabDetails) | (1 << tabCargo) | (1 << tabFinances) | (1 << tabRoute);

        static Vehicles::VehicleHead* getVehicle(const Window* self)
        {
            auto* veh = EntityManager::get<Vehicles::VehicleHead>(EntityId(self->number));
            if (veh == nullptr)
            {
                WindowManager::close(WindowType::vehicle, self->number);
                return nullptr;
            }
            return veh;
        }

        static void setActiveTabs(Window* const self);
        static void textInput(Window& self, const WidgetIndex_t callingWidget, const char* const input);
        static void renameVehicle(Window* const self, const WidgetIndex_t widgetIndex);
        static void switchTab(Window* const self, const WidgetIndex_t widgetIndex);
        static void setCaptionEnableState(Window* const self);
        static void onPickup(Window* const self, const WidgetIndex_t pickupWidx);
        static void event8(Window& self);
        static void event9(Window& self);
        static size_t getNumCars(Ui::Window* const self);
        static void drawTabs(Window* const window, Gfx::RenderTarget* const rt);
        static void pickupToolUpdate(Window& self, const int16_t x, const int16_t y);
        static void pickupToolDown(Window& self, const int16_t x, const int16_t y);
        static void pickupToolAbort(Window& self);
        static size_t getNumCars(Ui::Window* const self);
        static void drawTabs(Window* const window, Gfx::RenderTarget* const rt);
        static std::optional<Vehicles::Car> getCarFromScrollView(Window* const self, const int16_t y);
        static std::pair<uint32_t, StringId> getPickupImageIdandTooltip(const Vehicles::VehicleHead& head, const bool isPlaced);
    }

    namespace Details
    {
        static constexpr Ui::Size kMinWindowSize = { 192, 148 };
        static constexpr Ui::Size kMaxWindowSize = { 400, 440 };

        enum widx
        {
            buildNew = Common::widx::tabRoute + 1,
            pickup,
            remove,
            carList
        };

        // 0x00500434
        static WindowEventList events;
        constexpr uint64_t enabledWidgets = (1 << widx::buildNew) | (1 << widx::pickup) | (1 << widx::remove) | (1 << widx::carList) | Common::enabledWidgets;
        constexpr uint64_t holdableWidgets = 0;

        static Widget widgets[] = {
            commonWidgets(265, 177, StringIds::title_vehicle_details),
            makeWidget({ 240, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_build_new_vehicle_for),
            makeWidget({ 240, 68 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_remove_from_track),
            makeWidget({ 240, 96 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rubbish_bin, StringIds::tooltip_sell_or_drag_vehicle),
            makeWidget({ 3, 44 }, { 237, 110 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd()
        };
    }

    namespace Cargo
    {
        static constexpr Ui::Size kMinWindowSize = { 192, 142 };
        static constexpr Ui::Size kMaxWindowSize = { 400, 440 };

        enum widx
        {
            refit = 9,
            cargoList = 10,
        };

        // 0x005004A8
        static WindowEventList events;
        constexpr uint64_t enabledWidgets = (1 << widx::refit) | (1 << widx::cargoList) | Common::enabledWidgets;
        constexpr uint64_t holdableWidgets = 0;

        static Widget widgets[] = {
            commonWidgets(265, 177, StringIds::title_vehicle_cargo),
            makeWidget({ 240, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::refit_cargo_button, StringIds::refit_vehicle_tip),
            makeWidget({ 3, 44 }, { 259, 120 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd()
        };
    }

    namespace Finances
    {
        static constexpr Ui::Size kMinWindowSize = { 400, 202 };
        static constexpr Ui::Size kMaxWindowSize = kMinWindowSize;

        static WindowEventList events;
        constexpr uint64_t enabledWidgets = Common::enabledWidgets;
        constexpr uint64_t holdableWidgets = 0;

        // 0x00522470
        static Widget widgets[] = {
            commonWidgets(636, 319, StringIds::title_company_finances),
            widgetEnd(),
        };
    }

    namespace Route
    {
        static constexpr Ui::Size kMinWindowSize = { 265, 202 };
        static constexpr Ui::Size kMaxWindowSize = { 600, 440 };

        enum widx
        {
            tool = Common::widx::tabRoute + 1, // Only used to hold the tool does nothing
            localMode,
            expressMode,
            routeList,
            orderForceUnload,
            orderWait,
            orderSkip,
            orderDelete,
            orderUp,
            orderDown,
            orderReverse
        };

        // 0x00500554
        static WindowEventList events;
        constexpr uint64_t enabledWidgets = (1ULL << widx::routeList) | (1ULL << widx::orderForceUnload) | (1ULL << widx::orderWait) | (1ULL << widx::orderSkip) | (1ULL << widx::orderDelete) | (1ULL << widx::orderUp) | (1ULL << widx::orderDown) | (1ULL << widx::orderReverse) | Common::enabledWidgets;
        constexpr uint64_t holdableWidgets = 0;
        constexpr auto lineHeight = 10;

        static Widget widgets[] = {
            commonWidgets(265, 189, StringIds::title_vehicle_route),
            makeWidget({ 0, 0 }, { 1, 1 }, WidgetType::none, WindowColour::primary),
            makeWidget({ 3, 44 }, { 118, 12 }, WidgetType::button, WindowColour::secondary, StringIds::local_mode_button),
            makeWidget({ 121, 44 }, { 119, 12 }, WidgetType::button, WindowColour::secondary, StringIds::express_mode_button),
            makeWidget({ 3, 58 }, { 237, 120 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::tooltip_route_scrollview),
            makeWidget({ 240, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::route_force_unload, StringIds::tooltip_route_insert_force_unload),
            makeWidget({ 240, 68 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::route_wait, StringIds::tooltip_route_insert_wait_full_cargo),
            makeWidget({ 240, 92 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::route_skip, StringIds::tooltip_route_skip_next_order),
            makeWidget({ 240, 116 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::route_delete, StringIds::tooltip_route_delete_order),
            makeWidget({ 240, 140 }, { 24, 12 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::red_arrow_up, StringIds::tooltip_route_move_order_up),
            makeWidget({ 240, 152 }, { 24, 12 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::red_arrow_down, StringIds::tooltip_route_move_order_down),
            makeWidget({ 240, 164 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_right_turnaround, StringIds::reverseOrderTableTooltip),
            widgetEnd(),
        };
    }

    static loco_global<Vehicles::VehicleBogie*, 0x0113614E> _dragCarComponent;
    static loco_global<EntityId, 0x01136156> _dragVehicleHead;
    static loco_global<int32_t, 0x01136264> _1136264;
    static loco_global<uint8_t, 0x01136264> _ghostAirportNode;
    static loco_global<World::Pos3, 0x0113625E> _ghostVehiclePos;
    static loco_global<StationId, 0x0113625A> _ghostAirportStationId;
    static loco_global<uint32_t, 0x0113625A> _ghostLandTrackAndDirection;

    namespace Main
    {
        static constexpr Ui::Size kWindowSize = { 265, 177 };
        static constexpr Ui::Size kMinWindowSize = { 192, 177 };
        static constexpr Ui::Size kMaxWindowSize = { 600, 440 };

        enum widx
        {
            viewport = Common::widx::tabRoute + 1,
            status = 10,
            speedControl = 11,
            stopStart = 12,
            pickup = 13,
            passSignal = 14,
            changeDirection = 15,
            centreViewport = 16,
        };

        static Widget widgets[] = {
            commonWidgets(265, 177, StringIds::stringid),
            makeWidget({ 3, 44 }, { 237, 120 }, WidgetType::viewport, WindowColour::secondary),
            makeWidget({ 3, 155 }, { 237, 21 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 240, 46 }, { 24, 115 }, WidgetType::slider, WindowColour::secondary),
            makeWidget({ 240, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::red_flag, StringIds::tooltip_stop_start),
            makeWidget({ 240, 68 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::null, StringIds::tooltip_remove_from_track),
            makeWidget({ 240, 92 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::pass_signal, StringIds::tooltip_pass_signal_at_danger),
            makeWidget({ 240, 116 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_right_turnaround, StringIds::tooltip_change_direction),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::null, StringIds::move_main_view_to_show_this),
            widgetEnd()
        };

        constexpr uint64_t interactiveWidgets = (1 << widx::stopStart) | (1 << widx::pickup) | (1 << widx::passSignal) | (1 << widx::changeDirection) | (1 << widx::centreViewport);
        constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::speedControl) | interactiveWidgets;
        constexpr uint64_t holdableWidgets = 1 << widx::speedControl;

        // 0x005003C0
        static WindowEventList events;

        // 0x004B5D82
        static void resetDisabledWidgets(Window* const self)
        {
            self->disabledWidgets = 0;
        }

        // 0x004B5D88
        // 0x004B32F9
        static void createViewport(Window& self)
        {
            if (self.currentTab != (Common::widx::tabMain - Common::widx::tabMain))
            {
                return;
            }

            self.callPrepareDraw();

            auto vehHead = Common::getVehicle(&self);
            if (vehHead == nullptr)
            {
                return;
            }
            Vehicles::Vehicle train(*vehHead);

            // If picked up no need for viewport drawn
            if (vehHead->tileX == -1)
            {
                self.viewportRemove(0);
                self.invalidate();
                return;
            }

            // By default focus on the veh2 id and if there are cars focus on the body of the first car
            EntityId targetEntity = train.veh2->id;
            if (!train.cars.empty())
            {
                targetEntity = train.cars.firstCar.front->id;
                // Always true so above is pointless
                if (train.cars.firstCar.front->getSubType() == Vehicles::VehicleEntityType::bogie)
                {
                    targetEntity = train.cars.firstCar.body->id;
                }
            }

            // Compute views.
            SavedView view = {
                targetEntity,
                (1 << 15) | (1 << 14),
                ZoomLevel::full,
                static_cast<int8_t>(self.viewports[0]->getRotation()),
                0
            };

            ViewportFlags flags = ViewportFlags::none;
            if (self.viewports[0] != nullptr)
            {
                if (self.savedView == view)
                    return;

                flags = self.viewports[0]->flags;
                self.viewportRemove(0);
                ViewportManager::collectGarbage();
            }
            else
            {
                if (Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
                    flags |= ViewportFlags::gridlines_on_landscape;
            }

            self.savedView = view;

            // 0x004B5E88 start
            if (self.viewports[0] == nullptr)
            {
                auto widget = &self.widgets[widx::viewport];
                auto origin = Ui::Point(widget->left + self.x + 1, widget->top + self.y + 1);
                auto size = Ui::Size(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(&self, 0, origin, size, self.savedView.zoomLevel, targetEntity);
                self.invalidate();
                self.flags |= WindowFlags::viewportNoScrolling;
            }
            // 0x004B5E88 end

            if (self.viewports[0] != nullptr)
            {
                self.viewports[0]->flags = flags;
                self.invalidate();
            }
        }

        static const uint16_t rowHeights[vehicleTypeCount] = {
            22,
            22,
            22,
            22,
            82,
            45
        };

        static void initEvents();

        // 0x004B60DC
        static Window* create(const EntityId head)
        {
            auto* const self = WindowManager::createWindow(WindowType::vehicle, kWindowSize, WindowFlags::flag_11 | WindowFlags::flag_8 | WindowFlags::resizable, &events);
            self->widgets = widgets;
            self->enabledWidgets = enabledWidgets;
            self->number = enumValue(head);
            const auto* vehicle = Common::getVehicle(self);
            if (vehicle == nullptr)
            {
                return self;
            }
            self->owner = vehicle->owner;
            self->rowHeight = rowHeights[static_cast<uint8_t>(vehicle->vehicleType)];
            self->currentTab = 0;
            self->frameNo = 0;
            resetDisabledWidgets(self);
            self->minWidth = kMinWindowSize.width;
            self->minHeight = kMinWindowSize.height;
            self->maxWidth = kMaxWindowSize.width;
            self->maxHeight = kMaxWindowSize.height;
            self->var_85C = -1;
            WindowManager::close(WindowType::dragVehiclePart, 0);
            _dragCarComponent = nullptr;
            _dragVehicleHead = EntityId::null;

            const auto* skin = ObjectManager::get<InterfaceSkinObject>();
            self->setColour(WindowColour::secondary, skin->colour_0A);
            return self;
        }

        // 0x004B6033
        Window* open(const Vehicles::VehicleBase* vehicle)
        {
            if (vehicle == nullptr)
            {
                return nullptr;
            }
            const auto head = vehicle->getHead();
            auto* self = WindowManager::find(WindowType::vehicle, enumValue(head));
            if (self != nullptr)
            {
                if (Input::isToolActive(self->type, self->number))
                {
                    Input::toolCancel();
                }
                self = WindowManager::bringToFront(WindowType::vehicle, enumValue(head));
            }
            if (self == nullptr)
            {
                self = create(head);
                self->savedView.clear();
            }
            self->currentTab = 0;
            self->invalidate();
            self->widgets = widgets;
            self->enabledWidgets = enabledWidgets;
            self->holdableWidgets = holdableWidgets;
            self->eventHandlers = &events;
            self->activatedWidgets = 0;
            resetDisabledWidgets(self);
            self->initScrollWidgets();
            createViewport(*self);
            return self;
        }

        // 0x004B288F
        static void onChangeDirection(Window* const self)
        {
            if (Input::isToolActive(self->type, self->number, widx::pickup))
            {
                getGameState().pickupDirection = getGameState().pickupDirection ^ 1;
                return;
            }
            GameCommands::setErrorTitle(StringIds::cant_reverse_train);
            GameCommands::VehicleReverseArgs args{};
            args.head = static_cast<EntityId>(self->number);
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        static void onCentreViewportControl(Window* const self)
        {
            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::dropdown_viewport_move);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::dropdown_viewport_focus);

            Widget* widget = &self->widgets[widx::centreViewport];
            Dropdown::showText(
                self->x + widget->left,
                self->y + widget->top,
                widget->width(),
                widget->height(),
                self->getColour(WindowColour::secondary),
                2,
                0);

            Dropdown::setItemSelected(0);
            Dropdown::setHighlightedItem(0);
        }

        // 0x004B24D1
        static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::closeButton:
                    WindowManager::close(&self);
                    break;
                case Common::widx::caption:
                    Common::renameVehicle(&self, widgetIndex);
                    break;
                case Common::widx::tabMain:
                case Common::widx::tabDetails:
                case Common::widx::tabCargo:
                case Common::widx::tabFinances:
                case Common::widx::tabRoute:
                    Common::switchTab(&self, widgetIndex);
                    break;
                case widx::pickup:
                    Common::onPickup(&self, widx::pickup);
                    break;
                case widx::changeDirection:
                    onChangeDirection(&self);
                    break;
                case widx::passSignal:
                {
                    GameCommands::VehiclePassSignalArgs args{};
                    args.head = EntityId(self.number);

                    GameCommands::setErrorTitle(StringIds::cant_pass_signal_at_danger);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
        }

        // 0x004B30F3
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, self.number, Common::widx::tabMain);
            WindowManager::invalidateWidget(WindowType::vehicle, self.number, widx::status);
            WindowManager::invalidateWidget(WindowType::vehicle, self.number, widx::pickup);
            WindowManager::invalidateWidget(WindowType::vehicle, self.number, widx::passSignal);
            WindowManager::invalidateWidget(WindowType::vehicle, self.number, widx::changeDirection);

            if (self.isDisabled(widx::pickup))
            {
                Input::toolCancel(WindowType::vehicle, self.number);
                return;
            }

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            // If vehicle not placed put into pickup mode if window in focus
            if (head->isPlaced())
            {
                return;
            }

            if (!WindowManager::isInFront(&self))
                return;

            if (head->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, self.number))
            {
                Common::onPickup(&self, widx::pickup);
            }
        }

        // 0x004B3210
        static void onResize(Window& self)
        {
            Common::setCaptionEnableState(&self);
            self.setSize(kMinWindowSize, kMaxWindowSize);

            if (self.viewports[0] != nullptr)
            {
                auto head = Common::getVehicle(&self);
                if (head == nullptr)
                {
                    return;
                }
                uint16_t newWidth = self.width - 30;
                if (head->owner != CompanyManager::getControllingId())
                    newWidth += 22;

                uint16_t newHeight = self.height - 59;
                if (head->hasVehicleFlags(VehicleFlags::manualControl) && head->owner == CompanyManager::getControllingId())
                {
                    newWidth -= 27;
                }

                auto& viewport = self.viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    self.invalidate();
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->viewWidth = newWidth << viewport->zoom;
                    viewport->viewHeight = newHeight << viewport->zoom;
                    self.savedView.clear();
                }
            }
            createViewport(self);
        }

        // 0x004B274B
        static void stopStartOpen(Window* const self)
        {
            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::stop);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::start);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::manual);

            auto dropdownCount = 2;
            auto head = Common::getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            if (head->mode == TransportMode::rail && isDriverCheatEnabled())
            {
                dropdownCount = 3;
            }

            Widget* widget = &self->widgets[widx::stopStart];
            Dropdown::showText(
                self->x + widget->left,
                self->y + widget->top,
                widget->width(),
                widget->height(),
                self->getColour(WindowColour::secondary),
                dropdownCount,
                0);

            auto selected = 0; // Stop
            if (!head->hasVehicleFlags(VehicleFlags::commandStop))
            {
                selected = 1; // Start
            }
            if (head->hasVehicleFlags(VehicleFlags::manualControl))
            {
                selected = 2; // Manual
            }

            Dropdown::setItemSelected(selected);
            Dropdown::setHighlightedItem(selected == 0 ? 1 : 0); // Stop becomes start highlighted. Manual or Start becomes Stop highlighted
        }

        // 0x004B2637
        static void onSpeedControl(Window* const self)
        {
            Input::setClickRepeatTicks(31);
            auto pos = Input::getScrollLastLocation();
            auto speed = pos.y - (self->y + self->widgets[widx::speedControl].top + 58);
            speed = -(std::clamp(speed, -40, 40));

            GameCommands::VehicleSpeedControlArgs args{};
            args.head = EntityId(self->number);
            args.speed = speed;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        // 0x004B251A
        static void onMouseDown(Window& self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::stopStart:
                    stopStartOpen(&self);
                    break;
                case widx::speedControl:
                    onSpeedControl(&self);
                    break;
                case widx::centreViewport:
                    onCentreViewportControl(&self);
                    break;
            }
        }

        // 0x004B253A
        static void onStopStartDropdown(Window* const self, const int16_t itemIndex)
        {
            auto item = itemIndex == -1 ? Dropdown::getHighlightedItem() : itemIndex;
            if (item == -1 || item > 2)
            {
                return;
            }

            static const std::pair<StringId, VehicleChangeRunningModeArgs::Mode> itemToGameCommandInfo[3] = {
                { StringIds::cant_stop_string_id, VehicleChangeRunningModeArgs::Mode::stopVehicle },
                { StringIds::cant_start_string_id, VehicleChangeRunningModeArgs::Mode::startVehicle },
                { StringIds::cant_select_manual_mode_string_id, VehicleChangeRunningModeArgs::Mode::driveManually },
            };

            auto [errorTitle, mode] = itemToGameCommandInfo[item];
            GameCommands::setErrorTitle(errorTitle);
            FormatArguments args{};
            auto head = Common::getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            args.skip(6);
            args.push(head->name);
            args.push(head->ordinalNumber);

            VehicleChangeRunningModeArgs vargs{};
            vargs.head = EntityId(self->number);
            vargs.mode = mode;
            GameCommands::doCommand(vargs, GameCommands::Flags::apply);
        }

        static void onCentreViewportDropdown(Window* const self, const int16_t itemIndex)
        {
            if (itemIndex <= 0)
            {
                // Centre main window on vehicle, without locking.
                self->viewportCentreMain();
            }

            // Focus main viewport on vehicle
            if (itemIndex == 1)
            {
                auto vehHead = Common::getVehicle(self);
                if (vehHead == nullptr)
                {
                    return;
                }
                Vehicles::Vehicle train(*vehHead);
                EntityId targetEntity = train.veh2->id;

                // Focus viewport on vehicle, with locking.
                auto main = WindowManager::getMainWindow();
                main->viewportFocusOnEntity(targetEntity);
            }
        }

        // 0x004B253A
        static void onDropdown(Window& self, const WidgetIndex_t widgetIndex, const int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::stopStart:
                    onStopStartDropdown(&self, itemIndex);
                    break;
                case widx::centreViewport:
                    onCentreViewportDropdown(&self, itemIndex);
                    break;
            }
        }

        // 0x004B2545
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != widx::pickup)
            {
                return;
            }
            Common::pickupToolUpdate(self, x, y);
        }

        // 0x004B2550
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != widx::pickup)
            {
                return;
            }
            Common::pickupToolDown(self, x, y);
        }

        // 0x004B255B
        static void onToolAbort(Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex != widx::pickup)
            {
                return;
            }
            Common::pickupToolAbort(self);
        }

        // 0x004B31F2
        static std::optional<FormatArguments> tooltip(Ui::Window& self, WidgetIndex_t)
        {
            FormatArguments args{};

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return {};
            }
            args.skip(2);
            args.push(StringIds::getVehicleType(head->vehicleType));
            return args;
        }

        // 0x004B1EB5
        static void prepareDraw(Window& self)
        {
            if (self.widgets != widgets)
            {
                self.widgets = widgets;
                self.initScrollWidgets();
            }

            Common::setActiveTabs(&self);
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            Vehicles::Vehicle train(*head);

            self.widgets[widx::stopStart].type = WidgetType::buttonWithImage;
            self.widgets[widx::pickup].type = WidgetType::buttonWithImage;
            self.widgets[widx::passSignal].type = WidgetType::buttonWithImage;
            self.widgets[widx::changeDirection].type = WidgetType::buttonWithImage;

            if (head->mode != TransportMode::rail)
            {
                self.widgets[widx::passSignal].type = WidgetType::none;
            }

            if (head->mode == TransportMode::air || head->mode == TransportMode::water)
            {
                self.widgets[widx::changeDirection].type = WidgetType::none;
            }

            self.disabledWidgets &= ~interactiveWidgets;

            auto veh1 = train.veh1;
            if (train.cars.empty())
            {
                self.disabledWidgets |= (1 << widx::pickup);
            }

            if (veh1->var_3C >= 0x3689)
            {
                self.disabledWidgets |= (1 << widx::pickup) | (1 << widx::changeDirection);
            }

            if (head->mode == TransportMode::air || head->mode == TransportMode::water)
            {
                if (head->status != Vehicles::Status::stopped && head->status != Vehicles::Status::loading && head->tileX != -1)
                {
                    self.disabledWidgets |= (1 << widx::pickup);
                }
            }

            if (head->tileX == -1)
            {
                self.disabledWidgets |= (1 << widx::stopStart) | (1 << widx::passSignal) | (1 << widx::changeDirection) | (1 << widx::centreViewport);
                if (Input::isToolActive(WindowType::vehicle, self.number))
                {
                    self.disabledWidgets &= ~(1 << widx::changeDirection); //???
                }
            }

            if (head->status != Vehicles::Status::waitingAtSignal)
            {
                self.disabledWidgets |= (1 << widx::passSignal);
            }

            auto company = CompanyManager::get(head->owner);
            FormatArguments args{};
            if (CompanyManager::isPlayerCompany(head->owner))
            {
                args.push(StringIds::company_vehicle);
            }
            else
            {
                args.push(StringIds::competitor_vehicle);
            }
            args.push(company->name);
            args.skip(2);
            args.push(head->name);
            args.push(head->ordinalNumber);

            uint32_t stopStartImage = ImageIds::red_flag;
            if (head->hasVehicleFlags(VehicleFlags::manualControl))
            {
                stopStartImage = ImageIds::yellow_flag;
            }
            else if (head->hasVehicleFlags(VehicleFlags::commandStop))
            {
                stopStartImage = ImageIds::red_flag;
            }
            else
            {
                stopStartImage = ImageIds::green_flag;
            }
            self.widgets[widx::stopStart].image = stopStartImage;

            auto [pickupImage, pickupTooltip] = Common::getPickupImageIdandTooltip(*head, head->isPlaced());
            self.widgets[widx::pickup].image = Gfx::recolour(pickupImage);
            self.widgets[widx::pickup].tooltip = pickupTooltip;

            self.widgets[widx::speedControl].type = WidgetType::none;

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;
            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
            self.widgets[Common::widx::caption].right = self.width - 2;
            self.widgets[Common::widx::closeButton].left = self.width - 15;
            self.widgets[Common::widx::closeButton].right = self.width - 3;

            int viewportRight = self.width - 26;
            if (head->hasVehicleFlags(VehicleFlags::manualControl))
            {
                if (CompanyManager::isPlayerCompany(head->owner))
                {
                    viewportRight -= 27;
                    self.widgets[widx::speedControl].type = WidgetType::slider;
                }
            }

            self.widgets[widx::viewport].right = viewportRight;
            self.widgets[widx::viewport].bottom = self.height - 1 - 13;

            self.widgets[widx::status].top = self.height - 1 - 13 + 2;
            self.widgets[widx::status].bottom = self.height - 1 - 13 + 2 + 9;
            self.widgets[widx::status].right = self.width - 14;

            self.widgets[widx::stopStart].right = self.width - 2;
            self.widgets[widx::pickup].right = self.width - 2;
            self.widgets[widx::passSignal].right = self.width - 2;
            self.widgets[widx::changeDirection].right = self.width - 2;

            self.widgets[widx::stopStart].left = self.width - 2 - 23;
            self.widgets[widx::pickup].left = self.width - 2 - 23;
            self.widgets[widx::passSignal].left = self.width - 2 - 23;
            self.widgets[widx::changeDirection].left = self.width - 2 - 23;

            self.widgets[widx::speedControl].left = self.width - 2 - 23 - 26;
            self.widgets[widx::speedControl].right = self.width - 2 - 23 - 26 + 23;

            if (!CompanyManager::isPlayerCompany(head->owner))
            {
                self.widgets[widx::stopStart].type = WidgetType::none;
                self.widgets[widx::pickup].type = WidgetType::none;
                self.widgets[widx::passSignal].type = WidgetType::none;
                self.widgets[widx::changeDirection].type = WidgetType::none;
                self.widgets[widx::viewport].right += 22;
            }

            self.widgets[widx::centreViewport].right = self.widgets[widx::viewport].right - 1;
            self.widgets[widx::centreViewport].bottom = self.widgets[widx::viewport].bottom - 1;
            self.widgets[widx::centreViewport].left = self.widgets[widx::viewport].right - 1 - 23;
            self.widgets[widx::centreViewport].top = self.widgets[widx::viewport].bottom - 1 - 23;
            Widget::leftAlignTabs(self, Common::widx::tabMain, Common::widx::tabRoute);
        }

        // 0x004B226D
        static void draw(Window& self, Gfx::RenderTarget* const rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            Widget& pickupButton = self.widgets[widx::pickup];
            if (pickupButton.type != WidgetType::none)
            {
                if ((pickupButton.image & 0x20000000) != 0 && !self.isDisabled(widx::pickup))
                {
                    drawingCtx.drawImage(
                        rt,
                        self.x + pickupButton.left,
                        self.y + pickupButton.top,
                        Gfx::recolour(pickupButton.image, CompanyManager::getCompanyColour(self.owner)));
                }
            }

            auto veh = Common::getVehicle(&self);
            if (veh == nullptr)
            {
                return;
            }
            {
                auto status = veh->getStatus();
                FormatArguments args = {};
                args.push(status.status1);
                args.push(status.status1Args);
                args.push(status.status2);
                args.push(status.status2Args);

                StringId strFormat = StringIds::black_stringid_stringid;
                if (status.status2 == StringIds::null)
                {
                    strFormat = StringIds::black_stringid;
                }

                drawingCtx.drawStringLeftClipped(
                    *rt,
                    self.x + self.widgets[widx::status].left - 1,
                    self.y + self.widgets[widx::status].top - 1,
                    self.widgets[widx::status].width() - 1,
                    Colour::black,
                    strFormat,
                    &args);
            }

            Widget& speedWidget = self.widgets[widx::speedControl];
            if (speedWidget.type != WidgetType::none)
            {
                drawingCtx.drawImage(
                    rt,
                    self.x + speedWidget.left,
                    self.y + speedWidget.top + 10,
                    Gfx::recolour(ImageIds::speed_control_track, self.getColour(WindowColour::secondary).c()));

                drawingCtx.drawStringCentred(
                    *rt,
                    self.x + speedWidget.midX(),
                    self.y + speedWidget.top + 4,
                    Colour::black,
                    StringIds::tiny_power);

                drawingCtx.drawStringCentred(
                    *rt,
                    self.x + speedWidget.midX(),
                    self.y + speedWidget.bottom - 10,
                    Colour::black,
                    StringIds::tiny_brake);

                drawingCtx.drawImage(
                    rt,
                    self.x + speedWidget.left + 1,
                    self.y + speedWidget.top + 57 - veh->manualPower,
                    Gfx::recolour(ImageIds::speed_control_thumb, self.getColour(WindowColour::secondary).c()));
            }

            if (self.viewports[0] != nullptr)
            {
                self.drawViewports(rt);
                Widget::drawViewportCentreButton(rt, &self, widx::centreViewport);
            }
            else if (Input::isToolActive(self.type, self.number))
            {
                FormatArguments args = {};
                args.push(StringIds::getVehicleType(veh->vehicleType));
                Ui::Point origin;
                Widget& button = self.widgets[widx::viewport];
                origin.x = self.x + button.midX();
                origin.y = self.y + button.midY();
                drawingCtx.drawStringCentredWrapped(
                    *rt,
                    origin,
                    button.width() - 6,
                    Colour::black,
                    StringIds::click_on_view_select_string_id_start,
                    &args);
            }
        }

        static void initEvents()
        {
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onDropdown = onDropdown;
            events.onUpdate = onUpdate;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.onToolAbort = onToolAbort;
            events.textInput = Common::textInput;
            events.viewportRotate = createViewport;
            events.tooltip = tooltip;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace Details
    {
        // 0x4B60CC
        Window* open(const Vehicles::VehicleBase* vehicle)
        {
            auto self = Main::open(vehicle);
            if (self != nullptr)
            {
                self->callOnMouseUp(Common::widx::tabDetails);
            }
            return self;
        }

        static void cloneVehicle(Window* self)
        {
            static loco_global<EntityId, 0x0113642A> _113642A;
            auto head = Common::getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            GameCommands::setErrorTitle(StringIds::cant_clone_vehicle);
            if (GameCommands::do_80(head->head))
            {
                auto* newVehicle = EntityManager::get<Vehicles::VehicleBase>(_113642A);
                if (newVehicle != nullptr)
                {
                    OpenLoco::Ui::Windows::Vehicle::Details::open(newVehicle);
                }
            }
        }

        // 0x004B3823
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::closeButton:
                    WindowManager::close(&self);
                    break;
                case Common::widx::caption:
                    Common::renameVehicle(&self, widgetIndex);
                    break;
                case Common::widx::tabMain:
                case Common::widx::tabDetails:
                case Common::widx::tabCargo:
                case Common::widx::tabFinances:
                case Common::widx::tabRoute:
                    Common::switchTab(&self, widgetIndex);
                    break;
                case widx::pickup:
                    Common::onPickup(&self, widx::pickup);
                    break;
                case widx::remove:
                {
                    auto head = Common::getVehicle(&self);
                    if (head == nullptr)
                    {
                        break;
                    }
                    FormatArguments args{};
                    args.skip(10);
                    args.push(head->name);
                    args.push(head->ordinalNumber);
                    GameCommands::setErrorTitle(StringIds::cant_sell_string_id);

                    GameCommands::VehicleSellArgs gcArgs{};
                    gcArgs.car = head->id;

                    GameCommands::doCommand(gcArgs, GameCommands::Flags::apply);
                    break;
                }
            }
        }

        // 0x004B3D73
        static void onResize(Window& self)
        {
            Common::setCaptionEnableState(&self);
            self.setSize(kMinWindowSize, kMaxWindowSize);
        }

        static void onMouseDown(Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex != widx::buildNew)
                return;

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::dropdown_modify_vehicle);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::dropdown_clone_vehicle);

            Widget* widget = &self.widgets[widx::buildNew];
            Dropdown::showText(
                self.x + widget->left,
                self.y + widget->top,
                widget->width(),
                widget->height(),
                self.getColour(WindowColour::secondary),
                2,
                0);

            Dropdown::setItemSelected(0);
            Dropdown::setHighlightedItem(0);
        }

        // 0x004B253A
        static void onDropdown(Window& self, const WidgetIndex_t widgetIndex, const int16_t itemIndex)
        {
            if (widgetIndex != widx::buildNew)
                return;

            if (itemIndex <= 0)
            {
                BuildVehicle::open(self.number, 0);
            }
            else if (itemIndex == 1)
            {
                cloneVehicle(&self);
            }
        }

        // 0x004B3C45
        // "Show <vehicle> design details and options" tab in vehicle window
        static void onUpdate(Window& self)
        {
            if (EntityId(self.number) == _dragVehicleHead)
            {
                if (WindowManager::find(WindowType::dragVehiclePart) == nullptr)
                {
                    _dragVehicleHead = EntityId::null;
                    _dragCarComponent = nullptr;
                    self.invalidate();
                }
            }

            self.frameNo += 1;
            self.callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, self.number, Common::widx::tabDetails);

            if (_dragVehicleHead == EntityId::null && self.isActivated(widx::remove))
            {
                self.activatedWidgets &= ~(1ULL << widx::remove);
                WindowManager::invalidateWidget(WindowType::vehicle, self.number, widx::remove);
            }

            if (self.isDisabled(widx::pickup))
            {
                Input::toolCancel(WindowType::vehicle, self.number);
                return;
            }

            auto vehicle = Common::getVehicle(&self);
            if (vehicle == nullptr)
            {
                return;
            }
            if (vehicle->isPlaced())
                return;

            if (!WindowManager::isInFrontAlt(&self))
                return;

            if (vehicle->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, self.number))
            {
                Common::onPickup(&self, widx::pickup);
            }
        }

        // 0x004B385F
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != widx::pickup)
            {
                return;
            }
            Common::pickupToolUpdate(self, x, y);
        }

        // 0x004B386A
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != widx::pickup)
            {
                return;
            }
            Common::pickupToolDown(self, x, y);
        }

        // 0x004B3875
        static void onToolAbort(Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex != widx::pickup)
            {
                return;
            }
            Common::pickupToolAbort(self);
        }

        // 0x4B38FA
        static void getScrollSize(Ui::Window& self, [[maybe_unused]] const uint32_t scrollIndex, [[maybe_unused]] uint16_t* const width, uint16_t* const height)
        {
            *height = static_cast<uint16_t>(Common::getNumCars(&self) * self.rowHeight);
        }

        // 0x004B3B54
        static void scrollMouseDown(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
        {
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            if (head->owner != CompanyManager::getControllingId())
            {
                return;
            }

            auto car = Common::getCarFromScrollView(&self, y);
            if (!car)
            {
                return;
            }

            OpenLoco::Vehicles::Vehicle train{ *head };
            for (auto c : train.cars)
            {
                if (c.front == car->front)
                {
                    DragVehiclePart::open(c);
                    break;
                }
            }
        }

        // 0x004B399E
        static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
        {
            Input::setTooltipTimeout(2000);
            self.flags &= ~WindowFlags::notScrollView;
            auto car = Common::getCarFromScrollView(&self, y);
            StringId tooltipFormat = StringIds::null;
            EntityId tooltipContent = EntityId::null;
            if (car)
            {
                tooltipFormat = StringIds::buffer_337;
                tooltipContent = car->front->id;
            }
            if (EntityId(self.rowHover) != tooltipContent)
            {
                self.rowHover = enumValue(tooltipContent);
                self.invalidate();
            }

            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_337));
            if (StringManager::locoStrlen(buffer) != 0)
            {
                if (self.widgets[widx::carList].tooltip == tooltipFormat && self.var_85C == enumValue(tooltipContent))
                {
                    return;
                }
            }

            self.widgets[widx::carList].tooltip = tooltipFormat;
            self.var_85C = enumValue(tooltipContent);
            ToolTip::closeAndReset();

            if (tooltipContent == EntityId::null)
            {
                return;
            }

            ToolTip::set_52336E(true);

            auto vehicleObj = ObjectManager::get<VehicleObject>(car->front->objectId);
            {
                FormatArguments args{};
                args.push(vehicleObj->name);
                buffer = StringManager::formatString(buffer, StringIds::tooltip_stringid, &args);
            }

            {
                FormatArguments args{};
                args.push(car->front->creationDay);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_built, &args);
            }

            if (vehicleObj->power != 0 && (vehicleObj->mode == TransportMode::road || vehicleObj->mode == TransportMode::rail))
            {
                FormatArguments args{};
                args.push(vehicleObj->power);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_power, &args);
            }

            if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
            {
                FormatArguments args{};
                args.push<uint32_t>(StringManager::internalLengthToComma1DP(vehicleObj->getLength()));
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_length, &args);
            }

            {
                FormatArguments args{};
                args.push(vehicleObj->weight);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_weight, &args);
            }

            {
                FormatArguments args{};
                args.push(vehicleObj->speed);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_max_speed, &args);
            }

            if (vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
            {
                FormatArguments args{};
                args.push(vehicleObj->rackSpeed);
                auto rackRailObj = ObjectManager::get<TrackExtraObject>(vehicleObj->rackRailType);
                args.push(rackRailObj->name);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_speed_on_stringid, &args);
            }

            {
                FormatArguments args{};
                args.push(car->front->refundCost);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_value, &args);
            }

            if (car->front->reliability != 0)
            {
                FormatArguments args{};
                args.push(car->front->reliability / 256);
                buffer = StringManager::formatString(buffer, StringIds::vehicle_details_tooltip_reliability, &args);
            }
        }

        // 0x004B3880 TODO: common across 3 tabs
        static std::optional<FormatArguments> tooltip(Ui::Window& self, WidgetIndex_t)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_vehicle_list);

            auto vehicle = Common::getVehicle(&self);
            if (vehicle == nullptr)
            {
                return {};
            }
            args.push(StringIds::getVehicleType(vehicle->vehicleType));
            return args;
        }

        // 0x004B3B18
        static Ui::CursorId cursor(Window& self, const int16_t widgetIdx, [[maybe_unused]] const int16_t x, const int16_t y, const Ui::CursorId fallback)
        {
            if (widgetIdx != widx::carList)
            {
                return fallback;
            }

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return fallback;
            }
            if (head->owner != CompanyManager::getControllingId())
            {
                return fallback;
            }

            auto selectedCar = Common::getCarFromScrollView(&self, y);
            if (!selectedCar)
            {
                return fallback;
            }
            return CursorId::openHand;
        }

        static const std::map<VehicleType, uint32_t> additionalVehicleButtonByVehicleType{
            { VehicleType::train, InterfaceSkin::ImageIds::build_additional_train },
            { VehicleType::bus, InterfaceSkin::ImageIds::build_additional_bus },
            { VehicleType::truck, InterfaceSkin::ImageIds::build_additional_truck },
            { VehicleType::tram, InterfaceSkin::ImageIds::build_additional_tram },
            { VehicleType::aircraft, InterfaceSkin::ImageIds::build_additional_aircraft },
            { VehicleType::ship, InterfaceSkin::ImageIds::build_additional_ship },
        };

        constexpr auto kVehicleDetailsOffset = 2;
        constexpr auto kVehicleDetailsLineHeight = 12;
        constexpr auto kVehicleDetailsTextHeight2Lines = kVehicleDetailsOffset + kVehicleDetailsLineHeight * 2;
        constexpr auto kVehicleDetailsTextHeight3Lines = kVehicleDetailsOffset + kVehicleDetailsLineHeight * 3;

        static auto getVehicleDetailsHeight(const OpenLoco::TransportMode transportMode)
        {
            return transportMode == TransportMode::rail || transportMode == TransportMode::road ? kVehicleDetailsTextHeight3Lines : kVehicleDetailsTextHeight2Lines;
        }

        // 0x004B3300
        static void prepareDraw(Window& self)
        {
            if (self.widgets != widgets)
            {
                self.widgets = widgets;
                self.initScrollWidgets();
            }

            Common::setActiveTabs(&self);

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            auto args = FormatArguments();
            args.push(head->name);
            args.push(head->ordinalNumber);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;
            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
            self.widgets[Common::widx::caption].right = self.width - 2;
            self.widgets[Common::widx::closeButton].left = self.width - 15;
            self.widgets[Common::widx::closeButton].right = self.width - 3;
            Widget::leftAlignTabs(self, Common::widx::tabMain, Common::widx::tabRoute);

            self.widgets[widx::carList].right = self.width - 26;
            self.widgets[widx::carList].bottom = self.height - getVehicleDetailsHeight(head->getTransportMode());

            self.widgets[widx::buildNew].right = self.width - 2;
            self.widgets[widx::buildNew].left = self.width - 25;
            self.widgets[widx::pickup].right = self.width - 2;
            self.widgets[widx::pickup].left = self.width - 25;
            self.widgets[widx::remove].right = self.width - 2;
            self.widgets[widx::remove].left = self.width - 25;

            self.widgets[widx::buildNew].type = WidgetType::buttonWithImage;
            self.widgets[widx::pickup].type = WidgetType::buttonWithImage;
            self.widgets[widx::remove].type = WidgetType::buttonWithImage;
            // Differs to main tab! Unsure why.
            if (head->isPlaced())
            {
                self.widgets[widx::pickup].type = WidgetType::none;
            }
            if (head->owner != CompanyManager::getControllingId())
            {
                self.widgets[widx::buildNew].type = WidgetType::none;
                self.widgets[widx::pickup].type = WidgetType::none;
                self.widgets[widx::remove].type = WidgetType::none;
                self.widgets[widx::carList].right = self.width - 4;
            }

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            auto buildImage = skin->img + additionalVehicleButtonByVehicleType.at(head->vehicleType);

            self.widgets[widx::buildNew].image = Gfx::recolour(buildImage, CompanyManager::getCompanyColour(self.owner)) | Widget::kImageIdColourSet;

            Vehicles::Vehicle train{ *head };
            if (train.cars.empty())
            {
                self.disabledWidgets |= 1 << widx::pickup;
            }
            else
            {
                self.disabledWidgets &= ~(1ULL << widx::pickup);
            }

            auto [pickupImage, pickupTooltip] = Common::getPickupImageIdandTooltip(*head, head->isPlaced());
            self.widgets[widx::pickup].image = Gfx::recolour(pickupImage);
            self.widgets[widx::pickup].tooltip = pickupTooltip;
        }

        // 0x004B3542
        static void draw(Window& self, Gfx::RenderTarget* const rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            // TODO: identical to main tab (doesn't appear to do anything useful)
            if (self.widgets[widx::pickup].type != WidgetType::none)
            {
                if ((self.widgets[widx::pickup].image & (1 << 29)) && !self.isDisabled(widx::pickup))
                {
                    auto image = Gfx::recolour(self.widgets[widx::pickup].image, CompanyManager::getCompanyColour(self.owner));
                    drawingCtx.drawImage(rt, self.widgets[widx::pickup].left + self.x, self.widgets[widx::pickup].top + self.y, image);
                }
            }

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            OpenLoco::Vehicles::Vehicle train{ *head };
            Ui::Point pos = { static_cast<int16_t>(self.x + 3), static_cast<int16_t>(self.y + self.height - getVehicleDetailsHeight(head->getTransportMode()) + kVehicleDetailsOffset) };

            {
                FormatArguments args{};
                args.push(train.veh2->totalPower);
                args.push<uint32_t>(train.veh2->totalWeight);
                StringId str = StringIds::vehicle_details_weight;
                if (train.veh2->mode == TransportMode::rail || train.veh2->mode == TransportMode::road)
                {
                    str = StringIds::vehicle_details_total_power_and_weight;
                }
                drawingCtx.drawStringLeftClipped(*rt, pos.x, pos.y, self.width - 6, Colour::black, str, &args);
            }

            {
                pos.y += kVehicleDetailsLineHeight;
                FormatArguments args{};
                args.push<uint16_t>(train.veh2->maxSpeed == kSpeed16Null ? 0 : train.veh2->maxSpeed.getRaw());
                args.push<uint16_t>(train.veh2->rackRailMaxSpeed == kSpeed16Null ? 0 : train.veh2->rackRailMaxSpeed.getRaw());
                args.push<uint16_t>(train.veh2->reliability == 0 ? 64 : train.veh2->reliability);
                StringId str = StringIds::vehicle_details_max_speed_and_reliability;
                if (train.veh1->var_49 != 0)
                {
                    str = StringIds::vehicle_details_max_speed_and_rack_rail_and_reliability;
                }
                drawingCtx.drawStringLeftClipped(*rt, pos.x, pos.y, self.width - 16, Colour::black, str, &args);
            }

            {
                // Draw car count and vehicle length
                pos.y += kVehicleDetailsLineHeight;
                FormatArguments args = {};
                StringId str = StringIds::vehicle_length;
                args.push<uint32_t>(StringManager::internalLengthToComma1DP(head->getVehicleTotalLength()));
                if (train.veh2->mode == TransportMode::rail && head->getCarCount() > 1)
                {
                    str = StringIds::vehicle_car_count_and_length;
                    args.push<uint32_t>(head->getCarCount());
                }
                drawingCtx.drawStringLeftClipped(*rt, pos.x, pos.y, self.width - 16, Colour::black, str, &args);
            }
        }

        // 0x004B36A3
        static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t i)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            drawingCtx.clearSingle(rt, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            OpenLoco::Vehicles::Vehicle train{ *head };
            Ui::Point pos{ 0, 0 };
            for (auto& car : train.cars)
            {
                StringId carStr = StringIds::black_stringid;
                if (EntityId(self.rowHover) == car.front->id)
                {
                    carStr = StringIds::wcolour2_stringid;

                    int16_t top = pos.y;
                    int16_t bottom = pos.y + self.rowHeight - 1;
                    if (_dragCarComponent != nullptr)
                    {
                        bottom = pos.y;
                        top = pos.y - 1;
                        carStr = StringIds::black_stringid;
                    }
                    drawingCtx.fillRect(rt, 0, top, self.width, bottom, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                }

                int16_t y = pos.y + (self.rowHeight - 22) / 2;
                uint8_t al = 0;
                uint8_t ah = 0;
                if (car.front == _dragCarComponent)
                {
                    al = 12;
                    ah = self.getColour(WindowColour::secondary).u8();
                }
                auto x = Common::sub_4B743B(al, ah, 0, y, car.front, &rt);

                auto vehicleObj = ObjectManager::get<VehicleObject>(car.front->objectId);
                FormatArguments args{};
                args.push(vehicleObj->name);
                x += 2;
                y = pos.y + (self.rowHeight / 2) - 6;
                drawingCtx.drawStringLeft(rt, x, y, Colour::black, carStr, &args);

                pos.y += self.rowHeight;
            }

            if (EntityId(self.rowHover) == train.tail->id && _dragCarComponent != nullptr)
            {
                drawingCtx.fillRect(rt, 0, pos.y - 1, self.width, pos.y, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
            }
        }

        static void initEvents()
        {
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onDropdown = onDropdown;
            events.onUpdate = onUpdate;
            events.event_08 = Common::event8;
            events.event_09 = Common::event9;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.onToolAbort = onToolAbort;
            events.getScrollSize = getScrollSize;
            events.scrollMouseDown = scrollMouseDown;
            events.scrollMouseOver = scrollMouseOver;
            events.textInput = Common::textInput;
            events.tooltip = tooltip;
            events.cursor = cursor;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
            events.drawScroll = drawScroll;
        }

        static Ui::Window* getVehicleDetailsWindow(const Ui::Point& pos)
        {
            auto vehicleWindow = WindowManager::findAt(pos);
            if (vehicleWindow == nullptr || vehicleWindow->type != WindowType::vehicle || vehicleWindow->currentTab != (Common::widx::tabDetails - Common::widx::tabMain))
            {
                return nullptr;
            }
            return vehicleWindow;
        }

        static Vehicles::VehicleBase* getCarFromScrollViewPos(Ui::Window& self, const Ui::Point& pos)
        {
            Input::setPressedWidgetIndex(widx::carList);
            auto res = Ui::ScrollView::getPart(&self, &self.widgets[widx::carList], pos.x, pos.y);
            if (res.area != ScrollPart::view)
            {
                return nullptr;
            }

            auto y = self.rowHeight / 2 + res.scrollviewLoc.y;
            auto car = Common::getCarFromScrollView(&self, y);
            if (!car)
            {
                auto head = Common::getVehicle(&self);
                if (head == nullptr)
                {
                    return nullptr;
                }
                Vehicles::Vehicle train(*head);
                return train.tail;
            }
            return car->front;
        }

        void scrollDrag(const Ui::Point& pos)
        {
            auto vehicleWindow = getVehicleDetailsWindow(pos);
            if (vehicleWindow == nullptr)
            {
                return;
            }

            auto targetWidget = vehicleWindow->findWidgetAt(pos.x, pos.y);
            switch (targetWidget)
            {
                case widx::remove:
                {
                    if (!vehicleWindow->isActivated(widx::remove))
                    {
                        vehicleWindow->activatedWidgets |= (1 << widx::remove);
                        WindowManager::invalidateWidget(WindowType::vehicle, vehicleWindow->number, widx::remove);
                    }
                    return;
                }
                case widx::carList:
                {
                    auto car = getCarFromScrollViewPos(*vehicleWindow, pos);
                    if (car != nullptr)
                    {
                        vehicleWindow->flags &= ~WindowFlags::notScrollView;
                        if (car->id != EntityId(vehicleWindow->rowHover))
                        {
                            vehicleWindow->rowHover = enumValue(car->id);
                        }
                        vehicleWindow->invalidate();
                    }

                    // TODO: define constant for hot zone
                    if (pos.y < vehicleWindow->widgets[widx::carList].top + vehicleWindow->y + 5)
                    {
                        Ui::ScrollView::verticalNudgeUp(vehicleWindow, vehicleWindow->getScrollDataIndex(widx::carList), widx::carList);
                    }
                    else if (pos.y > vehicleWindow->widgets[widx::carList].bottom + vehicleWindow->y - 5)
                    {
                        Ui::ScrollView::verticalNudgeDown(vehicleWindow, vehicleWindow->getScrollDataIndex(widx::carList), widx::carList);
                    }
                    break;
                }
            }
            if (vehicleWindow->isActivated(widx::remove))
            {
                vehicleWindow->activatedWidgets &= ~(1ULL << widx::remove);
                WindowManager::invalidateWidget(WindowType::vehicle, vehicleWindow->number, widx::remove);
            }
        }

        void scrollDragEnd(const Ui::Point& pos)
        {
            if (_dragCarComponent == nullptr)
            {
                return;
            }

            auto vehicleWindow = getVehicleDetailsWindow(pos);
            if (vehicleWindow == nullptr)
            {
                return;
            }

            auto targetWidget = vehicleWindow->findWidgetAt(pos.x, pos.y);
            switch (targetWidget)
            {
                case widx::remove:
                {

                    GameCommands::VehicleSellArgs gcArgs{};
                    gcArgs.car = (*_dragCarComponent)->id;

                    GameCommands::doCommand(gcArgs, GameCommands::Flags::apply);
                    break;
                }
                case widx::carList:
                {
                    auto car = getCarFromScrollViewPos(*vehicleWindow, pos);
                    if (car != nullptr)
                    {
                        GameCommands::VehicleRearrangeArgs args{};
                        args.source = (*_dragCarComponent)->id;
                        args.dest = car->id;
                        GameCommands::doCommand(args, GameCommands::Flags::apply);
                    }
                    break;
                }
            }
        }
    }

    namespace Cargo
    {
        static void onRefitButton(Window* const self, const WidgetIndex_t wi);

        static bool canRefit(Vehicles::VehicleHead* headVehicle)
        {
            if (!CompanyManager::isPlayerCompany(headVehicle->owner))
            {
                return false;
            }

            OpenLoco::Vehicles::Vehicle train(*headVehicle);

            if (train.cars.empty())
            {
                return false;
            }

            auto object = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->objectId);
            return (object->hasFlags(VehicleObjectFlags::refittable));
        }

        // 004B3DDE
        static void prepareDraw(Window& self)
        {
            if (self.widgets != widgets)
            {
                self.widgets = widgets;
                self.initScrollWidgets();
            }

            Common::setActiveTabs(&self);

            auto* headVehicle = Common::getVehicle(&self);
            if (headVehicle == nullptr)
            {
                return;
            }
            FormatArguments args = {};
            args.push(headVehicle->name);
            args.push(headVehicle->ordinalNumber);

            widgets[Common::widx::frame].right = self.width - 1;
            widgets[Common::widx::frame].bottom = self.height - 1;
            widgets[Common::widx::panel].right = self.width - 1;
            widgets[Common::widx::panel].bottom = self.height - 1;
            widgets[Common::widx::caption].right = self.width - 2;
            widgets[Common::widx::closeButton].left = self.width - 15;
            widgets[Common::widx::closeButton].right = self.width - 3;
            widgets[widx::cargoList].right = self.width - 26;
            widgets[widx::cargoList].bottom = self.height - 27;
            widgets[widx::refit].right = self.width - 2;
            widgets[widx::refit].left = self.width - 25;
            widgets[widx::refit].type = WidgetType::buttonWithImage;
            if (!canRefit(headVehicle))
            {
                widgets[widx::refit].type = WidgetType::none;
                widgets[widx::cargoList].right = self.width - 26 + 22;
            }

            Widget::leftAlignTabs(self, Common::widx::tabMain, Common::widx::tabRoute);
        }

        // 004B3F0D
        static void draw(Ui::Window& self, Gfx::RenderTarget* const rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            // draw total cargo
            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            head->generateCargoTotalString(buffer);
            FormatArguments args = {};
            args.push<StringId>(StringIds::buffer_1250);
            drawingCtx.drawStringLeftClipped(*rt, self.x + 3, self.y + self.height - 25, self.width - 15, Colour::black, StringIds::total_stringid, &args);

            // draw cargo capacity
            buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            head->generateCargoCapacityString(buffer);
            args = {};
            args.push<StringId>(StringIds::buffer_1250);
            drawingCtx.drawStringLeftClipped(*rt, self.x + 3, self.y + self.height - 13, self.width - 15, Colour::black, StringIds::vehicle_capacity_stringid, &args);
        }

        // based on 0x004B40C7
        static void drawCargoText(Gfx::RenderTarget& rt, const int16_t x, int16_t& y, const StringId strFormat, uint8_t cargoQty, uint8_t cargoType, StationId stationId)
        {
            if (cargoQty == 0)
            {
                return;
            }

            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
            auto unitNameFormat = cargoQty == 1 ? cargoObj->unitNameSingular : cargoObj->unitNamePlural;
            auto station = StationManager::get(stationId);
            FormatArguments args{};
            args.push(StringIds::cargo_from);
            args.push(unitNameFormat);
            args.push<uint32_t>(cargoQty);
            args.push(station->name);
            args.push(station->town);
            drawingCtx.drawStringLeft(rt, x, y, Colour::black, strFormat, &args);
            y += 10;
        }

        // 004B3F62
        static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t i)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            drawingCtx.clearSingle(rt, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));
            auto* head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            Vehicles::Vehicle train{ *head };
            int16_t y = 0;
            for (auto& car : train.cars)
            {
                StringId strFormat = StringIds::black_stringid;
                auto front = car.front;
                auto body = car.body;
                if (front->id == EntityId(self.rowHover))
                {
                    drawingCtx.fillRect(rt, 0, y, self.width, y + self.rowHeight - 1, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                    strFormat = StringIds::wcolour2_stringid;
                }
                // Get width of the drawing
                auto width = Common::sub_4B743B(1, 0, 0, y, front, &rt);
                // Actually draw it
                width = Common::sub_4B743B(0, 0, 24 - width, (self.rowHeight - 22) / 2 + y, car.front, &rt);

                if (body->primaryCargo.type != 0xFF)
                {

                    int16_t cargoTextHeight = self.rowHeight / 2 + y - ((self.rowHeight - 22) / 2) - 10;
                    if (front->secondaryCargo.qty != 0 || body->primaryCargo.qty != 0)
                    {
                        if (body->primaryCargo.qty == 0 || front->secondaryCargo.qty == 0)
                        {
                            cargoTextHeight += 5;
                        }
                        drawCargoText(rt, width, cargoTextHeight, strFormat, body->primaryCargo.qty, body->primaryCargo.type, body->primaryCargo.townFrom);
                        drawCargoText(rt, width, cargoTextHeight, strFormat, front->secondaryCargo.qty, front->secondaryCargo.type, front->secondaryCargo.townFrom);
                    }
                    else
                    {
                        FormatArguments args{};
                        args.push<StringId>(StringIds::cargo_empty);
                        drawingCtx.drawStringLeft(rt, width, cargoTextHeight + 5, Colour::black, strFormat, &args);
                    }
                }

                y += self.rowHeight;
            }
        }

        // 004B41BD
        static void onMouseUp(Window& self, const WidgetIndex_t i)
        {
            switch (i)
            {
                case Common::widx::closeButton:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tabMain:
                case Common::widx::tabDetails:
                case Common::widx::tabCargo:
                case Common::widx::tabFinances:
                case Common::widx::tabRoute:
                    Common::switchTab(&self, i);
                    break;

                case Common::widx::caption:
                    Common::renameVehicle(&self, i);
                    break;
            }
        }

        // 004B41E2
        static void onMouseDown(Window& self, const WidgetIndex_t i)
        {
            switch (i)
            {
                case widx::refit:
                    onRefitButton(&self, i);
                    break;
            }
        }

        // 004B41E9
        static void onDropdown(Window& self, const WidgetIndex_t i, const int16_t dropdownIndex)
        {
            switch (i)
            {
                case widx::refit:
                {
                    if (dropdownIndex == -1)
                        break;

                    GameCommands::VehicleRefitArgs args{};
                    args.head = static_cast<EntityId>(self.number);
                    args.cargoType = Dropdown::getItemArgument(dropdownIndex, 3);

                    GameCommands::setErrorTitle(StringIds::cant_refit_vehicle);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
            }
        }

        static void onRefitButton(Window* const self, const WidgetIndex_t wi)
        {
            auto* head = Common::getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            Vehicles::Vehicle train(*head);
            auto vehicleObject = ObjectManager::get<VehicleObject>(train.cars.firstCar.front->objectId);
            auto maxPrimaryCargo = vehicleObject->maxCargo[0];
            auto primaryCargoId = Numerics::bitScanForward(vehicleObject->cargoTypes[0]);

            int32_t index = 0;
            for (uint16_t cargoId = 0; cargoId < ObjectManager::getMaxObjects(ObjectType::cargo); cargoId++)
            {
                auto cargoObject = ObjectManager::get<CargoObject>(cargoId);
                if (cargoObject == nullptr)
                    continue;

                if (!cargoObject->hasFlags(CargoObjectFlags::refit))
                    continue;

                StringId format = StringIds::dropdown_stringid;
                if (cargoId == train.cars.firstCar.body->primaryCargo.type)
                {
                    format = StringIds::dropdown_stringid_selected;
                }

                auto args = FormatArguments();
                args.push<StringId>(cargoObject->unitNamePlural);
                args.push<uint32_t>(Vehicles::getNumUnitsForCargo(maxPrimaryCargo, primaryCargoId, cargoId));
                args.push<uint16_t>(cargoId);
                Dropdown::add(index, format, args);
                index++;
            }

            Widget& button = self->widgets[wi];

            Dropdown::showText(
                self->x + button.left,
                self->y + button.top,
                button.width(),
                button.height(),
                self->getColour(WindowColour::secondary),
                index,
                0);
            Dropdown::setHighlightedItem(0);
        }

        // 0x004B4339
        static std::optional<FormatArguments> tooltip(Ui::Window& self, WidgetIndex_t)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_vehicle_list);

            auto vehicle = Common::getVehicle(&self);
            if (vehicle == nullptr)
            {
                return {};
            }
            args.push(StringIds::getVehicleType(vehicle->vehicleType));
            return args;
        }

        // 0x004B4360
        static void getScrollSize(Ui::Window& self, [[maybe_unused]] const uint32_t scrollIndex, [[maybe_unused]] uint16_t* const width, uint16_t* const height)
        {
            *height = static_cast<uint16_t>(Common::getNumCars(&self) * self.rowHeight);
        }

        static char* generateCargoTooltipDetails(char* buffer, const StringId cargoFormat, const uint8_t cargoType, const uint8_t maxCargo, const uint32_t acceptedCargoTypes)
        {
            if (cargoType == 0xFF)
            {
                return buffer;
            }

            {
                auto cargoObj = ObjectManager::get<CargoObject>(cargoType);
                FormatArguments args{};
                args.push(maxCargo == 1 ? cargoObj->unitNameSingular : cargoObj->unitNamePlural);
                args.push<uint32_t>(maxCargo);
                buffer = StringManager::formatString(buffer, cargoFormat, &args);
            }

            auto availableCargoTypes = acceptedCargoTypes & ~(1 << cargoType);
            if (availableCargoTypes != 0)
            {
                *buffer++ = ' ';
                *buffer++ = '(';

                while (availableCargoTypes != 0)
                {
                    auto type = Numerics::bitScanForward(availableCargoTypes);
                    availableCargoTypes &= ~(1 << type);

                    auto cargoObj = ObjectManager::get<CargoObject>(type);
                    FormatArguments args{};
                    args.push(cargoObj->name);
                    buffer = StringManager::formatString(buffer, StringIds::stats_or_string, &args);
                    *buffer++ = ' ';
                }
                --buffer;
                *buffer++ = ')';
                *buffer++ = '\0';
            }
            return buffer;
        }

        // 0x004B4404
        static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
        {
            Input::setTooltipTimeout(2000);
            self.flags &= ~WindowFlags::notScrollView;
            auto car = Common::getCarFromScrollView(&self, y);
            StringId tooltipFormat = StringIds::null;
            EntityId tooltipContent = EntityId::null;
            if (car)
            {
                tooltipFormat = StringIds::buffer_337;
                tooltipContent = car->front->id;
                if (EntityId(self.rowHover) != tooltipContent)
                {
                    self.rowHover = enumValue(tooltipContent);
                    self.invalidate();
                }
            }

            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_337));
            if (*buffer != '\0')
            {
                if (self.widgets[widx::cargoList].tooltip == tooltipFormat && EntityId(self.var_85C) == tooltipContent)
                {
                    return;
                }
            }

            self.widgets[widx::cargoList].tooltip = tooltipFormat;
            self.var_85C = enumValue(tooltipContent);
            ToolTip::closeAndReset();

            if (tooltipContent == EntityId::null)
            {
                return;
            }

            ToolTip::set_52336E(true);

            {
                auto vehicleObj = ObjectManager::get<VehicleObject>(car->front->objectId);
                FormatArguments args{};
                args.push(vehicleObj->name);
                buffer = StringManager::formatString(buffer, StringIds::cargo_capacity_tooltip, &args);
            }

            auto body = car->body;
            auto front = car->front;
            buffer = generateCargoTooltipDetails(buffer, StringIds::cargo_capacity, body->primaryCargo.type, body->primaryCargo.maxQty, body->primaryCargo.acceptedTypes);
            buffer = generateCargoTooltipDetails(buffer, StringIds::cargo_capacity_plus, front->secondaryCargo.type, front->secondaryCargo.maxQty, front->secondaryCargo.acceptedTypes);
        }

        // 0x004B4607
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, 6);
        }

        // 0x004B4621
        static void onResize(Window& self)
        {
            Common::setCaptionEnableState(&self);
            self.setSize(kMinWindowSize, kMaxWindowSize);
        }

        static void initEvents()
        {
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.draw = draw;
            events.drawScroll = drawScroll;
            events.prepareDraw = prepareDraw;
            events.onDropdown = onDropdown;
            events.textInput = Common::textInput;
            events.tooltip = tooltip;
            events.getScrollSize = getScrollSize;
            events.scrollMouseOver = scrollMouseOver;
            events.event_08 = Common::event8;
            events.event_09 = Common::event9;
            events.onUpdate = onUpdate;
            events.onResize = onResize;
        }
    }

    namespace Finances
    {
        // 0x004B56CE
        static void prepareDraw(Window& self)
        {
            if (self.widgets != widgets)
            {
                self.widgets = widgets;
                self.initScrollWidgets();
            }

            Common::setActiveTabs(&self);

            auto vehicle = Common::getVehicle(&self);
            if (vehicle == nullptr)
            {
                return;
            }
            auto args = FormatArguments();
            args.push(vehicle->name);
            args.push(vehicle->ordinalNumber);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;
            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
            self.widgets[Common::widx::caption].right = self.width - 2;
            self.widgets[Common::widx::closeButton].left = self.width - 15;
            self.widgets[Common::widx::closeButton].right = self.width - 3;

            Widget::leftAlignTabs(self, Common::widx::tabMain, Common::widx::tabRoute);
        }

        // 0x004B576C
        static void draw(Ui::Window& self, Gfx::RenderTarget* const rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto pos = Ui::Point(self.x + 4, self.y + 46);

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            Vehicles::Vehicle train(*head);
            auto veh1 = train.veh1;
            if (veh1->lastIncome.day != -1)
            {
                auto args = FormatArguments();
                args.push<uint32_t>(veh1->lastIncome.day);
                // Last income on: {DATE DMY}
                drawingCtx.drawStringLeft(*rt, pos.x, pos.y, Colour::black, StringIds::last_income_on_date, &args);
                pos.y += 10;
                for (int i = 0; i < 4; i++)
                {
                    auto cargoType = veh1->lastIncome.cargoTypes[i];
                    if (cargoType == 0xFF)
                        continue;

                    auto cargoObject = ObjectManager::get<CargoObject>(cargoType);

                    auto str = veh1->lastIncome.cargoQtys[i] == 1 ? cargoObject->unitNameSingular : cargoObject->unitNamePlural;

                    args = FormatArguments();
                    args.push(str);
                    args.push<uint32_t>(veh1->lastIncome.cargoQtys[i]);
                    args.push(veh1->lastIncome.cargoDistances[i]);
                    args.push<uint16_t>(veh1->lastIncome.cargoAges[i]);
                    args.push<currency32_t>(veh1->lastIncome.cargoProfits[i]);
                    // {STRINGID} transported {INT16} blocks in {INT16} days = {CURRENCY32}
                    drawingCtx.drawStringLeftWrapped(*rt, pos.x + 4, pos.y, self.width - 12, Colour::black, StringIds::transported_blocks_in_days, &args);

                    // TODO: fix function to take pointer to offset
                    pos.y += 12;
                }
            }
            else
            {
                // Last income: N/A"
                drawingCtx.drawStringLeft(*rt, pos.x, pos.y, Colour::black, StringIds::last_income_na);
                pos.y += 10;
            }

            pos.y += 5;

            if (head->lastAverageSpeed != 0)
            {
                // Last journey average speed: {VELOCITY}
                auto args = FormatArguments();
                args.push(head->lastAverageSpeed);
                drawingCtx.drawStringLeft(*rt, pos.x, pos.y, Colour::black, StringIds::last_journey_average_speed, &args);
                pos.y += 10 + 5;
            }

            {
                // Monthly Running Cost: {CURRENCY32}
                auto args = FormatArguments();
                args.push(head->calculateRunningCost());
                drawingCtx.drawStringLeft(*rt, pos.x, pos.y, Colour::black, StringIds::vehicle_monthly_running_cost, &args);
                pos.y += 10;
            }

            {
                // Monthly Profit: {CURRENCY32}
                auto args = FormatArguments();
                auto monthlyProfit = (train.veh2->totalRecentProfit()) / 4;
                args.push(monthlyProfit);
                drawingCtx.drawStringLeft(*rt, pos.x, pos.y, Colour::black, StringIds::vehicle_monthly_profit, &args);
                pos.y += 10 + 5;
            }

            {
                // Sale value of vehicle: {CURRENCY32}
                auto args = FormatArguments();
                args.push(train.head->totalRefundCost);
                pos.y = self.y + self.height - 14;
                drawingCtx.drawStringLeft(*rt, pos.x, pos.y, Colour::black, StringIds::sale_value_of_vehicle, &args);
            }
        }

        // 0x004B5945
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::closeButton:
                    WindowManager::close(&self);
                    break;
                case Common::widx::caption:
                    Common::renameVehicle(&self, widgetIndex);
                    break;
                case Common::widx::tabMain:
                case Common::widx::tabDetails:
                case Common::widx::tabCargo:
                case Common::widx::tabFinances:
                case Common::widx::tabRoute:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x004B5977
        static std::optional<FormatArguments> tooltip(Ui::Window& self, WidgetIndex_t)
        {
            FormatArguments args{};
            auto veh0 = Common::getVehicle(&self);
            if (veh0 == nullptr)
            {
                return {};
            }
            args.skip(2);
            args.push(StringIds::getVehicleType(veh0->vehicleType));
            return args;
        }

        // 0x004B5995
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, Common::widx::tabFinances);
        }

        // 0x004B59AF
        static void onResize(Window& self)
        {
            Common::setCaptionEnableState(&self);
            self.setSize(kMinWindowSize, kMaxWindowSize);
        }

        static void initEvents()
        {
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = onUpdate;
            events.textInput = Common::textInput;
            events.tooltip = tooltip;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace Route
    {
        static loco_global<uint8_t, 0x00113646A> _113646A;

        static Vehicles::OrderRingView getOrderTable(const Vehicles::VehicleHead* const head)
        {
            return Vehicles::OrderRingView(head->orderTableOffset);
        }

        // 0x004B509B
        static void close(Window& self)
        {
            if (Input::isToolActive(self.type, self.number))
            {
                Input::toolCancel();
            }
        }

        static void orderDeleteCommand(Vehicles::VehicleHead* const head, const uint32_t orderOffset)
        {
            GameCommands::VehicleOrderDeleteArgs args{};
            args.head = head->id;
            args.orderOffset = orderOffset - head->orderTableOffset;

            GameCommands::setErrorTitle(StringIds::empty);
            GameCommands::doCommand(args, GameCommands::Flags::apply);

            Vehicles::OrderManager::generateNumDisplayFrames(head);
        }

        // 0x004B4F6D
        static void onOrderDelete(Vehicles::VehicleHead* const head, const int16_t orderId)
        {
            // No deleteable orders
            if (head->sizeOfOrderTable <= 1)
            {
                return;
            }

            // orderId can be -1 at this point for none selected
            auto i = 0;
            const Vehicles::Order* last = nullptr;
            for (const auto& order : getOrderTable(head))
            {
                if (i == orderId)
                {
                    orderDeleteCommand(head, order.getOffset());
                    return;
                }
                last = &order;
                i++;
            }
            // No order selected so delete the last one
            if (last != nullptr)
            {
                orderDeleteCommand(head, last->getOffset());
            }
        }

        // 0x004B4C14
        static bool orderUpCommand(Vehicles::VehicleHead* const head, const uint32_t orderOffset)
        {
            GameCommands::VehicleOrderUpArgs args{};
            args.head = head->id;
            args.orderOffset = orderOffset - head->orderTableOffset;

            GameCommands::setErrorTitle(StringIds::empty);
            auto result = GameCommands::doCommand(args, GameCommands::Flags::apply);

            Vehicles::OrderManager::generateNumDisplayFrames(head); // Note: order changed, check if this matters.
            return result != GameCommands::FAILURE;
        }

        // 0x004B4CCB based on
        static bool orderDownCommand(Vehicles::VehicleHead* const head, const uint32_t orderOffset)
        {
            GameCommands::VehicleOrderDownArgs args{};
            args.head = head->id;
            args.orderOffset = orderOffset - head->orderTableOffset;

            GameCommands::setErrorTitle(StringIds::empty);
            auto result = GameCommands::doCommand(args, GameCommands::Flags::apply);

            Vehicles::OrderManager::generateNumDisplayFrames(head); // Note: order changed, check if this matters.
            return result != GameCommands::FAILURE;
        }

        static bool orderReverseCommand(Vehicles::VehicleHead* const head)
        {
            GameCommands::VehicleOrderReverseArgs args{};
            args.head = head->id;

            GameCommands::setErrorTitle(StringIds::empty);
            auto result = GameCommands::doCommand(args, GameCommands::Flags::apply);

            Vehicles::OrderManager::generateNumDisplayFrames(head);
            return result != GameCommands::FAILURE;
        }

        // 0x004B4BC1 / 0x004B4C78 based on
        static bool onOrderMove(Vehicles::VehicleHead* const head, const int16_t orderId, bool(orderMoveFunc)(Vehicles::VehicleHead*, uint32_t))
        {
            // No moveable orders
            if (head->sizeOfOrderTable <= 1)
                return false;
            // Valid orderId should be positive (avoid -1 / null)
            if (orderId < 0)
                return false;

            auto* order = getOrderTable(head).atIndex(orderId);
            if (order != nullptr)
            {
                return orderMoveFunc(head, order->getOffset());
            }
            return false;
        }

        // 0x004B4B43
        static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex)
        {
            auto* head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            switch (widgetIndex)
            {
                case Common::widx::closeButton:
                    WindowManager::close(&self);
                    break;
                case Common::widx::caption:
                    Common::renameVehicle(&self, widgetIndex);
                    break;
                case Common::widx::tabMain:
                case Common::widx::tabDetails:
                case Common::widx::tabCargo:
                case Common::widx::tabFinances:
                case Common::widx::tabRoute:
                    Common::switchTab(&self, widgetIndex);
                    break;
                case widx::orderDelete:
                {

                    onOrderDelete(head, self.var_842);
                    if (self.var_842 == -1)
                    {
                        return;
                    }

                    // Refresh selection (check if we are now at no order selected)
                    auto* order = getOrderTable(head).atIndex(self.var_842);

                    // If no order selected anymore
                    if (order == nullptr)
                    {
                        self.var_842 = -1;
                    }
                    break;
                }
                case widx::localMode:
                {
                    if (!CompanyManager::isPlayerCompany(head->owner))
                        return;

                    Vehicles::Vehicle train(*head);
                    if ((train.veh1->var_48 & Vehicles::Flags48::expressMode) != Vehicles::Flags48::none)
                    {
                        GameCommands::setErrorTitle(StringIds::empty);
                        VehicleChangeRunningModeArgs args{};
                        args.head = head->id;
                        args.mode = VehicleChangeRunningModeArgs::Mode::toggleLocalExpress;
                        GameCommands::doCommand(args, GameCommands::Flags::apply);
                    }
                    break;
                }
                case widx::expressMode:
                {
                    if (!CompanyManager::isPlayerCompany(head->owner))
                        return;

                    Vehicles::Vehicle train(*head);
                    if ((train.veh1->var_48 & Vehicles::Flags48::expressMode) == Vehicles::Flags48::none)
                    {
                        GameCommands::setErrorTitle(StringIds::empty);
                        VehicleChangeRunningModeArgs args{};
                        args.head = head->id;
                        args.mode = VehicleChangeRunningModeArgs::Mode::toggleLocalExpress;
                        GameCommands::doCommand(args, GameCommands::Flags::apply);
                    }
                    break;
                }
                case widx::orderSkip:
                {
                    GameCommands::setErrorTitle(StringIds::empty);
                    GameCommands::VehicleOrderSkipArgs args{};
                    args.head = EntityId(self.number);
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    break;
                }
                case widx::orderUp:
                    if (onOrderMove(head, self.var_842, orderUpCommand))
                    {
                        if (self.var_842 <= 0)
                        {
                            return;
                        }
                        self.var_842--;
                    }
                    break;
                case widx::orderDown:
                    if (onOrderMove(head, self.var_842, orderDownCommand))
                    {
                        if (self.var_842 < 0)
                        {
                            return;
                        }
                        auto* order = getOrderTable(head).atIndex(self.var_842);
                        if (order != nullptr)
                        {
                            self.var_842++;
                        }
                    }
                    break;
                case widx::orderReverse:
                {
                    orderReverseCommand(head);
                    break;
                }
            }
        }

        // 0x004B564E
        static void onResize(Window& self)
        {
            Common::setCaptionEnableState(&self);
            self.setSize(kMinWindowSize, kMaxWindowSize);
        }

        // 0x004B4DD3
        static void createOrderDropdown(Window* const self, const WidgetIndex_t i, const StringId orderType)
        {
            auto head = Common::getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            auto index = 0;
            for (uint16_t cargoId = 0; cargoId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoId)
            {
                if (!(head->var_4E & (1 << cargoId)))
                {
                    continue;
                }

                auto cargoObj = ObjectManager::get<CargoObject>(cargoId);
                FormatArguments args{};
                args.push(cargoObj->name);
                args.push(cargoObj->unitInlineSprite);
                args.push(cargoId);
                Dropdown::add(index, orderType, args);
                index++;
            }

            auto x = self->widgets[i].left + self->x;
            auto y = self->widgets[i].top + self->y;
            auto width = self->widgets[i].width();
            auto height = self->widgets[i].height();
            Dropdown::showText(x, y, width, height, self->getColour(WindowColour::secondary), index, 0);
            Dropdown::setHighlightedItem(0);
        }

        // 0x004B4B8C
        static void onMouseDown(Window& self, const WidgetIndex_t i)
        {
            switch (i)
            {
                case widx::orderForceUnload:
                    createOrderDropdown(&self, i, StringIds::orders_unload_all2);
                    break;
                case widx::orderWait:
                    createOrderDropdown(&self, i, StringIds::orders_wait_for_full_load_of2);
                    break;
            }
        }

        // order : al (first 3 bits)
        // order argument : eax (3 - 32 bits), cx
        // Note will move orders so do not use while iterating OrderRingView
        // 0x004B4ECB
        static void addNewOrder(Window* const self, const Vehicles::Order& order)
        {
            auto head = Common::getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            auto chosenOffset = head->sizeOfOrderTable - 1;
            if (self->var_842 != -1)
            {
                auto* chosenOrder = getOrderTable(head).atIndex(self->var_842);
                if (chosenOrder != nullptr)
                {
                    chosenOffset = chosenOrder->getOffset() - head->orderTableOffset;
                }
            }

            GameCommands::setErrorTitle(StringIds::orders_cant_insert);
            auto previousSize = head->sizeOfOrderTable;

            GameCommands::VehicleOrderInsertArgs args{};
            args.head = head->id;
            args.orderOffset = chosenOffset;
            args.rawOrder = order.getRaw();
            GameCommands::doCommand(args, GameCommands::Flags::apply);

            Vehicles::OrderManager::generateNumDisplayFrames(head);

            if (head->sizeOfOrderTable == previousSize)
            {
                return;
            }

            if (self->var_842 == -1)
            {
                return;
            }

            self->var_842++;
        }

        // 0x004B4BAC
        static void onDropdown(Window& self, const WidgetIndex_t i, const int16_t dropdownIndex)
        {
            auto item = dropdownIndex == -1 ? Dropdown::getHighlightedItem() : dropdownIndex;
            if (item == -1)
            {
                return;
            }
            switch (i)
            {
                case widx::orderForceUnload:
                {
                    Vehicles::OrderUnloadAll unload(Dropdown::getItemArgument(item, 3));
                    addNewOrder(&self, unload);
                    break;
                }
                case widx::orderWait:
                {
                    Vehicles::OrderWaitFor wait(Dropdown::getItemArgument(item, 3));
                    addNewOrder(&self, wait);
                    break;
                }
            }
        }

        // 0x004B55D1
        // "Show <vehicle> route details" tab in vehicle window
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, self.number, 8);

            if (!WindowManager::isInFront(&self))
                return;
            auto* head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            if (head->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, self.number))
            {
                if (Input::toolSet(&self, widx::tool, CursorId::crosshair))
                {
                    self.invalidate();
                    Vehicles::OrderManager::generateNumDisplayFrames(head);
                }
            }
        }

        // 0x004B4D74
        static std::optional<FormatArguments> tooltip(Ui::Window& self, WidgetIndex_t)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_orders_list);
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return {};
            }
            args.push(StringIds::getVehicleType(head->vehicleType));
            return args;
        }

        // 0x004B5A1A
        static std::pair<Ui::ViewportInteraction::InteractionItem, Ui::ViewportInteraction::InteractionArg> sub_4B5A1A(Window& self, const int16_t x, const int16_t y)
        {
            registers regs{};
            regs.esi = X86Pointer(&self);
            regs.ax = x;
            regs.cx = y;
            regs.bl = 0; // Not set during function but needed to indicate failure
            call(0x004B5A1A, regs);
            static loco_global<int16_t, 0x0113623C> _mapX;
            static loco_global<int16_t, 0x0113623E> _mapY;
            Ui::ViewportInteraction::InteractionArg output;
            output.pos.x = _mapX;
            output.pos.y = _mapY;
            output.object = reinterpret_cast<void*>(regs.edx);
            return std::make_pair(static_cast<Ui::ViewportInteraction::InteractionItem>(regs.bl), output);
        }

        // 0x004B5088
        static void toolCancel(Window& self, [[maybe_unused]] const WidgetIndex_t widgetIdx)
        {
            self.invalidate();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::unk_04);
            Gfx::invalidateScreen();
        }

        static void onToolDown(Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            auto [type, args] = sub_4B5A1A(self, x, y);
            switch (type)
            {
                case Ui::ViewportInteraction::InteractionItem::track:
                {
                    // 0x004B5160
                    auto tileElement = static_cast<TileElement*>(args.object);
                    auto trackElement = tileElement->as<TrackElement>();
                    if (trackElement == nullptr)
                        break;
                    auto height = trackElement->baseHeight();
                    auto trackId = trackElement->trackId();
                    const auto& trackPiece = World::TrackData::getTrackPiece(trackId);
                    const auto& trackPart = trackPiece[trackElement->sequenceIndex()];

                    auto offsetToFirstTile = Math::Vector::rotate(Pos2{ trackPart.x, trackPart.y }, trackElement->unkDirection());
                    auto firstTilePos = args.pos - offsetToFirstTile;
                    const auto tPos = World::toTileSpace(firstTilePos);
                    height -= trackPart.z;

                    Vehicles::OrderRouteWaypoint waypoint(tPos, height / 8, trackElement->unkDirection(), trackId);
                    Audio::playSound(Audio::SoundId::waypoint, Input::getDragLastLocation().x);
                    addNewOrder(&self, waypoint);
                    break;
                }
                case Ui::ViewportInteraction::InteractionItem::water:
                {
                    // Water
                    auto heights = TileManager::getHeight(args.pos);
                    auto height = heights.landHeight;
                    if (heights.waterHeight != 0)
                    {
                        height = heights.waterHeight;
                    }
                    Audio::playSound(Audio::SoundId::waypoint, Input::getDragLastLocation().x);

                    const auto tPos = World::toTileSpace(args.pos);
                    Vehicles::OrderRouteWaypoint waypoint(tPos, height / 8, 0, 0);
                    addNewOrder(&self, waypoint);
                    break;
                }
                case Ui::ViewportInteraction::InteractionItem::stationLabel:
                {
                    Audio::playSound(Audio::SoundId::waypoint, Input::getDragLastLocation().x);
                    const auto stationId = StationId(args.value);
                    Vehicles::OrderStopAt station(stationId);
                    addNewOrder(&self, station);
                    break;
                }
                case Ui::ViewportInteraction::InteractionItem::road:
                {
                    // 0x004B5223
                    auto* tileElement = static_cast<TileElement*>(args.object);
                    auto* roadElement = tileElement->as<RoadElement>();
                    if (roadElement == nullptr)
                        break;
                    auto height = roadElement->baseHeight();
                    auto roadId = roadElement->roadId();
                    const auto& roadPiece = World::TrackData::getRoadPiece(roadId);
                    const auto& roadPart = roadPiece[roadElement->sequenceIndex()];

                    auto offsetToFirstTile = Math::Vector::rotate(Pos2{ roadPart.x, roadPart.y }, roadElement->unkDirection());
                    auto firstTilePos = args.pos - offsetToFirstTile;
                    const auto tPos = World::toTileSpace(firstTilePos);
                    height -= roadPart.z;

                    Vehicles::OrderRouteWaypoint waypoint(tPos, height / 8, roadElement->unkDirection(), roadId);
                    Audio::playSound(Audio::SoundId::waypoint, Input::getDragLastLocation().x);
                    addNewOrder(&self, waypoint);
                    break;
                }

                default:
                    break;
            }
        }

        // 0x004B50CE
        static Ui::CursorId toolCursor(Window& self, const int16_t x, const int16_t y, const Ui::CursorId fallback, bool& out)
        {
            auto typeP = sub_4B5A1A(self, x, y);
            out = typeP.first != Ui::ViewportInteraction::InteractionItem::noInteraction;
            if (out)
            {
                return CursorId::inwardArrows;
            }
            return fallback;
        }

        // 0x004B4D9B
        static void getScrollSize(Ui::Window& self, [[maybe_unused]] const uint32_t scrollIndex, [[maybe_unused]] uint16_t* const width, uint16_t* const height)
        {
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            auto table = getOrderTable(head);
            *height = lineHeight * std::distance(table.begin(), table.end());

            // Space for the 'end of orders' item
            *height += lineHeight;
        }

        static void scrollMouseDown(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
        {
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            auto item = y / lineHeight;
            Vehicles::Order* selectedOrder = getOrderTable(head).atIndex(item);
            if (selectedOrder == nullptr)
            {
                item = -1;
            }

            auto toolWindow = Input::toolGetActiveWindow();
            // If another vehicle window is open and has focus (tool)
            if (toolWindow != nullptr && toolWindow->type == self.type && toolWindow->number != self.number)
            {
                if (item == -1)
                {
                    // Copy complete order list
                    Audio::playSound(Audio::SoundId::waypoint, Input::getDragLastLocation().x);
                    std::vector<std::shared_ptr<Vehicles::Order>> clonedOrders;
                    for (auto& existingOrders : getOrderTable(head))
                    {
                        clonedOrders.push_back(existingOrders.clone());
                    }
                    for (auto& order : clonedOrders)
                    {
                        addNewOrder(toolWindow, *order);
                    }
                    WindowManager::bringToFront(toolWindow);
                }
                else
                {
                    // Copy a single entry on the order list
                    Audio::playSound(Audio::SoundId::waypoint, Input::getDragLastLocation().x);
                    auto clonedOrder = selectedOrder->clone();
                    addNewOrder(toolWindow, *clonedOrder);
                    WindowManager::bringToFront(toolWindow);
                }
                return;
            }

            if (item != self.var_842)
            {
                self.var_842 = item;
                self.invalidate();
                return;
            }

            if (selectedOrder == nullptr)
            {
                return;
            }
            switch (selectedOrder->getType())
            {
                case Vehicles::OrderType::StopAt:
                case Vehicles::OrderType::RouteThrough:
                {
                    auto* stationOrder = static_cast<Vehicles::OrderStation*>(selectedOrder);
                    auto station = StationManager::get(stationOrder->getStation());
                    auto main = WindowManager::getMainWindow();
                    if (main)
                    {
                        main->viewportCentreOnTile({ station->x, station->y, static_cast<coord_t>(station->z + 32) });
                    }

                    break;
                }
                case Vehicles::OrderType::RouteWaypoint:
                {
                    auto* routeOrder = selectedOrder->as<Vehicles::OrderRouteWaypoint>();
                    if (routeOrder != nullptr)
                    {
                        auto main = WindowManager::getMainWindow();
                        if (main)
                        {
                            auto position = routeOrder->getWaypoint();
                            position.x += 16;
                            position.y += 16;
                            position.z += 32;
                            main->viewportCentreOnTile(position);
                        }
                    }
                    break;
                }
                case Vehicles::OrderType::UnloadAll:
                case Vehicles::OrderType::WaitFor:
                case Vehicles::OrderType::End:
                    // These orders don't have a location to centre on
                    break;
            }
        }

        // 0x004B530C
        static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
        {
            self.flags &= ~WindowFlags::notScrollView;
            auto item = y / lineHeight;
            if (self.rowHover != item)
            {
                self.rowHover = item;
                self.invalidate();
            }
        }

        // 0x004B5339
        static Ui::CursorId cursor(Window& self, const int16_t widgetIdx, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y, const Ui::CursorId fallback)
        {
            if (widgetIdx != widx::routeList)
            {
                return fallback;
            }

            if (Input::isToolActive(self.type, self.number))
            {
                return CursorId::inwardArrows;
            }
            return fallback;
        }

        // 0x004B56B8 TODO Rename
        static void createViewport(Window& self)
        {
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            Vehicles::OrderManager::generateNumDisplayFrames(head);
        }

        // 0x004B468C
        static void prepareDraw(Window& self)
        {
            if (self.widgets != widgets)
            {
                self.widgets = widgets;
                self.initScrollWidgets();
            }

            Common::setActiveTabs(&self);
            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            FormatArguments args{};
            args.push(head->name);
            args.push(head->ordinalNumber);

            self.widgets[widx::routeList].tooltip = Input::isToolActive(self.type, self.number) ? StringIds::tooltip_route_scrollview_copy : StringIds::tooltip_route_scrollview;

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;
            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
            self.widgets[Common::widx::caption].right = self.width - 2;
            self.widgets[Common::widx::closeButton].left = self.width - 15;
            self.widgets[Common::widx::closeButton].right = self.width - 3;

            self.widgets[widx::routeList].right = self.width - 26;
            self.widgets[widx::routeList].bottom = self.height - 14;

            self.widgets[widx::orderForceUnload].right = self.width - 2;
            self.widgets[widx::orderWait].right = self.width - 2;
            self.widgets[widx::orderSkip].right = self.width - 2;
            self.widgets[widx::orderDelete].right = self.width - 2;
            self.widgets[widx::orderUp].right = self.width - 2;
            self.widgets[widx::orderDown].right = self.width - 2;
            self.widgets[widx::orderReverse].right = self.width - 2;

            self.widgets[widx::orderForceUnload].left = self.width - 25;
            self.widgets[widx::orderWait].left = self.width - 25;
            self.widgets[widx::orderSkip].left = self.width - 25;
            self.widgets[widx::orderDelete].left = self.width - 25;
            self.widgets[widx::orderUp].left = self.width - 25;
            self.widgets[widx::orderDown].left = self.width - 25;
            self.widgets[widx::orderReverse].left = self.width - 25;

            self.disabledWidgets |= (1 << widx::orderForceUnload) | (1 << widx::orderWait) | (1 << widx::orderSkip) | (1 << widx::orderDelete);
            if (head->sizeOfOrderTable != 1)
            {
                self.disabledWidgets &= ~((1 << widx::orderSkip) | (1 << widx::orderDelete));
            }
            if (head->var_4E != 0)
            {
                self.disabledWidgets &= ~((1 << widx::orderWait) | (1 << widx::orderForceUnload));
            }

            // Express / local
            self.activatedWidgets &= ~((1 << widx::expressMode) | (1 << widx::localMode));
            Vehicles::Vehicle train(*head);
            if ((train.veh1->var_48 & Vehicles::Flags48::expressMode) != Vehicles::Flags48::none)
                self.activatedWidgets |= (1 << widx::expressMode);
            else
                self.activatedWidgets |= (1 << widx::localMode);

            WidgetType type = head->owner == CompanyManager::getControllingId() ? WidgetType::buttonWithImage : WidgetType::none;
            self.widgets[widx::orderForceUnload].type = type;
            self.widgets[widx::orderWait].type = type;
            self.widgets[widx::orderSkip].type = type;
            self.widgets[widx::orderDelete].type = type;
            self.widgets[widx::orderUp].type = type;
            self.widgets[widx::orderDown].type = type;
            self.widgets[widx::orderReverse].type = type;

            if (type == WidgetType::none)
            {
                self.widgets[widx::routeList].right += 22;
                self.enabledWidgets &= ~(1 << widx::expressMode | 1 << widx::localMode);
            }
            else
            {
                self.enabledWidgets |= (1 << widx::expressMode | 1 << widx::localMode);
            }

            self.widgets[widx::expressMode].right = self.widgets[widx::routeList].right;
            self.widgets[widx::expressMode].left = (self.widgets[widx::expressMode].right - 3) / 2 + 3;
            self.widgets[widx::localMode].right = self.widgets[widx::expressMode].left - 1;

            self.disabledWidgets |= (1 << widx::orderUp) | (1 << widx::orderDown);
            if (self.var_842 != -1)
            {
                self.disabledWidgets &= ~((1 << widx::orderUp) | (1 << widx::orderDown));
            }
            Widget::leftAlignTabs(self, Common::widx::tabMain, Common::widx::tabRoute);
        }

        // 0x004B4866
        static void draw(Window& self, Gfx::RenderTarget* const rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            if (Input::isToolActive(WindowType::vehicle, self.number))
            {
                // Location at bottom left edge of window
                Ui::Point loc{ static_cast<int16_t>(self.x + 3), static_cast<int16_t>(self.y + self.height - 13) };

                drawingCtx.drawStringLeftClipped(*rt, loc.x, loc.y, self.width - 14, Colour::black, StringIds::route_click_on_waypoint);
            }
        }

        const std::array<StringId, 6> orderString = {
            {
                StringIds::orders_end,
                StringIds::orders_stop_at,
                StringIds::orders_route_through,
                StringIds::orders_route_thought_waypoint,
                StringIds::orders_unload_all,
                StringIds::orders_wait_for_full_load_of,
            }
        };

        static constexpr std::array<uint32_t, 63> kNumberCircle = {
            {
                ImageIds::number_circle_01,
                ImageIds::number_circle_02,
                ImageIds::number_circle_03,
                ImageIds::number_circle_04,
                ImageIds::number_circle_05,
                ImageIds::number_circle_06,
                ImageIds::number_circle_07,
                ImageIds::number_circle_08,
                ImageIds::number_circle_09,
                ImageIds::number_circle_10,
                ImageIds::number_circle_11,
                ImageIds::number_circle_12,
                ImageIds::number_circle_13,
                ImageIds::number_circle_14,
                ImageIds::number_circle_15,
                ImageIds::number_circle_16,
                ImageIds::number_circle_17,
                ImageIds::number_circle_18,
                ImageIds::number_circle_19,
                ImageIds::number_circle_20,
                ImageIds::number_circle_21,
                ImageIds::number_circle_22,
                ImageIds::number_circle_23,
                ImageIds::number_circle_24,
                ImageIds::number_circle_25,
                ImageIds::number_circle_26,
                ImageIds::number_circle_27,
                ImageIds::number_circle_28,
                ImageIds::number_circle_29,
                ImageIds::number_circle_30,
                ImageIds::number_circle_31,
                ImageIds::number_circle_32,
                ImageIds::number_circle_33,
                ImageIds::number_circle_34,
                ImageIds::number_circle_35,
                ImageIds::number_circle_36,
                ImageIds::number_circle_37,
                ImageIds::number_circle_38,
                ImageIds::number_circle_39,
                ImageIds::number_circle_40,
                ImageIds::number_circle_41,
                ImageIds::number_circle_42,
                ImageIds::number_circle_43,
                ImageIds::number_circle_44,
                ImageIds::number_circle_45,
                ImageIds::number_circle_46,
                ImageIds::number_circle_47,
                ImageIds::number_circle_48,
                ImageIds::number_circle_49,
                ImageIds::number_circle_50,
                ImageIds::number_circle_51,
                ImageIds::number_circle_52,
                ImageIds::number_circle_53,
                ImageIds::number_circle_54,
                ImageIds::number_circle_55,
                ImageIds::number_circle_56,
                ImageIds::number_circle_57,
                ImageIds::number_circle_58,
                ImageIds::number_circle_59,
                ImageIds::number_circle_60,
                ImageIds::number_circle_61,
                ImageIds::number_circle_62,
                ImageIds::number_circle_63,
            }
        };

        // 0x004B4A58 based on
        static void sub_4B4A58(Window& self, Gfx::RenderTarget& rt, const StringId strFormat, FormatArguments& args, Vehicles::Order& order, int16_t& y)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Ui::Point loc = { 8, static_cast<int16_t>(y - 1) };
            drawingCtx.drawStringLeft(rt, &loc, Colour::black, strFormat, &args);
            if (order.hasFlags(Vehicles::OrderFlags::HasNumber))
            {
                if (Input::isToolActive(self.type, self.number))
                {
                    auto imageId = kNumberCircle[_113646A - 1];
                    drawingCtx.drawImage(&rt, loc.x + 3, loc.y + 1, Gfx::recolour(imageId, Colour::white));
                }
                _113646A++;
            }
        }

        // 0x004B48BA
        static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t i)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            drawingCtx.clearSingle(rt, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

            auto head = Common::getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            Vehicles::Vehicle train(*head);

            auto rowNum = 0;
            if (head->sizeOfOrderTable == 1)
            {
                drawingCtx.drawStringLeft(rt, 8, 0, Colour::black, StringIds::no_route_defined);
                rowNum++; // Used to move down the text
            }

            _113646A = 1; // Number ?symbol? TODO: make not a global
            for (auto& order : getOrderTable(head))
            {
                int16_t y = rowNum * lineHeight;
                auto strFormat = StringIds::black_stringid;
                if (self.var_842 == rowNum)
                {
                    drawingCtx.fillRect(rt, 0, y, self.width, y + 9, PaletteIndex::index_0A, Drawing::RectFlags::none);
                    strFormat = StringIds::white_stringid;
                }
                if (self.rowHover == rowNum)
                {
                    strFormat = StringIds::wcolour2_stringid;
                    drawingCtx.fillRect(rt, 0, y, self.width, y + 9, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                }

                FormatArguments args{};
                args.push(orderString[static_cast<uint8_t>(order.getType())]);
                switch (order.getType())
                {
                    case Vehicles::OrderType::End:
                    case Vehicles::OrderType::RouteWaypoint:
                        // Fall through
                        break;
                    case Vehicles::OrderType::StopAt:
                    case Vehicles::OrderType::RouteThrough:
                    {
                        auto* stationOrder = static_cast<Vehicles::OrderStation*>(&order);
                        stationOrder->setFormatArguments(args);
                        break;
                    }
                    case Vehicles::OrderType::UnloadAll:
                    case Vehicles::OrderType::WaitFor:
                    {

                        auto* cargoOrder = static_cast<Vehicles::OrderCargo*>(&order);
                        cargoOrder->setFormatArguments(args);
                        break;
                    }
                }

                sub_4B4A58(self, rt, strFormat, args, order, y);
                if (head->currentOrder + head->orderTableOffset == order.getOffset())
                {
                    drawingCtx.drawStringLeft(rt, 1, y - 1, Colour::black, StringIds::orders_current_order);
                }

                rowNum++;
            }

            // Output the end of orders
            Ui::Point loc = { 8, static_cast<int16_t>(rowNum * lineHeight) };
            auto strFormat = StringIds::black_stringid;
            if (self.var_842 == rowNum)
            {
                drawingCtx.fillRect(rt, 0, loc.y, self.width, loc.y + lineHeight, PaletteIndex::index_0A, Drawing::RectFlags::none);
                strFormat = StringIds::white_stringid;
            }
            if (self.rowHover == rowNum)
            {
                strFormat = StringIds::wcolour2_stringid;
                drawingCtx.fillRect(rt, 0, loc.y, self.width, loc.y + lineHeight, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
            }

            loc.y -= 1;
            auto args = FormatArguments::common(orderString[0]);
            drawingCtx.drawStringLeft(rt, &loc, Colour::black, strFormat, &args);
        }

        static void initEvents()
        {
            events.onClose = close;
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onDropdown = onDropdown;
            events.onUpdate = onUpdate;
            events.event_08 = Common::event8;
            events.event_09 = Common::event9;
            events.onToolDown = onToolDown;
            events.onToolAbort = toolCancel;
            events.toolCursor = toolCursor;
            events.getScrollSize = getScrollSize;
            events.scrollMouseDown = scrollMouseDown;
            events.scrollMouseOver = scrollMouseOver;
            events.textInput = Common::textInput;
            events.viewportRotate = createViewport;
            events.tooltip = tooltip;
            events.cursor = cursor;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
            events.drawScroll = drawScroll;
        }
    }

    namespace Common
    {

        struct TabInformation
        {
            const widx widgetIndex;
            Widget* widgets;
            WindowEventList* events;
            const uint64_t* enabledWidgets;
            const uint64_t* holdableWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { widx::tabMain, Main::widgets, &Main::events, &Main::enabledWidgets, &Main::holdableWidgets },
            { widx::tabDetails, Details::widgets, &Details::events, &Details::enabledWidgets, &Details::holdableWidgets },
            { widx::tabCargo, Cargo::widgets, &Cargo::events, &Cargo::enabledWidgets, &Cargo::holdableWidgets },
            { widx::tabFinances, Finances::widgets, &Finances::events, &Finances::enabledWidgets, &Finances::holdableWidgets },
            { widx::tabRoute, Route::widgets, &Route::events, &Route::enabledWidgets, &Route::holdableWidgets }
        };

        static void setActiveTabs(Window* const self)
        {
            self->activatedWidgets &= ~((1 << widx::tabMain) | (1 << widx::tabDetails) | (1 << widx::tabCargo) | (1 << widx::tabFinances) | (1 << widx::tabRoute));
            self->activatedWidgets |= 1ULL << (widx::tabMain + self->currentTab);
        }

        static std::pair<uint32_t, StringId> getPickupImageIdandTooltip(const Vehicles::VehicleHead& head, const bool isPlaced)
        {
            uint32_t image = 0;
            StringId tooltip = 0;
            switch (head.mode)
            {
                case TransportMode::rail:
                {
                    auto trackObj = ObjectManager::get<TrackObject>(head.trackType);
                    image = trackObj->image + (isPlaced ? 16 : 17);
                    tooltip = isPlaced ? StringIds::tooltip_remove_from_track : StringIds::tooltip_place_on_track;
                    break;
                }
                case TransportMode::road:
                {
                    auto roadObjId = head.trackType == LastGameOptionManager::kNoLastOption ? LastGameOptionManager::getLastTrackType() : head.trackType;
                    auto roadObj = ObjectManager::get<RoadObject>(roadObjId);
                    image = roadObj->image + (isPlaced ? 32 : 33);
                    tooltip = isPlaced ? StringIds::tooltip_remove_from_track : StringIds::tooltip_place_on_track;
                    break;
                }
                case TransportMode::air:
                {
                    image = isPlaced ? ImageIds::airport_pickup : ImageIds::airport_place;
                    tooltip = isPlaced ? StringIds::tooltip_remove_from_airport : StringIds::tooltip_place_on_airport;
                    break;
                }
                case TransportMode::water:
                {
                    auto waterObj = ObjectManager::get<WaterObject>();
                    image = waterObj->image + (isPlaced ? 58 : 59);
                    tooltip = isPlaced ? StringIds::tooltip_remove_from_water : StringIds::tooltip_place_on_dock;
                    break;
                }
            }
            return std::make_pair(image, tooltip);
        }

        // 0x004B26C0
        static void textInput(Window& self, const WidgetIndex_t callingWidget, const char* const input)
        {
            if (callingWidget != widx::caption)
            {
                return;
            }

            if (strlen(input) == 0)
            {
                return;
            }

            GameCommands::setErrorTitle(StringIds::cant_rename_this_vehicle);
            GameCommands::VehicleRenameArgs args{};
            args.head = EntityId(self.number);
            std::memcpy(args.buffer, input, 36);
            args.i = 1;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            args.head = EntityId(0);
            args.i = 2;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
            args.i = 0;
            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        // 0x0050029C
        static constexpr std::array<std::array<CursorId, 2>, 6> kTypeToToolCursor = {
            {
                { { CursorId::placeTrain, CursorId::placeTrainAlt } },
                { { CursorId::placeBus, CursorId::placeBusAlt } },
                { { CursorId::placeTruck, CursorId::placeTruckAlt } },
                { { CursorId::placeTram, CursorId::placeTramAlt } },
                { { CursorId::placePlane, CursorId::placePlane } },
                { { CursorId::placeShip, CursorId::placeShip } },
            }
        };

        // 0x00427595
        static std::optional<GameCommands::VehicleWaterPlacementArgs> getVehicleWaterPlacementArgsFromCursor(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
            if (!pos)
            {
                return {};
            }

            // Search 8x8 area centerd on mouse pos
            const auto centerPos = *pos + World::Pos2(16, 16);
            World::Pos2 initialPos = *pos - World::toWorldSpace(World::TilePos2(4, 4));
            int32_t bestDistance = std::numeric_limits<int32_t>::max();
            World::Pos3 bestLoc{};

            for (tile_coord_t i = 0; i < 8; ++i)
            {
                for (tile_coord_t j = 0; j < 8; ++j)
                {
                    const auto loc = initialPos + World::toWorldSpace(World::TilePos2{ i, j });
                    if (!World::validCoords(loc))
                    {
                        continue;
                    }

                    auto tile = World::TileManager::get(loc);
                    for (auto& el : tile)
                    {
                        auto* elStation = el.as<StationElement>();
                        if (elStation == nullptr)
                        {
                            continue;
                        }

                        if (elStation->stationType() != StationType::docks)
                        {
                            continue;
                        }

                        if (elStation->multiTileIndex() != 0)
                        {
                            continue;
                        }

                        auto firstTile = loc - World::kOffsets[elStation->multiTileIndex()];
                        auto* dockObject = ObjectManager::get<DockObject>(elStation->objectId());
                        auto boatLoc = firstTile + World::toWorldSpace(TilePos2{ 1, 1 }) + Math::Vector::rotate(dockObject->boatPosition, elStation->rotation());

                        auto distance = Math::Vector::manhattanDistance(boatLoc, centerPos);
                        if (distance < bestDistance)
                        {
                            bestDistance = distance;
                            bestLoc = World::Pos3(loc.x, loc.y, elStation->baseHeight());
                        }
                    }
                }
            }

            if (bestDistance == std::numeric_limits<int32_t>::max())
            {
                return {};
            }

            GameCommands::VehicleWaterPlacementArgs args;
            args.pos = bestLoc;
            args.head = head.id;
            return { args };
        }

        static void removeBoatGhost(const Vehicles::VehicleHead& head)
        {
            // Note: dont use isPlaced as we need to know if its a ghost
            // consider creating isGhostPlaced
            if (head.tileX != -1 && head.has38Flags(Vehicles::Flags38::isGhost))
            {
                GameCommands::VehiclePickupWaterArgs gcArgs{};
                gcArgs.head = head.id;
                auto flags = GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::ghost;
                GameCommands::doCommand(gcArgs, flags);
            }
            _1136264 = -1;
        }

        // 0x004B2B9E
        static void pickupToolUpdateWater(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto placementArgs = getVehicleWaterPlacementArgsFromCursor(head, x, y);

            if (!placementArgs)
            {
                removeBoatGhost(head);
                return;
            }

            if (_1136264 == 0 && *_ghostVehiclePos == placementArgs->pos)
            {
                return;
            }
            _ghostVehiclePos = placementArgs->pos;
            removeBoatGhost(head);
            if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply | GameCommands::Flags::ghost | GameCommands::Flags::noErrorWindow) != GameCommands::FAILURE)
            {
                _1136264 = 0;
            }
        }

        // 0x00426D52
        // used to return NodeMovementFlags on ebx
        static std::optional<World::Pos3> getAirportMovementNodeLoc(const StationId stationId, uint8_t node)
        {
            auto* station = StationManager::get(stationId);
            auto tile = TileManager::get(World::Pos2{ station->unk_tile_x, station->unk_tile_y });
            World::StationElement* elStation = nullptr;
            for (auto& el : tile)
            {
                elStation = el.as<StationElement>();
                if (elStation == nullptr)
                {
                    continue;
                }

                if (elStation->baseZ() != station->unk_tile_z / 4)
                {
                    elStation = nullptr;
                    continue;
                }
                break;
            }

            if (elStation == nullptr)
            {
                return {};
            }

            auto* airportObj = ObjectManager::get<AirportObject>(elStation->objectId());
            const auto& movementNode = airportObj->movementNodes[node];
            auto nodeOffset = Math::Vector::rotate(World::Pos2(movementNode.x, movementNode.y) - World::Pos2(16, 16), elStation->rotation()) + World::Pos2(16, 16);
            auto nodeLoc = World::Pos3{ nodeOffset.x, nodeOffset.y, movementNode.y } + World::Pos3{ station->unk_tile_x, station->unk_tile_y, station->unk_tile_z };
            if (!movementNode.hasFlags(AirportMovementNodeFlags::taxiing))
            {
                nodeLoc.z = station->unk_tile_z + 255;
                if (!movementNode.hasFlags(AirportMovementNodeFlags::inFlight))
                {
                    nodeLoc.z = 30 * 32;
                }
            }
            return { nodeLoc };
        }

        // 0x00426F0B
        static std::optional<GameCommands::VehicleAirPlacementArgs> getVehicleAirPlacementArgsFromCursor(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~ViewportInteraction::InteractionItemFlags::station);
            auto* elStation = static_cast<World::StationElement*>(res.first.object);
            if (res.first.type != ViewportInteraction::InteractionItem::airport)
            {
                res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::surface | ViewportInteraction::InteractionItemFlags::water));
                const auto& interaction = res.first;
                if (interaction.type == ViewportInteraction::InteractionItem::noInteraction)
                {
                    return {};
                }

                bool stationFound = false;
                for (auto& station : StationManager::stations())
                {
                    if ((station.flags & StationFlags::flag_6) == StationFlags::none)
                    {
                        continue;
                    }
                    if ((station.flags & StationFlags::flag_5) != StationFlags::none)
                    {
                        continue;
                    }

                    if (std::abs(interaction.pos.x - station.unk_tile_x) > 5 * World::kTileSize)
                    {
                        continue;
                    }
                    if (std::abs(interaction.pos.y - station.unk_tile_y) > 5 * World::kTileSize)
                    {
                        continue;
                    }

                    auto tile = TileManager::get(World::Pos2{ station.unk_tile_x, station.unk_tile_y });
                    for (auto& el : tile)
                    {
                        elStation = el.as<StationElement>();
                        if (elStation == nullptr)
                        {
                            continue;
                        }

                        if (elStation->isFlag5() || elStation->isGhost())
                        {
                            continue;
                        }

                        if (elStation->stationType() != StationType::airport)
                        {
                            continue;
                        }

                        stationFound = true;
                        break;
                    }
                }

                if (!stationFound)
                {
                    return {};
                }
            }

            if (elStation->isFlag5() || elStation->isGhost())
            {
                return {};
            }

            GameCommands::VehicleAirPlacementArgs placementArgs;
            placementArgs.stationId = elStation->stationId();
            placementArgs.head = head.id;
            auto* airportObj = ObjectManager::get<AirportObject>(elStation->objectId());

            int32_t bestDistance = std::numeric_limits<int32_t>::max();
            uint8_t bestNode = 0;
            for (auto node = airportObj->numMovementNodes - 1; node > -1; node--)
            {
                const auto& movementNode = airportObj->movementNodes[node];
                if (!movementNode.hasFlags(AirportMovementNodeFlags::terminal))
                {
                    continue;
                }
                auto nodeLoc = getAirportMovementNodeLoc(placementArgs.stationId, node);
                if (!nodeLoc)
                {
                    continue;
                }

                auto viewPos = World::gameToScreen(*nodeLoc, res.second->getRotation());
                auto uiPos = res.second->viewportToScreen(viewPos);
                auto distance = Math::Vector::manhattanDistance(uiPos, Point{ x, y });
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestNode = node;
                }
            }

            if (bestDistance == std::numeric_limits<int32_t>::max())
            {
                return {};
            }

            placementArgs.airportNode = bestNode;
            return { placementArgs };
        }

        static void removeAirplaneGhost(const Vehicles::VehicleHead& head)
        {
            // Note: dont use isPlaced as we need to know if its a ghost
            // consider creating isGhostPlaced
            if (head.tileX != -1 && head.has38Flags(Vehicles::Flags38::isGhost))
            {
                GameCommands::VehiclePickupAirArgs gcArgs{};
                gcArgs.head = head.id;
                auto flags = GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::ghost;
                GameCommands::doCommand(gcArgs, flags);
            }
            _ghostAirportStationId = StationId::null;
        }

        // 0x004B2AFA
        static void pickupToolUpdateAir(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto placementArgs = getVehicleAirPlacementArgsFromCursor(head, x, y);
            if (!placementArgs)
            {
                removeAirplaneGhost(head);
                return;
            }

            if (_ghostAirportStationId != StationId::null && *_ghostAirportStationId == placementArgs->stationId && *_ghostAirportNode == placementArgs->airportNode)
            {
                return;
            }

            removeAirplaneGhost(head);
            if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply | GameCommands::Flags::ghost | GameCommands::Flags::noErrorWindow) != GameCommands::FAILURE)
            {
                _ghostAirportNode = placementArgs->airportNode;
                _ghostAirportStationId = placementArgs->stationId;
            }
        }

        // 0x004A43E4
        static uint16_t getRoadProgressAtCursor(const Point& cursorLoc, Ui::Viewport& viewport, const RoadElement& roadElement, const World::Pos3& loc)
        {
            // Get the coordinates of the first tile of the possibly multitile road
            const auto& roadDataArr = World::TrackData::getRoadPiece(roadElement.roadId());
            const auto& roadData = roadDataArr[roadElement.sequenceIndex()];
            auto roadOffset2 = Math::Vector::rotate(World::Pos2(roadData.x, roadData.y), roadElement.unkDirection());
            auto roadOffset = World::Pos3(roadOffset2.x, roadOffset2.y, roadData.z);
            auto roadFirstTile = loc - roadOffset;

            // Get the movement info for this specific road id
            uint16_t trackAndDirection = roadElement.unkDirection() | (roadElement.roadId() << 3);
            const auto moveInfoArr = World::TrackData::getRoadPlacementSubPositon(trackAndDirection);

            // This iterates the movement info trying to find the distance along the road that is as close as possible
            // to the cursors location.
            int32_t bestDistance = std::numeric_limits<int32_t>::max();
            uint16_t bestProgress = 0;
            for (const auto& moveInfo : moveInfoArr)
            {
                auto potentialLoc = roadFirstTile + moveInfo.loc;
                auto viewPos = World::gameToScreen(potentialLoc, viewport.getRotation());
                auto uiPos = viewport.viewportToScreen(viewPos);
                auto distance = Math::Vector::manhattanDistance(uiPos, cursorLoc);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestProgress = std::distance(&*moveInfoArr.begin(), &moveInfo);
                }
            }
            return bestProgress;
        }

        // 0x00478415
        static std::optional<GameCommands::VehiclePlacementArgs> getRoadAtCursor(const int16_t x, const int16_t y)
        {
            auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~ViewportInteraction::InteractionItemFlags::roadAndTram);
            if (interaction.type != ViewportInteraction::InteractionItem::road)
            {
                return {};
            }

            // Get the best progress along the road relative to the cursor
            auto* roadElement = static_cast<World::RoadElement*>(interaction.object);
            World::Pos3 loc(interaction.pos.x, interaction.pos.y, roadElement->baseHeight());
            auto progress = getRoadProgressAtCursor({ x, y }, *viewport, *roadElement, loc);

            // Get the coordinates of the first tile of the possibly multitile road
            const auto& roadDataArr = World::TrackData::getRoadPiece(roadElement->roadId());
            const auto& roadData = roadDataArr[roadElement->sequenceIndex()];
            auto roadOffset2 = Math::Vector::rotate(World::Pos2(roadData.x, roadData.y), roadElement->unkDirection());
            auto roadOffset = World::Pos3(roadOffset2.x, roadOffset2.y, roadData.z);
            auto roadFirstTile = loc - roadOffset;

            GameCommands::VehiclePlacementArgs placementArgs;
            placementArgs.pos = roadFirstTile;
            placementArgs.trackProgress = progress;
            placementArgs.trackAndDirection = roadElement->unkDirection() | (roadElement->roadId() << 3);
            return { placementArgs };
        }

        // 0x00479707
        static std::optional<GameCommands::VehiclePlacementArgs> getVehicleRoadPlacementArgsFromCursor(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto placementArgs = getRoadAtCursor(x, y);
            if (!placementArgs)
            {
                return {};
            }

            placementArgs->head = head.id;
            const auto moveInfoArr = World::TrackData::getRoadPlacementSubPositon(placementArgs->trackAndDirection);
            const auto& moveInfo = moveInfoArr[placementArgs->trackProgress];

            uint8_t unkYaw = moveInfo.yaw + (WindowManager::getCurrentRotation() << 4);
            unkYaw -= 0x37;

            if (getGameState().pickupDirection != 0)
            {
                unkYaw -= 0x20;
            }
            unkYaw &= 0x3F;
            if (unkYaw <= 0x20)
            {
                const auto& unkItem = TrackData::getUnkRoad(placementArgs->trackAndDirection);
                placementArgs->pos += unkItem.pos;
                if (unkItem.rotationEnd < 12)
                {
                    placementArgs->pos -= World::Pos3{ World::kRotationOffset[unkItem.rotationEnd], 0 };
                }
                placementArgs->trackProgress = std::max<uint16_t>(static_cast<uint16_t>(moveInfoArr.size()) - placementArgs->trackProgress, 0);
                if (placementArgs->trackProgress >= moveInfoArr.size())
                {
                    placementArgs->trackProgress = static_cast<uint16_t>(moveInfoArr.size()) - 1;
                }
                placementArgs->trackAndDirection ^= (1 << 2);
            }
            return placementArgs;
        }

        // 0x004A43E4
        static uint16_t getTrackProgressAtCursor(const Point& cursorLoc, Ui::Viewport& viewport, const TrackElement& trackElement, const World::Pos3& loc)
        {
            // Get the coordinates of the first tile of the possibly multitile track
            const auto& trackDataArr = World::TrackData::getTrackPiece(trackElement.trackId());
            const auto& trackData = trackDataArr[trackElement.sequenceIndex()];
            auto trackOffset2 = Math::Vector::rotate(World::Pos2(trackData.x, trackData.y), trackElement.unkDirection());
            auto trackOffset = World::Pos3(trackOffset2.x, trackOffset2.y, trackData.z);
            auto trackFirstTile = loc - trackOffset;

            // Get the movement info for this specific track id
            uint16_t trackAndDirection = trackElement.unkDirection() | (trackElement.trackId() << 3);
            const auto moveInfoArr = World::TrackData::getTrackSubPositon(trackAndDirection);

            // This iterates the movement info trying to find the distance along the track that is as close as possible
            // to the cursors location.
            int32_t bestDistance = std::numeric_limits<int32_t>::max();
            uint16_t bestProgress = 0;
            for (const auto& moveInfo : moveInfoArr)
            {
                auto potentialLoc = trackFirstTile + moveInfo.loc;
                auto viewPos = World::gameToScreen(potentialLoc, viewport.getRotation());
                auto uiPos = viewport.viewportToScreen(viewPos);
                auto distance = Math::Vector::manhattanDistance(uiPos, cursorLoc);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestProgress = std::distance(&*moveInfoArr.begin(), &moveInfo);
                }
            }
            return bestProgress;
        }

        // 0x004A40C5
        static std::optional<GameCommands::VehiclePlacementArgs> getTrackAtCursor(const int16_t x, const int16_t y)
        {
            auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~ViewportInteraction::InteractionItemFlags::track);
            if (interaction.type != ViewportInteraction::InteractionItem::track)
            {
                return {};
            }

            // Get the best progress along the track relative to the cursor
            auto* trackElement = static_cast<World::TrackElement*>(interaction.object);
            World::Pos3 loc(interaction.pos.x, interaction.pos.y, trackElement->baseHeight());
            auto progress = getTrackProgressAtCursor({ x, y }, *viewport, *trackElement, loc);

            // Get the coordinates of the first tile of the possibly multitile road
            const auto& trackDataArr = World::TrackData::getTrackPiece(trackElement->trackId());
            const auto& trackData = trackDataArr[trackElement->sequenceIndex()];
            auto trackOffset2 = Math::Vector::rotate(World::Pos2(trackData.x, trackData.y), trackElement->unkDirection());
            auto trackOffset = World::Pos3(trackOffset2.x, trackOffset2.y, trackData.z);
            auto trackFirstTile = loc - trackOffset;

            GameCommands::VehiclePlacementArgs placementArgs;
            placementArgs.pos = trackFirstTile;
            placementArgs.trackProgress = progress;
            placementArgs.trackAndDirection = trackElement->unkDirection() | (trackElement->trackId() << 3);
            return { placementArgs };
        }

        // 0x004B6444
        static std::optional<GameCommands::VehiclePlacementArgs> getVehicleRailPlacementArgsFromCursor(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto placementArgs = getTrackAtCursor(x, y);
            if (!placementArgs)
            {
                return {};
            }

            placementArgs->head = head.id;
            const auto moveInfoArr = World::TrackData::getTrackSubPositon(placementArgs->trackAndDirection);
            const auto& moveInfo = moveInfoArr[placementArgs->trackProgress];

            uint8_t unkYaw = moveInfo.yaw + (WindowManager::getCurrentRotation() << 4);
            unkYaw -= 0x37;
            if (getGameState().pickupDirection != 0)
            {
                unkYaw -= 0x20;
            }
            unkYaw &= 0x3F;
            if (unkYaw <= 0x20)
            {
                const auto& unkItem = TrackData::getUnkTrack(placementArgs->trackAndDirection);
                placementArgs->pos += unkItem.pos;
                if (unkItem.rotationEnd < 12)
                {
                    placementArgs->pos -= World::Pos3{ World::kRotationOffset[unkItem.rotationEnd], 0 };
                }
                placementArgs->trackProgress = std::max<uint16_t>(static_cast<uint16_t>(moveInfoArr.size()) - placementArgs->trackProgress, 0);
                if (placementArgs->trackProgress >= moveInfoArr.size())
                {
                    placementArgs->trackProgress = static_cast<uint16_t>(moveInfoArr.size()) - 1;
                }
                placementArgs->trackAndDirection ^= (1 << 2);
            }
            return placementArgs;
        }

        static void removeLandGhost(const Vehicles::VehicleHead& head)
        {
            // Note: dont use isPlaced as we need to know if its a ghost
            // consider creating isGhostPlaced
            if (head.tileX != -1 && head.has38Flags(Vehicles::Flags38::isGhost))
            {
                GameCommands::VehiclePickupArgs args{};
                args.head = head.id;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::ghost);
            }
            _1136264 = -1;
        }

        // 0x004B2A1D
        template<typename GetPlacementArgsFunc>
        static void pickupToolUpdateLand(const Vehicles::VehicleHead& head, const int16_t x, const int16_t y, GetPlacementArgsFunc&& getPlacementArgs)
        {
            auto placementArgs = getPlacementArgs(head, x, y);
            if (!placementArgs)
            {
                removeLandGhost(head);
                return;
            }

            if (_1136264 != -1 && *_ghostLandTrackAndDirection == placementArgs->trackAndDirection && *_ghostVehiclePos == placementArgs->pos && *_1136264 == placementArgs->trackProgress)
            {
                return;
            }

            removeLandGhost(head);
            if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply | GameCommands::Flags::ghost | GameCommands::Flags::noErrorWindow) != GameCommands::FAILURE)
            {
                _ghostLandTrackAndDirection = placementArgs->trackAndDirection;
                _ghostVehiclePos = placementArgs->pos;
                _1136264 = placementArgs->trackProgress;
            }
        }

        // 0x004B29C0
        static void pickupToolUpdate(Window& self, const int16_t x, const int16_t y)
        {
            auto* head = getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            ToolManager::setToolCursor(kTypeToToolCursor[static_cast<uint8_t>(head->vehicleType)][getGameState().pickupDirection != 0 ? 1 : 0]);

            switch (head->mode)
            {
                case TransportMode::rail:
                    pickupToolUpdateLand(*head, x, y, getVehicleRailPlacementArgsFromCursor);
                    break;
                case TransportMode::road:
                    pickupToolUpdateLand(*head, x, y, getVehicleRoadPlacementArgsFromCursor);
                    break;
                case TransportMode::air:
                    pickupToolUpdateAir(*head, x, y);
                    break;
                case TransportMode::water:
                    pickupToolUpdateWater(*head, x, y);
                    break;
            }
        }

        // 0x004B2D8A
        static void pickupToolError(const Vehicles::VehicleHead& head)
        {
            auto args = FormatArguments::common();
            args.skip(6);
            args.push(head.name);
            args.push(head.ordinalNumber);
            Error::open(StringIds::cant_place_string_id_here, StringIds::null);
        }

        static void pickupToolPlacementCommandCallback(uint32_t gameCommandResult, Window& self, EntityId vehicleHead)
        {
            if (gameCommandResult == GameCommands::FAILURE)
            {
                return;
            }

            if (Input::hasKeyModifier(Input::KeyModifier::shift))
            {
                VehicleChangeRunningModeArgs args{};
                args.head = vehicleHead;
                args.mode = VehicleChangeRunningModeArgs::Mode::startVehicle;
                GameCommands::doCommand(args, GameCommands::Flags::apply);
            }

            Input::toolCancel();
            self.callOnMouseUp(Common::widx::tabMain);
        }

        // 0x004B2E18
        static void pickupToolDownAir(Window& self, const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto placementArgs = getVehicleAirPlacementArgsFromCursor(head, x, y);
            if (!placementArgs)
            {
                pickupToolError(head);
                return;
            }

            if (*_ghostAirportStationId == placementArgs->stationId && *_ghostAirportNode == placementArgs->airportNode)
            {
                if (head.tileX != -1 && head.has38Flags(Vehicles::Flags38::isGhost))
                {
                    // Will convert inplace vehicle into non ghost
                    placementArgs->convertGhost = true;
                }
            }
            if (!placementArgs->convertGhost)
            {
                removeAirplaneGhost(head);
            }
            auto args = FormatArguments::common();
            args.skip(6);
            args.push(head.name);
            args.push(head.ordinalNumber);
            GameCommands::setErrorTitle(StringIds::cant_place_string_id_here);

            auto result = GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply);
            pickupToolPlacementCommandCallback(result, self, head.head);
        }

        // 0x004B2F1C
        static void pickupToolDownWater(Window& self, const Vehicles::VehicleHead& head, const int16_t x, const int16_t y)
        {
            auto placementArgs = getVehicleWaterPlacementArgsFromCursor(head, x, y);
            if (!placementArgs)
            {
                pickupToolError(head);
                return;
            }

            if (_1136264 == 0 && *_ghostVehiclePos == placementArgs->pos)
            {
                if (head.tileX != -1 && head.has38Flags(Vehicles::Flags38::isGhost))
                {
                    // Will convert inplace vehicle into non ghost
                    placementArgs->convertGhost = true;
                }
            }
            if (!placementArgs->convertGhost)
            {
                removeBoatGhost(head);
            }
            auto args = FormatArguments::common();
            args.skip(6);
            args.push(head.name);
            args.push(head.ordinalNumber);
            GameCommands::setErrorTitle(StringIds::cant_place_string_id_here);

            auto result = GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply);
            pickupToolPlacementCommandCallback(result, self, head.head);
        }

        // 0x004B2C95
        template<typename GetPlacementArgsFunc>
        static void pickupToolDownLand(Window& self, const Vehicles::VehicleHead& head, const int16_t x, const int16_t y, GetPlacementArgsFunc&& getPlacementArgs)
        {
            auto placementArgs = getPlacementArgs(head, x, y);
            if (!placementArgs)
            {
                pickupToolError(head);
                return;
            }

            if (*_ghostLandTrackAndDirection == placementArgs->trackAndDirection && *_ghostVehiclePos == placementArgs->pos && *_1136264 == placementArgs->trackProgress)
            {
                if (head.tileX != -1 && head.has38Flags(Vehicles::Flags38::isGhost))
                {
                    // Will convert inplace vehicle into non ghost
                    placementArgs->convertGhost = true;
                }
            }
            if (!placementArgs->convertGhost)
            {
                removeLandGhost(head);
            }
            auto args = FormatArguments::common();
            args.skip(6);
            args.push(head.name);
            args.push(head.ordinalNumber);
            GameCommands::setErrorTitle(StringIds::cant_place_string_id_here);

            auto result = GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply);
            pickupToolPlacementCommandCallback(result, self, head.head);
        }

        // 0x004B2C74
        static void pickupToolDown(Window& self, const int16_t x, const int16_t y)
        {
            auto* head = getVehicle(&self);
            if (head == nullptr)
            {
                return;
            }
            switch (head->mode)
            {
                case TransportMode::rail:
                    pickupToolDownLand(self, *head, x, y, getVehicleRailPlacementArgsFromCursor);
                    break;
                case TransportMode::road:
                    pickupToolDownLand(self, *head, x, y, getVehicleRoadPlacementArgsFromCursor);
                    break;
                case TransportMode::air:
                    pickupToolDownAir(self, *head, x, y);
                    break;
                case TransportMode::water:
                    pickupToolDownWater(self, *head, x, y);
                    break;
            }
        }

        // 0x004B3035
        static void pickupToolAbort(Window& self)
        {
            // TODO: refactor to use removeAirplaneGhost family of functions
            auto* head = EntityManager::get<Vehicles::VehicleHead>(EntityId(self.number));
            if (head == nullptr)
            {
                return;
            }
            if (head->tileX == -1 || !head->has38Flags(Vehicles::Flags38::isGhost))
            {
                self.invalidate();
                return;
            }

            switch (head->mode)
            {
                case TransportMode::rail:
                case TransportMode::road:
                {
                    GameCommands::VehiclePickupArgs args{};
                    args.head = head->id;
                    auto flags = GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::ghost;
                    GameCommands::doCommand(args, flags);
                    break;
                }
                case TransportMode::air:
                {
                    GameCommands::VehiclePickupAirArgs gcArgs{};
                    gcArgs.head = head->id;
                    auto flags = GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::ghost;
                    GameCommands::doCommand(gcArgs, flags);
                    break;
                }
                case TransportMode::water:
                {
                    GameCommands::VehiclePickupWaterArgs gcArgs{};
                    gcArgs.head = head->id;
                    auto flags = GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::ghost;
                    GameCommands::doCommand(gcArgs, flags);
                    break;
                }
            }
            self.invalidate();
        }

        // 0x004B2680
        static void renameVehicle(Window* self, WidgetIndex_t widgetIndex)
        {
            auto vehicle = getVehicle(self);
            if (vehicle != nullptr)
            {
                FormatArguments args{};
                args.push(StringIds::getVehicleType(vehicle->vehicleType)); // 0
                args.skip(6);
                args.push(StringIds::getVehicleType(vehicle->vehicleType)); // 8
                TextInput::openTextInput(self, StringIds::title_name_vehicle, StringIds::prompt_enter_new_vehicle_name, vehicle->name, widgetIndex, &vehicle->ordinalNumber);
            }
        }

        // 0x004B2566
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            Input::toolCancel(self->type, self->number);
            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - Common::widx::tabMain;
            self->frameNo = 0;
            self->flags &= ~WindowFlags::flag_16;
            self->var_85C = -1;
            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                ViewportManager::collectGarbage();
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tabMain];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = *tabInfo.holdableWidgets;

            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabledWidgets = 0;
            Main::resetDisabledWidgets(self);
            self->invalidate();
            self->rowHover = -1;
            self->var_842 = -1;
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x004B1E94
        static void setCaptionEnableState(Window* const self)
        {
            self->enabledWidgets |= 1 << widx::caption;
            auto head = getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            if (head->owner != CompanyManager::getControllingId())
            {
                self->enabledWidgets &= ~static_cast<uint64_t>(1 << widx::caption);
            }
        }

        // 0x004B45DD, 0x004B55A7, 0x004B3C1B
        static void event8(Window& self)
        {
            self.flags |= WindowFlags::notScrollView;
        }

        // 0x004B45E5, 0x004B55B6, 0x004B3C23
        static void event9(Window& self)
        {
            if (self.hasFlags(WindowFlags::notScrollView))
            {
                if (self.rowHover != -1)
                {
                    self.rowHover = -1;
                    self.invalidate();
                }
            }
        }

        // 0x004B28E2
        static void onPickup(Window* const self, const WidgetIndex_t pickupWidx)
        {
            self->invalidate();
            auto head = getVehicle(self);
            if (head == nullptr)
            {
                return;
            }
            if (!head->isPlaced())
            {
                CursorId cursor = kTypeToToolCursor[static_cast<uint8_t>(head->vehicleType)][getGameState().pickupDirection != 0 ? 1 : 0];
                if (Input::toolSet(self, pickupWidx, cursor))
                {
                    _1136264 = -1;
                }
                return;
            }

            GameCommands::setErrorTitle(StringIds::cant_remove_string_id);
            FormatArguments args{};
            args.skip(10);
            args.push(head->name);
            args.push(head->ordinalNumber);

            bool success = false;
            switch (head->mode)
            {
                case TransportMode::rail:
                case TransportMode::road:
                {
                    GameCommands::VehiclePickupArgs gcArgs{};
                    gcArgs.head = head->id;
                    success = GameCommands::doCommand(gcArgs, GameCommands::Flags::apply) != GameCommands::FAILURE;
                    break;
                }
                case TransportMode::air:
                {
                    GameCommands::VehiclePickupAirArgs gcArgs{};
                    gcArgs.head = head->id;
                    success = GameCommands::doCommand(gcArgs, GameCommands::Flags::apply) != GameCommands::FAILURE;
                    break;
                }
                case TransportMode::water:
                {
                    GameCommands::VehiclePickupWaterArgs gcArgs{};
                    gcArgs.head = head->id;
                    success = GameCommands::doCommand(gcArgs, GameCommands::Flags::apply) != GameCommands::FAILURE;
                    break;
                }
            }
            if (success)
            {
                self->callOnMouseUp(widx::tabDetails);
            }
        }

        static size_t getNumCars(Ui::Window* const self)
        {
            auto* head = getVehicle(self);
            if (head == nullptr)
            {
                return 0;
            }
            Vehicles::Vehicle train(*head);

            if (train.cars.empty())
            {
                return 0;
            }

            return train.cars.size();
        }

        // TODO: Move to a more appropriate file used by many windows
        int16_t sub_4B743B(uint8_t al, uint8_t ah, int16_t cx, int16_t dx, Vehicles::VehicleBase* vehicle, Gfx::RenderTarget* const pDrawpixelinfo)
        {
            registers regs{};
            regs.al = al;
            regs.ah = ah;
            regs.cx = cx;
            regs.dx = dx;
            regs.esi = X86Pointer(vehicle);
            regs.edi = X86Pointer(pDrawpixelinfo);
            call(0x004B743B, regs);
            return regs.cx;
        }

        // 0x004B5CC1
        static std::optional<Vehicles::Car> getCarFromScrollView(Window* const self, const int16_t y)
        {
            auto* head = getVehicle(self);
            if (head == nullptr)
            {
                return {};
            }
            Vehicles::Vehicle train(*head);

            auto heightOffset = y;
            for (auto& car : train.cars)
            {
                heightOffset -= self->rowHeight;
                if (heightOffset <= 0)
                {
                    return { car };
                }
            }
            return {};
        }

        struct TabIcons
        {
            uint32_t image;
            uint8_t frameSpeed;
        };

        static const std::map<VehicleType, TabIcons> tabIconByVehicleType{
            { VehicleType::train, { InterfaceSkin::ImageIds::tab_vehicle_train_frame0, 1 } },
            { VehicleType::bus, { InterfaceSkin::ImageIds::tab_vehicle_bus_frame0, 1 } },
            { VehicleType::truck, { InterfaceSkin::ImageIds::tab_vehicle_truck_frame0, 1 } },
            { VehicleType::tram, { InterfaceSkin::ImageIds::tab_vehicle_tram_frame0, 1 } },
            { VehicleType::aircraft, { InterfaceSkin::ImageIds::tab_vehicle_aircraft_frame0, 2 } },
            { VehicleType::ship, { InterfaceSkin::ImageIds::tab_vehicle_ship_frame0, 3 } },
        };

        // 0x004B5F0D
        static void drawTabs(Window* const self, Gfx::RenderTarget* const rt)
        {
            auto skin = OpenLoco::ObjectManager::get<InterfaceSkinObject>();

            auto vehicle = Common::getVehicle(self);
            if (vehicle == nullptr)
            {
                return;
            }
            auto vehicleType = vehicle->vehicleType;

            auto mainTab = tabIconByVehicleType.at(vehicleType);
            int frame = 0;
            if (self->currentTab == 0)
            {
                frame = (self->frameNo >> mainTab.frameSpeed) & 0x7;
            }

            Widget::drawTab(
                self,
                rt,
                Gfx::recolour(skin->img + mainTab.image + frame, CompanyManager::getCompanyColour(self->owner)),
                widx::tabMain);

            frame = 0;
            if (self->currentTab == 1)
            {
                frame = (self->frameNo >> 1) & 0xF;
            }
            Widget::drawTab(
                self,
                rt,
                skin->img + InterfaceSkin::ImageIds::tab_wrench_frame0 + frame,
                widx::tabDetails);

            frame = 0;
            if (self->currentTab == 2)
            {
                frame = (self->frameNo >> 3) & 0x3;
            }
            Widget::drawTab(
                self,
                rt,
                skin->img + InterfaceSkin::ImageIds::tab_cargo_delivered_frame0 + frame,
                widx::tabCargo);

            frame = 0;
            if (self->currentTab == 4)
            {
                frame = (self->frameNo >> 4) & 0x3;
            }
            Widget::drawTab(
                self,
                rt,
                skin->img + InterfaceSkin::ImageIds::tab_routes_frame_0 + frame,
                widx::tabRoute);

            frame = 0;
            if (self->currentTab == 3)
            {
                frame = (self->frameNo >> 1) & 0xF;
            }
            Widget::drawTab(
                self,
                rt,
                skin->img + InterfaceSkin::ImageIds::tab_finances_frame0 + frame,
                widx::tabFinances);
        }
    }

    // 0x004B949C
    bool rotate()
    {
        if (Input::isToolActive(WindowType::vehicle))
        {
            if (ToolManager::getToolWidgetIndex() == Main::widx::pickup || ToolManager::getToolWidgetIndex() == Details::widx::pickup)
            {
                getGameState().pickupDirection = getGameState().pickupDirection ^ 1;
                return true;
            }
        }

        return false;
    }

    void registerHooks()
    {
        Main::initEvents();
        Cargo::initEvents();
        Details::initEvents();
        Finances::initEvents();
        Route::initEvents();
    }
}
