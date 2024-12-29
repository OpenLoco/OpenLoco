#include "Economy/Economy.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/CargoObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "ScenarioObjective.h"
#include "SceneManager.h"
#include "Ui/Dropdown.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CheckboxWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/GroupBoxWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Ui::Windows::ScenarioOptions
{
    static constexpr Ui::Size32 kChallengeWindowSize = { 366, 197 };
    static constexpr Ui::Size32 kCompaniesWindowSize = { 366, 260 };
    static constexpr Ui::Size32 kOtherWindowSize = { 366, 217 };

    namespace Common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_challenge,
            tab_companies,
            tab_finances,
            tab_scenario,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_challenge) | (1 << widx::tab_companies) | (1 << widx::tab_finances) | (1 << widx::tab_scenario);

        static constexpr auto makeCommonWidgets(int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { 366, frameHeight }, WindowColour::primary),
                makeWidget({ 1, 1 }, { 364, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ 351, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { 366, 175 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_scenario_challenge),
                Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_options),
                Widgets::Tab({ 65, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_financial_options),
                Widgets::Tab({ 96, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_scenario_options));
        }

        // 0x00440082
        static void update(Window& window)
        {
            window.frameNo++;
            window.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::scenarioOptions, window.number, window.currentTab + widx::tab_challenge);
        }

        // 0x004400A4
        static void drawTabs(Window* window, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Challenge tab
            {
                static constexpr uint32_t challengeTabImageIds[] = {
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
                {
                    imageId += challengeTabImageIds[(window->frameNo / 4) % std::size(challengeTabImageIds)];
                }
                else
                {
                    imageId += challengeTabImageIds[0];
                }

                Widget::drawTab(window, drawingCtx, imageId, widx::tab_challenge);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_companies;
                Widget::drawTab(window, drawingCtx, imageId, widx::tab_companies);
            }

            // Finances tab
            {
                static constexpr uint32_t financesTabImageIds[] = {
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
                {
                    imageId += financesTabImageIds[(window->frameNo / 2) % std::size(financesTabImageIds)];
                }
                else
                {
                    imageId += financesTabImageIds[0];
                }

                Widget::drawTab(window, drawingCtx, imageId, widx::tab_finances);
            }

            // Scenario details tab
            if (window->widgets[widx::tab_scenario].type != WidgetType::none)
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_scenario_details;
                Widget::drawTab(window, drawingCtx, imageId, widx::tab_scenario);
            }
        }

        static void draw(Window& window, Gfx::DrawingContext& drawingCtx)
        {
            window.draw(drawingCtx);
            drawTabs(&window, drawingCtx);
        }

        static void prepareDraw(Window& self);

        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
    }

    namespace Challenge
    {
        enum widx
        {
            objective_type = 8,
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

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(197, StringIds::title_scenario_challenge),
            makeDropdownWidgets({ 10, 52 }, { 346, 12 }, WindowColour::secondary),
            makeStepperWidgets({ 10, 67 }, { 163, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 193, 67 }, { 163, 12 }, WindowColour::secondary),
            Widgets::Checkbox({ 10, 83 }, { 346, 12 }, WindowColour::secondary, StringIds::and_be_the_top_company),
            Widgets::Checkbox({ 10, 98 }, { 346, 12 }, WindowColour::secondary, StringIds::and_be_within_the_top_companies),
            Widgets::Checkbox({ 10, 113 }, { 346, 12 }, WindowColour::secondary, StringIds::with_a_time_limit),
            makeStepperWidgets({ 256, 112 }, { 100, 12 }, WindowColour::secondary, StringIds::time_limit_years_value)

        );

        // 0x0043FC91
        static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            Common::draw(window, drawingCtx);

            auto point = Point(window.x + 5, window.y + widgets[widx::check_time_limit].bottom + 10);
            tr.drawStringLeft(point, Colour::black, StringIds::challenge_label);

            FormatArguments args{};
            OpenLoco::Scenario::formatChallengeArguments(Scenario::getObjective(), Scenario::getObjectiveProgress(), args);

            point.y += 10;
            tr.drawStringLeftWrapped(point, window.width - 10, Colour::black, StringIds::challenge_value, args);
        }

        static constexpr StringId objectiveTypeLabelIds[] = {
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
            {
                return;
            }

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
                    {
                        Dropdown::add(i, StringIds::dropdown_stringid, objectiveTypeLabelIds[i]);
                    }

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
                            {
                                stepSize = 100;
                            }
                            else if (clickRepeatTicks >= 100)
                            {
                                stepSize = 1000;
                            }
                            else if (clickRepeatTicks >= 200)
                            {
                                stepSize = 10000;
                            }

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
                            {
                                stepSize = 100;
                            }
                            else if (clickRepeatTicks >= 100)
                            {
                                stepSize = 1000;
                            }
                            else if (clickRepeatTicks >= 200)
                            {
                                stepSize = 10000;
                            }

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
                        {
                            numCargoObjects++;
                        }
                    }

                    Widget& target = self.widgets[widx::objective_cargo];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), numCargoObjects, 0x80);

                    uint16_t dropdownIndex = 0;
                    for (uint16_t cargoIdx = 0; cargoIdx < kMaxCargoObjects; cargoIdx++)
                    {
                        auto cargoObject = ObjectManager::get<CargoObject>(cargoIdx);
                        if (cargoObject == nullptr)
                        {
                            continue;
                        }

                        Dropdown::add(dropdownIndex, StringIds::dropdown_stringid, cargoObject->name);
                        cargoByDropdownIndex[dropdownIndex] = cargoIdx;

                        if (cargoIdx == Scenario::getObjective().deliveredCargoType)
                        {
                            Dropdown::setItemSelected(dropdownIndex);
                        }

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
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

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

            self.widgets[widx::objective_type].text = objectiveTypeLabelIds[enumValue(Scenario::getObjective().type)];
            self.widgets[widx::objective_cargo].type = WidgetType::none;
            self.widgets[widx::objective_cargo_btn].type = WidgetType::none;
            self.widgets[widx::time_limit_value].type = WidgetType::none;
            self.widgets[widx::time_limit_value_down].type = WidgetType::none;
            self.widgets[widx::time_limit_value_up].type = WidgetType::none;

            auto args = FormatArguments(self.widgets[widx::objective_value].textArgs);

            switch (Scenario::getObjective().type)
            {
                case Scenario::ObjectiveType::companyValue:
                    args.push<int32_t>(Scenario::getObjective().companyValue);
                    self.widgets[widx::objective_value].text = StringIds::challenge_monetary_value;
                    break;

                case Scenario::ObjectiveType::vehicleProfit:
                    args.push<int32_t>(Scenario::getObjective().monthlyVehicleProfit);
                    self.widgets[widx::objective_value].text = StringIds::challenge_monetary_value;
                    break;

                case Scenario::ObjectiveType::performanceIndex:
                    args.push<int16_t>(Scenario::getObjective().performanceIndex * 10);
                    self.widgets[widx::objective_value].text = StringIds::challenge_performance_index;
                    break;

                case Scenario::ObjectiveType::cargoDelivery:
                    args.push<int32_t>(Scenario::getObjective().deliveredCargoAmount);
                    self.widgets[widx::objective_value].text = StringIds::challenge_delivered_cargo;

                    auto cargo = ObjectManager::get<CargoObject>(Scenario::getObjective().deliveredCargoType);
                    self.widgets[widx::objective_cargo].text = cargo->name;
                    self.widgets[widx::objective_cargo].type = WidgetType::combobox;
                    self.widgets[widx::objective_cargo_btn].type = WidgetType::button;
                    break;
            }

            self.activatedWidgets &= ~((1 << widx::check_be_top_company) | (1 << widx::check_be_within_top_three_companies) | (1 << widx::check_time_limit));

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::beTopCompany) != Scenario::ObjectiveFlags::none)
            {
                self.activatedWidgets |= 1 << widx::check_be_top_company;
            }

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::beWithinTopThreeCompanies) != Scenario::ObjectiveFlags::none)
            {
                self.activatedWidgets |= 1 << widx::check_be_within_top_three_companies;
            }

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::withinTimeLimit) != Scenario::ObjectiveFlags::none)
            {
                self.activatedWidgets |= 1 << widx::check_time_limit;
                self.widgets[widx::time_limit_value].type = WidgetType::textbox;
                self.widgets[widx::time_limit_value_down].type = WidgetType::button;
                self.widgets[widx::time_limit_value_up].type = WidgetType::button;

                auto args2 = FormatArguments(self.widgets[widx::time_limit_value].textArgs);
                args2.push<uint16_t>(Scenario::getObjective().timeLimitYears);
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x0043EE58
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::scenarioOptions, 0);
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
            {
                ToolManager::toolCancel();
            }

            window = WindowManager::bringToFront(WindowType::scenarioOptions, 0);
        }

        if (window == nullptr)
        {
            // 0x0043EEFF start
            window = WindowManager::createWindowCentred(WindowType::scenarioOptions, kOtherWindowSize, WindowFlags::none, Challenge::getEvents());
            window->setWidgets(Challenge::widgets);
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

        window->currentTab = 0;
        window->invalidate();

        window->setWidgets(Challenge::widgets);
        window->enabledWidgets = Challenge::enabledWidgets;
        window->holdableWidgets = Challenge::holdableWidgets;
        window->eventHandlers = &Challenge::getEvents();
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
            max_competing_companies = 8,
            max_competing_companies_down,
            max_competing_companies_up,
            delay_before_competing_companies_start,
            delay_before_competing_companies_start_down,
            delay_before_competing_companies_start_up,
            groupbox_preferred_ai,
            preferred_intelligence,
            preferred_intelligence_btn,
            preferred_aggressiveness,
            preferred_aggressiveness_btn,
            preferred_competitiveness,
            preferred_competitiveness_btn,
            groupbox_forbid_competitor_vehicles,
            competitor_forbid_trains,
            competitor_forbid_buses,
            competitor_forbid_trucks,
            competitor_forbid_trams,
            competitor_forbid_aircraft,
            competitor_forbid_ships,
            groupbox_forbid_player_vehicles,
            player_forbid_trains,
            player_forbid_buses,
            player_forbid_trucks,
            player_forbid_trams,
            player_forbid_aircraft,
            player_forbid_ships,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(327, StringIds::title_company_options),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WindowColour::secondary, StringIds::max_competing_companies_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, WindowColour::secondary, StringIds::delay_before_competing_companies_start_months),
            Widgets::GroupBox({ 5, 102 - 14 - 5 }, { 356, 63 }, WindowColour::secondary, StringIds::selection_of_competing_companies),
            makeDropdownWidgets({ 246, 102 - 4 }, { 110, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 246, 117 - 4 }, { 110, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 246, 132 - 4 }, { 110, 12 }, WindowColour::secondary),
            Widgets::GroupBox({ 5, 150 }, { 356, 50 }, WindowColour::secondary, StringIds::forbid_competing_companies_from_using),
            Widgets::Checkbox({ 15, 166 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_trains),
            Widgets::Checkbox({ 15, 180 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_trams),
            Widgets::Checkbox({ 130, 166 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_buses),
            Widgets::Checkbox({ 130, 180 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_trucks),
            Widgets::Checkbox({ 260, 166 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_aircraft),
            Widgets::Checkbox({ 260, 180 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_ships),
            Widgets::GroupBox({ 5, 202 }, { 356, 50 }, WindowColour::secondary, StringIds::forbid_player_companies_from_using),
            Widgets::Checkbox({ 15, 219 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_trains),
            Widgets::Checkbox({ 15, 233 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_trams),
            Widgets::Checkbox({ 130, 219 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_buses),
            Widgets::Checkbox({ 130, 233 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_trucks),
            Widgets::Checkbox({ 260, 219 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_aircraft),
            Widgets::Checkbox({ 260, 233 }, { 341, 12 }, WindowColour::secondary, StringIds::forbid_ships)

        );

        const uint64_t enabledWidgets = Common::enabledWidgets | (1ULL << widx::max_competing_companies_down) | (1ULL << widx::max_competing_companies_up) | (1ULL << widx::delay_before_competing_companies_start_down) | (1ULL << widx::delay_before_competing_companies_start_up) | (1ULL << widx::preferred_intelligence) | (1ULL << widx::preferred_intelligence_btn) | (1ULL << widx::preferred_aggressiveness) | (1ULL << widx::preferred_aggressiveness_btn) | (1ULL << widx::preferred_competitiveness) | (1ULL << widx::preferred_competitiveness_btn) | (1ULL << widx::competitor_forbid_trains) | (1ULL << widx::competitor_forbid_buses) | (1ULL << widx::competitor_forbid_trucks) | (1ULL << widx::competitor_forbid_trams) | (1ULL << widx::competitor_forbid_aircraft) | (1ULL << widx::competitor_forbid_ships) | (1ULL << widx::player_forbid_trains) | (1ULL << widx::player_forbid_buses) | (1ULL << widx::player_forbid_trucks) | (1ULL << widx::player_forbid_trams) | (1ULL << widx::player_forbid_aircraft) | (1ULL << widx::player_forbid_ships);
        const uint64_t holdableWidgets = (1ULL << widx::max_competing_companies_down) | (1ULL << widx::max_competing_companies_up) | (1ULL << widx::delay_before_competing_companies_start_down) | (1ULL << widx::delay_before_competing_companies_start_up);

        // 0x0043F4EB
        static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            Common::draw(window, drawingCtx);

            auto point = Point(window.x + 10, window.y + widgets[widx::max_competing_companies].top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::max_competing_companies);

            point.y = window.y + widgets[widx::delay_before_competing_companies_start].top + 1;
            tr.drawStringLeft(point, Colour::black, StringIds::delay_before_competing_companies_start);

            point.y = window.y + widgets[widx::preferred_intelligence].top + 1;
            tr.drawStringLeft(point, Colour::black, StringIds::preferred_intelligence);

            point.y = window.y + widgets[widx::preferred_aggressiveness].top + 1;
            tr.drawStringLeft(point, Colour::black, StringIds::preferred_aggressiveness);

            point.y = window.y + widgets[widx::preferred_competitiveness].top + 1;
            tr.drawStringLeft(point, Colour::black, StringIds::preferred_competitiveness);
        }

        static StringId preferenceLabelIds[] = {
            StringIds::preference_any,
            StringIds::preference_low,
            StringIds::preference_medium,
            StringIds::preference_high,
        };

        // 0x0043F67C
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                return;
            }

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
                    {
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);
                    }

                    Dropdown::setItemSelected(state.preferredAIIntelligence);
                    break;
                }

                case widx::preferred_aggressiveness_btn:
                {
                    Widget& target = self.widgets[widx::preferred_aggressiveness];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                    {
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);
                    }

                    Dropdown::setItemSelected(state.preferredAIAggressiveness);
                    break;
                }

                case widx::preferred_competitiveness_btn:
                {
                    Widget& target = self.widgets[widx::preferred_competitiveness];
                    Dropdown::show(self.x + target.left, self.y + target.top, target.width() - 4, target.height(), self.getColour(WindowColour::secondary), std::size(preferenceLabelIds), 0x80);

                    for (size_t i = 0; i < std::size(preferenceLabelIds); i++)
                    {
                        Dropdown::add(i, StringIds::dropdown_stringid, preferenceLabelIds[i]);
                    }

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
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

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
                    if (!isEditorMode())
                    {
                        CompanyManager::determineAvailableVehicles();
                        WindowManager::invalidate(WindowType::vehicleList);
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
                    if (!isEditorMode())
                    {
                        CompanyManager::determineAvailableVehicles();
                        WindowManager::invalidate(WindowType::buildVehicle);
                        WindowManager::invalidate(WindowType::vehicleList);
                    }
                    break;
                }
            }
        }

        // 0x0043F40C
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            {
                auto args = FormatArguments(self.widgets[widx::max_competing_companies].textArgs);
                args.push<uint16_t>(CompanyManager::getMaxCompetingCompanies());
            }

            {
                auto args = FormatArguments(self.widgets[widx::delay_before_competing_companies_start].textArgs);
                args.push<uint16_t>(CompanyManager::getCompetitorStartDelay());
            }

            auto& state = getGameState();

            self.widgets[widx::preferred_intelligence].text = preferenceLabelIds[state.preferredAIIntelligence];
            self.widgets[widx::preferred_aggressiveness].text = preferenceLabelIds[state.preferredAIAggressiveness];
            self.widgets[widx::preferred_competitiveness].text = preferenceLabelIds[state.preferredAICompetitiveness];

            self.activatedWidgets &= ~((1ULL << widx::competitor_forbid_trains) | (1ULL << widx::competitor_forbid_buses) | (1ULL << widx::competitor_forbid_trucks) | (1ULL << widx::competitor_forbid_trams) | (1ULL << widx::competitor_forbid_aircraft) | (1ULL << widx::competitor_forbid_ships) | (1ULL << widx::player_forbid_trains) | (1ULL << widx::player_forbid_buses) | (1ULL << widx::player_forbid_trucks) | (1ULL << widx::player_forbid_trams) | (1ULL << widx::player_forbid_aircraft) | (1ULL << widx::player_forbid_ships));

            // TODO(avgeffen): replace with wicked smart widget-id kerfuffle, someday.
            self.activatedWidgets |= static_cast<uint64_t>(state.forbiddenVehiclesCompetitors) << widx::competitor_forbid_trains;
            self.activatedWidgets |= static_cast<uint64_t>(state.forbiddenVehiclesPlayers) << widx::player_forbid_trains;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Finances
    {
        enum widx
        {
            starting_loan = 8,
            starting_loan_down,
            starting_loan_up,
            max_loan_size,
            max_loan_size_down,
            max_loan_size_up,
            loan_interest_rate,
            loan_interest_rate_down,
            loan_interest_rate_up,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(217, StringIds::title_financial_options),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WindowColour::secondary, StringIds::starting_loan_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, WindowColour::secondary, StringIds::max_loan_size_value),
            makeStepperWidgets({ 256, 82 }, { 100, 12 }, WindowColour::secondary, StringIds::loan_interest_rate_value)

        );

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);
        const uint64_t holdableWidgets = (1 << widx::starting_loan_down) | (1 << widx::starting_loan_up) | (1 << widx::max_loan_size_down) | (1 << widx::max_loan_size_up) | (1 << widx::loan_interest_rate_down) | (1 << widx::loan_interest_rate_up);

        // 0x0043F97D
        static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            Common::draw(window, drawingCtx);

            auto point = Point(window.x + 10, window.y + widgets[widx::starting_loan].top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::starting_loan);

            point.y = window.y + widgets[widx::max_loan_size].top + 1;
            tr.drawStringLeft(point, Colour::black, StringIds::max_loan_size);

            point.y = window.y + widgets[widx::loan_interest_rate].top + 1;
            tr.drawStringLeft(point, Colour::black, StringIds::loan_interest_rate);
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
                    {
                        CompanyManager::setMaxLoanSize(CompanyManager::getStartingLoanSize());
                    }
                    break;

                case widx::max_loan_size_down:
                    CompanyManager::setMaxLoanSize(std::max<int16_t>(CompanyManager::getMaxLoanSize() - 50, Scenario::kMinLoanSizeUnits));
                    if (CompanyManager::getStartingLoanSize() > CompanyManager::getMaxLoanSize())
                    {
                        CompanyManager::setStartingLoanSize((CompanyManager::getMaxLoanSize()));
                    }
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
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

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
            currency32_t loanSizeInCurrency = Economy::getInflationAdjustedCost(CompanyManager::getStartingLoanSize(), 0, 8) / 100 * 100;
            return static_cast<uint32_t>(loanSizeInCurrency);
        }

        // 0x0043F8CF
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            {

                auto args = FormatArguments(self.widgets[widx::starting_loan].textArgs);

                uint32_t loanSizeInCurrency = getLoanSizeInCurrency();
                args.push<uint32_t>(loanSizeInCurrency);
            }

            {
                auto args = FormatArguments(self.widgets[widx::max_loan_size].textArgs);

                uint64_t maxLoanSizeInCurrency = Economy::getInflationAdjustedCost(CompanyManager::getMaxLoanSize(), 0, 8) / 100 * 100;
                args.push(static_cast<uint32_t>(maxLoanSizeInCurrency));
            }

            {
                auto args = FormatArguments(self.widgets[widx::loan_interest_rate].textArgs);

                auto& state = getGameState();
                args.push<uint32_t>(state.loanInterestRate);
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace ScenarioTab
    {
        enum widx
        {
            change_name_btn = 8,
            scenario_group,
            scenario_group_btn,
            change_details_btn,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(217, StringIds::title_scenario_options),
            Widgets::Button({ 281, 52 }, { 75, 12 }, WindowColour::secondary, StringIds::change),
            makeDropdownWidgets({ 196, 67 }, { 160, 12 }, WindowColour::secondary, StringIds::empty),
            Widgets::Button({ 281, 82 }, { 75, 12 }, WindowColour::secondary, StringIds::change)

        );

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::change_name_btn) | (1 << widx::scenario_group) | (1 << widx::scenario_group_btn) | (1 << widx::change_details_btn);
        const uint64_t holdableWidgets = 0;

        // 0x0043F004
        static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            Common::draw(window, drawingCtx);

            {
                // Prepare scenario name text.
                char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                strncpy(buffer, S5::getOptions().scenarioName, 512);

                FormatArguments args{};
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

                auto point = Point(xPos, yPos);
                tr.drawStringLeftClipped(point, width, Colour::black, StringIds::scenario_name_stringid, args);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::scenario_group].top + 1;

                auto point = Point(xPos, yPos);
                tr.drawStringLeft(point, Colour::black, StringIds::scenario_group);
            }

            {
                const int16_t xPos = window.x + 10;
                int16_t yPos = window.y + widgets[widx::change_details_btn].top + 1;

                auto point = Point(xPos, yPos);
                tr.drawStringLeft(point, Colour::black, StringIds::scenario_details);
            }

            {
                // Prepare scenario details text.
                char* buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                strncpy(buffer, S5::getOptions().scenarioDetails, 512);

                FormatArguments args{};
                args.push(StringIds::buffer_2039);

                auto* stex = ObjectManager::get<ScenarioTextObject>();
                if (stex != nullptr)
                {
                    args.rewind();
                    args.push(stex->details);
                }

                auto& target = window.widgets[widx::change_details_btn];
                auto point = Point(window.x + 16, window.y + 12 + target.top);
                tr.drawStringLeftWrapped(point, target.left - 26, Colour::black, StringIds::black_stringid, args);
            }
        }

        static StringId scenarioGroupLabelIds[] = {
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
                {
                    Dropdown::add(i, StringIds::dropdown_stringid, scenarioGroupLabelIds[i]);
                }

                Dropdown::setItemSelected(S5::getOptions().difficulty);
            }
        }

        // 0x0043F11F
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

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

            self.widgets[widx::scenario_group].text = scenarioGroupLabelIds[S5::getOptions().difficulty];
        }

        static void unloadScenarioTextObjects()
        {
            if (ObjectManager::get<ScenarioTextObject>() == nullptr)
            {
                return;
            }

            LoadedObjectHandle handle = { ObjectType::scenarioText, 0 };
            auto header = ObjectManager::getHeader(handle);
            ObjectManager::unload(header);
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
                    unloadScenarioTextObjects();
                    self.invalidate();
                    break;
                }

                case widx::change_details_btn:
                {
                    strncpy(S5::getOptions().scenarioDetails, input, sizeof(S5::Options::scenarioDetails) - 1);
                    S5::getOptions().scenarioDetails[sizeof(S5::Options::scenarioDetails) - 1] = '\0';
                    unloadScenarioTextObjects();
                    self.invalidate();
                    break;
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::update,
            .textInput = textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t* enabledWidgets;
            const uint64_t* holdableWidgets;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { Challenge::widgets,   widx::tab_challenge,  Challenge::getEvents(),   &Challenge::enabledWidgets,   &Challenge::holdableWidgets },
            { Companies::widgets,   widx::tab_companies,  Companies::getEvents(),   &Companies::enabledWidgets,   &Companies::holdableWidgets },
            { Finances::widgets,    widx::tab_finances,   Finances::getEvents(),    &Finances::enabledWidgets,    &Finances::holdableWidgets },
            { ScenarioTab::widgets, widx::tab_scenario,   ScenarioTab::getEvents(), &ScenarioTab::enabledWidgets, &ScenarioTab::holdableWidgets }
        };
        // clang-format on

        static void prepareDraw(Window& self)
        {
            // Activate the current tab.
            self.activatedWidgets &= ~((1 << widx::tab_challenge) | (1 << widx::tab_companies) | (1 << widx::tab_finances) | (1 << widx::tab_scenario));
            widx widgetIndex = tabInformationByTabOffset[self.currentTab].widgetIndex;
            self.activatedWidgets |= (1ULL << widgetIndex);

            if (isEditorMode())
            {
                // Disable close button in the scenario editor.
                self.widgets[Common::widx::close_button].type = WidgetType::none;
                self.widgets[widx::tab_scenario].type = WidgetType::tab;
            }
            else
            {
                // Disable scenario details tab in-game.
                self.widgets[Common::widx::close_button].type = WidgetType::buttonWithImage;
                self.widgets[widx::tab_scenario].type = WidgetType::none;
            }

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
            if (ToolManager::isToolActive(self->type, self->number))
            {
                ToolManager::toolCancel();
            }

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_challenge;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->disabledWidgets = 0;

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_challenge];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = *tabInfo.holdableWidgets;
            self->eventHandlers = &tabInfo.events;
            self->activatedWidgets = 0;
            self->setWidgets(tabInfo.widgets);

            self->invalidate();

            const auto newSize = [widgetIndex]() {
                if (widgetIndex == widx::tab_challenge)
                {
                    return kChallengeWindowSize;
                }
                else if (widgetIndex == widx::tab_companies)
                {
                    return kCompaniesWindowSize;
                }
                else
                {
                    return kOtherWindowSize;
                }
            }();

            self->setSize(newSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }
    }
}
