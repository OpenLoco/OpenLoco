#include "../Economy/Economy.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CargoObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/ScenarioTextObject.h"
#include "../S5/S5.h"
#include "../Scenario.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ScenarioOptions
{
    static const Gfx::ui_size_t challengeWindowSize = { 366, 197 };
    static const Gfx::ui_size_t companiesWindowSize = { 366, 327 };
    static const Gfx::ui_size_t otherWindowSize = { 366, 217 };

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

    namespace Common
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

#define commonWidgets(frameHeight, windowCaptionId)                                                                         \
    makeWidget({ 0, 0 }, { 366, frameHeight }, widget_type::frame, 0),                                                      \
        makeWidget({ 1, 1 }, { 364, 13 }, widget_type::caption_25, 0, windowCaptionId),                                     \
        makeWidget({ 0, 41 }, { 366, 175 }, widget_type::panel, 1),                                                         \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_scenario_challenge), \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_company_options),   \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_financial_options), \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_scenario_options)

        // Defined at the bottom of this file.
        static void initEvents();

        // 0x00440082
        static void update(window* window)
        {
            window->frame_no++;
            window->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::scenarioOptions, window->number, window->current_tab + widx::tab_challenge);
        }

        // 0x004400A4
        static void drawTabs(window* window, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Challenge tab
            {
                static const uint32_t challengeTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cup_frame0,
                    InterfaceSkin::ImageIds::tab_cup_frame1,
                    InterfaceSkin::ImageIds::tab_cup_frame2,
                    InterfaceSkin::ImageIds::tab_cup_frame3,
                    InterfaceSkin::ImageIds::tab_cup_frame4,
                    InterfaceSkin::ImageIds::tab_cup_frame5,
                    InterfaceSkin::ImageIds::tab_cup_frame6,
                    InterfaceSkin::ImageIds::tab_cup_frame7,
                    InterfaceSkin::ImageIds::tab_cup_frame8,
                    InterfaceSkin::ImageIds::tab_cup_frame9,
                    InterfaceSkin::ImageIds::tab_cup_frame10,
                    InterfaceSkin::ImageIds::tab_cup_frame11,
                    InterfaceSkin::ImageIds::tab_cup_frame12,
                    InterfaceSkin::ImageIds::tab_cup_frame13,
                    InterfaceSkin::ImageIds::tab_cup_frame14,
                    InterfaceSkin::ImageIds::tab_cup_frame15,
                };

                uint32_t imageId = skin->img;
                if (window->current_tab == widx::tab_challenge - widx::tab_challenge)
                    imageId += challengeTabImageIds[(window->frame_no / 4) % std::size(challengeTabImageIds)];
                else
                    imageId += challengeTabImageIds[0];

                Widget::draw_tab(window, context, imageId, widx::tab_challenge);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_companies;
                Widget::draw_tab(window, context, imageId, widx::tab_companies);
            }

            // Finances tab
            {
                static const uint32_t financesTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_finances_frame0,
                    InterfaceSkin::ImageIds::tab_finances_frame1,
                    InterfaceSkin::ImageIds::tab_finances_frame2,
                    InterfaceSkin::ImageIds::tab_finances_frame3,
                    InterfaceSkin::ImageIds::tab_finances_frame4,
                    InterfaceSkin::ImageIds::tab_finances_frame5,
                    InterfaceSkin::ImageIds::tab_finances_frame6,
                    InterfaceSkin::ImageIds::tab_finances_frame7,
                    InterfaceSkin::ImageIds::tab_finances_frame8,
                    InterfaceSkin::ImageIds::tab_finances_frame9,
                    InterfaceSkin::ImageIds::tab_finances_frame10,
                    InterfaceSkin::ImageIds::tab_finances_frame11,
                    InterfaceSkin::ImageIds::tab_finances_frame12,
                    InterfaceSkin::ImageIds::tab_finances_frame13,
                    InterfaceSkin::ImageIds::tab_finances_frame14,
                    InterfaceSkin::ImageIds::tab_finances_frame15,
                };

                uint32_t imageId = skin->img;
                if (window->current_tab == widx::tab_finances - widx::tab_challenge)
                    imageId += financesTabImageIds[(window->frame_no / 2) % std::size(financesTabImageIds)];
                else
                    imageId += financesTabImageIds[0];

                Widget::draw_tab(window, context, imageId, widx::tab_finances);
            }

            // Scenario details tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_scenario_details;
                Widget::draw_tab(window, context, imageId, widx::tab_scenario);
            }
        }

        static void draw(window* window, Gfx::Context* context)
        {
            window->draw(context);
            drawTabs(window, context);
        }

        static void prepareDraw(window* self);

        static void switchTab(window* self, widget_index widgetIndex);
    }

    namespace Challenge
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

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::objective_type) | (1 << widx::objective_type_btn) | (1 << widx::objective_value_down) | (1 << widx::objective_value_up) | (1 << widx::objective_cargo) | (1 << widx::objective_cargo_btn) | (1 << widx::check_be_top_company) | (1 << widx::check_be_within_top_three_companies) | (1 << widx::check_time_limit) | (1 << widx::time_limit_value_down) | (1 << widx::time_limit_value_up);
        const uint64_t holdableWidgets = (1 << widx::objective_value_down) | (1 << widx::objective_value_up) | (1 << widx::time_limit_value_down) | (1 << widx::time_limit_value_up);

        static widget_t widgets[] = {
            commonWidgets(197, StringIds::title_scenario_challenge),
            makeWidget({ 10, 52 }, { 346, 12 }, widget_type::wt_18, 1),
            makeWidget({ 344, 53 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeStepperWidgets({ 10, 67 }, { 163, 12 }, widget_type::wt_17, 1),
            makeWidget({ 193, 67 }, { 163, 12 }, widget_type::wt_18, 1),
            makeWidget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 10, 83 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::and_be_the_top_company),
            makeWidget({ 10, 98 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::and_be_within_the_top_companies),
            makeWidget({ 10, 113 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::with_a_time_limit),
            makeStepperWidgets({ 256, 112 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::time_limit_years_value),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0043FC91
        static void draw(Ui::window* window, Gfx::Context* context)
        {
            Common::draw(window, context);

            const int16_t xPos = window->x + 5;
            int16_t yPos = window->y + widgets[widx::check_time_limit].bottom + 10;
            Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::challenge_label);

            FormatArguments args = {};
            OpenLoco::Scenario::formatChallengeArguments(args);
            yPos += 10;
            Gfx::drawString_495224(*context, xPos, yPos, window->width - 10, Colour::black, StringIds::challenge_value, &args);
        }

        static const string_id objectiveTypeLabelIds[] = {
            StringIds::objective_achieve_a_certain_company_value,
            StringIds::objective_achieve_a_certain_monthly_profit_from_vehicles,
            StringIds::objective_achieve_a_certain_performance_index,
            StringIds::objective_deliver_a_certain_amount_of_cargo,
        };

        static const uint8_t maxCargoObjects = static_cast<uint8_t>(ObjectManager::getMaxObjects(ObjectType::cargo));
        static int16_t cargoByDropdownIndex[maxCargoObjects] = { -1 };

        // 0x0043FD51
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
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
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::objective_type_btn:
                {
                    widget_t& target = self->widgets[widx::objective_type];
                    Dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(objectiveTypeLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(objectiveTypeLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, objectiveTypeLabelIds[i]);

                    Dropdown::setItemSelected(*objectiveType);
                    break;
                }

                case widx::objective_value_down:
                {
                    switch (*objectiveType)
                    {
                        case Scenario::objective_type::company_value:
                            *objectiveCompanyValue = std::max<uint32_t>(*objectiveCompanyValue - 100000, Scenario::min_objective_company_value);
                            break;

                        case Scenario::objective_type::vehicle_profit:
                            *objectiveMonthlyVehicleProfit = std::max<uint32_t>(*objectiveMonthlyVehicleProfit - 1000, Scenario::min_objective_monthly_profit_from_vehicles);
                            break;

                        case Scenario::objective_type::performance_index:
                            *objectivePerformanceIndex = std::max<uint8_t>(*objectivePerformanceIndex - 5, Scenario::min_objective_performance_index);
                            break;

                        case Scenario::objective_type::cargo_delivery:
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

                            *objectiveDeliveredCargoAmount = std::max<uint32_t>(newDeliveredCargoAmount, Scenario::min_objective_delivered_cargo);
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
                        case Scenario::objective_type::company_value:
                            *objectiveCompanyValue = std::min<uint32_t>(*objectiveCompanyValue + 100000, Scenario::max_objective_company_value);
                            break;

                        case Scenario::objective_type::vehicle_profit:
                            *objectiveMonthlyVehicleProfit = std::min<uint32_t>(*objectiveMonthlyVehicleProfit + 1000, Scenario::max_objective_monthly_profit_from_vehicles);
                            break;

                        case Scenario::objective_type::performance_index:
                            *objectivePerformanceIndex = std::min<uint8_t>(*objectivePerformanceIndex + 5, Scenario::max_objective_performance_index);
                            break;

                        case Scenario::objective_type::cargo_delivery:
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

                            *objectiveDeliveredCargoAmount = std::max<uint32_t>(newDeliveredCargoAmount, Scenario::min_objective_delivered_cargo);
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
                        auto cargoObject = ObjectManager::get<CargoObject>(cargoIdx);
                        if (cargoObject != nullptr)
                            numCargoObjects++;
                    }

                    widget_t& target = self->widgets[widx::objective_cargo];
                    Dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], numCargoObjects, 0x80);

                    uint16_t dropdownIndex = 0;
                    for (uint16_t cargoIdx = 0; cargoIdx < maxCargoObjects; cargoIdx++)
                    {
                        auto cargoObject = ObjectManager::get<CargoObject>(cargoIdx);
                        if (cargoObject == nullptr)
                            continue;

                        Dropdown::add(dropdownIndex, StringIds::dropdown_stringid, cargoObject->name);
                        cargoByDropdownIndex[dropdownIndex] = cargoIdx;

                        if (cargoIdx == *objectiveDeliveredCargoType)
                            Dropdown::setItemSelected(dropdownIndex);

                        dropdownIndex++;
                    }
                    break;
                }

                case widx::time_limit_value_down:
                {
                    *objectiveTimeLimitYears = std::max<uint8_t>(*objectiveTimeLimitYears - 1, Scenario::min_objective_year_limit);
                    self->invalidate();
                    break;
                }

                case widx::time_limit_value_up:
                {
                    *objectiveTimeLimitYears = std::min<uint8_t>(*objectiveTimeLimitYears + 1, Scenario::max_objective_year_limit);
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x0043FCED
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(self, widgetIndex);
                    break;

                case check_be_top_company:
                    *objectiveFlags ^= Scenario::objective_flags::be_top_company;
                    self->invalidate();
                    break;

                case check_be_within_top_three_companies:
                    *objectiveFlags ^= Scenario::objective_flags::be_within_top_three_companies;
                    self->invalidate();
                    break;

                case check_time_limit:
                    *objectiveFlags ^= Scenario::objective_flags::within_time_limit;
                    self->invalidate();
                    break;
            }
        }

        // 0x0043FB0C
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            widgets[widx::objective_type].text = objectiveTypeLabelIds[*objectiveType];
            widgets[widx::objective_cargo].type = widget_type::none;
            widgets[widx::objective_cargo_btn].type = widget_type::none;
            widgets[widx::time_limit_value].type = widget_type::none;
            widgets[widx::time_limit_value_down].type = widget_type::none;
            widgets[widx::time_limit_value_up].type = widget_type::none;

            switch (*objectiveType)
            {
                case Scenario::objective_type::company_value:
                    *(int32_t*)&*commonFormatArgs = *objectiveCompanyValue;
                    widgets[widx::objective_value].text = StringIds::challenge_monetary_value;
                    break;

                case Scenario::objective_type::vehicle_profit:
                    *(int32_t*)&*commonFormatArgs = *objectiveMonthlyVehicleProfit;
                    widgets[widx::objective_value].text = StringIds::challenge_monetary_value;
                    break;

                case Scenario::objective_type::performance_index:
                    *(int16_t*)&*commonFormatArgs = *objectivePerformanceIndex * 10;
                    widgets[widx::objective_value].text = StringIds::challenge_performance_index;
                    break;

                case Scenario::objective_type::cargo_delivery:
                    *(int32_t*)&*commonFormatArgs = *objectiveDeliveredCargoAmount;
                    widgets[widx::objective_value].text = StringIds::challenge_delivered_cargo;

                    auto cargo = ObjectManager::get<CargoObject>(*objectiveDeliveredCargoType);
                    widgets[widx::objective_cargo].text = cargo->name;
                    widgets[widx::objective_cargo].type = widget_type::wt_18;
                    widgets[widx::objective_cargo_btn].type = widget_type::wt_11;
                    break;
            }

            self->activated_widgets &= ~((1 << widx::check_be_top_company) | (1 << widx::check_be_within_top_three_companies) | (1 << widx::check_time_limit));

            if ((*objectiveFlags & Scenario::objective_flags::be_top_company) != 0)
                self->activated_widgets |= 1 << widx::check_be_top_company;

            if ((*objectiveFlags & Scenario::objective_flags::be_within_top_three_companies) != 0)
                self->activated_widgets |= 1 << widx::check_be_within_top_three_companies;

            if ((*objectiveFlags & Scenario::objective_flags::within_time_limit) != 0)
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
            events.on_dropdown = onDropdown;
            events.on_mouse_down = onMouseDown;
            events.on_mouse_up = onMouseUp;
            events.on_update = Common::update;
            events.prepare_draw = prepareDraw;
        }
    }

    // 0x0043EE58
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::scenarioOptions, 0);
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
                Input::toolCancel();

            window = WindowManager::bringToFront(WindowType::scenarioOptions, 0);
        }

        if (window == nullptr)
        {
            // 0x0043EEFF start
            window = WindowManager::createWindowCentred(WindowType::scenarioOptions, otherWindowSize, 0, &Challenge::events);
            window->widgets = Challenge::widgets;
            window->enabled_widgets = Challenge::enabledWidgets;
            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
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
        Common::initEvents();

        window->current_tab = 0;
        window->invalidate();

        window->widgets = Challenge::widgets;
        window->enabled_widgets = Challenge::enabledWidgets;
        window->holdable_widgets = Challenge::holdableWidgets;
        window->event_handlers = &Challenge::events;
        window->activated_widgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }

    namespace Companies
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
            commonWidgets(327, StringIds::title_company_options),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::max_competing_companies_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::delay_before_competing_companies_start_months),
            makeWidget({ 246, 102 }, { 110, 12 }, widget_type::wt_18, 1),
            makeWidget({ 344, 103 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 246, 117 }, { 110, 12 }, widget_type::wt_18, 1),
            makeWidget({ 344, 118 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 246, 132 }, { 110, 12 }, widget_type::wt_18, 1),
            makeWidget({ 344, 133 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 15, 160 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_trains),
            makeWidget({ 15, 172 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_buses),
            makeWidget({ 15, 184 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_trucks),
            makeWidget({ 15, 196 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_trams),
            makeWidget({ 15, 208 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_aircraft),
            makeWidget({ 15, 220 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_ships),
            makeWidget({ 15, 247 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_trains),
            makeWidget({ 15, 259 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_buses),
            makeWidget({ 15, 271 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_trucks),
            makeWidget({ 15, 283 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_trams),
            makeWidget({ 15, 295 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_aircraft),
            makeWidget({ 15, 307 }, { 341, 12 }, widget_type::checkbox, 1, StringIds::forbid_ships),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::max_competing_companies_down) | (1 << widx::max_competing_companies_up) | (1 << widx::delay_before_competing_companies_start_down) | (1 << widx::delay_before_competing_companies_start_up) | (1 << widx::preferred_intelligence) | (1 << widx::preferred_intelligence_btn) | (1 << widx::preferred_aggressiveness) | (1 << widx::preferred_aggressiveness_btn) | (1 << widx::preferred_competitiveness) | (1 << widx::preferred_competitiveness_btn) | (1 << widx::competitor_forbid_trains) | (1 << widx::competitor_forbid_buses) | (1 << widx::competitor_forbid_trucks) | (1 << widx::competitor_forbid_trams) | (1 << widx::competitor_forbid_aircraft) | (1 << widx::competitor_forbid_ships) | (1 << widx::player_forbid_trains) | (1 << widx::player_forbid_buses) | (1 << widx::player_forbid_trucks) | (1 << widx::player_forbid_trams) | (1 << widx::player_forbid_aircraft) | (1 << widx::player_forbid_ships);
        const uint64_t holdableWidgets = (1 << widx::max_competing_companies_down) | (1 << widx::max_competing_companies_up) | (1 << widx::delay_before_competing_companies_start_down) | (1 << widx::delay_before_competing_companies_start_up);

        static window_event_list events;

        // 0x0043F4EB
        static void draw(Ui::window* window, Gfx::Context* context)
        {
            Common::draw(window, context);

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::max_competing_companies].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::max_competing_companies);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::delay_before_competing_companies_start].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::delay_before_competing_companies_start);
            }

            {
                const int16_t xPos = window->x + 15;
                int16_t yPos = window->y + widgets[widx::preferred_intelligence].top - 14;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::selection_of_competing_companies);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::preferred_intelligence].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::preferred_intelligence);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::preferred_aggressiveness].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::preferred_aggressiveness);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::preferred_competitiveness].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::preferred_competitiveness);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::competitor_forbid_trains].top - 12;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::forbid_competing_companies_from_using);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::player_forbid_trains].top - 12;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::forbid_player_companies_from_using);
            }
        }

        static string_id preferenceLabelIds[] = {
            StringIds::preference_any,
            StringIds::preference_low,
            StringIds::preference_medium,
            StringIds::preference_high,
        };

        // 0x0043F67C
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
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
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::max_competing_companies_down:
                    *maxCompetingCompanies = std::max<int8_t>(*maxCompetingCompanies - 1, Scenario::min_competing_companies);
                    self->invalidate();
                    break;

                case widx::max_competing_companies_up:
                    *maxCompetingCompanies = std::min<uint8_t>(*maxCompetingCompanies + 1, Scenario::max_competing_companies);
                    self->invalidate();
                    break;

                case widx::delay_before_competing_companies_start_down:
                    *competitorStartDelay = std::max<int8_t>(*competitorStartDelay - 1, Scenario::min_competitor_start_delay);
                    self->invalidate();
                    break;

                case widx::delay_before_competing_companies_start_up:
                    *competitorStartDelay = std::min<uint8_t>(*competitorStartDelay + 1, Scenario::max_competitor_start_delay);
                    self->invalidate();
                    break;

                case widx::preferred_intelligence_btn:
                {
                    widget_t& target = self->widgets[widx::preferred_intelligence];
                    Dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);

                    Dropdown::setItemSelected(*preferredAIIntelligence);
                    break;
                }

                case widx::preferred_aggressiveness_btn:
                {
                    widget_t& target = self->widgets[widx::preferred_aggressiveness];
                    Dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);

                    Dropdown::setItemSelected(*preferredAIAggressiveness);
                    break;
                }

                case widx::preferred_competitiveness_btn:
                {
                    widget_t& target = self->widgets[widx::preferred_competitiveness];
                    Dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);

                    Dropdown::setItemSelected(*preferredAICompetitiveness);
                    break;
                }
            }
        }

        // 0x0043F60C
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(self, widgetIndex);
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
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

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
            events.on_dropdown = onDropdown;
            events.on_mouse_down = onMouseDown;
            events.on_mouse_up = onMouseUp;
            events.on_update = Common::update;
            events.prepare_draw = prepareDraw;
        }
    }

    namespace Finances
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
            commonWidgets(217, StringIds::title_financial_options),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::starting_loan_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::max_loan_size_value),
            makeStepperWidgets({ 256, 82 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::loan_interest_rate_value),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);
        const uint64_t holdableWidgets = (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);

        static window_event_list events;

        // 0x0043F97D
        static void draw(Ui::window* window, Gfx::Context* context)
        {
            Common::draw(window, context);

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::starting_loan].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::starting_loan);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::max_loan_size].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::max_loan_size);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::loan_interest_rate].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::loan_interest_rate);
            }
        }

        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::starting_loan_down:
                    *startingLoanSize = std::max<int16_t>(*startingLoanSize - 50, Scenario::min_start_loan_units);
                    break;

                case widx::starting_loan_up:
                    *startingLoanSize = std::min<uint16_t>(*startingLoanSize + 50, Scenario::max_start_loan_units);
                    if (*startingLoanSize > *maxLoanSize)
                        *maxLoanSize = *startingLoanSize;
                    break;

                case widx::max_loan_size_down:
                    *maxLoanSize = std::max<int16_t>(*maxLoanSize - 50, Scenario::min_loan_size_units);
                    if (*startingLoanSize > *maxLoanSize)
                        *startingLoanSize = *maxLoanSize;
                    break;

                case widx::max_loan_size_up:
                    *maxLoanSize = std::min<uint16_t>(*maxLoanSize + 50, Scenario::max_loan_size_units);
                    break;

                case widx::loan_interest_rate_down:
                    *loanInterestRate = std::max<int16_t>(*loanInterestRate - 1, Scenario::min_loan_interest_units);
                    break;

                case widx::loan_interest_rate_up:
                    *loanInterestRate = std::min<uint16_t>(*loanInterestRate + 1, Scenario::max_loan_interest_units);
                    break;
            }

            self->invalidate();
        }

        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0046E306
        static uint32_t getLoanSizeInCurrency()
        {
            uint64_t loanSizeInCurrency = Economy::getInflationAdjustedCost(*startingLoanSize, 0, 8) / 100 * 100;
            return static_cast<uint32_t>(loanSizeInCurrency);
        }

        // 0x0043F8CF
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            uint32_t loanSizeInCurrency = getLoanSizeInCurrency();
            *(uint32_t*)&commonFormatArgs[0] = loanSizeInCurrency;

            uint64_t maxLoanSizeInCurrency = Economy::getInflationAdjustedCost(*maxLoanSize, 0, 8) / 100 * 100;
            *(uint32_t*)&commonFormatArgs[2] = static_cast<uint32_t>(maxLoanSizeInCurrency);

            *(uint32_t*)&commonFormatArgs[4] = *loanInterestRate;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_down = onMouseDown;
            events.on_mouse_up = onMouseUp;
            events.on_update = Common::update;
            events.prepare_draw = prepareDraw;
        }
    }

    namespace Scenario
    {
        enum widx
        {
            change_name_btn = 7,
            scenario_group,
            scenario_group_btn,
            change_details_btn,
        };

        static widget_t widgets[] = {
            commonWidgets(217, StringIds::title_scenario_options),
            makeWidget({ 281, 52 }, { 75, 12 }, widget_type::wt_11, 1, StringIds::change),
            makeWidget({ 196, 67 }, { 160, 12 }, widget_type::wt_18, 1, StringIds::empty),
            makeWidget({ 344, 68 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 281, 82 }, { 75, 12 }, widget_type::wt_11, 1, StringIds::change),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::change_name_btn) | (1 << widx::scenario_group) | (1 << widx::scenario_group_btn) | (1 << widx::change_details_btn);
        const uint64_t holdableWidgets = 0;

        static window_event_list events;

        // 0x0043F004
        static void draw(Ui::window* window, Gfx::Context* context)
        {
            Common::draw(window, context);

            {
                // Prepare scenario name text.
                char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                strncpy(buffer, S5::getOptions().scenarioName, 512);
                commonFormatArgs[0] = StringIds::buffer_2039;

                auto* stex = ObjectManager::get<ScenarioTextObject>();
                if (stex != nullptr)
                    commonFormatArgs[0] = stex->name;

                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::change_name_btn].top + 1;
                int16_t width = widgets[widx::change_name_btn].left - 20;
                Gfx::drawString_494BBF(*context, xPos, yPos, width, Colour::black, StringIds::scenario_name_stringid, &*commonFormatArgs);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::scenario_group].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::scenario_group);
            }

            {
                const int16_t xPos = window->x + 10;
                int16_t yPos = window->y + widgets[widx::change_details_btn].top + 1;
                Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::scenario_details);
            }

            {
                // Prepare scenario details text.
                char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                strncpy(buffer, S5::getOptions().scenarioDetails, 512);
                commonFormatArgs[0] = StringIds::buffer_2039;

                auto* stex = ObjectManager::get<ScenarioTextObject>();
                if (stex != nullptr)
                    commonFormatArgs[0] = stex->details;

                auto& target = window->widgets[widx::change_details_btn];
                Gfx::drawString_495224(*context, window->x + 16, window->y + 12 + target.top, target.left - 26, Colour::black, StringIds::black_stringid, &*commonFormatArgs);
            }
        }

        static string_id scenarioGroupLabelIds[] = {
            StringIds::scenario_group_beginner,
            StringIds::scenario_group_easy,
            StringIds::scenario_group_medium,
            StringIds::scenario_group_challenging,
            StringIds::scenario_group_expert,
        };

        // 0x0043F14B
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == widx::scenario_group_btn && itemIndex != -1)
            {
                S5::getOptions().difficulty = itemIndex;
                self->invalidate();
            }
        }

        // 0x0043F140
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == widx::scenario_group_btn)
            {
                widget_t& target = self->widgets[widx::scenario_group];
                Dropdown::show(self->x + target.left, self->y + target.top, target.width() - 4, target.height(), self->colours[1], std::size(scenarioGroupLabelIds), 0x80);

                for (size_t i = 0; i < std::size(scenarioGroupLabelIds); i++)
                    Dropdown::add(i, StringIds::dropdown_stringid, scenarioGroupLabelIds[i]);

                Dropdown::setItemSelected(S5::getOptions().difficulty);
            }
        }

        // 0x0043F11F
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::change_name_btn:
                {
                    char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                    strncpy(buffer, S5::getOptions().scenarioName, 512);

                    TextInput::openTextInput(self, StringIds::scenario_name_title, StringIds::enter_name_for_scenario, StringIds::buffer_2039, widgetIndex, nullptr);
                    break;
                }

                case widx::change_details_btn:
                {
                    char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                    strncpy(buffer, S5::getOptions().scenarioDetails, 512);

                    TextInput::openTextInput(self, StringIds::scenario_details_title, StringIds::enter_description_of_this_scenario, StringIds::buffer_2039, widgetIndex, nullptr);
                    break;
                }
            }
        }

        // 0x0043EF8B
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            widgets[widx::scenario_group].text = scenarioGroupLabelIds[S5::getOptions().difficulty];
        }

        // 0x0043F156
        static void textInput(window* self, widget_index callingWidget, const char* input)
        {
            switch (callingWidget)
            {
                case widx::change_name_btn:
                {
                    strncpy(S5::getOptions().scenarioName, input, sizeof(S5::Options::scenarioName) - 1);
                    S5::getOptions().scenarioName[sizeof(S5::Options::scenarioName) - 1] = '\0';
                    self->invalidate();
                    break;
                }

                case widx::change_details_btn:
                {
                    strncpy(S5::getOptions().scenarioDetails, input, sizeof(S5::Options::scenarioDetails) - 1);
                    S5::getOptions().scenarioDetails[sizeof(S5::Options::scenarioDetails) - 1] = '\0';
                    self->invalidate();
                    break;
                }
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_dropdown = onDropdown;
            events.on_mouse_down = onMouseDown;
            events.on_mouse_up = onMouseUp;
            events.on_update = Common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = textInput;
        }
    }

    namespace Common
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
            { Challenge::widgets, widx::tab_challenge, &Challenge::events, &Challenge::enabledWidgets, &Challenge::holdableWidgets },
            { Companies::widgets, widx::tab_companies, &Companies::events, &Companies::enabledWidgets, &Companies::holdableWidgets },
            { Finances::widgets, widx::tab_finances, &Finances::events, &Finances::enabledWidgets, &Finances::holdableWidgets },
            { Scenario::widgets, widx::tab_scenario, &Scenario::events, &Scenario::enabledWidgets, &Scenario::holdableWidgets }
        };

        static void prepareDraw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1 << widx::tab_challenge) | (1 << widx::tab_companies) | (1 << widx::tab_challenge) | (1 << widx::tab_scenario));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Resize common widgets.
            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;
        }

        // 0x0043F16B
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_challenge;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->disabled_widgets = 0;

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_challenge];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = *tabInfo.holdableWidgets;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            self->invalidate();

            const Gfx::ui_size_t* newSize;
            if (widgetIndex == widx::tab_challenge)
                newSize = &challengeWindowSize;
            else if (widgetIndex == widx::tab_companies)
                newSize = &companiesWindowSize;
            else
                newSize = &otherWindowSize;

            self->setSize(*newSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        static void initEvents()
        {
            Challenge::initEvents();
            Companies::initEvents();
            Finances::initEvents();
            Scenario::initEvents();
        }
    }
}
