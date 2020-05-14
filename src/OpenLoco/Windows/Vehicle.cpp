#include "../Things/Vehicle.h"
#include "../CompanyManager.h"
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
        static void sub_4B5ECD(window* const self);
        static void sub_4B1E94(window* const self);
        static void drawTabs(window* const window, Gfx::drawpixelinfo_t* const context);

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
        // 0x00500554
        static window_event_list events;
        static widget_t widgets[] = {
            commonWidgets(265, 177, StringIds::stringid),
            widgetEnd()
        };
        constexpr uint64_t enabledWidgets = 0b11111110111110100;
        constexpr uint64_t holdableWidgets = 0;
    }

    static loco_global<int32_t, 0x0113614E> _113614E;
    static loco_global<int16_t, 0x01136156> _1136156;

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
            unk_10 = 10,
            unk_11 = 11,
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
        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << unk_11) | interactiveWidgets;
        constexpr uint64_t holdableWidgets = 1 << unk_11;

        // 0x005003C0
        static window_event_list events;

        // 0x004B5D82
        static void resetDisabledWidgets(window* const self)
        {
            self->disabled_widgets = 0;
        }

        // 0x004B5D88
        static void createViewport(window* const self)
        {
            if (self->current_tab != (common::widx::tab_main - common::widx::tab_main))
            {
                return;
            }

            self->callPrepareDraw();

            registers regs{};
            regs.esi = reinterpret_cast<uint32_t>(self);
            call(0x004B5D9B, regs);
        }

        static uint16_t rowHeights[vehicleTypeCount] = {
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
            self->holdable_widgets = (1 << widx::unk_11);
            self->event_handlers = &events;
            self->activated_widgets = 0;
            resetDisabledWidgets(self);
            self->initScrollWidgets();
            createViewport(self);
            return self;
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
                case widx::change_direction:
                case widx::centre_viewport:
                case widx::pass_signal:
                    throw std::runtime_error("Not implemented");
                    break;
            }
        }

        // 0x004B30F3
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 4);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 10);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 13);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 14);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 15);

            if (w->isDisabled(13))
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
                sub_4B28E2(w, 13);
            }
        }

        // 0x004B3210
        static void onResize(window* const self)
        {
            common::sub_4B1E94(self);
            self->setSize(minWindowSize, maxWindowSize);

            if (self->viewports[0] == nullptr)
            {
                createViewport(self);
                return;
            }
        }

        // 0x004B251A
        static void onMouseDown(window* const self, const widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::stop_start:
                case widx::unk_11:
                    throw std::runtime_error("Not implemented");
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

        // 0x004B32F9
        static void viewportRotate(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            call(0x004B32F9, regs);
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
                    image = 0x20000000 | (track->var_1E + 17);
                    tooltip = 255;
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | (track->var_1E + 16);
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

                    image = 0x20000000 | (road->var_0E + 33);
                    tooltip = 255;
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | (road->var_0E + 32);
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
                    image = 0x20000000 | (water->var_06 + 59);
                    tooltip = 259;
                    if (vehHead->tile_x != -1)
                    {
                        if ((vehHead->var_38 & 0x10) == 0)
                        {
                            image = 0x20000000 | (water->var_06 + 58);
                            tooltip = 258;
                        }
                    }
                    break;
                }
            }
            self->widgets[widx::pickup].image = image;
            self->widgets[widx::pickup].tooltip = tooltip;

            self->widgets[widx::unk_11].type = widget_type::none;

            self->widgets[0x0].right = self->width - 1;
            self->widgets[0x0].bottom = self->height - 1;
            self->widgets[0x3].right = self->width - 1;
            self->widgets[0x3].bottom = self->height - 1;
            self->widgets[0x1].right = self->width - 2;
            self->widgets[0x2].left = self->width - 15;
            self->widgets[0x2].right = self->width - 3;

            int ax = self->width - 26;
            if (vehHead->var_0C & 0x40)
            {
                if (isPlayerCompany(vehHead->owner))
                {
                    ax -= 27;
                    self->widgets[widx::unk_11].type = widget_type::wt_5;
                }
            }

            self->widgets[0x9].right = ax;
            self->widgets[0x9].bottom = self->height - 1 - 13;
            self->widgets[0x9].top = self->height - 1 - 13 + 2;
            self->widgets[0x9].bottom = self->height - 1 - 13 + 2 + 9;

            self->widgets[0xA].right = self->width - 14;

            self->widgets[0xC].right = self->width - 2;
            self->widgets[widx::pickup].right = self->width - 2;
            self->widgets[0xE].right = self->width - 2;
            self->widgets[0xF].right = self->width - 2;

            self->widgets[0xC].left = self->width - 2 - 23;
            self->widgets[widx::pickup].left = self->width - 2 - 23;
            self->widgets[0xE].left = self->width - 2 - 23;
            self->widgets[0xF].left = self->width - 2 - 23;

            self->widgets[widx::unk_11].left = self->width - 2 - 23 - 26;
            self->widgets[widx::unk_11].right = self->width - 2 - 23 - 26 + 23;

            if (!isPlayerCompany(vehHead->owner))
            {
                self->widgets[0xC].type = widget_type::none;
                self->widgets[widx::pickup].type = widget_type::none;
                self->widgets[0xE].type = widget_type::none;
                self->widgets[0xF].type = widget_type::none;
                self->widgets[0x9].right += 22;
            }

            self->widgets[0x10].right = self->widgets[0x9].right - 1;
            self->widgets[0x10].bottom = self->widgets[0x9].bottom - 1;
            self->widgets[0x10].left = self->widgets[0x9].right - 1 - 23;
            self->widgets[0x10].top = self->widgets[0x9].bottom - 1 - 23;
            common::sub_4B5ECD(self);
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
                    self->x + self->widgets[0xA].left - 1,
                    self->y + self->widgets[0xA].top - 1,
                    self->widgets[0xA].width() - 1,
                    Colour::black,
                    bx,
                    &args);
            }

            widget_t& speedWidget = self->widgets[widx::unk_11];
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
            events.viewport_rotate = viewportRotate;
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

        static void sub_4B41FF(window* w, widget_index wi);

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

            auto object = ObjectManager::get<vehicle_object>(train.cars[0].carComponents[0].front->object_id);
            return (object->flags & FlagsE0::refittable);
        }

#pragma warning(disable : 4505)
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

            common::sub_4B5ECD(self);
        }

        static void sub_4B6658(OpenLoco::vehicle_head* vehicle)
        {
            // TODO: implement
        }

        // 004B3F0D
        static void draw(Ui::window* const self, Gfx::drawpixelinfo_t* const context)
        {
            self->draw(context);
            common::drawTabs(self, context);

            sub_4B6658(common::getVehicle(self));

            FormatArguments args = {};
            args.push<string_id>(StringIds::buffer_1250);

            Gfx::drawString_494BBF(*context, self->x + 3, self->y + self->height - 13, self->width - 15, Colour::black, StringIds::str_1282, &args);
        }

        // 004B3F62
        static void drawScroll(window* const self, Gfx::drawpixelinfo_t* const pDrawpixelinfo, const uint32_t i)
        {
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
                    sub_4B41FF(self, i);
                    break;
            }
        }

        // 004B41E9
        static void onDropdown(window* const self, const widget_index i, const int16_t i1)
        {
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

        static void sub_4B41FF(window* const self, const widget_index wi)
        {
            Things::Vehicle::Vehicle train(common::getVehicle(self));
            auto vehicleObject = ObjectManager::get<vehicle_object>(train.cars[0].carComponents[0].front->object_id);
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
                if (cargoId == train.cars[0].carComponents[0].body->cargo_type)
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

        // 0x004B4404
        static void scrollMouseOver(window* const self, const int16_t x, const int16_t y, const uint8_t scrollIndex)
        {
        }

        // 0x004B45DD
        static void ui__vehicle__event_3_8(window* const self)
        {
            self->flags |= WindowFlags::not_scroll_view;
        }

        // 0x004B45E5
        static void ui__vehicle__event_3_9(window* const self)
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
            events.event_08 = ui__vehicle__event_3_8;
            events.event_09 = ui__vehicle__event_3_9;
            events.on_update = onUpdate;
            events.on_resize = onResize;
        }
    }

    namespace route_details
    {
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
                sub_4B28E2(self, 13);
            }
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
        static void sub_4B5ECD(window* const self)
        {
        }

        // 0x004B1E94
        static void sub_4B1E94(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            call(0x004B1E94, regs);
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
        VehicleDetails::events.on_update = VehicleDetails::onUpdate;
        route::events.on_update = route_details::onUpdate;
    }
}
