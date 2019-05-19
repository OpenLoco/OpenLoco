#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../scenario.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::ScenarioOptions
{
    static const gfx::ui_size_t challengeWindowSize = { 366, 197 };
    static const gfx::ui_size_t companiesWindowSize = { 366, 327 };
    static const gfx::ui_size_t otherWindowSize = { 366, 217 };

    static loco_global<uint8_t, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoPercentage;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;

    static loco_global<uint16_t[10], 0x0112C826> commonFormatArgs;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            panel,
            tab_challenge,
            tab_companies,
            tab_finances,
            tab_scenario,
        };

        const uint64_t enabledWidgets = (1 << widx::tab_challenge) | (1 << widx::tab_companies) | (1 << widx::tab_finances) | (1 << widx::tab_scenario);

#define commonWidgets(frameHeight, windowCaptionId)                                                                             \
    make_widget({ 0, 0 }, { 366, frameHeight }, widget_type::frame, 0),                                                         \
        make_widget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, windowCaptionId),                                        \
        make_widget({ 0, 41 }, { 366, 175 }, widget_type::panel, 1),                                                            \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_scenario_challenge), \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_options),   \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_financial_options), \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_scenario_options)

        // Defined at the bottom of this file.
        static void initEvents();

        // 0x004400A4
        static void drawTabs(window* window, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)window;

            call(0x004400A4, regs);
        }

        static void draw(window* window, gfx::drawpixelinfo_t* dpi)
        {
            window->draw(dpi);
            drawTabs(window, dpi);
        }

        static void prepare_draw(window* self);

        static void switchTab(window* self, widget_index widgetIndex);
    }

    namespace challenge
    {
        enum widx
        {
            objective_type = 7,
            objective_type_btn,
            objective_value,
            objective_value_down,
            objective_value_up,
            objective_cargo,
            objective_cargo_btn,
            check_be_top_company,
            check_be_within_top_three_companies,
            check_time_limit,
            time_limit_value,
            time_limit_value_down,
            time_limit_value_up,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::objective_type) | (1 << widx::objective_type_btn) | (1 << widx::objective_value_down) | (1 << widx::objective_value_up) | (1 << widx::objective_cargo) | (1 << widx::objective_cargo_btn) | (1 << widx::check_be_top_company) | (1 << widx::check_be_within_top_three_companies) | (1 << widx::check_time_limit) | (1 << widx::time_limit_value_down) | (1 << widx::time_limit_value_up);
        const uint64_t holdableWidgets = (1 << widx::objective_value_down) | (1 << widx::objective_value_up) | (1 << widx::time_limit_value_down) | (1 << widx::time_limit_value_up);

        static widget_t widgets[] = {
            commonWidgets(197, string_ids::title_scenario_challenge),
            make_widget({ 10, 52 }, { 346, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 53 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_stepper_widgets({ 10, 67 }, { 163, 12 }, widget_type::wt_17, 1),
            make_widget({ 193, 67 }, { 163, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 83 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::and_be_the_top_company),
            make_widget({ 10, 98 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::and_be_within_the_top_companies),
            make_widget({ 10, 113 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::with_a_time_limit),
            make_stepper_widgets({ 256, 112 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::time_limit_years_value),
            widget_end(),
        };

        static window_event_list events;

        // 0x0043FC91
        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            const int16_t xPos = window->x + 5;
            int16_t yPos = window->y + widgets[widx::check_time_limit].bottom + 10;
            gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::challenge_label);

            call(0x004384E9);
            yPos += 10;
            gfx::draw_string_495224(*dpi, xPos, yPos, window->width - 10, colour::black, string_ids::challenge_value, &*commonFormatArgs);
        }

        // 0x0043FD14
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                // TODO(avgeffen): Implement.
            }
        }

        // 0x0043FCED
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::tab_challenge:
                case common::widx::tab_companies:
                case common::widx::tab_finances:
                case common::widx::tab_scenario:
                    common::switchTab(self, widgetIndex);
                    break;

                case check_be_top_company:
                    *objectiveFlags ^= scenario::objective_flags::be_top_company;
                    self->invalidate();
                    break;

                case check_be_within_top_three_companies:
                    *objectiveFlags ^= scenario::objective_flags::be_within_top_three_companies;
                    self->invalidate();
                    break;

                case check_time_limit:
                    *objectiveFlags ^= scenario::objective_flags::within_time_limit;
                    self->invalidate();
                    break;
            }
        }

        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            static string_id objectiveLabelIds[] = {
                string_ids::achieve_a_certain_company_value,
                string_ids::achieve_a_certain_monthly_profit_from_vehicles,
                string_ids::achieve_a_certain_performance_index,
                string_ids::deliver_a_certain_amount_of_cargo,
            };

            widgets[widx::objective_type].text = objectiveLabelIds[*objectiveType];

            widgets[widx::objective_cargo].type = widget_type::none;
            widgets[widx::objective_cargo_btn].type = widget_type::none;
            widgets[widx::time_limit_value].type = widget_type::none;
            widgets[widx::time_limit_value_down].type = widget_type::none;
            widgets[widx::time_limit_value_up].type = widget_type::none;

            switch (*objectiveType)
            {
                // TODO(avgeffen): Use a constant for each objective type.
                case 0:
                    *(uint32_t*)&*commonFormatArgs = *objectiveCompanyValue;
                    widgets[widx::objective_value].text = string_ids::challenge_monetary_value;
                    break;

                case 1:
                    *(uint32_t*)&*commonFormatArgs = *objectiveMonthlyVehicleProfit;
                    widgets[widx::objective_value].text = string_ids::challenge_monetary_value;
                    break;

                case 2:
                    *(uint16_t*)&*commonFormatArgs = *objectivePerformanceIndex;
                    widgets[widx::objective_value].text = string_ids::challenge_performance_index;
                    break;

                case 3:
                    *(uint32_t*)&*commonFormatArgs = *objectiveDeliveredCargoPercentage;
                    widgets[widx::objective_value].text = string_ids::challenge_delivered_cargo;

                    auto cargo = objectmgr::get<cargo_object>(*objectiveDeliveredCargoType);
                    widgets[widx::objective_cargo].text = cargo->name;
                    widgets[widx::objective_cargo].type = widget_type::wt_18;
                    widgets[widx::objective_cargo_btn].type = widget_type::wt_11;
                    break;
            }

            self->activated_widgets &= ~((1 << widx::check_be_top_company) | (1 << widx::check_be_within_top_three_companies) | (1 << widx::check_time_limit));

            if ((*objectiveFlags & scenario::objective_flags::be_top_company) != 0)
                self->activated_widgets |= 1 << widx::check_be_top_company;

            if ((*objectiveFlags & scenario::objective_flags::be_within_top_three_companies) != 0)
                self->activated_widgets |= 1 << widx::check_be_within_top_three_companies;

            if ((*objectiveFlags & scenario::objective_flags::within_time_limit) != 0)
            {
                self->activated_widgets |= 1 << widx::check_time_limit;
                widgets[widx::time_limit_value].type = widget_type::wt_17;
                widgets[widx::time_limit_value_down].type = widget_type::wt_11;
                widgets[widx::time_limit_value_up].type = widget_type::wt_11;
                commonFormatArgs[3] = *objectiveTimeLimitYears;
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.prepare_draw = prepare_draw;
        }
    }

    // 0x0043EE58
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::scenarioOptions, 0);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::scenarioOptions, 0);
        }

        if (window == nullptr)
        {
            // 0x0043EEFF start
            window = WindowManager::createWindowCentred(WindowType::scenarioOptions, otherWindowSize, 0, &challenge::events);
            window->widgets = challenge::widgets;
            window->enabled_widgets = challenge::enabledWidgets;
            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;

            auto skin = objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_0B;
                window->colours[1] = skin->colour_0E;
            }
            // 0x0043EEFF end

            window->width = otherWindowSize.width;
            window->height = otherWindowSize.height;
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = 0;
        window->invalidate();

        window->widgets = challenge::widgets;
        window->enabled_widgets = challenge::enabledWidgets;
        window->holdable_widgets = challenge::holdableWidgets;
        window->event_handlers = &challenge::events;
        window->activated_widgets = 0;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        return window;
    }

    namespace companies
    {
        static widget_t widgets[] = {
            commonWidgets(327, string_ids::title_company_options),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::max_competing_companies_value),
            make_stepper_widgets({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::delay_before_competing_companies_start_months),
            make_widget({ 246, 102 }, { 110, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 103 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 246, 117 }, { 110, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 118 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 246, 132 }, { 110, 12 }, widget_type::wt_18, 1),
            make_widget({ 344, 133 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 15, 160 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_trains),
            make_widget({ 15, 172 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_buses),
            make_widget({ 15, 184 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_trucks),
            make_widget({ 15, 196 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_trams),
            make_widget({ 15, 208 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_aircraft),
            make_widget({ 15, 220 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_ships),
            make_widget({ 15, 247 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_trains),
            make_widget({ 15, 259 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_buses),
            make_widget({ 15, 271 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_trucks),
            make_widget({ 15, 283 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_trams),
            make_widget({ 15, 295 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_aircraft),
            make_widget({ 15, 307 }, { 341, 12 }, widget_type::checkbox, 1, string_ids::forbid_ships),
            widget_end(),
        };

        const uint64_t enabledWidgets = 0b1111111111111111111101101111000;
        const uint64_t holdableWidgets = 0;

        static window_event_list events;

        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            // TODO(avgeffen): Implement.
        }

        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::tab_challenge:
                case common::widx::tab_companies:
                case common::widx::tab_finances:
                case common::widx::tab_scenario:
                    common::switchTab(self, widgetIndex);
                    break;

                    // TODO(avgeffen): Implement.
            }
        }

        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            // TODO(avgeffen): Implement.
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.prepare_draw = prepare_draw;
        }
    }

    namespace finances
    {
        static widget_t widgets[] = {
            commonWidgets(217, string_ids::title_financial_options),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::starting_loan_value),
            make_stepper_widgets({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::max_loan_size_value),
            make_stepper_widgets({ 256, 82 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::loan_interest_rate_value),
            widget_end(),
        };

        const uint64_t enabledWidgets = 0b1101101101111000;
        const uint64_t holdableWidgets = 0b1101100000000;

        static window_event_list events;

        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            // TODO(avgeffen): Implement.
        }

        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::tab_challenge:
                case common::widx::tab_companies:
                case common::widx::tab_finances:
                case common::widx::tab_scenario:
                    common::switchTab(self, widgetIndex);
                    break;

                    // TODO(avgeffen): Implement.
            }
        }

        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            // TODO(avgeffen): Implement.
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.prepare_draw = prepare_draw;
        }
    }

    namespace scenario
    {
        static widget_t widgets[] = {
            commonWidgets(217, string_ids::title_scenario_options),
            make_widget({ 281, 52 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::change),
            make_widget({ 196, 67 }, { 160, 12 }, widget_type::wt_18, 1, string_ids::empty),
            make_widget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 281, 82 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::change),
            widget_end(),
        };

        const uint64_t enabledWidgets = 0b11111111000;
        const uint64_t holdableWidgets = 0b1101101100000000;

        static window_event_list events;

        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            // TODO(avgeffen): Implement.
        }

        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::tab_challenge:
                case common::widx::tab_companies:
                case common::widx::tab_finances:
                case common::widx::tab_scenario:
                    common::switchTab(self, widgetIndex);
                    break;

                    // TODO(avgeffen): Implement.
            }
        }

        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            // TODO(avgeffen): Implement.
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.prepare_draw = prepare_draw;
        }
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t* enabledWidgets;
            const uint64_t* holdableWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { challenge::widgets, widx::tab_challenge, &challenge::events, &challenge::enabledWidgets, &challenge::holdableWidgets },
            { companies::widgets, widx::tab_companies, &companies::events, &companies::enabledWidgets, &companies::holdableWidgets },
            { finances::widgets, widx::tab_finances, &finances::events, &finances::enabledWidgets, &finances::holdableWidgets },
            { scenario::widgets, widx::tab_scenario, &scenario::events, &scenario::enabledWidgets, &scenario::holdableWidgets }
        };

        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1 << widx::tab_challenge) | (1 << widx::tab_companies) | (1 << widx::tab_challenge) | (1 << widx::tab_scenario));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        // 0x0043F16B
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            textinput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_challenge;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);
            self->disabled_widgets = 0;

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_challenge];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = *tabInfo.holdableWidgets;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            self->invalidate();

            const gfx::ui_size_t* newSize;
            if (widgetIndex == widx::tab_challenge)
                newSize = &challengeWindowSize;
            else if (widgetIndex == widx::tab_companies)
                newSize = &companiesWindowSize;
            else
                newSize = &otherWindowSize;

            self->set_size(*newSize);
            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        static void initEvents()
        {
            challenge::initEvents();
            companies::initEvents();
            finances::initEvents();
            scenario::initEvents();
        }
    }
}
