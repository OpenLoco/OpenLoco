#include "../Things/Vehicle.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/CargoObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/WaterObject.h"
#include "../OpenLoco.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Vehicle
{
    namespace common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_main = 4,
            tab_vehicle_details = 5,
            tab_cargo = 6,
            tab_finances = 7,
            tab_route = 8,
        };

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                    \
    makeWidget({ 0, 0 }, { (frameWidth), (frameHeight) }, widget_type::frame, 0),                                                                  \
        makeWidget({ 1, 1 }, { (frameWidth)-2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                                 \
        makeWidget({ (frameWidth)-15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),           \
        makeWidget({ 0, 41 }, { 265, 136 }, widget_type::panel, 1),                                                                                \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_main),     \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_details), \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_cargo),   \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_finance), \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_route)

        constexpr uint64_t enabledWidgets = (1 << close_button) | (1 << tab_main) | (1 << tab_vehicle_details) | (1 << tab_cargo) | (1 << tab_finances) | (1 << tab_route);

        static OpenLoco::vehicle_head* getVehicle(const window* self)
        {
            return ThingManager::get<OpenLoco::vehicle_head>(self->number);
        }

        static void setActiveTabs(window* const self);
        static void textInput(window* const self, const widget_index callingWidget, char* const input);
        static void renameVehicle(window* const self, const widget_index widgetIndex);
        static void switchTab(window* const self, const widget_index widgetIndex);
        static void repositionTabs(window* const self);
        static void sub_4B1E94(window* const self);
        static void drawTabs(window* const window, Gfx::drawpixelinfo_t* const context);
        static vehicle_bogie* sub_4B5CC1(window* const self, const int16_t y);
    }

    namespace VehicleDetails
    {
        // 0x00500434
        static window_event_list events;
        constexpr uint64_t enabledWidgets = 0b1111111110100;
        constexpr uint64_t holdableWidgets = 0;

        static widget_t widgets[] = {
            commonWidgets(265, 177, StringIds::stringid),
            widgetEnd()
        };
    }

    namespace cargo
    {
        // 0x005004A8
        static window_event_list events;
        constexpr uint64_t enabledWidgets = 0b11111110100;
        constexpr uint64_t holdableWidgets = 0;
    }

    namespace finances
    {
        // 0x00522470
        static widget_t widgets[] = {
            commonWidgets(636, 319, StringIds::title_company_finances),
            widgetEnd(),
        };

        //constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select) | (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        //const uint64_t holdableWidgets = (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        constexpr uint64_t enabledWidgets = 0b111110100;
        constexpr uint64_t holdableWidgets = 0;

        static window_event_list events;

    }

    namespace route
    {
        enum widx
        {
            unk_9 = common::widx::tab_route + 1,
            route_list,
            order_force_unload,
            order_wait,
            order_skip,
            order_delete,
            order_up,
            order_down
        };
        // 0x00500554
        static window_event_list events;
        static widget_t widgets[] = {
            commonWidgets(265, 177, StringIds::stringid),
            makeWidget({ 0, 0 }, { 1, 1 }, widget_type::none, 0),
            makeWidget({ 3, 44 }, { 237, 120 }, widget_type::scrollview, 1, vertical, StringIds::tooltip_route_scrollview),
            makeWidget({ 240, 44 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::route_force_unload, StringIds::tooltip_route_insert_force_unload),
            makeWidget({ 240, 68 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::route_wait, StringIds::tooltip_route_insert_wait_full_cargo),
            makeWidget({ 240, 92 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::route_skip, StringIds::tooltip_route_skip_next_order),
            makeWidget({ 240, 116 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::route_delete, StringIds::tooltip_route_delete_order),
            makeWidget({ 240, 140 }, { 24, 12 }, widget_type::wt_9, 1, ImageIds::red_arrow_up, StringIds::tooltip_route_move_order_up),
            makeWidget({ 240, 152 }, { 24, 12 }, widget_type::wt_9, 1, ImageIds::red_arrow_down, StringIds::tooltip_route_move_order_down),
            widgetEnd(),
        };
        constexpr uint64_t enabledWidgets = 0b11111110111110100;
        constexpr uint64_t holdableWidgets = 0;
    }

    static loco_global<int32_t, 0x0113614E> _113614E;
    static loco_global<int16_t, 0x01136156> _1136156;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;
    static loco_global<uint8_t, 0x00508F14> _screenFlags;

    static void sub_4B28E2(window* w, int dx)
    {
        registers regs;

        regs.dx = dx;
        regs.esi = (uintptr_t)w;

        call(0x004B28E2, regs);
    }

    namespace Main
    {
        static const Gfx::ui_size_t windowSize = { 265, 177 };
        static const Gfx::ui_size_t minWindowSize = { 192, 177 };
        static const Gfx::ui_size_t maxWindowSize = { 600, 440 };

        enum widx
        {
            viewport = common::widx::tab_route + 1,
            status = 10,
            speed_control = 11,
            stop_start = 12,
            pickup = 13,
            pass_signal = 14,
            change_direction = 15,
            centre_viewport = 16,
        };

        static widget_t widgets[] = {
            commonWidgets(265, 177, StringIds::stringid),
            makeWidget({ 3, 44 }, { 237, 120 }, widget_type::viewport, 1),
            makeWidget({ 3, 155 }, { 237, 21 }, widget_type::wt_13, 1),
            makeWidget({ 240, 46 }, { 24, 115 }, widget_type::wt_5, 1),
            makeWidget({ 240, 44 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::stop_start, StringIds::tooltip_stop_start),
            makeWidget({ 240, 68 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::tooltip_remove_from_track),
            makeWidget({ 240, 92 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::pass_signal, StringIds::tooltip_pass_signal_at_danger),
            makeWidget({ 240, 116 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::construction_right_turnaround, StringIds::tooltip_change_direction),
            makeWidget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::move_main_view_to_show_this),
            widgetEnd()
        };

        constexpr uint64_t interactiveWidgets = (1 << stop_start) | (1 << pickup) | (1 << pass_signal) | (1 << change_direction) | (1 << centre_viewport);
        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << speed_control) | interactiveWidgets;
        constexpr uint64_t holdableWidgets = 1 << speed_control;

        // 0x005003C0
        static window_event_list events;

        // 0x004B5D82
        static void resetDisabledWidgets(window* const self)
        {
            self->disabled_widgets = 0;
        }

        // 0x004B5D88
        // 0x004B32F9
        static void createViewport(window* const self)
        {
            if (self->current_tab != (common::widx::tab_main - common::widx::tab_main))
            {
                return;
            }

            self->callPrepareDraw();

            auto vehHead = common::getVehicle(self);
            Things::Vehicle::Vehicle train(vehHead);
            if (vehHead->tile_x == -1)
            {
                if (self->viewports[0] == nullptr)
                {
                    return;
                }

                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                self->invalidate();
                return;
            }

            thing_id_t targetThing = train.veh2->id;
            if (!train.cars.empty())
            {
                targetThing = train.cars.firstCar.front->id;
                // Always true so above is pointless
                if (train.cars.firstCar.front->type == VehicleThingType::bogie)
                {
                    targetThing = train.cars.firstCar.body->id;
                }
            }

            // Compute views.
            SavedView view = {
                targetThing,
                (1 << 15) | (1 << 14),
                ZoomLevel::full,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                0
            };

            uint16_t flags = 0;
            if (self->viewports[0] != nullptr)
            {
                if (self->saved_view == view)
                    return;

                flags = self->viewports[0]->flags;
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                ViewportManager::collectGarbage();
            }
            else
            {
                if ((Config::get().flags & Config::flags::gridlines_on_landscape) != 0)
                    flags |= ViewportFlags::gridlines_on_landscape;
            }

            self->saved_view = view;

            // 0x004B5E88 start
            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto origin = Gfx::point_t(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = Gfx::ui_size_t(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(self, 0, origin, size, self->saved_view.zoomLevel, targetThing);
                self->invalidate();
                self->flags |= WindowFlags::viewport_no_scrolling;
            }
            // 0x004B5E88 end

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->flags = flags;
                self->invalidate();
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
        static window* create(const uint16_t head)
        {
            auto* const self = WindowManager::createWindow(WindowType::vehicle, windowSize, WindowFlags::flag_11 | WindowFlags::flag_8 | WindowFlags::resizable, &events);
            self->widgets = widgets;
            self->enabled_widgets = enabledWidgets;
            self->number = head;
            const auto* vehicle = ThingManager::get<OpenLoco::vehicle_head>(head);
            self->owner = vehicle->owner;
            self->row_height = rowHeights[static_cast<uint8_t>(vehicle->vehicleType)];
            self->current_tab = 0;
            self->frame_no = 0;
            resetDisabledWidgets(self);
            self->min_width = minWindowSize.width;
            self->min_height = minWindowSize.height;
            self->max_width = maxWindowSize.width;
            self->max_height = maxWindowSize.height;
            self->var_85C = -1;
            WindowManager::close(WindowType::dragVehiclePart, 0);
            _113614E = -1;
            _1136156 = -1;

            const auto* skin = ObjectManager::get<interface_skin_object>();
            self->colours[1] = skin->colour_0A;
            return self;
        }

        // 0x004B6033
        window* open(const OpenLoco::vehicle* vehicle)
        {
            const auto head = vehicle->head;
            auto* self = WindowManager::find(WindowType::vehicle, head);
            if (self != nullptr)
            {
                if (Input::isToolActive(self->type, self->number))
                {
                    Input::toolCancel();
                }
                self = WindowManager::bringToFront(WindowType::vehicle, head);
            }
            if (self == nullptr)
            {
                self = create(head);
                self->saved_view.clear();
            }
            self->current_tab = 0;
            self->invalidate();
            self->widgets = widgets;
            self->enabled_widgets = enabledWidgets;
            self->holdable_widgets = (1 << widx::speed_control);
            self->event_handlers = &events;
            self->activated_widgets = 0;
            resetDisabledWidgets(self);
            self->initScrollWidgets();
            createViewport(self);
            return self;
        }

        // 0x004B28E2
        static void onPickup(window* self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<uint32_t>(self);
            call(0x004B28E2, regs);
        }

        // 0x004B288F
        static void onChangeDirection(window* self)
        {
            // very simple
            registers regs{};
            regs.esi = reinterpret_cast<uint32_t>(self);
            // Bug in game command 3 requires to set edi to a vehicle prior to calling
            regs.edi = reinterpret_cast<uint32_t>(common::getVehicle(self));
            call(0x004B288F, regs);
        }

        // 0x004B24D1
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;
                case common::widx::caption:
                    common::renameVehicle(self, widgetIndex);
                    break;
                case common::widx::tab_main:
                case common::widx::tab_vehicle_details:
                case common::widx::tab_cargo:
                case common::widx::tab_finances:
                case common::widx::tab_route:
                    common::switchTab(self, widgetIndex);
                    break;
                case widx::pickup:
                    onPickup(self);
                    break;
                case widx::change_direction:
                    onChangeDirection(self);
                    break;
                case widx::pass_signal:
                    gGameCommandErrorTitle = StringIds::cant_pass_signal_at_danger;
                    GameCommands::do_4(self->number);
                    break;
                case widx::centre_viewport:
                    self->viewportCentreMain();
                    break;
            }
        }

        // 0x004B30F3
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, common::widx::tab_main);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, widx::status);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, widx::pickup);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, widx::pass_signal);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, widx::change_direction);

            if (w->isDisabled(widx::pickup))
            {
                Input::toolCancel(WindowType::vehicle, w->number);
                return;
            }

            auto vehicle = common::getVehicle(w);

            if (vehicle->tile_x != -1 && (vehicle->var_38 & Things::Vehicle::Flags38::unk_4) == 0)
            {
                return;
            }

            if (!WindowManager::isInFront(w))
                return;

            if (vehicle->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, widx::pickup);
            }
        }

        // 0x004B3210
        static void onResize(window* const self)
        {
            common::sub_4B1E94(self);
            self->setSize(minWindowSize, maxWindowSize);

            if (self->viewports[0] != nullptr)
            {
                auto veh0 = common::getVehicle(self);
                uint16_t newWidth = self->width - 30;
                if (veh0->owner != CompanyManager::getControllingId())
                    newWidth += 22;

                uint16_t newHeight = self->height - 59;
                if (veh0->var_0C & (1 << 6) && veh0->owner == CompanyManager::getControllingId())
                {
                    newWidth -= 27;
                }

                auto& viewport = self->viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    self->invalidate();
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->view_width = newWidth << viewport->zoom;
                    viewport->view_height = newHeight << viewport->zoom;
                    self->saved_view.clear();
                }
            }
            createViewport(self);
        }

        // 0x004B274B
        static void stopStartOpen(window* const self)
        {
            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::stop);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::start);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::manual);

            auto dropdownCount = 2;
            auto veh0 = common::getVehicle(self);
            if (veh0->mode == TransportMode::rail && (_screenFlags & ScreenFlags::unknown_6))
            {
                dropdownCount = 3;
            }

            widget_t* widget = &self->widgets[widx::stop_start];
            Dropdown::showText(
                self->x + widget->left,
                self->y + widget->top,
                widget->width(),
                widget->height(),
                self->colours[1],
                dropdownCount,
                0);

            auto highlighted = 0; // Stop
            if (!(veh0->var_0C & (1 << 1)))
            {
                highlighted = 1; // Start
            }
            if (veh0->var_0C & (1 << 6))
            {
                highlighted = 2; // Manual
            }

            Dropdown::setItemSelected(highlighted);
            Dropdown::setHighlightedItem(highlighted & 2); // Stop or Manual (Start becomes Stop highlighted)
        }

        // 0x004B2637
        static void onSpeedControl(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<uint32_t>(self);
            call(0x004B2637, regs);
        }

        // 0x004B251A
        static void onMouseDown(window* const self, const widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::stop_start:
                    stopStartOpen(self);
                    break;
                case widx::speed_control:
                    onSpeedControl(self);
                    break;
            }
        }

        // 0x004B253A
        static void onDropdown(window* const self, const widget_index widgetIndex, const int16_t itemIndex)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            regs.edx = widgetIndex;
            regs.eax = itemIndex;
            call(0x004B253A, regs);
        }

        // 0x004B2545
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004B2545, regs);
        }

        // 0x004B2550
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004B2550, regs);
        }

        // 0x004B255B
        static void onToolAbort(window& self, const widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            call(0x004B255B, regs);
        }

        // 0x004B31F2
        static void tooltip(FormatArguments& args, Ui::window* const self, const widget_index widgetIndex)
        {
            auto vehHead = common::getVehicle(self);
            args.skip(2);
            args.push(StringIds::getVehicleType(vehHead->vehicleType));
        }

        static loco_global<uint8_t, 0x00525FC5> _525FC5;

        // 0x004B1EB5
        static void prepareDraw(window* const self)
        {
            if (self->widgets != widgets)
            {
                self->widgets = widgets;
                self->initScrollWidgets();
            }

            common::setActiveTabs(self);
            auto vehHead = common::getVehicle(self);
            Things::Vehicle::Vehicle train(vehHead);

            self->widgets[widx::stop_start].type = widget_type::wt_9;
            self->widgets[widx::pickup].type = widget_type::wt_9;
            self->widgets[widx::pass_signal].type = widget_type::wt_9;
            self->widgets[widx::change_direction].type = widget_type::wt_9;

            if (vehHead->mode != TransportMode::rail)
            {
                self->widgets[widx::pass_signal].type = widget_type::none;
            }

            if (vehHead->mode == TransportMode::air || vehHead->mode == TransportMode::water)
            {
                self->widgets[widx::change_direction].type = widget_type::none;
            }

            self->disabled_widgets &= ~interactiveWidgets;

            auto second = train.veh1;
            if (train.cars.empty())
            {
                self->disabled_widgets |= (1 << widx::pickup);
            }

            if (second->var_3C >= 0x3689)
            {
                self->disabled_widgets |= (1 << widx::pickup) | (1 << widx::change_direction);
            }

            if (vehHead->mode == TransportMode::air || vehHead->mode == TransportMode::water)
            {
                if (vehHead->var_5D != 1 && vehHead->var_5D != 6 && vehHead->tile_x != -1)
                {
                    self->disabled_widgets |= (1 << widx::pickup);
                }
            }

            if (vehHead->tile_x == -1)
            {
                self->disabled_widgets |= (1 << widx::stop_start) | (1 << widx::pass_signal) | (1 << widx::change_direction) | (1 << widx::centre_viewport);
                if (Input::isToolActive(WindowType::vehicle, self->number))
                {
                    self->disabled_widgets &= ~(1 << widx::change_direction); //???
                }
            }

            if (vehHead->var_5D != 3)
            {
                self->disabled_widgets |= (1 << widx::pass_signal);
            }

            auto company = CompanyManager::get(vehHead->owner);
            FormatArguments args{};
            if (isPlayerCompany(vehHead->owner))
            {
                args.push(StringIds::company_vehicle);
            }
            else
            {
                args.push(StringIds::competitor_vehicle);
            }
            args.push(company->name);
            args.skip(2);
            args.push(vehHead->var_22);
            args.push(vehHead->var_44);

            uint32_t ecx;
            if ((vehHead->var_0C & 0x40) != 0)
            {
                ecx = 2371; //yellow flag
            }
            else if ((vehHead->var_0C & 0x01) != 0)
            {
                ecx = 2369; // red flag
            }
            else
            {
                ecx = 2370; // green flag
            }
            self->widgets[0xC].image = ecx;

            uint32_t image = 0;
            string_id tooltip = 0;
            switch (vehHead->mode)
            {
                case TransportMode::rail:
                {
                    auto track = ObjectManager::get<track_object>(vehHead->track_type);
                    image = 0x20000000 | (track->image + 17);
                    tooltip = 255;
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | (track->image + 16);
                            tooltip = 254;
                        }
                    }
                    break;
                }
                case TransportMode::road:
                {
                    uint8_t roadType = vehHead->track_type;
                    if (roadType == 0xFF)
                    {
                        roadType = _525FC5;
                    }
                    auto road = ObjectManager::get<road_object>(roadType);

                    image = 0x20000000 | (road->image + 33);
                    tooltip = 255;
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | (road->image + 32);
                            tooltip = 254;
                        }
                    }
                    break;
                }
                case TransportMode::air:
                {
                    image = 0x20000000 | 2373; // air traffic tower with downward arrow
                    tooltip = 257;             // Place {POP16}{STRINGID} in airport
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | 2372; // air traffic tower with upward arrow
                            tooltip = 256;             // Remove {POP16}{STRINGID} from airport
                        }
                    }
                    break;
                }
                case TransportMode::water:
                {
                    auto water = ObjectManager::get<water_object>();
                    image = 0x20000000 | (water->image + 59);
                    tooltip = 259;
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | (water->image + 58);
                            tooltip = 258;
                        }
                    }
                    break;
                }
            }
            self->widgets[widx::pickup].image = image;
            self->widgets[widx::pickup].tooltip = tooltip;

            self->widgets[widx::speed_control].type = widget_type::none;

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;
            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
            self->widgets[common::widx::caption].right = self->width - 2;
            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            int ax = self->width - 26;
            if (vehHead->var_0C & 0x40)
            {
                if (isPlayerCompany(vehHead->owner))
                {
                    ax -= 27;
                    self->widgets[widx::speed_control].type = widget_type::wt_5;
                }
            }

            self->widgets[widx::viewport].right = ax;
            self->widgets[widx::viewport].bottom = self->height - 1 - 13;

            self->widgets[widx::status].top = self->height - 1 - 13 + 2;
            self->widgets[widx::status].bottom = self->height - 1 - 13 + 2 + 9;
            self->widgets[widx::status].right = self->width - 14;

            self->widgets[widx::stop_start].right = self->width - 2;
            self->widgets[widx::pickup].right = self->width - 2;
            self->widgets[widx::pass_signal].right = self->width - 2;
            self->widgets[widx::change_direction].right = self->width - 2;

            self->widgets[widx::stop_start].left = self->width - 2 - 23;
            self->widgets[widx::pickup].left = self->width - 2 - 23;
            self->widgets[widx::pass_signal].left = self->width - 2 - 23;
            self->widgets[widx::change_direction].left = self->width - 2 - 23;

            self->widgets[widx::speed_control].left = self->width - 2 - 23 - 26;
            self->widgets[widx::speed_control].right = self->width - 2 - 23 - 26 + 23;

            if (!isPlayerCompany(vehHead->owner))
            {
                self->widgets[widx::stop_start].type = widget_type::none;
                self->widgets[widx::pickup].type = widget_type::none;
                self->widgets[widx::pass_signal].type = widget_type::none;
                self->widgets[widx::change_direction].type = widget_type::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_viewport].left = self->widgets[widx::viewport].right - 1 - 23;
            self->widgets[widx::centre_viewport].top = self->widgets[widx::viewport].bottom - 1 - 23;
            common::repositionTabs(self);
        }

        struct VehicleStatus
        {
            uint16_t bx;
            uint32_t eax;
            uint16_t cx;
            uint32_t edx;
        };

        static VehicleStatus sub_4B671C(const OpenLoco::vehicle_head* veh)
        {
            registers regs = {};
            regs.esi = (int32_t)veh;

            call(0x004B671C, regs);

            VehicleStatus status = {};
            status.bx = regs.bx;
            status.eax = regs.eax;
            status.cx = regs.cx;
            status.edx = regs.edx;
            return status;
        }

        // 0x004B226D
        static void draw(window* const self, Gfx::drawpixelinfo_t* const context)
        {
            self->draw(context);
            common::drawTabs(self, context);

            widget_t& pickupButton = self->widgets[widx::pickup];
            if (pickupButton.type != widget_type::none)
            {
                if ((pickupButton.image & 0x20000000) != 0 && !self->isDisabled(widx::pickup))
                {
                    Gfx::drawImage(
                        context,
                        self->x + pickupButton.left,
                        self->y + pickupButton.top,
                        Gfx::recolour(pickupButton.image, CompanyManager::getCompanyColour(self->owner)));
                }
            }

            auto veh = common::getVehicle(self);
            {
                auto status = sub_4B671C(veh);
                FormatArguments args = {};
                args.push(status.bx);
                args.push(status.eax);
                args.push(status.cx);
                args.push(status.edx);

                string_id bx = 456;
                if (status.cx == 0xFFFF)
                {
                    bx = 455;
                }

                Gfx::drawString_494BBF(
                    *context,
                    self->x + self->widgets[widx::status].left - 1,
                    self->y + self->widgets[widx::status].top - 1,
                    self->widgets[widx::status].width() - 1,
                    Colour::black,
                    bx,
                    &args);
            }

            widget_t& speedWidget = self->widgets[widx::speed_control];
            if (speedWidget.type != widget_type::none)
            {
                Gfx::drawImage(
                    context,
                    self->x + speedWidget.left,
                    self->y + speedWidget.top + 10,
                    Gfx::recolour(3545, self->colours[1]));

                Gfx::drawStringCentred(
                    *context,
                    self->x + speedWidget.mid_x(),
                    self->y + speedWidget.top + 4,
                    Colour::black,
                    1924);

                Gfx::drawStringCentred(
                    *context,
                    self->x + speedWidget.mid_x(),
                    self->y + speedWidget.bottom - 10,
                    Colour::black,
                    1925);

                Gfx::drawImage(
                    context,
                    self->x + speedWidget.left + 1,
                    self->y + speedWidget.top + 57 - veh->var_6E,
                    Gfx::recolour(3546, self->colours[1]));
            }

            if (self->viewports[0] != nullptr)
            {
                self->drawViewports(context);
                Widget::drawViewportCentreButton(context, self, 0x10);
            }
            else if (Input::isToolActive(self->type, self->number))
            {
                FormatArguments args = {};
                args.push(StringIds::getVehicleType(veh->vehicleType));
                Gfx::point_t origin;
                widget_t& button = self->widgets[0x9];
                origin.x = self->x + button.mid_x();
                origin.y = self->y + button.mid_y();
                Gfx::drawStringCentredWrapped(
                    context,
                    &origin,
                    button.width() - 6,
                    Colour::black,
                    334, // "{COLOUR BLACK}(click on view to set {STRINGID} starting position)"
                    &args);
            }
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.on_update = onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.on_tool_abort = onToolAbort;
            events.text_input = common::textInput;
            events.viewport_rotate = createViewport;
            events.tooltip = tooltip;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace VehicleDetails
    {
        // 0x004B3C45
        // "Show <vehicle> design details and options" tab in vehicle window
        static void onUpdate(window* w)
        {
            if (w->number == _1136156)
            {
                if (WindowManager::find(WindowType::dragVehiclePart) == nullptr)
                {
                    _1136156 = -1;
                    _113614E = -1;
                    w->invalidate();
                }
            }

            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 5);

            if (_1136156 == -1 && w->isActivated(11))
            {
                WindowManager::invalidateWidget(WindowType::vehicle, w->number, 11);
            }

            if (w->isDisabled(10))
            {
                Input::toolCancel(WindowType::vehicle, w->number);
                return;
            }

            auto vehicle = common::getVehicle(w);
            if (vehicle->tile_x != -1 && (vehicle->var_38 & Things::Vehicle::Flags38::unk_4) == 0)
                return;

            if (!WindowManager::isInFrontAlt(w))
                return;

            if (vehicle->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 10);
            }
        }

        static void initEvents()
        {
            events.on_mouse_up = nullptr;
            events.on_resize = nullptr;
            events.on_update = onUpdate;
            events.event_08 = nullptr;
            events.event_09 = nullptr;
            events.on_tool_update = nullptr;
            events.on_tool_down = nullptr;
            events.on_mouse_down = nullptr;
            events.on_tool_abort = nullptr;
            events.get_scroll_size = nullptr;
            events.scroll_mouse_down = nullptr;
            events.scroll_mouse_over = nullptr;
            events.text_input = common::textInput;
            events.tooltip = nullptr;
            events.cursor = nullptr;
            events.prepare_draw = nullptr;
            events.draw = nullptr;
            events.draw_scroll = nullptr;
        }
    }

    namespace cargo
    {
        enum widx
        {
            refit_button = 9,
            cargo_list = 10,
        };

        static widget_t widgets[] = {
            commonWidgets(265, 177, StringIds::title_vehicle_cargo),
            makeWidget({ 240, 44 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::SPR_2386, StringIds::refit_vehicle_tip), // 9
            makeWidget({ 3, 44 }, { 259, 120 }, widget_type::scrollview, 1, vertical),                                     // 10
            widgetEnd()
        };

        static void onRefitButton(window* w, widget_index wi);

        static bool canRefit(OpenLoco::vehicle_head* headVehicle)
        {
            if (!isPlayerCompany(headVehicle->owner))
            {
                return false;
            }

            OpenLoco::Things::Vehicle::Vehicle train(headVehicle);

            if (train.cars.empty())
            {
                return false;
            }

            auto object = ObjectManager::get<vehicle_object>(train.cars.firstCar.front->object_id);
            return (object->flags & FlagsE0::refittable);
        }

        // 004B3DDE
        static void prepareDraw(window* const self)
        {
            if (self->widgets != widgets)
            {
                self->widgets = widgets;
                self->initScrollWidgets();
            }

            common::setActiveTabs(self);

            auto* headVehicle = common::getVehicle(self);
            auto ax = headVehicle->var_44;
            auto cx = headVehicle->var_22;

            FormatArguments args = {};
            args.push(cx);
            args.push(ax);

            widgets[common::widx::frame].right = self->width - 1;
            widgets[common::widx::frame].bottom = self->height - 1;
            widgets[common::widx::panel].right = self->width - 1;
            widgets[common::widx::panel].bottom = self->height - 1;
            widgets[common::widx::close_button].left = self->width - 15;
            widgets[common::widx::close_button].right = self->width - 3;
            widgets[widx::cargo_list].right = self->width - 26;
            widgets[widx::cargo_list].bottom = self->height - 14;
            widgets[widx::refit_button].right = self->width - 2;
            widgets[widx::refit_button].left = self->width - 25;
            widgets[widx::refit_button].type = widget_type::wt_9;
            if (!canRefit(headVehicle))
            {
                widgets[widx::refit_button].type = widget_type::none;
                widgets[widx::cargo_list].right = self->width - 26 + 22;
            }

            common::repositionTabs(self);
        }

        static void generateCargoTotalString(OpenLoco::vehicle_head* vehicle, char* buffer)
        {
            uint32_t cargoTotals[ObjectManager::getMaxObjects(object_type::cargo)]{};
            Things::Vehicle::Vehicle train(vehicle);
            for (auto& car : train.cars)
            {
                auto front = car.front;
                auto body = car.body;
                if (front->cargo_type != 0xFF)
                {
                    cargoTotals[front->cargo_type] += front->secondaryCargoQty;
                }
                if (body->cargo_type != 0xFF)
                {
                    cargoTotals[body->cargo_type] += body->primaryCargoQty;
                }
            }

            bool hasCargo = false;
            for (size_t cargoType = 0; cargoType < ObjectManager::getMaxObjects(object_type::cargo); ++cargoType)
            {
                auto cargoTotal = cargoTotals[cargoType];
                if (cargoTotal == 0)
                {
                    continue;
                }

                // On all but first loop insert a ", "
                if (hasCargo)
                {
                    *buffer++ = ',';
                    *buffer++ = ' ';
                }
                hasCargo = true;
                auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
                auto unitNameFormat = cargoTotal == 1 ? cargoObj->unit_name_singular : cargoObj->unit_name_plural;
                FormatArguments args{};
                args.push(cargoTotal);
                buffer = StringManager::formatString(buffer, unitNameFormat, &args);
            }

            if (!hasCargo)
            {
                StringManager::formatString(buffer, StringIds::cargo_empty_2);
            }
        }

        // 004B3F0D
        static void draw(Ui::window* const self, Gfx::drawpixelinfo_t* const context)
        {
            self->draw(context);
            common::drawTabs(self, context);

            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            generateCargoTotalString(common::getVehicle(self), buffer);

            FormatArguments args = {};
            args.push<string_id>(StringIds::buffer_1250);

            Gfx::drawString_494BBF(*context, self->x + 3, self->y + self->height - 13, self->width - 15, Colour::black, StringIds::str_1282, &args);
        }

        static int16_t sub_4B743B(uint8_t al, uint8_t ah, int16_t cx, int16_t dx, vehicle_base* vehicle, Gfx::drawpixelinfo_t* const pDrawpixelinfo)
        {
            registers regs{};
            regs.al = al;
            regs.ah = ah;
            regs.cx = cx;
            regs.dx = dx;
            regs.esi = reinterpret_cast<uint32_t>(vehicle);
            regs.edi = reinterpret_cast<uint32_t>(pDrawpixelinfo);
            call(0x004B743B, regs);
            return regs.cx;
        }

        // based on 0x004B40C7
        static void drawCargoText(Gfx::drawpixelinfo_t* const pDrawpixelinfo, const int16_t x, int16_t& y, const string_id strFormat, uint8_t cargoQty, uint8_t cargoType, station_id_t stationId)
        {
            if (cargoQty == 0)
            {
                return;
            }

            auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
            auto unitNameFormat = cargoQty == 1 ? cargoObj->unit_name_singular : cargoObj->unit_name_plural;
            auto station = StationManager::get(stationId);
            FormatArguments args{};
            args.push(StringIds::cargo_from);
            args.push(unitNameFormat);
            args.push<uint32_t>(cargoQty);
            args.push(station->name);
            args.push(station->town);
            Gfx::drawString_494B3F(*pDrawpixelinfo, x, y, Colour::black, strFormat, &args);
            y += 10;
        }

        // 004B3F62
        static void drawScroll(window* const self, Gfx::drawpixelinfo_t* const pDrawpixelinfo, const uint32_t i)
        {
            Gfx::clearSingle(*pDrawpixelinfo, Colour::getShade(self->colours[1], 4));
            Things::Vehicle::Vehicle train{ common::getVehicle(self) };
            int16_t y = 0;
            for (auto& car : train.cars)
            {
                string_id strFormat = StringIds::black_stringid;
                auto front = car.front;
                auto body = car.body;
                if (front->id == self->row_hover)
                {
                    Gfx::fillRect(pDrawpixelinfo, 0, y, self->width, y + self->row_height - 1, 0x2000030);
                    strFormat = StringIds::wcolour2_stringid;
                }
                // Get width of the drawing
                auto width = sub_4B743B(1, 0, 0, y, front, pDrawpixelinfo);
                // Actually draw it
                width = sub_4B743B(0, 0, 24 - width, (self->row_height - 22) / 2 + y, car.front, pDrawpixelinfo);

                if (body->cargo_type != 0xFF)
                {

                    int16_t cargoTextHeight = self->row_height / 2 + y - ((self->row_height - 22) / 2) - 10;
                    if (front->secondaryCargoQty != 0 || body->primaryCargoQty != 0)
                    {
                        if (body->primaryCargoQty == 0 || front->secondaryCargoQty == 0)
                        {
                            cargoTextHeight += 5;
                        }
                        drawCargoText(pDrawpixelinfo, width, cargoTextHeight, strFormat, body->primaryCargoQty, body->cargo_type, body->townCargoFrom);
                        drawCargoText(pDrawpixelinfo, width, cargoTextHeight, strFormat, front->secondaryCargoQty, front->cargo_type, front->townCargoFrom);
                    }
                    else
                    {
                        FormatArguments args{};
                        args.push<string_id>(StringIds::cargo_empty);
                        Gfx::drawString_494B3F(*pDrawpixelinfo, width, cargoTextHeight + 5, Colour::black, strFormat, &args);
                    }
                }

                y += self->row_height;
            }
        }

        // 004B41BD
        static void onMouseUp(window* const self, const widget_index i)
        {
            switch (i)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_main:
                case common::widx::tab_vehicle_details:
                case common::widx::tab_cargo:
                case common::widx::tab_finances:
                case common::widx::tab_route:
                    common::switchTab(self, i);
                    break;

                case common::widx::caption:
                    common::renameVehicle(self, i);
                    break;
            }
        }

        // 004B41E2
        static void onMouseDown(window* const self, const widget_index i)
        {
            switch (i)
            {
                case widx::refit_button:
                    onRefitButton(self, i);
                    break;
            }
        }

        // 004B41E9
        static void onDropdown(window* const self, const widget_index i, const int16_t dropdownIndex)
        {
            switch (i)
            {
                case widx::refit_button:
                    if (dropdownIndex == -1)
                        break;

                    gGameCommandErrorTitle = StringIds::cant_refit_vehicle;
                    GameCommands::do_64(self->number, Dropdown::getItemArgument(dropdownIndex, 3));
                    break;
            }
        }

        static uint32_t sub_42F6B6(uint32_t eax, uint8_t bl, uint8_t cl)
        {
            registers regs;
            regs.eax = eax;
            regs.bl = bl;
            regs.cl = cl;
            call(0x0042F6B6, regs);
            return regs.eax;
        }

        static void onRefitButton(window* const self, const widget_index wi)
        {
            Things::Vehicle::Vehicle train(common::getVehicle(self));
            auto vehicleObject = ObjectManager::get<vehicle_object>(train.cars.firstCar.front->object_id);
            auto eax = vehicleObject->max_primary_cargo;
            auto bl = Utility::bitScanForward(vehicleObject->primary_cargo_types);

            int32_t index = 0;
            for (uint16_t cargoId = 0; cargoId < ObjectManager::getMaxObjects(object_type::cargo); cargoId++)
            {
                auto cargoObject = ObjectManager::get<cargo_object>(cargoId);
                if (cargoObject == nullptr)
                    continue;

                if ((cargoObject->var_12 & 2) == 0)
                    continue;

                string_id format = StringIds::dropdown_stringid;
                if (cargoId == train.cars.firstCar.body->cargo_type)
                {
                    format = StringIds::dropdown_stringid_selected;
                }

                auto args = FormatArguments();
                args.push<string_id>(cargoObject->unit_name_plural);
                args.push<uint32_t>(sub_42F6B6(eax, bl, cargoId));
                args.push<uint16_t>(cargoId);
                Dropdown::add(index, format, args);
                index++;
            }

            widget_t& button = self->widgets[wi];

            Dropdown::showText(
                self->x + button.left,
                self->y + button.top,
                button.width(),
                button.height(),
                self->colours[1],
                index,
                0);
            Dropdown::setHighlightedItem(0);
        }

        // 0x004B4339
        static void tooltip(FormatArguments& args, Ui::window* const self, const widget_index widgetIndex)
        {
            args.push(StringIds::tooltip_scroll_vehicle_list);

            auto vehicle = common::getVehicle(self);
            args.push(StringIds::getVehicleType(vehicle->vehicleType));
        }

        // 0x004B4360
        static void getScrollSize(Ui::window* const self, const uint32_t scrollIndex, uint16_t* const width, uint16_t* const height)
        {
            Things::Vehicle::Vehicle train(common::getVehicle(self));

            *height = 0;
            if (train.cars.empty())
            {
                return;
            }

            int rows = train.cars.size();
            *height = rows * self->row_height;
        }

        static char* generateCargoTooltipDetails(char* buffer, const string_id cargoFormat, const uint8_t cargoType, const uint8_t maxCargo, const uint32_t acceptedCargoTypes)
        {
            if (cargoType == 0xFF)
            {
                return buffer;
            }

            {
                auto cargoObj = ObjectManager::get<cargo_object>(cargoType);
                FormatArguments args{};
                args.push(maxCargo == 1 ? cargoObj->unit_name_singular : cargoObj->unit_name_plural);
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
                    auto type = Utility::bitScanForward(availableCargoTypes);
                    availableCargoTypes &= ~(1 << type);

                    auto cargoObj = ObjectManager::get<cargo_object>(type);
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
        static void scrollMouseOver(window* const self, const int16_t x, const int16_t y, const uint8_t scrollIndex)
        {
            Input::setTooltipTimeout(2000);
            self->flags &= ~WindowFlags::not_scroll_view;
            auto car = common::sub_4B5CC1(self, y);
            string_id tooltipFormat = StringIds::null;
            thing_id_t tooltipContent = ThingId::null;
            if (car != nullptr)
            {
                tooltipFormat = StringIds::buffer_337;
                tooltipContent = car->id;
                if (self->row_hover != tooltipContent)
                {
                    self->row_hover = tooltipContent;
                    self->invalidate();
                }
            }

            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_337));
            if (*buffer != '\0')
            {
                if (self->widgets[widx::cargo_list].tooltip == tooltipFormat && self->var_85C == tooltipContent)
                {
                    return;
                }
            }

            self->widgets[widx::cargo_list].tooltip = tooltipFormat;
            self->var_85C = tooltipContent;
            ToolTip::closeAndReset();

            if (tooltipContent == ThingId::null)
            {
                return;
            }

            ToolTip::set_52336E(true);

            {
                auto vehicleObj = ObjectManager::get<vehicle_object>(car->object_id);
                FormatArguments args{};
                args.push(vehicleObj->name);
                buffer = StringManager::formatString(buffer, StringIds::cargo_capacity_tooltip, &args);
            }
            // TODO get first body from car
            vehicle_body* body = reinterpret_cast<OpenLoco::vehicle*>(car)->nextVehicleComponent()->nextVehicleComponent()->asVehicleBody();
            buffer = generateCargoTooltipDetails(buffer, StringIds::cargo_capacity, body->cargo_type, body->max_cargo, body->accepted_cargo_types);
            buffer = generateCargoTooltipDetails(buffer, StringIds::cargo_capacity_plus, car->cargo_type, car->max_cargo, car->accepted_cargo_types);
        }

        // 0x004B45DD
        static void event8(window* const self)
        {
            self->flags |= WindowFlags::not_scroll_view;
        }

        // 0x004B45E5
        static void event9(window* const self)
        {
            if (self->flags & WindowFlags::not_scroll_view)
            {
                if (self->row_hover != -1)
                {
                    self->row_hover = -1;
                    self->invalidate();
                }
            }
        }

        // 0x004B4607
        static void onUpdate(window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(self->type, self->number, 6);
        }

        // 0x004B4621
        static void onResize(window* const self)
        {
            self->setSize({ 192, 142 }, { 400, 440 });
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.draw = draw;
            events.draw_scroll = drawScroll;
            events.prepare_draw = prepareDraw;
            events.on_dropdown = onDropdown;
            events.text_input = common::textInput;
            events.tooltip = tooltip;
            events.get_scroll_size = getScrollSize;
            events.scroll_mouse_over = scrollMouseOver;
            events.event_08 = event8;
            events.event_09 = event9;
            events.on_update = onUpdate;
            events.on_resize = onResize;
        }
    }

    namespace finances
    {
        // 0x004B56CE
        static void onPrepareDraw(window* self)
        {
            auto vehicle = common::getVehicle(self);
            auto args = FormatArguments();
            args.push(vehicle->var_22);
            args.push(vehicle->var_44);

            self->widgets[0].right = self->width - 1;
            self->widgets[0].bottom = self->height - 1;
            self->widgets[3].right = self->width - 1;
            self->widgets[3].bottom = self->height - 1;
            self->widgets[1].right = self->width - 2;
            self->widgets[2].left = self->width - 15;
            self->widgets[2].right = self->width - 3;

            common::repositionTabs(self);
        }

        // 0x004B576C
        static void draw(Ui::window* const self, Gfx::drawpixelinfo_t* const context)
        {
            self->draw(context);
            common::drawTabs(self, context);

            auto pos = Gfx::point_t(self->x + 4, self->y + 46);

            auto vehHead = common::getVehicle(self);
            Things::Vehicle::Vehicle train(vehHead);
            auto veh1 = train.veh1;
            self->draw(context);
            if (veh1->var_53 != -1)
            {
                auto args = FormatArguments();
                args.push<uint32_t>(veh1->var_53);
                // Last income on: {DATE DMY}
                Gfx::drawString_494B3F(*context, pos.x, pos.y, Colour::black, 1829, &args);
                pos.y += 10;

                pos.x = self->x + 8;
                for (int i = 0; i < 4; i++)
                {
                    auto al = veh1->var_57[i];
                    if (al == -1)
                        continue;

                    auto cargoObject = ObjectManager::get<cargo_object>(al);

                    auto str = veh1->var_5b[i] == 1 ? cargoObject->unit_name_singular : cargoObject->unit_name_plural;

                    args = FormatArguments();
                    args.push(str);
                    args.push<uint32_t>(veh1->var_5b[i]);
                    args.push(veh1->var_63[i]);
                    args.push<uint16_t>(veh1->var_6B[i]);
                    args.push<currency32_t>(veh1->var_6F[i]);
                    // {STRINGID} transported {INT16} blocks in {INT16} days = {CURRENCY32}
                    Gfx::drawString_495224(*context, pos.x, pos.y, self->width - 12, Colour::black, 1830, &args);

                    // TODO: fix function to take pointer to offset
                    pos.y += 12;
                }
            }
            else
            {
                // Last income: N/A"
                Gfx::drawString_494B3F(*context, pos.x, pos.y, Colour::black, 1828);
                pos.y += 10;
            }

            pos.y += 5;

            //if (veh->var_77 != 0)
            //{
            //    // Last journey average speed: {VELOCITY}
            //    auto args = FormatArguments();
            //    args.push(veh->var_77);
            //    Gfx::drawString_494B3F(*context, pos.x, pos.y, Colour::black, 1985);
            //    pos.y += 10 + 5;
            //}

            // Monthly Running Cost: {CURRENCY32}
            // Monthly Profit: {CURRENCY32}
            // Sale value of vehicle: {CURRENCY32}
        }

        // 0x004B5945
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;
                case common::widx::caption:
                    common::renameVehicle(self, widgetIndex);
                    break;
                case common::widx::tab_main:
                case common::widx::tab_vehicle_details:
                case common::widx::tab_cargo:
                case common::widx::tab_finances:
                case common::widx::tab_route:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x004B5977
        static void tooltip(FormatArguments& args, Ui::window* const self, const widget_index widgetIndex)
        {
            auto veh0 = common::getVehicle(self);
            args.skip(2);
            args.push(StringIds::getVehicleType(veh0->vehicleType));
        }

        // 0x004B5995
        static void onUpdate(window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(self->type, self->number, common::widx::tab_finances);
        }

        // 0x004B59AF
        static void onResize(window* const self)
        {
            common::sub_4B1E94(self);
            self->setSize({ 400, 202 }, { 400, 202 });
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = onUpdate;
            events.text_input = common::textInput;
            events.tooltip = tooltip;
            events.prepare_draw = onPrepareDraw;
            events.draw = draw;
        }
    }

    namespace route
    {
        static loco_global<uint8_t, 0x00113646A> _113646A;
        constexpr uint32_t max_orders = 256000; // TODO: MOVE
        static loco_global<uint8_t[max_orders], 0x00987C5C> _dword_987C5C;

        // TODO: Move to orders file this is duplicated
        static const uint8_t dword_4FE070[] = {
            1,
            2,
            2,
            6,
            1,
            1,
        };

        // 0x00470824
        static void sub_470824(OpenLoco::vehicle_head* head)
        {
            registers regs{};
            regs.esi = reinterpret_cast<uint32_t>(head);
            call(0x00470824, regs);
        }

        // 0x004B509B
        static void close(window* const self)
        {
            if (Input::isToolActive(self->type, self->number))
            {
                Input::toolCancel();
            }
        }

        static void orderDelete(vehicle_head* const head, const uint32_t orderOffset)
        {
            gGameCommandErrorTitle = StringIds::empty;
            GameCommands::do_36(head->id, orderOffset - head->orderTableOffset);
            sub_470824(head);
        }

        // 0x004B4F6D
        static void onOrderDelete(vehicle_head* const head, const int16_t orderId)
        {
            // No deleteable orders
            if (head->sizeOfOrderTable <= 1)
            {
                return;
            }

            // First item on the list is the local/express button
            if (orderId == 0)
            {
                return;
            }

            // Order id can be -1 at this point for none selected
            auto i = 0;
            auto orderOffset = head->orderTableOffset;
            auto previousOffset = orderOffset;
            auto orderType = -1; // Just to do one loop in all cases TODO ?do while?
            while (orderType != 0)
            {
                if (i == orderId - 1)
                {
                    orderDelete(head, orderOffset);
                    return;
                }
                orderType = _dword_987C5C[orderOffset] & 0x7;
                // Will only occur when no order found, so deletes the last one
                if (orderType == 0)
                {
                    // Passes the previous iterations offset
                    orderDelete(head, previousOffset);
                    return;
                }
                previousOffset = orderOffset;
                orderOffset += dword_4FE070[orderType];
                i++;
            }
            return;
        }

        // 0x004B4C14
        static bool orderUp(vehicle_head* const head, const uint32_t orderOffset)
        {
            gGameCommandErrorTitle = StringIds::empty;
            auto result = GameCommands::do_75(head->id, orderOffset - head->orderTableOffset);
            sub_470824(head); // Note order changed check if this matters.
            return result != GameCommands::FAILURE;
        }

        // 0x004B4CCB based on
        static bool orderDown(vehicle_head* const head, const uint32_t orderOffset)
        {
            gGameCommandErrorTitle = StringIds::empty;
            auto result = GameCommands::do_76(head->id, orderOffset - head->orderTableOffset);
            sub_470824(head); // Note order changed check if this matters.
            return result != GameCommands::FAILURE;
        }

        // 0x004B4BC1 / 0x004B4C78 based on
        static bool onOrderMove(vehicle_head* const head, const int16_t orderId, bool(orderMoveFunc)(vehicle_head*, uint32_t))
        {
            // No moveable orders
            if (head->sizeOfOrderTable <= 1)
                return false;
            // Null orderId and item 0 (local/express) no action
            if (orderId <= 0)
                return false;

            auto i = 0;
            auto orderOffset = head->orderTableOffset;
            auto orderType = -1; // Just to do one loop in all cases TODO ?do while?
            while (orderType != 0)
            {
                if (i == orderId - 1)
                {
                    return orderMoveFunc(head, orderOffset);
                }
                orderType = _dword_987C5C[orderOffset] & 0x7;
                orderOffset += dword_4FE070[orderType];
                i++;
            }
            return false;
        }

        // 0x004B4B43
        static void onMouseUp(window* const self, const widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;
                case common::widx::caption:
                    common::renameVehicle(self, widgetIndex);
                    break;
                case common::widx::tab_main:
                case common::widx::tab_vehicle_details:
                case common::widx::tab_cargo:
                case common::widx::tab_finances:
                case common::widx::tab_route:
                    common::switchTab(self, widgetIndex);
                    break;
                case widx::order_delete:
                {

                    onOrderDelete(common::getVehicle(self), self->var_842);
                    if (self->var_842 == -1)
                    {
                        return;
                    }
                    auto i = 0;
                    auto orderOffset = common::getVehicle(self)->orderTableOffset;
                    auto orderType = -1; // Just to do one loop in all cases TODO ?do while?
                    while (orderType != 0)
                    {
                        if (i == self->var_842)
                        {
                            return;
                        }
                        orderType = _dword_987C5C[orderOffset] & 0x7;
                        if (orderType == 0)
                        {
                            self->var_842 = -1;
                            return;
                        }
                        orderOffset += dword_4FE070[orderType];
                        i++;
                    }
                }
                break;
                case widx::order_skip:
                    gGameCommandErrorTitle = StringIds::empty;
                    GameCommands::do_37(self->number);
                    break;
                case widx::order_up:
                    if (onOrderMove(common::getVehicle(self), self->var_842, orderUp))
                    {
                        if (self->var_842 <= 1)
                        {
                            return;
                        }
                        self->var_842--;
                    }
                    break;
                case widx::order_down:
                    if (onOrderMove(common::getVehicle(self), self->var_842, orderDown))
                    {
                        if (self->var_842 <= 0)
                        {
                            return;
                        }
                        auto i = 0;
                        auto orderOffset = common::getVehicle(self)->orderTableOffset;
                        auto orderType = -1; // Just to do one loop in all cases TODO ?do while?
                        while (orderType != 0)
                        {
                            orderType = _dword_987C5C[orderOffset] & 0x7;
                            if (orderType == 0)
                            {
                                return;
                            }
                            orderOffset += dword_4FE070[orderType];
                            i++;
                            if (i == self->var_842)
                            {
                                orderType = _dword_987C5C[orderOffset] & 0x7;
                                if (orderType != 0)
                                {
                                    self->var_842++;
                                }
                                return;
                            }
                        }
                    }
                    break;
            }
        }
        // 0x004B564E
        static void onResize(window* const self)
        {
            common::sub_4B1E94(self);
            self->setSize({ 265, 178 }, { 600, 440 });
        }

        // 0x004B55D1
        // "Show <vehicle> route details" tab in vehicle window
        static void onUpdate(window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, self->number, 8);

            if (!WindowManager::isInFront(self))
                return;

            if (common::getVehicle(self)->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, self->number))
            {
                if (Input::toolSet(self, widx::unk_9, 12))
                {
                    self->invalidate();
                    sub_470824(common::getVehicle(self));
                }
            }
        }

        // 0x004B4D74
        static void tooltip(FormatArguments& args, Ui::window* const self, const widget_index widgetIndex)
        {
            args.push(StringIds::tooltip_scroll_orders_list);
            auto head = common::getVehicle(self);
            args.push(StringIds::getVehicleType(head->vehicleType));
        }

        // TODO: same as other tabs
        // 0x004B55A7
        static void event8(window* const self)
        {
            self->flags |= WindowFlags::not_scroll_view;
        }

        // TODO: same as other tabs
        // 0x004B55B6
        static void event9(window* const self)
        {
            if (self->flags & WindowFlags::not_scroll_view)
            {
                if (self->row_hover != -1)
                {
                    self->row_hover = -1;
                    self->invalidate();
                }
            }
        }

        // 0x004B4D9B
        static void getScrollSize(Ui::window* const self, const uint32_t scrollIndex, uint16_t* const width, uint16_t* const height)
        {
            auto head = common::getVehicle(self);

            *height = 10;
            auto orderOffset = head->orderTableOffset;
            auto orderType = -1; // Just to do one loop in all cases TODO ?do while?
            while (orderType != 0)
            {
                orderType = _dword_987C5C[orderOffset] & 0x7;
                orderOffset += dword_4FE070[orderType];
                *height += 10;
            }
        }

        // 0x004B530C
        static void scrollMouseOver(window* const self, const int16_t x, const int16_t y, const uint8_t scrollIndex)
        {
            self->flags &= ~WindowFlags::not_scroll_view;
            auto item = y / 10;
            if (self->row_hover != item)
            {
                self->row_hover = item;
                self->invalidate();
            }
        }

        // 0x004B5339
        static Ui::cursor_id cursor(window* const self, const int16_t widgetIdx, const int16_t x, const int16_t y, const Ui::cursor_id fallback)
        {
            if (widgetIdx != widx::route_list)
            {
                return fallback;
            }

            if (Input::isToolActive(self->type, self->number))
            {
                return cursor_id::arrows_inward;
            }
            return fallback;
        }

        // 0x004B468C
        static void prepareDraw(window* const self)
        {
            if (self->widgets != widgets)
            {
                self->widgets = widgets;
                self->initScrollWidgets();
            }

            common::setActiveTabs(self);
            auto head = common::getVehicle(self);
            FormatArguments args{};
            args.push(head->var_22);
            args.push(head->var_44);

            self->widgets[widx::route_list].tooltip = Input::isToolActive(self->type, self->number) ? StringIds::tooltip_route_scrollview_copy : StringIds::tooltip_route_scrollview;

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;
            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
            self->widgets[common::widx::caption].right = self->width - 2;
            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[widx::route_list].right = self->width - 26;
            self->widgets[widx::route_list].bottom = self->height - 14;
            self->widgets[widx::order_force_unload].right = self->width - 2;
            self->widgets[widx::order_wait].right = self->width - 2;
            self->widgets[widx::order_skip].right = self->width - 2;
            self->widgets[widx::order_delete].right = self->width - 2;
            self->widgets[widx::order_up].right = self->width - 2;
            self->widgets[widx::order_down].right = self->width - 2;
            self->widgets[widx::order_force_unload].left = self->width - 25;
            self->widgets[widx::order_wait].left = self->width - 25;
            self->widgets[widx::order_skip].left = self->width - 25;
            self->widgets[widx::order_delete].left = self->width - 25;
            self->widgets[widx::order_up].left = self->width - 25;
            self->widgets[widx::order_down].left = self->width - 25;

            self->disabled_widgets |= (1 << widx::order_force_unload) | (1 << widx::order_wait) | (1 << widx::order_skip) | (1 << widx::order_delete);
            if (head->sizeOfOrderTable != 1)
            {
                self->disabled_widgets &= ~((1 << widx::order_skip) | (1 << widx::order_delete));
            }
            if (head->var_4E != 0)
            {
                self->disabled_widgets &= ~((1 << widx::order_wait) | (1 << widx::order_force_unload));
            }

            widget_type type = head->owner == CompanyManager::getControllingId() ? widget_type::wt_9 : widget_type::none;
            self->widgets[widx::order_force_unload].type = type;
            self->widgets[widx::order_wait].type = type;
            self->widgets[widx::order_skip].type = type;
            self->widgets[widx::order_delete].type = type;
            self->widgets[widx::order_up].type = type;
            self->widgets[widx::order_down].type = type;
            if (type == widget_type::none)
            {
                self->widgets[widx::route_list].right += 22;
            }

            self->disabled_widgets |= (1 << widx::order_up) | (1 << widx::order_down);
            if (self->var_842 != -1)
            {
                self->disabled_widgets &= ~((1 << widx::order_up) | (1 << widx::order_down));
            }
            common::repositionTabs(self);
        }

        // 0x004B4866
        static void draw(window* const self, Gfx::drawpixelinfo_t* const context)
        {
            self->draw(context);
            common::drawTabs(self, context);

            if (Input::isToolActive(WindowType::vehicle, self->number))
            {
                // Location at bottom left edge of window
                Gfx::point_t loc{ self->x + 3, self->y + self->height - 13 };

                Gfx::drawString_494BBF(*context, loc.x, loc.y, self->width - 14, Colour::black, StringIds::route_click_on_waypoint);
            }
        }

        const std::array<string_id, 6> orderString = {
            StringIds::orders_end,
            StringIds::orders_stop_at,
            StringIds::orders_route_through,
            StringIds::orders_route_thought_waypoint,
            StringIds::orders_unload_all,
            StringIds::orders_wait_for_full_load_of,
        };

        // TODO: Move to orders file this is duplicated
        static const uint8_t byte_4FE088[] = {
            0,
            11,
            11,
            3,
            4,
            4,
            0,
            0,
        };

        // TODO: Go up to 64?
        static const std::array<uint32_t, 9> numberCircle = {
            ImageIds::number_circle_1,
            ImageIds::number_circle_2,
            ImageIds::number_circle_3,
            ImageIds::number_circle_4,
            ImageIds::number_circle_5,
            ImageIds::number_circle_6,
            ImageIds::number_circle_7,
            ImageIds::number_circle_8,
            ImageIds::number_circle_9,
        };

        // 0x004B49F8
        static void sub_4B49F8(const uint32_t orderOffset, FormatArguments& args)
        {
            station_id_t stationId = ((_dword_987C5C[orderOffset] & 0xC0) << 2) + _dword_987C5C[orderOffset + 1];
            auto station = StationManager::get(stationId);
            args.push(station->name);
            args.push(station->town);
        }

        // 0x004B4A31
        static void sub_4B4A31(const uint32_t orderOffset, FormatArguments& args)
        {
            uint16_t cargoId = _dword_987C5C[orderOffset] >> 3;
            auto cargoObj = ObjectManager::get<cargo_object>(cargoId);
            args.push(cargoObj->name);
            args.push(cargoObj->unit_inline_sprite);
        }

        // 0x004B4A58 based on
        static void sub_4B4A58(window* const self, Gfx::drawpixelinfo_t* const context, const string_id strFormat, FormatArguments& args, const uint8_t orderType, int16_t& y)
        {
            Gfx::point_t loc = { 8, y - 1 };
            Gfx::drawString_494B3F(*context, &loc, Colour::black, strFormat, &args);
            if (byte_4FE088[orderType] & (1 << 1))
            {
                if (Input::isToolActive(self->type, self->number))
                {
                    auto imageId = numberCircle[_113646A - 1];
                    Gfx::drawImage(context, loc.x + 3, loc.y + 1, Gfx::recolour(imageId, Colour::white));
                }
                _113646A++;
            }
        }

        // 0x004B49F1
        static void drawScroll(window* const self, Gfx::drawpixelinfo_t* const pDrawpixelinfo, const uint32_t i)
        {
            Gfx::clearSingle(*pDrawpixelinfo, Colour::getShade(self->colours[1], 4));

            auto head = common::getVehicle(self);
            Things::Vehicle::Vehicle train(head);
            auto strFormat = StringIds::black_stringid;
            if (self->row_hover == 0)
            {
                Gfx::fillRect(pDrawpixelinfo, 0, 0, self->width, 9, 0x2000030);
                strFormat = StringIds::wcolour2_stringid;
            }

            {
                FormatArguments args{};
                args.push(train.veh1->var_48 & (1 << 1) ? StringIds::express_seperator : StringIds::local_seperator);
                Gfx::drawString_494B3F(*pDrawpixelinfo, 8, -1, Colour::black, strFormat, &args);
            }
            if (head->orderTableOffset == 1)
            {
                Gfx::drawString_494B3F(*pDrawpixelinfo, 8, 9, Colour::black, StringIds::no_route_defined);
            }

            auto orderNum = 1;
            _113646A = 1; // Number ?symbol? TODO: make not a global
            auto orderOffset = head->orderTableOffset;
            auto orderType = -1; // Just to do one loop in all cases TODO ?do while?
            while (orderType != 0)
            {
                orderType = _dword_987C5C[orderOffset] & 0x7;
                int16_t y = orderNum * 10;
                strFormat = StringIds::black_stringid;
                if (self->var_842 == orderNum)
                {
                    Gfx::fillRect(pDrawpixelinfo, 0, y, self->width, y + 9, Colour::aquamarine);
                    strFormat = StringIds::white_stringid;
                }
                if (self->row_hover == orderNum)
                {
                    strFormat = StringIds::wcolour2_stringid;
                    Gfx::fillRect(pDrawpixelinfo, 0, y, self->width, y + 9, 0x2000030);
                }

                FormatArguments args{};
                args.push(orderString[orderType]);
                switch (orderType)
                {
                    case 0: // end of list
                    case 3: // route through waypoints
                        // Fall through
                        break;
                    case 1: // stop at
                    case 2: // route through
                        sub_4B49F8(orderOffset, args);
                        break;
                    case 4: // unload all of
                    case 5: // wait for full load of
                        sub_4B4A31(orderOffset, args);
                        break;
                }

                sub_4B4A58(self, pDrawpixelinfo, strFormat, args, orderType, y);
                if (head->currentOrder + head->orderTableOffset == orderOffset)
                {
                    Gfx::drawString_494B3F(*pDrawpixelinfo, 1, y - 1, Colour::black, StringIds::orders_current_order);
                }

                orderOffset += dword_4FE070[orderType];
                orderNum++;
            }
        }

        static void initEvents()
        {
            events.on_close = close;
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down;
            events.on_dropdown;
            events.on_update = onUpdate;
            events.event_08 = event8;
            events.event_09 = event9;
            events.on_tool_down;
            events.on_tool_abort;
            events.event_15;
            events.get_scroll_size = getScrollSize;
            events.scroll_mouse_down;
            events.scroll_mouse_over = scrollMouseOver;
            events.text_input = common::textInput;
            events.viewport_rotate;
            events.tooltip = tooltip;
            events.cursor = cursor;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.draw_scroll = drawScroll;
        }
    }

    namespace common
    {

        struct TabInformation
        {
            const widx widgetIndex;
            widget_t* widgets;
            window_event_list* events;
            const uint64_t* enabledWidgets;
            const uint64_t* holdableWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { widx::tab_main, Main::widgets, &Main::events, &Main::enabledWidgets, &Main::holdableWidgets },
            { widx::tab_vehicle_details, VehicleDetails::widgets, &VehicleDetails::events, &VehicleDetails::enabledWidgets, &Main::holdableWidgets },
            { widx::tab_cargo, cargo::widgets, &cargo::events, &cargo::enabledWidgets, &Main::holdableWidgets },
            { widx::tab_finances, finances::widgets, &finances::events, &finances::enabledWidgets, &Main::holdableWidgets },
            { widx::tab_route, route::widgets, &route::events, &route::enabledWidgets, &Main::holdableWidgets }
        };

        static void setActiveTabs(window* const self)
        {
            self->activated_widgets &= ~((1 << widx::tab_main) | (1 << widx::tab_vehicle_details) | (1 << widx::tab_cargo) | (1 << widx::tab_finances) | (1 << widx::tab_route));
            self->activated_widgets |= 1ULL << (widx::tab_main + self->current_tab);
        }

        // 0x004B26C0
        static void textInput(window* const self, const widget_index callingWidget, char* const input)
        {
            registers regs;
            regs.dx = callingWidget;
            regs.esi = reinterpret_cast<int32_t>(self);
            regs.cl = 1;
            regs.edi = reinterpret_cast<uintptr_t>(input);
            call(0x004B26C0, regs);
        }

        // 0x004B2680
        static void renameVehicle(window* self, widget_index widgetIndex)
        {
            auto vehicle = getVehicle(self);
            if (vehicle != nullptr)
            {
                FormatArguments args{};
                args.push(StringIds::getVehicleType(vehicle->vehicleType)); // 0
                args.skip(6);
                args.push(StringIds::getVehicleType(vehicle->vehicleType)); // 8
                TextInput::openTextInput(self, StringIds::title_name_vehicle, StringIds::prompt_enter_new_vehicle_name, vehicle->var_22, widgetIndex, &vehicle->var_44);
            }
        }

        // 0x004B2566
        static void switchTab(window* self, widget_index widgetIndex)
        {
            Input::toolCancel(self->type, self->number);
            TextInput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - common::widx::tab_main;
            self->frame_no = 0;
            self->flags &= ~WindowFlags::flag_16;
            self->var_85C = -1;
            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                ViewportManager::collectGarbage();
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_main];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = *tabInfo.holdableWidgets;
            ;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabled_widgets = 0;
            Main::resetDisabledWidgets(self);
            self->invalidate();
            self->row_hover = -1;
            self->var_842 = -1;
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x004B5ECD
        static void repositionTabs(window* const self)
        {
            int16_t xPos = self->widgets[widx::tab_main].left;
            const int16_t tabWidth = self->widgets[widx::tab_main].right - xPos;

            for (uint8_t i = widx::tab_main; i <= widx::tab_route; i++)
            {
                if (self->isDisabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x004B1E94
        static void sub_4B1E94(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            call(0x004B1E94, regs);
        }

        // 0x004B5CC1
        static vehicle_bogie* sub_4B5CC1(window* const self, const int16_t y)
        {
            Things::Vehicle::Vehicle train(getVehicle(self));

            auto heightOffset = y;
            for (auto& car : train.cars)
            {
                heightOffset -= self->row_height;
                if (heightOffset <= 0)
                {
                    return car.front;
                }
            }
            return nullptr;
        }

        struct TabIcons
        {
            uint32_t image;
            int frameCount;
        };

        static const std::map<VehicleType, TabIcons> tabIconByVehicleType{
            { VehicleType::train, { InterfaceSkin::ImageIds::spr_203, 1 } },
            { VehicleType::bus, { InterfaceSkin::ImageIds::spr_219, 1 } },
            { VehicleType::truck, { InterfaceSkin::ImageIds::spr_235, 1 } },
            { VehicleType::tram, { InterfaceSkin::ImageIds::spr_227, 1 } },
            { VehicleType::aircraft, { InterfaceSkin::ImageIds::spr_211, 2 } },
            { VehicleType::ship, { InterfaceSkin::ImageIds::spr_243, 3 } },
        };

        // 0x004B5F0D
        static void drawTabs(window* const self, Gfx::drawpixelinfo_t* const context)
        {
            auto skin = OpenLoco::ObjectManager::get<interface_skin_object>();

            auto vehicle = common::getVehicle(self);
            auto vehicleType = vehicle->vehicleType;

            int frame = 0;
            if (self->current_tab == 0)
            {
            }

            Widget::draw_tab(
                self,
                context,
                Gfx::recolour(skin->img + tabIconByVehicleType.at(vehicleType).image, CompanyManager::getCompanyColour(self->owner)),
                widx::tab_main);

            frame = 0;
            if (self->current_tab == 1)
            {
                frame = (self->frame_no >> 1) & 0xF;
            }
            Widget::draw_tab(
                self,
                context,
                skin->img + InterfaceSkin::ImageIds::spr_97 + frame,
                widx::tab_vehicle_details);

            frame = 0;
            if (self->current_tab == 2)
            {
                frame = (self->frame_no >> 3) & 0x3;
            }
            Widget::draw_tab(
                self,
                context,
                skin->img + InterfaceSkin::ImageIds::tab_cargo_delivered_frame0 + frame,
                widx::tab_cargo);

            frame = 0;
            if (self->current_tab == 4)
            {
                frame = (self->frame_no >> 4) & 0x3;
            }
            Widget::draw_tab(
                self,
                context,
                skin->img + InterfaceSkin::ImageIds::tab_routes_frame_0 + frame,
                widx::tab_route);

            frame = 0;
            if (self->current_tab == 3)
            {
                frame = (self->frame_no >> 1) & 0xF;
            }
            Widget::draw_tab(
                self,
                context,
                skin->img + InterfaceSkin::ImageIds::tab_finances_frame0 + frame,
                widx::tab_finances);
        }
    }

    void registerHooks()
    {
        Main::initEvents();
        cargo::initEvents();
        VehicleDetails::initEvents();
        finances::initEvents();
        route::initEvents();
    }
}
