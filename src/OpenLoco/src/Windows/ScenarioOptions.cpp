#include "CompanyManager.h"
#include "Economy/Economy.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/CargoObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "ScenarioObjective.h"
#include "Ui/Dropdown.h"
#include "Ui/WindowManager.h"
#include "Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ScenarioOptions
{
    static constexpr Ui::Size kChallengeWindowSize = { 366, 197 };
    static constexpr Ui::Size kCompaniesWindowSize = { 366, 327 };
    static constexpr Ui::Size kOtherWindowSize = { 366, 217 };

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

#define commonWidgets(frameHeight, windowCaptionId)                                                                                             \
    makeWidget({ 0, 0 }, { 366, frameHeight }, WidgetType::frame, WindowColour::primary),                                                       \
        makeWidget({ 1, 1 }, { 364, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                      \
        makeWidget({ 0, 41 }, { 366, 175 }, WidgetType::panel, WindowColour::secondary),                                                        \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_scenario_challenge), \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_options),   \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_financial_options), \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_scenario_options)

        // Defined at the bottom of this file.
        static void initEvents();

        // 0x00440082
        static void update(Window& window)
        {
            window.frameNo++;
            window.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::scenarioOptions, window.number, window.currentTab + widx::tab_challenge);
        }

        // 0x004400A4
        static void drawTabs(Window* window, Gfx::RenderTarget* rt)
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
                if (window->currentTab == widx::tab_challenge - widx::tab_challenge)
                    imageId += challengeTabImageIds[(window->frameNo / 4) % std::size(challengeTabImageIds)];
                else
                    imageId += challengeTabImageIds[0];

                Widget::drawTab(window, rt, imageId, widx::tab_challenge);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_companies;
                Widget::drawTab(window, rt, imageId, widx::tab_companies);
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
                if (window->currentTab == widx::tab_finances - widx::tab_challenge)
                    imageId += financesTabImageIds[(window->frameNo / 2) % std::size(financesTabImageIds)];
                else
                    imageId += financesTabImageIds[0];

                Widget::drawTab(window, rt, imageId, widx::tab_finances);
            }

            // Scenario details tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_scenario_details;
                Widget::drawTab(window, rt, imageId, widx::tab_scenario);
            }
        }

        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            window.draw(rt);
            drawTabs(&window, rt);
        }

        static void prepareDraw(Window& self);

        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
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

        static Widget widgets[] = {
            commonWidgets(197, StringIds::title_scenario_challenge),
            makeDropdownWidgets({ 10, 52 }, { 346, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeStepperWidgets({ 10, 67 }, { 163, 12 }, WidgetType::textbox, WindowColour::secondary),
            makeDropdownWidgets({ 193, 67 }, { 163, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeWidget({ 10, 83 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::and_be_the_top_company),
            makeWidget({ 10, 98 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::and_be_within_the_top_companies),
            makeWidget({ 10, 113 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::with_a_time_limit),
            makeStepperWidgets({ 256, 112 }, { 100, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::time_limit_years_value),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0043FC91
        static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
        {
            Common::draw(window, rt);

            const int16_t xPos = window.x + 5;
            int16_t yPos = window.y + widgets[widx::check_time_limit].bottom + 10;
            Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::challenge_label);

            FormatArguments args = {};
            OpenLoco::Scenario::formatChallengeArguments(args);
            yPos += 10;
            Gfx::drawStringLeftWrapped(*rt, xPos, yPos, window.width - 10, Colour::black, StringIds::challenge_value, &args);
        }

        static const string_id objectiveTypeLabelIds[] = {
            StringIds::objective_achieve_a_certain_company_value,
            StringIds::objective_achieve_a_certain_monthly_profit_from_vehicles,
            StringIds::objective_achieve_a_certain_performance_index,
            StringIds::objective_deliver_a_certain_amount_of_cargo,
        };

        static constexpr uint8_t kMaxCargoObjects = static_cast<uint8_t>(ObjectManager::getMaxObjects(ObjectType::cargo));
        static int16_t cargoByDropdownIndex[kMaxCargoObjects] = { -1 };

        // 0x0043FD51
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            switch (widgetIndex)
            {
                case widx::objective_type_btn:
                    Scenario::getObjective().type = static_cast<Scenario::ObjectiveType>(itemIndex);
                    self.invalidate();
                    break;

                case widx::objective_cargo_btn:
                {
                    Scenario::getObjective().deliveredCargoType = cargoByDropdownIndex[itemIndex];
                    self.invalidate();
                }
            }
        }

        // 0x0043FD14
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::objective_type_btn:
                {
                    Widget& target = self.widgets[widx::objective_type];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(objectiveTypeLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(objectiveTypeLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, objectiveTypeLabelIds[i]);

                    Dropdown::setItemSelected(enumValue(Scenario::getObjective().type));
                    break;
                }

                case widx::objective_value_down:
                {
                    switch (Scenario::getObjective().type)
                    {
                        case Scenario::ObjectiveType::companyValue:
                            Scenario::getObjective().companyValue = std::max<uint32_t>(Scenario::getObjective().companyValue - 100000, Scenario::kMinObjectiveCompanyValue);
                            break;

                        case Scenario::ObjectiveType::vehicleProfit:
                            Scenario::getObjective().monthlyVehicleProfit = std::max<uint32_t>(Scenario::getObjective().monthlyVehicleProfit - 1000, Scenario::kMinObjectiveMonthlyProfitFromVehicles);
                            break;

                        case Scenario::ObjectiveType::performanceIndex:
                            Scenario::getObjective().performanceIndex = std::max<uint8_t>(Scenario::getObjective().performanceIndex - 5, Scenario::kMinObjectivePerformanceIndex);
                            break;

                        case Scenario::ObjectiveType::cargoDelivery:
                        {
                            uint16_t stepSize{};
                            uint16_t clickRepeatTicks = Input::getClickRepeatTicks();
                            if (clickRepeatTicks < 100)
                                stepSize = 100;
                            else if (clickRepeatTicks >= 100)
                                stepSize = 1000;
                            else if (clickRepeatTicks >= 200)
                                stepSize = 10000;

                            // Round off cargo to the nearest multiple of the step size.
                            uint16_t cargoFactor = (Scenario::getObjective().deliveredCargoAmount - stepSize) / stepSize;
                            uint32_t newDeliveredCargoAmount = cargoFactor * stepSize;

                            Scenario::getObjective().deliveredCargoAmount = std::max<uint32_t>(newDeliveredCargoAmount, Scenario::kMinObjectiveDeliveredCargo);
                            break;
                        }
                    }

                    self.invalidate();
                    break;
                }

                case widx::objective_value_up:
                {
                    switch (Scenario::getObjective().type)
                    {
                        case Scenario::ObjectiveType::companyValue:
                            Scenario::getObjective().companyValue = std::min<uint32_t>(Scenario::getObjective().companyValue + 100000, Scenario::kMaxObjectiveCompanyValue);
                            break;

                        case Scenario::ObjectiveType::vehicleProfit:
                            Scenario::getObjective().monthlyVehicleProfit = std::min<uint32_t>(Scenario::getObjective().monthlyVehicleProfit + 1000, Scenario::kMaxObjectiveMonthlyProfitFromVehicles);
                            break;

                        case Scenario::ObjectiveType::performanceIndex:
                            Scenario::getObjective().performanceIndex = std::min<uint8_t>(Scenario::getObjective().performanceIndex + 5, Scenario::kMaxObjectivePerformanceIndex);
                            break;

                        case Scenario::ObjectiveType::cargoDelivery:
                        {
                            uint16_t stepSize{};
                            uint16_t clickRepeatTicks = Input::getClickRepeatTicks();
                            if (clickRepeatTicks < 100)
                                stepSize = 100;
                            else if (clickRepeatTicks >= 100)
                                stepSize = 1000;
                            else if (clickRepeatTicks >= 200)
                                stepSize = 10000;

                            // Round off cargo to the nearest multiple of the step size.
                            uint16_t cargoFactor = (Scenario::getObjective().deliveredCargoAmount + stepSize) / stepSize;
                            uint32_t newDeliveredCargoAmount = cargoFactor * stepSize;

                            Scenario::getObjective().deliveredCargoAmount = std::max<uint32_t>(newDeliveredCargoAmount, Scenario::kMinObjectiveDeliveredCargo);
                            break;
                        }
                    }

                    self.invalidate();
                    break;
                }

                case widx::objective_cargo_btn:
                {
                    uint16_t numCargoObjects = 0;
                    for (uint16_t cargoIdx = 0; cargoIdx < kMaxCargoObjects; cargoIdx++)
                    {
                        auto cargoObject = ObjectManager::get<CargoObject>(cargoIdx);
                        if (cargoObject != nullptr)
                            numCargoObjects++;
                    }

                    Widget& target = self.widgets[widx::objective_cargo];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), numCargoObjects, 0x80);

                    uint16_t dropdownIndex = 0;
                    for (uint16_t cargoIdx = 0; cargoIdx < kMaxCargoObjects; cargoIdx++)
                    {
                        auto cargoObject = ObjectManager::get<CargoObject>(cargoIdx);
                        if (cargoObject == nullptr)
                            continue;

                        Dropdown::add(dropdownIndex, StringIds::dropdown_stringid, cargoObject->name);
                        cargoByDropdownIndex[dropdownIndex] = cargoIdx;

                        if (cargoIdx == Scenario::getObjective().deliveredCargoType)
                            Dropdown::setItemSelected(dropdownIndex);

                        dropdownIndex++;
                    }
                    break;
                }

                case widx::time_limit_value_down:
                {
                    Scenario::getObjective().timeLimitYears = std::max<uint8_t>(Scenario::getObjective().timeLimitYears - 1, Scenario::kMinObjectiveYearLimit);
                    self.invalidate();
                    break;
                }

                case widx::time_limit_value_up:
                {
                    Scenario::getObjective().timeLimitYears = std::min<uint8_t>(Scenario::getObjective().timeLimitYears + 1, Scenario::kMaxObjectiveYearLimit);
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x0043FCED
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case check_be_top_company:
                    Scenario::getObjective().flags ^= Scenario::ObjectiveFlags::beTopCompany;
                    self.invalidate();
                    break;

                case check_be_within_top_three_companies:
                    Scenario::getObjective().flags ^= Scenario::ObjectiveFlags::beWithinTopThreeCompanies;
                    self.invalidate();
                    break;

                case check_time_limit:
                    Scenario::getObjective().flags ^= Scenario::ObjectiveFlags::withinTimeLimit;
                    self.invalidate();
                    break;
            }
        }

        // 0x0043FB0C
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            widgets[widx::objective_type].text = objectiveTypeLabelIds[enumValue(Scenario::getObjective().type)];
            widgets[widx::objective_cargo].type = WidgetType::none;
            widgets[widx::objective_cargo_btn].type = WidgetType::none;
            widgets[widx::time_limit_value].type = WidgetType::none;
            widgets[widx::time_limit_value_down].type = WidgetType::none;
            widgets[widx::time_limit_value_up].type = WidgetType::none;

            auto args = FormatArguments();

            switch (Scenario::getObjective().type)
            {
                case Scenario::ObjectiveType::companyValue:
                    args.push<int32_t>(Scenario::getObjective().companyValue);
                    widgets[widx::objective_value].text = StringIds::challenge_monetary_value;
                    break;

                case Scenario::ObjectiveType::vehicleProfit:
                    args.push<int32_t>(Scenario::getObjective().monthlyVehicleProfit);
                    widgets[widx::objective_value].text = StringIds::challenge_monetary_value;
                    break;

                case Scenario::ObjectiveType::performanceIndex:
                    args.push<int16_t>(Scenario::getObjective().performanceIndex * 10);
                    args.skip(2);
                    widgets[widx::objective_value].text = StringIds::challenge_performance_index;
                    break;

                case Scenario::ObjectiveType::cargoDelivery:
                    args.push<int32_t>(Scenario::getObjective().deliveredCargoAmount);
                    widgets[widx::objective_value].text = StringIds::challenge_delivered_cargo;

                    auto cargo = ObjectManager::get<CargoObject>(Scenario::getObjective().deliveredCargoType);
                    widgets[widx::objective_cargo].text = cargo->name;
                    widgets[widx::objective_cargo].type = WidgetType::combobox;
                    widgets[widx::objective_cargo_btn].type = WidgetType::button;
                    break;
            }

            self.activatedWidgets &= ~((1 << widx::check_be_top_company) | (1 << widx::check_be_within_top_three_companies) | (1 << widx::check_time_limit));

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::beTopCompany)
                != 0)
                self.activatedWidgets |= 1 << widx::check_be_top_company;

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::beWithinTopThreeCompanies) != 0)
                self.activatedWidgets |= 1 << widx::check_be_within_top_three_companies;

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::withinTimeLimit) != 0)
            {
                self.activatedWidgets |= 1 << widx::check_time_limit;
                widgets[widx::time_limit_value].type = WidgetType::textbox;
                widgets[widx::time_limit_value_down].type = WidgetType::button;
                widgets[widx::time_limit_value_up].type = WidgetType::button;
                args.skip(2);
                args.push<uint16_t>(Scenario::getObjective().timeLimitYears);
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onDropdown = onDropdown;
            events.onMouseDown = onMouseDown;
            events.onMouseUp = onMouseUp;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
        }
    }

    // 0x0043EE58
    Window* open()
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
            window = WindowManager::createWindowCentred(WindowType::scenarioOptions, kOtherWindowSize, 0, &Challenge::events);
            window->widgets = Challenge::widgets;
            window->enabledWidgets = Challenge::enabledWidgets;
            window->number = 0;
            window->currentTab = 0;
            window->frameNo = 0;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            if (skin != nullptr)
            {
                window->setColour(WindowColour::primary, skin->colour_0B);
                window->setColour(WindowColour::secondary, skin->colour_0E);
            }
            // 0x0043EEFF end

            window->width = kOtherWindowSize.width;
            window->height = kOtherWindowSize.height;
        }

        // TODO(avgeffen): only needs to be called once.
        Common::initEvents();

        window->currentTab = 0;
        window->invalidate();

        window->widgets = Challenge::widgets;
        window->enabledWidgets = Challenge::enabledWidgets;
        window->holdableWidgets = Challenge::holdableWidgets;
        window->eventHandlers = &Challenge::events;
        window->activatedWidgets = 0;

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

        static Widget widgets[] = {
            commonWidgets(327, StringIds::title_company_options),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::max_competing_companies_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::delay_before_competing_companies_start_months),
            makeDropdownWidgets({ 246, 102 }, { 110, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeDropdownWidgets({ 246, 117 }, { 110, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeDropdownWidgets({ 246, 132 }, { 110, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeWidget({ 15, 160 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_trains),
            makeWidget({ 15, 172 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_buses),
            makeWidget({ 15, 184 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_trucks),
            makeWidget({ 15, 196 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_trams),
            makeWidget({ 15, 208 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_aircraft),
            makeWidget({ 15, 220 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_ships),
            makeWidget({ 15, 247 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_trains),
            makeWidget({ 15, 259 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_buses),
            makeWidget({ 15, 271 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_trucks),
            makeWidget({ 15, 283 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_trams),
            makeWidget({ 15, 295 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_aircraft),
            makeWidget({ 15, 307 }, { 341, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::forbid_ships),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::max_competing_companies_down) | (1 << widx::max_competing_companies_up) | (1 << widx::delay_before_competing_companies_start_down) | (1 << widx::delay_before_competing_companies_start_up) | (1 << widx::preferred_intelligence) | (1 << widx::preferred_intelligence_btn) | (1 << widx::preferred_aggressiveness) | (1 << widx::preferred_aggressiveness_btn) | (1 << widx::preferred_competitiveness) | (1 << widx::preferred_competitiveness_btn) | (1 << widx::competitor_forbid_trains) | (1 << widx::competitor_forbid_buses) | (1 << widx::competitor_forbid_trucks) | (1 << widx::competitor_forbid_trams) | (1 << widx::competitor_forbid_aircraft) | (1 << widx::competitor_forbid_ships) | (1 << widx::player_forbid_trains) | (1 << widx::player_forbid_buses) | (1 << widx::player_forbid_trucks) | (1 << widx::player_forbid_trams) | (1 << widx::player_forbid_aircraft) | (1 << widx::player_forbid_ships);
        const uint64_t holdableWidgets = (1 << widx::max_competing_companies_down) | (1 << widx::max_competing_companies_up) | (1 << widx::delay_before_competing_companies_start_down) | (1 << widx::delay_before_competing_companies_start_up);

        static WindowEventList events;

        // 0x0043F4EB
        static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
        {
            Common::draw(window, rt);

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::max_competing_companies].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::max_competing_companies);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::delay_before_competing_companies_start].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::delay_before_competing_companies_start);
            }

            {
                const int16_t xPos = window.x + 15;
                int16_t yPos = window.y + widgets[widx::preferred_intelligence].top - 14;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::selection_of_competing_companies);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::preferred_intelligence].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::preferred_intelligence);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::preferred_aggressiveness].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::preferred_aggressiveness);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::preferred_competitiveness].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::preferred_competitiveness);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::competitor_forbid_trains].top - 12;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::forbid_competing_companies_from_using);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::player_forbid_trains].top - 12;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::forbid_player_companies_from_using);
            }
        }

        static string_id preferenceLabelIds[] = {
            StringIds::preference_any,
            StringIds::preference_low,
            StringIds::preference_medium,
            StringIds::preference_high,
        };

        // 0x0043F67C
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            auto& state = getGameState();

            switch (widgetIndex)
            {
                case widx::preferred_intelligence_btn:
                    state.preferredAIIntelligence = itemIndex;
                    break;

                case widx::preferred_aggressiveness_btn:
                    state.preferredAIAggressiveness = itemIndex;
                    break;

                case widx::preferred_competitiveness_btn:
                    state.preferredAICompetitiveness = itemIndex;
                    break;
            }

            self.invalidate();
        }

        // 0x0043F639
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            auto& state = getGameState();

            switch (widgetIndex)
            {
                case widx::max_competing_companies_down:
                    CompanyManager::setMaxCompetingCompanies(std::max<int8_t>(CompanyManager::getMaxCompetingCompanies() - 1, Scenario::kMinCompetingCompanies));
                    self.invalidate();
                    break;

                case widx::max_competing_companies_up:
                    CompanyManager::setMaxCompetingCompanies(std::min<uint8_t>(CompanyManager::getMaxCompetingCompanies() + 1, Scenario::kMaxCompetingCompanies));
                    self.invalidate();
                    break;

                case widx::delay_before_competing_companies_start_down:
                    CompanyManager::setCompetitorStartDelay(std::max<int8_t>(CompanyManager::getCompetitorStartDelay() - 1, Scenario::kMinCompetitorStartDelay));
                    self.invalidate();
                    break;

                case widx::delay_before_competing_companies_start_up:
                    CompanyManager::setCompetitorStartDelay(std::min<uint8_t>(CompanyManager::getCompetitorStartDelay() + 1, Scenario::kMaxCompetitorStartDelay));
                    self.invalidate();
                    break;

                case widx::preferred_intelligence_btn:
                {
                    Widget& target = self.widgets[widx::preferred_intelligence];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);

                    Dropdown::setItemSelected(state.preferredAIIntelligence);
                    break;
                }

                case widx::preferred_aggressiveness_btn:
                {
                    Widget& target = self.widgets[widx::preferred_aggressiveness];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);

                    Dropdown::setItemSelected(state.preferredAIAggressiveness);
                    break;
                }

                case widx::preferred_competitiveness_btn:
                {
                    Widget& target = self.widgets[widx::preferred_competitiveness];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);

                    Dropdown::setItemSelected(state.preferredAICompetitiveness);
                    break;
                }
            }
        }

        // 0x0043F60C
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            auto& state = getGameState();

            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::competitor_forbid_trains:
                case widx::competitor_forbid_buses:
                case widx::competitor_forbid_trucks:
                case widx::competitor_forbid_trams:
                case widx::competitor_forbid_aircraft:
                case widx::competitor_forbid_ships:
                {
                    uint16_t targetVehicle = static_cast<uint16_t>(widgetIndex - widx::competitor_forbid_trains);
                    uint16_t newForbiddenVehicles = state.forbiddenVehiclesCompetitors ^ (1 << targetVehicle);
                    // TODO(avgeffen): Add a constant for this mask.
                    if (newForbiddenVehicles != 0b111111)
                    {
                        state.forbiddenVehiclesCompetitors = newForbiddenVehicles;
                        self.invalidate();
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
                    uint16_t newForbiddenVehicles = state.forbiddenVehiclesPlayers ^ (1 << targetVehicle);
                    // TODO(avgeffen): Add a constant for this mask.
                    if (newForbiddenVehicles != 0b111111)
                    {
                        state.forbiddenVehiclesPlayers = newForbiddenVehicles;
                        self.invalidate();
                    }
                    break;
                }
            }
        }

        // 0x0043F40C
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            auto args = FormatArguments();
            args.push<uint16_t>(CompanyManager::getMaxCompetingCompanies());
            args.push<uint16_t>(CompanyManager::getCompetitorStartDelay());

            auto& state = getGameState();

            self.widgets[widx::preferred_intelligence].text = preferenceLabelIds[state.preferredAIIntelligence];
            self.widgets[widx::preferred_aggressiveness].text = preferenceLabelIds[state.preferredAIAggressiveness];
            self.widgets[widx::preferred_competitiveness].text = preferenceLabelIds[state.preferredAICompetitiveness];

            self.activatedWidgets &= ~((1 << widx::competitor_forbid_trains) | (1 << widx::competitor_forbid_buses) | (1 << widx::competitor_forbid_trucks) | (1 << widx::competitor_forbid_trams) | (1 << widx::competitor_forbid_aircraft) | (1 << widx::competitor_forbid_ships) | (1 << widx::player_forbid_trains) | (1 << widx::player_forbid_buses) | (1 << widx::player_forbid_trucks) | (1 << widx::player_forbid_trams) | (1 << widx::player_forbid_aircraft) | (1 << widx::player_forbid_ships));

            // TODO(avgeffen): replace with wicked smart widget-id kerfuffle, someday.
            self.activatedWidgets |= static_cast<uint64_t>(state.forbiddenVehiclesCompetitors) << widx::competitor_forbid_trains;
            self.activatedWidgets |= static_cast<uint64_t>(state.forbiddenVehiclesPlayers) << widx::player_forbid_trains;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onDropdown = onDropdown;
            events.onMouseDown = onMouseDown;
            events.onMouseUp = onMouseUp;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
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

        static Widget widgets[] = {
            commonWidgets(217, StringIds::title_financial_options),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::starting_loan_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::max_loan_size_value),
            makeStepperWidgets({ 256, 82 }, { 100, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::loan_interest_rate_value),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);
        const uint64_t holdableWidgets = (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);

        static WindowEventList events;

        // 0x0043F97D
        static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
        {
            Common::draw(window, rt);

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::starting_loan].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::starting_loan);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::max_loan_size].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::max_loan_size);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::loan_interest_rate].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::loan_interest_rate);
            }
        }

        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            auto& state = getGameState();

            switch (widgetIndex)
            {
                case widx::starting_loan_down:
                    CompanyManager::setStartingLoanSize(std::max<int16_t>(CompanyManager::getStartingLoanSize() - 50, Scenario::kMinStartLoanUnits));
                    break;

                case widx::starting_loan_up:
                    CompanyManager::setStartingLoanSize(std::min<uint16_t>(CompanyManager::getStartingLoanSize() + 50, Scenario::kMaxStartLoanUnits));
                    if (CompanyManager::getStartingLoanSize() > CompanyManager::getMaxLoanSize())
                        CompanyManager::setMaxLoanSize(CompanyManager::getStartingLoanSize());
                    break;

                case widx::max_loan_size_down:
                    CompanyManager::setMaxLoanSize(std::max<int16_t>(CompanyManager::getMaxLoanSize() - 50, Scenario::kMinLoanSizeUnits));
                    if (CompanyManager::getStartingLoanSize() > CompanyManager::getMaxLoanSize())
                        CompanyManager::setStartingLoanSize((CompanyManager::getMaxLoanSize()));
                    break;

                case widx::max_loan_size_up:
                    CompanyManager::setMaxLoanSize(std::min<uint16_t>(CompanyManager::getMaxLoanSize() + 50, Scenario::kMaxLoanSizeUnits));
                    break;

                case widx::loan_interest_rate_down:
                    state.loanInterestRate = std::max<int16_t>(state.loanInterestRate - 1, Scenario::kMinLoanInterestUnits);
                    break;

                case widx::loan_interest_rate_up:
                    state.loanInterestRate = std::min<uint16_t>(state.loanInterestRate + 1, Scenario::kMaxLoanInterestUnits);
                    break;
            }

            self.invalidate();
        }

        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x0046E306
        static uint32_t getLoanSizeInCurrency()
        {
            uint64_t loanSizeInCurrency = Economy::getInflationAdjustedCost(CompanyManager::getMaxLoanSize(), 0, 8) / 100 * 100;
            return static_cast<uint32_t>(loanSizeInCurrency);
        }

        // 0x0043F8CF
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            auto args = FormatArguments();

            uint32_t loanSizeInCurrency = getLoanSizeInCurrency();
            args.push<uint32_t>(loanSizeInCurrency);

            uint64_t maxLoanSizeInCurrency = Economy::getInflationAdjustedCost(CompanyManager::getMaxLoanSize(), 0, 8) / 100 * 100;
            args.push(static_cast<uint32_t>(maxLoanSizeInCurrency));

            auto& state = getGameState();
            args.push<uint32_t>(state.loanInterestRate);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onMouseDown = onMouseDown;
            events.onMouseUp = onMouseUp;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
        }
    }

    namespace ScenarioTab
    {
        enum widx
        {
            change_name_btn = 7,
            scenario_group,
            scenario_group_btn,
            change_details_btn,
        };

        static Widget widgets[] = {
            commonWidgets(217, StringIds::title_scenario_options),
            makeWidget({ 281, 52 }, { 75, 12 }, WidgetType::button, WindowColour::secondary, StringIds::change),
            makeDropdownWidgets({ 196, 67 }, { 160, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty),
            makeWidget({ 281, 82 }, { 75, 12 }, WidgetType::button, WindowColour::secondary, StringIds::change),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::change_name_btn) | (1 << widx::scenario_group) | (1 << widx::scenario_group_btn) | (1 << widx::change_details_btn);
        const uint64_t holdableWidgets = 0;

        static WindowEventList events;

        // 0x0043F004
        static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
        {
            Common::draw(window, rt);

            {
                auto args = FormatArguments();

                // Prepare scenario name text.
                char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                strncpy(buffer, S5::getOptions().scenarioName, 512);
                args.push(StringIds::buffer_2039);

                auto* stex = ObjectManager::get<ScenarioTextObject>();
                if (stex != nullptr)
                {
                    args.rewind();
                    args.push(stex->name);
                }

                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::change_name_btn].top + 1;
                int16_t width = widgets[widx::change_name_btn].left - 20;
                Gfx::drawStringLeftClipped(*rt, xPos, yPos, width, Colour::black, StringIds::scenario_name_stringid, &args);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::scenario_group].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::scenario_group);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::change_details_btn].top + 1;
                Gfx::drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::scenario_details);
            }

            {
                auto args = FormatArguments();

                // Prepare scenario details text.
                char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                strncpy(buffer, S5::getOptions().scenarioDetails, 512);
                args.push(StringIds::buffer_2039);

                auto* stex = ObjectManager::get<ScenarioTextObject>();
                if (stex != nullptr)
                {
                    args.rewind();
                    args.push(stex->details);
                }

                auto& target = window.widgets[widx::change_details_btn];
                Gfx::drawStringLeftWrapped(*rt, window.x + 16, window.y + 12 + target.top, target.left - 26, Colour::black, StringIds::black_stringid, &args);
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
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == widx::scenario_group_btn && itemIndex != -1)
            {
                S5::getOptions().difficulty = itemIndex;
                self.invalidate();
            }
        }

        // 0x0043F140
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::scenario_group_btn)
            {
                Widget& target = self.widgets[widx::scenario_group];
                Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(scenarioGroupLabelIds), 0x80);

                for (size_t i = 0; i < std::size(scenarioGroupLabelIds); i++)
                    Dropdown::add(i, StringIds::dropdown_stringid, scenarioGroupLabelIds[i]);

                Dropdown::setItemSelected(S5::getOptions().difficulty);
            }
        }

        // 0x0043F11F
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::tab_challenge:
                case Common::widx::tab_companies:
                case Common::widx::tab_finances:
                case Common::widx::tab_scenario:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::change_name_btn:
                {
                    char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                    strncpy(buffer, S5::getOptions().scenarioName, 512);
                    auto inputSize = std::size(S5::getOptions().scenarioName) - 1;

                    TextInput::openTextInput(&self, StringIds::scenario_name_title, StringIds::enter_name_for_scenario, StringIds::buffer_2039, widgetIndex, nullptr, inputSize);
                    break;
                }

                case widx::change_details_btn:
                {
                    char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                    strncpy(buffer, S5::getOptions().scenarioDetails, 512);
                    auto inputSize = std::size(S5::getOptions().scenarioDetails) - 1;

                    TextInput::openTextInput(&self, StringIds::scenario_details_title, StringIds::enter_description_of_this_scenario, StringIds::buffer_2039, widgetIndex, nullptr, inputSize);
                    break;
                }
            }
        }

        // 0x0043EF8B
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            widgets[widx::scenario_group].text = scenarioGroupLabelIds[S5::getOptions().difficulty];
        }

        // 0x0043F156
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            switch (callingWidget)
            {
                case widx::change_name_btn:
                {
                    strncpy(S5::getOptions().scenarioName, input, sizeof(S5::Options::scenarioName) - 1);
                    S5::getOptions().scenarioName[sizeof(S5::Options::scenarioName) - 1] = '\0';
                    self.invalidate();
                    break;
                }

                case widx::change_details_btn:
                {
                    strncpy(S5::getOptions().scenarioDetails, input, sizeof(S5::Options::scenarioDetails) - 1);
                    S5::getOptions().scenarioDetails[sizeof(S5::Options::scenarioDetails) - 1] = '\0';
                    self.invalidate();
                    break;
                }
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onDropdown = onDropdown;
            events.onMouseDown = onMouseDown;
            events.onMouseUp = onMouseUp;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = textInput;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t* enabledWidgets;
            const uint64_t* holdableWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { Challenge::widgets, widx::tab_challenge, &Challenge::events, &Challenge::enabledWidgets, &Challenge::holdableWidgets },
            { Companies::widgets, widx::tab_companies, &Companies::events, &Companies::enabledWidgets, &Companies::holdableWidgets },
            { Finances::widgets, widx::tab_finances, &Finances::events, &Finances::enabledWidgets, &Finances::holdableWidgets },
            { ScenarioTab::widgets, widx::tab_scenario, &ScenarioTab::events, &ScenarioTab::enabledWidgets, &ScenarioTab::holdableWidgets }
        };

        static void prepareDraw(Window& self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self.currentTab].widgets;
            if (self.widgets != tabWidgets)
            {
                self.widgets = tabWidgets;
                self.initScrollWidgets();
            }

            // Activate the current tab.
            self.activatedWidgets &= ~((1 << widx::tab_challenge) | (1 << widx::tab_companies) | (1 << widx::tab_finances) | (1 << widx::tab_scenario));
            widx widgetIndex = tabInformationByTabOffset[self.currentTab].widgetIndex;
            self.activatedWidgets |= (1ULL << widgetIndex);

            // Resize common widgets.
            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
        }

        // 0x0043F16B
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_challenge;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->disabledWidgets = 0;

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_challenge];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = *tabInfo.holdableWidgets;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;

            self->invalidate();

            const Ui::Size* newSize;
            if (widgetIndex == widx::tab_challenge)
                newSize = &kChallengeWindowSize;
            else if (widgetIndex == widx::tab_companies)
                newSize = &kCompaniesWindowSize;
            else
                newSize = &kOtherWindowSize;

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
            ScenarioTab::initEvents();
        }
    }
}
