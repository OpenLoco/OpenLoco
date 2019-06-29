#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/scenario_text_object.h"
#include "../s5/s5.h"
#include "../scenario.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::ScenarioOptions
{
    static const gfx::ui_size_t challengeWindowSize = { 366, 197 };
    static const gfx::ui_size_t companiesWindowSize = { 366, 327 };
    static const gfx::ui_size_t otherWindowSize = { 366, 217 };

    static loco_global<uint32_t, 0x00525E5E> currencyMultiplicationFactor;

    static loco_global<uint8_t, 0x00525FB7> maxCompetingCompanies;

    static loco_global<uint8_t, 0x00525FC6> loanInterestRate;

    static loco_global<uint8_t, 0x00526214> competitorStartDelay;
    static loco_global<uint8_t, 0x00526215> preferredAIIntelligence;
    static loco_global<uint8_t, 0x00526216> preferredAIAggressiveness;
    static loco_global<uint8_t, 0x00526217> preferredAICompetitiveness;
    static loco_global<uint16_t, 0x00526218> startingLoanSize;
    static loco_global<uint16_t, 0x0052621A> maxLoanSize;

    static loco_global<uint8_t, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoAmount;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;

    static loco_global<uint16_t, 0x00526248> forbiddenVehiclesPlayers;
    static loco_global<uint16_t, 0x0052624A> forbiddenVehiclesCompetitors;

    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;

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

        // 0x00440082
        static void update(window* window)
        {
            window->frame_no++;
            window->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::scenarioOptions, window->number, window->current_tab + widx::tab_challenge);
        }

        // 0x004400A4
        static void drawTabs(window* window, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Challenge tab
            {
                static const uint32_t challengeTabImageIds[] = {
                    interface_skin::image_ids::tab_cup_frame0,
                    interface_skin::image_ids::tab_cup_frame1,
                    interface_skin::image_ids::tab_cup_frame2,
                    interface_skin::image_ids::tab_cup_frame3,
                    interface_skin::image_ids::tab_cup_frame4,
                    interface_skin::image_ids::tab_cup_frame5,
                    interface_skin::image_ids::tab_cup_frame6,
                    interface_skin::image_ids::tab_cup_frame7,
                    interface_skin::image_ids::tab_cup_frame8,
                    interface_skin::image_ids::tab_cup_frame9,
                    interface_skin::image_ids::tab_cup_frame10,
                    interface_skin::image_ids::tab_cup_frame11,
                    interface_skin::image_ids::tab_cup_frame12,
                    interface_skin::image_ids::tab_cup_frame13,
                    interface_skin::image_ids::tab_cup_frame14,
                    interface_skin::image_ids::tab_cup_frame15,
                };

                uint32_t imageId = skin->img;
                if (window->current_tab == widx::tab_challenge - widx::tab_challenge)
                    imageId += challengeTabImageIds[(window->frame_no / 4) % std::size(challengeTabImageIds)];
                else
                    imageId += challengeTabImageIds[0];

                widget::draw_tab(window, dpi, imageId, widx::tab_challenge);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::tab_companies;
                widget::draw_tab(window, dpi, imageId, widx::tab_companies);
            }

            // Finances tab
            {
                static const uint32_t financesTabImageIds[] = {
                    interface_skin::image_ids::tab_finances_frame0,
                    interface_skin::image_ids::tab_finances_frame1,
                    interface_skin::image_ids::tab_finances_frame2,
                    interface_skin::image_ids::tab_finances_frame3,
                    interface_skin::image_ids::tab_finances_frame4,
                    interface_skin::image_ids::tab_finances_frame5,
                    interface_skin::image_ids::tab_finances_frame6,
                    interface_skin::image_ids::tab_finances_frame7,
                    interface_skin::image_ids::tab_finances_frame8,
                    interface_skin::image_ids::tab_finances_frame9,
                    interface_skin::image_ids::tab_finances_frame10,
                    interface_skin::image_ids::tab_finances_frame11,
                    interface_skin::image_ids::tab_finances_frame12,
                    interface_skin::image_ids::tab_finances_frame13,
                    interface_skin::image_ids::tab_finances_frame14,
                    interface_skin::image_ids::tab_finances_frame15,
                };

                uint32_t imageId = skin->img;
                if (window->current_tab == widx::tab_finances - widx::tab_challenge)
                    imageId += financesTabImageIds[(window->frame_no / 2) % std::size(financesTabImageIds)];
                else
                    imageId += financesTabImageIds[0];

                widget::draw_tab(window, dpi, imageId, widx::tab_finances);
            }

            // Scenario details tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::tab_scenario_details;
                widget::draw_tab(window, dpi, imageId, widx::tab_scenario);
            }
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

        static const string_id objectiveTypeLabelIds[] = {
            string_ids::objective_achieve_a_certain_company_value,
            string_ids::objective_achieve_a_certain_monthly_profit_from_vehicles,
            string_ids::objective_achieve_a_certain_performance_index,
            string_ids::objective_deliver_a_certain_amount_of_cargo,
        };

        static const uint8_t maxCargoObjects = static_cast<uint8_t>(objectmgr::get_max_objects(object_type::cargo));
        static int16_t cargoByDropdownIndex[maxCargoObjects] = { -1 };

        // 0x0043FD51
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            switch (widgetIndex)
            {
                case widx::objective_type_btn:
                    *objectiveType = itemIndex;
                    self->invalidate();
                    break;

                case widx::objective_cargo_btn:
                {
                    *objectiveDeliveredCargoType = cargoByDropdownIndex[itemIndex];
                    self->invalidate();
                }
            }
        }

        // 0x0043FD14
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::objective_type_btn:
                {
                    widget_t& target = self->widgets[widx::objective_type];
                    dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(objectiveTypeLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(objectiveTypeLabelIds); i++)
                        dropdown::add(i, string_ids::dropdown_stringid, objectiveTypeLabelIds[i]);

                    dropdown::set_item_selected(*objectiveType);
                    break;
                }

                case widx::objective_value_down:
                {
                    switch (*objectiveType)
                    {
                        case scenario::objective_type::company_value:
                            *objectiveCompanyValue = std::max<uint32_t>(*objectiveCompanyValue - 100000, scenario::min_objective_company_value);
                            break;

                        case scenario::objective_type::vehicle_profit:
                            *objectiveMonthlyVehicleProfit = std::max<uint32_t>(*objectiveMonthlyVehicleProfit - 1000, scenario::min_objective_monthly_profit_from_vehicles);
                            break;

                        case scenario::objective_type::performance_index:
                            *objectivePerformanceIndex = std::max<uint8_t>(*objectivePerformanceIndex - 5, scenario::min_objective_performance_index);
                            break;

                        case scenario::objective_type::cargo_delivery:
                        {
                            uint16_t stepSize{};
                            if (*_clickRepeatTicks < 100)
                                stepSize = 100;
                            else if (*_clickRepeatTicks >= 100)
                                stepSize = 1000;
                            else if (*_clickRepeatTicks >= 200)
                                stepSize = 10000;

                            // Round off cargo to the nearest multiple of the step size.
                            uint16_t cargoFactor = (*objectiveDeliveredCargoAmount - stepSize) / stepSize;
                            uint32_t newDeliveredCargoAmount = cargoFactor * stepSize;

                            *objectiveDeliveredCargoAmount = std::max<uint32_t>(newDeliveredCargoAmount, scenario::min_objective_delivered_cargo);
                            break;
                        }
                    }

                    self->invalidate();
                    break;
                }

                case widx::objective_value_up:
                {
                    switch (*objectiveType)
                    {
                        case scenario::objective_type::company_value:
                            *objectiveCompanyValue = std::min<uint32_t>(*objectiveCompanyValue + 100000, scenario::max_objective_company_value);
                            break;

                        case scenario::objective_type::vehicle_profit:
                            *objectiveMonthlyVehicleProfit = std::min<uint32_t>(*objectiveMonthlyVehicleProfit + 1000, scenario::max_objective_monthly_profit_from_vehicles);
                            break;

                        case scenario::objective_type::performance_index:
                            *objectivePerformanceIndex = std::min<uint8_t>(*objectivePerformanceIndex + 5, scenario::max_objective_performance_index);
                            break;

                        case scenario::objective_type::cargo_delivery:
                        {
                            uint16_t stepSize{};
                            if (*_clickRepeatTicks < 100)
                                stepSize = 100;
                            else if (*_clickRepeatTicks >= 100)
                                stepSize = 1000;
                            else if (*_clickRepeatTicks >= 200)
                                stepSize = 10000;

                            // Round off cargo to the nearest multiple of the step size.
                            uint16_t cargoFactor = (*objectiveDeliveredCargoAmount + stepSize) / stepSize;
                            uint32_t newDeliveredCargoAmount = cargoFactor * stepSize;

                            *objectiveDeliveredCargoAmount = std::max<uint32_t>(newDeliveredCargoAmount, scenario::min_objective_delivered_cargo);
                            break;
                        }
                    }

                    self->invalidate();
                    break;
                }

                case widx::objective_cargo_btn:
                {
                    uint16_t numCargoObjects = 0;
                    for (uint16_t cargoIdx = 0; cargoIdx < maxCargoObjects; cargoIdx++)
                    {
                        auto cargoObject = objectmgr::get<cargo_object>(cargoIdx);
                        if (cargoObject != nullptr)
                            numCargoObjects++;
                    }

                    widget_t& target = self->widgets[widx::objective_cargo];
                    dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], numCargoObjects, 0x80);

                    uint16_t dropdownIndex = 0;
                    for (uint16_t cargoIdx = 0; cargoIdx < maxCargoObjects; cargoIdx++)
                    {
                        auto cargoObject = objectmgr::get<cargo_object>(cargoIdx);
                        if (cargoObject == nullptr)
                            continue;

                        dropdown::add(dropdownIndex, string_ids::dropdown_stringid, cargoObject->name);
                        cargoByDropdownIndex[dropdownIndex] = cargoIdx;

                        if (cargoIdx == *objectiveDeliveredCargoType)
                            dropdown::set_item_selected(dropdownIndex);

                        dropdownIndex++;
                    }
                    break;
                }

                case widx::time_limit_value_down:
                {
                    *objectiveTimeLimitYears = std::max<uint8_t>(*objectiveTimeLimitYears - 1, scenario::min_objective_year_limit);
                    self->invalidate();
                    break;
                }

                case widx::time_limit_value_up:
                {
                    *objectiveTimeLimitYears = std::min<uint8_t>(*objectiveTimeLimitYears + 1, scenario::max_objective_year_limit);
                    self->invalidate();
                    break;
                }
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

            widgets[widx::objective_type].text = objectiveTypeLabelIds[*objectiveType];
            widgets[widx::objective_cargo].type = widget_type::none;
            widgets[widx::objective_cargo_btn].type = widget_type::none;
            widgets[widx::time_limit_value].type = widget_type::none;
            widgets[widx::time_limit_value_down].type = widget_type::none;
            widgets[widx::time_limit_value_up].type = widget_type::none;

            switch (*objectiveType)
            {
                case scenario::objective_type::company_value:
                    *(int32_t*)&*commonFormatArgs = *objectiveCompanyValue;
                    widgets[widx::objective_value].text = string_ids::challenge_monetary_value;
                    break;

                case scenario::objective_type::vehicle_profit:
                    *(int32_t*)&*commonFormatArgs = *objectiveMonthlyVehicleProfit;
                    widgets[widx::objective_value].text = string_ids::challenge_monetary_value;
                    break;

                case scenario::objective_type::performance_index:
                    *(int16_t*)&*commonFormatArgs = *objectivePerformanceIndex;
                    widgets[widx::objective_value].text = string_ids::challenge_performance_index;
                    break;

                case scenario::objective_type::cargo_delivery:
                    *(int32_t*)&*commonFormatArgs = *objectiveDeliveredCargoAmount;
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
            events.on_dropdown = on_dropdown;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
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
        enum widx
        {
            max_competing_companies = 7,
            max_competing_companies_down,
            max_competing_companies_up,
            delay_before_competing_companies_start,
            delay_before_competing_companies_start_down,
            delay_before_competing_companies_start_up,
            preferred_intelligence,
            preferred_intelligence_btn,
            preferred_aggressiveness,
            preferred_aggressiveness_btn,
            preferred_competitiveness,
            preferred_competitiveness_btn,
            competitor_forbid_trains,
            competitor_forbid_buses,
            competitor_forbid_trucks,
            competitor_forbid_trams,
            competitor_forbid_aircraft,
            competitor_forbid_ships,
            player_forbid_trains,
            player_forbid_buses,
            player_forbid_trucks,
            player_forbid_trams,
            player_forbid_aircraft,
            player_forbid_ships,
        };

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

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::max_competing_companies_down) | (1 << widx::max_competing_companies_up) | (1 << widx::delay_before_competing_companies_start_down) | (1 << widx::delay_before_competing_companies_start_up) | (1 << widx::preferred_intelligence) | (1 << widx::preferred_intelligence_btn) | (1 << widx::preferred_aggressiveness) | (1 << widx::preferred_aggressiveness_btn) | (1 << widx::preferred_competitiveness) | (1 << widx::preferred_competitiveness_btn) | (1 << widx::competitor_forbid_trains) | (1 << widx::competitor_forbid_buses) | (1 << widx::competitor_forbid_trucks) | (1 << widx::competitor_forbid_trams) | (1 << widx::competitor_forbid_aircraft) | (1 << widx::competitor_forbid_ships) | (1 << widx::player_forbid_trains) | (1 << widx::player_forbid_buses) | (1 << widx::player_forbid_trucks) | (1 << widx::player_forbid_trams) | (1 << widx::player_forbid_aircraft) | (1 << widx::player_forbid_ships);
        const uint64_t holdableWidgets = (1 << widx::max_competing_companies_down) | (1 << widx::max_competing_companies_up) | (1 << widx::delay_before_competing_companies_start_down) | (1 << widx::delay_before_competing_companies_start_up);

        static window_event_list events;

        // 0x0043F4EB
        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::max_competing_companies].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::max_competing_companies);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::delay_before_competing_companies_start].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::delay_before_competing_companies_start);
            }

            {
                const int16_t xPos = window->x + 15;
                int16_t yPos = window->y + widgets[widx::preferred_intelligence].top - 14;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::selection_of_competing_companies);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::preferred_intelligence].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::preferred_intelligence);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::preferred_aggressiveness].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::preferred_aggressiveness);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::preferred_competitiveness].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::preferred_competitiveness);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::competitor_forbid_trains].top - 12;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::forbid_competing_companies_from_using);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::player_forbid_trains].top - 12;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::forbid_player_companies_from_using);
            }
        }

        static string_id preferenceLabelIds[] = {
            string_ids::preference_any,
            string_ids::preference_low,
            string_ids::preference_medium,
            string_ids::preference_high,
        };

        // 0x0043F67C
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            switch (widgetIndex)
            {
                case widx::preferred_intelligence_btn:
                    *preferredAIIntelligence = itemIndex;
                    break;

                case widx::preferred_aggressiveness_btn:
                    *preferredAIAggressiveness = itemIndex;
                    break;

                case widx::preferred_competitiveness_btn:
                    *preferredAICompetitiveness = itemIndex;
                    break;
            }

            self->invalidate();
        }

        // 0x0043F639
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::max_competing_companies_down:
                    *maxCompetingCompanies = std::max<int8_t>(*maxCompetingCompanies - 1, scenario::min_competing_companies);
                    self->invalidate();
                    break;

                case widx::max_competing_companies_up:
                    *maxCompetingCompanies = std::min<uint8_t>(*maxCompetingCompanies + 1, scenario::max_competing_companies);
                    self->invalidate();
                    break;

                case widx::delay_before_competing_companies_start_down:
                    *competitorStartDelay = std::max<int8_t>(*competitorStartDelay - 1, scenario::min_competitor_start_delay);
                    self->invalidate();
                    break;

                case widx::delay_before_competing_companies_start_up:
                    *competitorStartDelay = std::min<uint8_t>(*competitorStartDelay + 1, scenario::max_competitor_start_delay);
                    self->invalidate();
                    break;

                case widx::preferred_intelligence_btn:
                {
                    widget_t& target = self->widgets[widx::preferred_intelligence];
                    dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        dropdown::add(i, string_ids::dropdown_stringid, preferenceLabelIds[i]);

                    dropdown::set_item_selected(*preferredAIIntelligence);
                    break;
                }

                case widx::preferred_aggressiveness_btn:
                {
                    widget_t& target = self->widgets[widx::preferred_aggressiveness];
                    dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        dropdown::add(i, string_ids::dropdown_stringid, preferenceLabelIds[i]);

                    dropdown::set_item_selected(*preferredAIAggressiveness);
                    break;
                }

                case widx::preferred_competitiveness_btn:
                {
                    widget_t& target = self->widgets[widx::preferred_competitiveness];
                    dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        dropdown::add(i, string_ids::dropdown_stringid, preferenceLabelIds[i]);

                    dropdown::set_item_selected(*preferredAICompetitiveness);
                    break;
                }
            }
        }

        // 0x0043F60C
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

                case widx::competitor_forbid_trains:
                case widx::competitor_forbid_buses:
                case widx::competitor_forbid_trucks:
                case widx::competitor_forbid_trams:
                case widx::competitor_forbid_aircraft:
                case widx::competitor_forbid_ships:
                {
                    uint16_t targetVehicle = static_cast<uint16_t>(widgetIndex - widx::competitor_forbid_trains);
                    uint16_t newForbiddenVehicles = *forbiddenVehiclesCompetitors ^ (1 << targetVehicle);
                    // TODO(avgeffen): Add a constant for this mask.
                    if (newForbiddenVehicles != 0b111111)
                    {
                        *forbiddenVehiclesCompetitors = newForbiddenVehicles;
                        self->invalidate();
                    }
                    break;
                }

                case widx::player_forbid_trains:
                case widx::player_forbid_buses:
                case widx::player_forbid_trucks:
                case widx::player_forbid_trams:
                case widx::player_forbid_aircraft:
                case widx::player_forbid_ships:
                {
                    uint16_t targetVehicle = static_cast<uint16_t>(widgetIndex - widx::player_forbid_trains);
                    uint16_t newForbiddenVehicles = *forbiddenVehiclesPlayers ^ (1 << targetVehicle);
                    // TODO(avgeffen): Add a constant for this mask.
                    if (newForbiddenVehicles != 0b111111)
                    {
                        *forbiddenVehiclesPlayers = newForbiddenVehicles;
                        self->invalidate();
                    }
                    break;
                }
            }
        }

        // 0x0043F40C
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            commonFormatArgs[0] = *maxCompetingCompanies;
            commonFormatArgs[1] = *competitorStartDelay;

            self->widgets[widx::preferred_intelligence].text = preferenceLabelIds[*preferredAIIntelligence];
            self->widgets[widx::preferred_aggressiveness].text = preferenceLabelIds[*preferredAIAggressiveness];
            self->widgets[widx::preferred_competitiveness].text = preferenceLabelIds[*preferredAICompetitiveness];

            self->activated_widgets &= ~((1 << widx::competitor_forbid_trains) | (1 << widx::competitor_forbid_buses) | (1 << widx::competitor_forbid_trucks) | (1 << widx::competitor_forbid_trams) | (1 << widx::competitor_forbid_aircraft) | (1 << widx::competitor_forbid_ships) | (1 << widx::player_forbid_trains) | (1 << widx::player_forbid_buses) | (1 << widx::player_forbid_trucks) | (1 << widx::player_forbid_trams) | (1 << widx::player_forbid_aircraft) | (1 << widx::player_forbid_ships));

            // TODO(avgeffen): replace with wicked smart widget-id kerfuffle, someday.
            self->activated_widgets |= *forbiddenVehiclesCompetitors << static_cast<uint64_t>(widx::competitor_forbid_trains);
            self->activated_widgets |= *forbiddenVehiclesPlayers << static_cast<uint64_t>(widx::player_forbid_trains);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_dropdown = on_dropdown;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
        }
    }

    namespace finances
    {
        enum widx
        {
            starting_loan = 7,
            starting_loan_down,
            starting_loan_up,
            max_loan_size,
            max_loan_size_down,
            max_loan_size_up,
            loan_interest_rate,
            loan_interest_rate_down,
            loan_interest_rate_up,
        };

        static widget_t widgets[] = {
            commonWidgets(217, string_ids::title_financial_options),
            make_stepper_widgets({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::starting_loan_value),
            make_stepper_widgets({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::max_loan_size_value),
            make_stepper_widgets({ 256, 82 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::loan_interest_rate_value),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);
        const uint64_t holdableWidgets = (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);

        static window_event_list events;

        // 0x0043F97D
        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::starting_loan].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::starting_loan);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::max_loan_size].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::max_loan_size);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::loan_interest_rate].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::loan_interest_rate);
            }
        }

        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::starting_loan_down:
                    *startingLoanSize = std::max<int16_t>(*startingLoanSize - 50, scenario::min_start_loan_units);
                    break;

                case widx::starting_loan_up:
                    *startingLoanSize = std::min<uint16_t>(*startingLoanSize + 50, scenario::max_start_loan_units);
                    if (*startingLoanSize > *maxLoanSize)
                        *maxLoanSize = *startingLoanSize;
                    break;

                case widx::max_loan_size_down:
                    *maxLoanSize = std::max<int16_t>(*maxLoanSize - 50, scenario::min_loan_size_units);
                    if (*startingLoanSize > *maxLoanSize)
                        *startingLoanSize = *maxLoanSize;
                    break;

                case widx::max_loan_size_up:
                    *maxLoanSize = std::min<uint16_t>(*maxLoanSize + 50, scenario::max_loan_size_units);
                    break;

                case widx::loan_interest_rate_down:
                    *loanInterestRate = std::max<int16_t>(*loanInterestRate - 1, scenario::min_loan_interest_units);
                    break;

                case widx::loan_interest_rate_up:
                    *loanInterestRate = std::min<uint16_t>(*loanInterestRate + 1, scenario::max_loan_interest_units);
                    break;
            }

            self->invalidate();
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
            }
        }

        // 0x0046E306
        static uint32_t getLoanSizeInCurrency()
        {
            uint64_t loanSizeInCurrency = ((*startingLoanSize * *currencyMultiplicationFactor) >> 8ULL) / 100 * 100;
            return static_cast<uint32_t>(loanSizeInCurrency);
        }

        // 0x0043F8CF
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            uint32_t loanSizeInCurrency = getLoanSizeInCurrency();
            *(uint32_t*)&commonFormatArgs[0] = loanSizeInCurrency;

            uint64_t maxLoanSizeInCurrency = ((*maxLoanSize * *currencyMultiplicationFactor) >> 8ULL) / 100 * 100;
            *(uint32_t*)&commonFormatArgs[2] = static_cast<uint32_t>(maxLoanSizeInCurrency);

            *(uint32_t*)&commonFormatArgs[4] = *loanInterestRate;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
        }
    }

    namespace scenario
    {
        enum widx
        {
            change_name_btn = 7,
            scenario_group,
            scenario_group_btn,
            change_details_btn,
        };

        static widget_t widgets[] = {
            commonWidgets(217, string_ids::title_scenario_options),
            make_widget({ 281, 52 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::change),
            make_widget({ 196, 67 }, { 160, 12 }, widget_type::wt_18, 1, string_ids::empty),
            make_widget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 281, 82 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::change),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::change_name_btn) | (1 << widx::scenario_group) | (1 << widx::scenario_group_btn) | (1 << widx::change_details_btn);
        const uint64_t holdableWidgets = 0;

        static window_event_list events;

        // 0x0043F004
        static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
        {
            common::draw(window, dpi);

            {
                // Prepare scenario name text.
                char* buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
                strncpy(buffer, s5::getOptions().scenarioName, 512);
                commonFormatArgs[0] = string_ids::buffer_2039;

                auto* stex = objectmgr::get<scenario_text_object>();
                if (stex != nullptr)
                    commonFormatArgs[0] = stex->name;

                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::change_name_btn].top + 1;
                int16_t width = widgets[widx::change_name_btn].left - 20;
                gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::scenario_name_stringid, &*commonFormatArgs);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::scenario_group].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::scenario_group);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::change_details_btn].top + 1;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::scenario_details);
            }

            {
                // Prepare scenario details text.
                char* buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
                strncpy(buffer, s5::getOptions().scenarioDetails, 512);
                commonFormatArgs[0] = string_ids::buffer_2039;

                auto* stex = objectmgr::get<scenario_text_object>();
                if (stex != nullptr)
                    commonFormatArgs[0] = stex->details;

                auto& target = window->widgets[widx::change_details_btn];
                gfx::draw_string_495224(*dpi, window->x + 16, window->y + 12 + target.top, target.left - 26, colour::black, string_ids::white_stringid2, &*commonFormatArgs);
            }
        }

        static string_id scenarioGroupLabelIds[] = {
            string_ids::scenario_group_beginner,
            string_ids::scenario_group_easy,
            string_ids::scenario_group_medium,
            string_ids::scenario_group_challenging,
            string_ids::scenario_group_expert,
        };

        // 0x0043F14B
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == widx::scenario_group_btn && itemIndex != -1)
            {
                s5::getOptions().difficulty = itemIndex;
                self->invalidate();
            }
        }

        // 0x0043F140
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == widx::scenario_group_btn)
            {
                widget_t& target = self->widgets[widx::scenario_group];
                dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(scenarioGroupLabelIds), 0x80);

                for (size_t i = 0; i < std::size(scenarioGroupLabelIds); i++)
                    dropdown::add(i, string_ids::dropdown_stringid, scenarioGroupLabelIds[i]);

                dropdown::set_item_selected(s5::getOptions().difficulty);
            }
        }

        // 0x0043F11F
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

                case widx::change_name_btn:
                {
                    char* buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
                    strncpy(buffer, s5::getOptions().scenarioName, 512);

                    textinput::open_textinput(self, string_ids::scenario_name_title, string_ids::enter_name_for_scenario, string_ids::buffer_2039, widgetIndex, nullptr);
                    break;
                }

                case widx::change_details_btn:
                {
                    char* buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
                    strncpy(buffer, s5::getOptions().scenarioDetails, 512);

                    textinput::open_textinput(self, string_ids::scenario_details_title, string_ids::enter_description_of_this_scenario, string_ids::buffer_2039, widgetIndex, nullptr);
                    break;
                }
            }
        }

        // 0x0043EF8B
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            widgets[widx::scenario_group].text = scenarioGroupLabelIds[s5::getOptions().difficulty];
        }

        // 0x0043F156
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            switch (callingWidget)
            {
                case widx::change_name_btn:
                {
                    strncpy(s5::getOptions().scenarioName, input, sizeof(s5::Options::scenarioName) - 1);
                    s5::getOptions().scenarioName[sizeof(s5::Options::scenarioName) - 1] = '\0';
                    self->invalidate();
                    break;
                }

                case widx::change_details_btn:
                {
                    strncpy(s5::getOptions().scenarioDetails, input, sizeof(s5::Options::scenarioDetails) - 1);
                    s5::getOptions().scenarioDetails[sizeof(s5::Options::scenarioDetails) - 1] = '\0';
                    self->invalidate();
                    break;
                }
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_dropdown = on_dropdown;
            events.on_mouse_down = on_mouse_down;
            events.on_mouse_up = on_mouse_up;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = text_input;
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
