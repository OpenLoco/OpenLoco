#include "Config.h"
#include "Date.h"
#include "GameCommands/Cheats/Cheat.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Scenario/Scenario.h"
#include "Ui/Dropdown.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/CheckboxWidget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/GroupBoxWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/StepperWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Math/Bound.hpp>

using OpenLoco::GameCommands::CheatCommand;

namespace OpenLoco::Ui::Windows::Cheats
{
    namespace Common
    {
        namespace Widx
        {
            enum
            {
                frame,
                title,
                close_button,
                panel,
                tab_finances,
                tab_companies,
                tab_vehicles,
                tab_towns,
            };
            // this should be 1 more than the number of widgets defined above in commonWidgets
            constexpr uint32_t nextWidx = 8;
        }

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                Widgets::Caption({ 1, 1 }, { frameWidth - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { frameWidth, frameHeight - 41 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab),
                Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab),
                Widgets::Tab({ 65, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab),
                Widgets::Tab({ 96, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab));
        }

        static void drawTabs(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

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
                if (self.currentTab == Widx::tab_finances - Widx::tab_finances)
                {
                    imageId += financesTabImageIds[(self.frameNo / 2) % std::size(financesTabImageIds)];
                }
                else
                {
                    imageId += financesTabImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, Widx::tab_finances);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_company;
                Widget::drawTab(self, drawingCtx, imageId, Widx::tab_companies);
            }

            // Vehicles tab
            {
                static constexpr uint32_t vehiclesTabImageIds[] = {
                    InterfaceSkin::ImageIds::vehicle_train_frame_0,
                    InterfaceSkin::ImageIds::vehicle_train_frame_1,
                    InterfaceSkin::ImageIds::vehicle_train_frame_2,
                    InterfaceSkin::ImageIds::vehicle_train_frame_3,
                    InterfaceSkin::ImageIds::vehicle_train_frame_4,
                    InterfaceSkin::ImageIds::vehicle_train_frame_5,
                    InterfaceSkin::ImageIds::vehicle_train_frame_6,
                    InterfaceSkin::ImageIds::vehicle_train_frame_7,
                };

                uint32_t imageId = skin->img;
                if (self.currentTab == Widx::tab_vehicles - Widx::tab_finances)
                {
                    imageId += vehiclesTabImageIds[(self.frameNo / 2) % std::size(vehiclesTabImageIds)];
                }
                else
                {
                    imageId += vehiclesTabImageIds[0];
                }

                auto companyId = CompanyManager::getControllingId();
                auto companyColour = CompanyManager::getCompanyColour(companyId);

                imageId = Gfx::recolour(imageId, companyColour);

                Widget::drawTab(self, drawingCtx, imageId, Widx::tab_vehicles);
            }

            // Towns tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_towns;
                Widget::drawTab(self, drawingCtx, imageId, Widx::tab_towns);
            }
        }

        static void switchTab(Window& self, WidgetIndex_t widgetIndex);
    }

    namespace Finances
    {
        static constexpr Ui::Size kWindowSize = { 250, 210 };

        namespace Widx
        {
            enum
            {
                cash_step_group = Common::Widx::nextWidx,
                cash_step_label,
                cash_step_value,
                cash_step_decrease,
                cash_step_increase,
                cash_step_apply,
                loan_group,
                loan_label,
                loan_value,
                loan_clear,
                time_group,
                year_label,
                year_step_value,
                year_step_decrease,
                year_step_increase,
                month_label,
                month_step_value,
                month_step_decrease,
                month_step_increase,
                day_label,
                day_step_value,
                day_step_decrease,
                day_step_increase,
                date_change_apply,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize.width, kWindowSize.height, StringIds::financial_cheats),
            // money
            Widgets::GroupBox({ 4, 48 }, { kWindowSize.width - 8, 33 }, WindowColour::secondary, StringIds::cheat_increase_funds),
            Widgets::Label({ 10, 62 }, { 70, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::cheat_amount),
            Widgets::stepperWidgets({ 80, 62 }, { 95, 12 }, WindowColour::secondary, StringIds::cheat_loan_value),
            Widgets::Button({ 180, 62 }, { 60, 12 }, WindowColour::secondary, StringIds::cheat_add),
            // loan
            Widgets::GroupBox({ 4, 86 }, { kWindowSize.width - 8, 33 }, WindowColour::secondary, StringIds::cheat_clear_loan),
            Widgets::Label({ 10, 100 }, { 70, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::company_current_loan),
            Widgets::TextBox({ 80, 100 }, { 95, 12 }, WindowColour::secondary, StringIds::cheat_loan_value),
            Widgets::Button({ 180, 100 }, { 60, 12 }, WindowColour::secondary, StringIds::cheat_clear),
            // date/time
            Widgets::GroupBox({ 4, 124 }, { kWindowSize.width - 8, 80 }, WindowColour::secondary, StringIds::cheat_date_change_apply),
            Widgets::Label({ 10, 138 }, { 70, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::cheat_year),
            Widgets::stepperWidgets({ 80, 138 }, { 95, 12 }, WindowColour::secondary, StringIds::cheat_year_value),
            Widgets::Label({ 10, 154 }, { 70, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::cheat_month),
            Widgets::stepperWidgets({ 80, 154 }, { 95, 12 }, WindowColour::secondary, StringIds::black_stringid),
            Widgets::Label({ 10, 170 }, { 70, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::cheat_day),
            Widgets::stepperWidgets({ 80, 170 }, { 95, 12 }, WindowColour::secondary, StringIds::cheat_day_value),
            Widgets::Button({ 10, 186 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_date_change_apply)

        );

        const uint64_t holdableWidgets
            = (1 << Widx::cash_step_decrease)
            | (1 << Widx::cash_step_increase)
            | (1 << Widx::year_step_decrease)
            | (1 << Widx::year_step_increase)
            | (1 << Widx::month_step_decrease)
            | (1 << Widx::month_step_increase)
            | (1 << Widx::day_step_decrease)
            | (1 << Widx::day_step_increase);

        static currency32_t _cashIncreaseStep = 10'000;
        static Date _date;

        static void prepareDraw(Window& self)
        {
            self.activatedWidgets = (1 << Common::Widx::tab_finances);

            // Add cash step label and value
            {
                auto& widget = self.widgets[Widx::cash_step_value];
                FormatArguments args{ widget.textArgs };
                args.push(_cashIncreaseStep);
            }

            // Loan label and value
            {
                auto& widget = self.widgets[Widx::loan_value];
                FormatArguments args{ widget.textArgs };
                auto company = CompanyManager::getPlayerCompany();
                args.push(company->currentLoan);
            }

            // Add year label and value
            {
                auto& widget = self.widgets[Widx::year_step_value];
                FormatArguments args{ widget.textArgs };
                args.push(_date.year);
            }

            // Add month label and value
            {
                auto& widget = self.widgets[Widx::month_step_value];
                FormatArguments args{ widget.textArgs };
                args.push(StringManager::monthToString(_date.month).second);
            }

            // Add day label and value
            {
                auto& widget = self.widgets[Widx::day_step_value];
                FormatArguments args{ widget.textArgs };
                args.push(_date.day + 1); // +1 since days in game are 0-based, but IRL they are 1-based
            }
        }

        static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets and tabs.
            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);
        }

        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self.type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::cash_step_apply:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::addCash;
                    args.param1 = _cashIncreaseStep;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    break;
                }

                case Widx::loan_clear:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::clearLoan;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidateWidget(self.type, self.number, Widx::loan_value);
                    break;
                }

                case Widx::date_change_apply:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::modifyDate;
                    args.param1 = _date.year;
                    args.param2 = enumValue(_date.month);
                    args.param3 = _date.day + 1; // +1 days again

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::timeToolbar);
                    break;
                }
            }
        }

        static int32_t clampDayToMonth(const Date& date)
        {
            return std::max<int32_t>(0, std::min<int32_t>(getMonthTotalDay(date.year, date.month) - 1, date.day));
        }

        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            currency32_t cashStepSize{};
            int32_t timeStepSize{};
            uint16_t clickRepeatTicks = Input::getClickRepeatTicks();

            if (clickRepeatTicks < 100)
            {
                cashStepSize = 1'000;
                timeStepSize = 1;
            }
            else if (clickRepeatTicks < 200)
            {
                cashStepSize = 10'000;
                timeStepSize = 10;
            }
            else if (clickRepeatTicks < 300)
            {
                cashStepSize = 100'000;
                timeStepSize = 100;
            }
            else
            {
                cashStepSize = 1'000'000;
                timeStepSize = 1'000;
            }

            switch (widgetIndex)
            {
                case Widx::cash_step_decrease:
                    _cashIncreaseStep = std::max<currency32_t>(_cashIncreaseStep - cashStepSize, 0);
                    WindowManager::invalidateWidget(self.type, self.number, Widx::cash_step_value);
                    break;

                case Widx::cash_step_increase:
                    _cashIncreaseStep = std::max<currency32_t>(_cashIncreaseStep + cashStepSize, 0);
                    WindowManager::invalidateWidget(self.type, self.number, Widx::cash_step_value);
                    break;

                case Widx::year_step_decrease:
                    _date.year = std::max<int32_t>(OpenLoco::Scenario::kMinYear, _date.year - timeStepSize);
                    break;

                case Widx::year_step_increase:
                    _date.year = Math::Bound::add(_date.year, timeStepSize);
                    break;

                case Widx::month_step_decrease:
                    _date.month = static_cast<MonthId>(std::max<int8_t>(0, (static_cast<int8_t>(_date.month) - timeStepSize)));
                    break;

                case Widx::month_step_increase:
                    _date.month = static_cast<MonthId>(std::min<int8_t>(11, (static_cast<int8_t>(_date.month) + timeStepSize)));
                    break;

                case Widx::day_step_decrease:
                    _date.day = std::max<int32_t>(0, _date.day - timeStepSize);
                    break;

                case Widx::day_step_increase:
                    _date.day = std::min<int32_t>(getMonthTotalDay(_date.year, _date.month) - 1, _date.day + timeStepSize);
                    break;
            }

            _date.day = clampDayToMonth(_date);
            WindowManager::invalidate(WindowType::cheats);
        }

        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, Common::Widx::tab_finances);
        }

        static void initDate()
        {
            _date = getCurrentDate();
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Companies
    {
        static constexpr Ui::Size kWindowSize = { 250, 188 };

        namespace Widx
        {
            enum
            {
                target_company_group = Common::Widx::nextWidx,
                target_company_dropdown,
                target_company_dropdown_btn,
                select_cheat_group,
                switch_company_button,
                acquire_company_assets_button,
                toggle_bankruptcy_button,
                toggle_jail_status_button,
                complete_challenge_button,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize.width, kWindowSize.height, StringIds::company_cheats),
            Widgets::GroupBox({ 4, 48 }, { kWindowSize.width - 8, 33 }, WindowColour::secondary, StringIds::cheat_select_target_company),
            Widgets::dropdownWidgets({ 10, 62 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::black_stringid),
            Widgets::GroupBox({ 4, 86 }, { kWindowSize.width - 8, 96 }, WindowColour::secondary, StringIds::cheat_select_cheat_to_apply),
            Widgets::Button({ 10, 100 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_switch_to_company),
            Widgets::Button({ 10, 116 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_acquire_company_assets),
            Widgets::Button({ 10, 132 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_toggle_bankruptcy),
            Widgets::Button({ 10, 148 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_toggle_jail_status),
            Widgets::Button({ 10, 164 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::completeChallenge)

        );

        static CompanyId _targetCompanyId{};

        static void prepareDraw(Window& self)
        {
            self.activatedWidgets = (1 << Common::Widx::tab_companies);

            if (_targetCompanyId == CompanyManager::getControllingId())
            {
                self.disabledWidgets |= (1 << Widx::switch_company_button) | (1 << Widx::acquire_company_assets_button);
            }
            else
            {
                self.disabledWidgets &= ~((1 << Widx::switch_company_button) | (1 << Widx::acquire_company_assets_button));
            }

            if (!CompanyManager::isPlayerCompany(_targetCompanyId))
            {
                self.disabledWidgets |= (1 << Widx::complete_challenge_button);
            }
            else
            {
                self.disabledWidgets &= ~(1 << Widx::complete_challenge_button);
            }

            // Current company name
            auto& widget = self.widgets[Widx::target_company_dropdown];
            auto args = FormatArguments(widget.textArgs);

            auto company = CompanyManager::get(_targetCompanyId);
            args.push(company->name);
        }

        static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets and tabs.
            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);
        }

        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self.type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::acquire_company_assets_button:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::acquireAssets;
                    args.param1 = enumValue(_targetCompanyId);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    Gfx::invalidateScreen();
                    return;
                }

                case Widx::switch_company_button:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::switchCompany;
                    args.param1 = enumValue(_targetCompanyId);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }

                case Widx::toggle_bankruptcy_button:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::toggleBankruptcy;
                    args.param1 = enumValue(_targetCompanyId);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }

                case Widx::toggle_jail_status_button:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::toggleJail;
                    args.param1 = enumValue(_targetCompanyId);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }

                case Widx::complete_challenge_button:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::completeChallenge;
                    args.param1 = enumValue(_targetCompanyId);

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }
            }
        }

        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            if (widgetIndex == Widx::target_company_dropdown_btn)
            {
                Dropdown::populateCompanySelect(&self, &self.widgets[widgetIndex - 1]);
            }
        }

        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                return;
            }

            if (widgetIndex == Widx::target_company_dropdown_btn)
            {
                _targetCompanyId = Dropdown::getCompanyIdFromSelection(itemIndex);
                self.invalidate();
            }
        }

        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, Common::Widx::tab_finances);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Vehicles
    {
        static constexpr Ui::Size kWindowSize = { 250, 188 };

        namespace Widx
        {
            enum
            {
                reliability_group = Common::Widx::nextWidx,
                reliablity_all_to_zero,
                reliablity_all_to_hundred,
                vehicle_locked_group,
                checkbox_display_locked_vehicles,
                checkbox_build_locked_vehicles,
                vehicle_cargo_group,
                checkbox_keep_cargo_modify_pickup,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize.width, kWindowSize.height, StringIds::vehicle_cheats),
            Widgets::GroupBox({ 4, 48 }, { kWindowSize.width - 8, 49 }, WindowColour::secondary, StringIds::cheat_set_vehicle_reliability),
            Widgets::Button({ 10, 62 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_reliability_zero),
            Widgets::Button({ 10, 78 }, { kWindowSize.width - 20, 12 }, WindowColour::secondary, StringIds::cheat_reliability_hundred),
            Widgets::GroupBox({ 4, 102 }, { kWindowSize.width - 8, 45 }, WindowColour::secondary, StringIds::cheat_build_vehicle_window),
            Widgets::Checkbox({ 10, 116 }, { 200, 12 }, WindowColour::secondary, StringIds::display_locked_vehicles, StringIds::tooltip_display_locked_vehicles),
            Widgets::Checkbox({ 25, 130 }, { 200, 12 }, WindowColour::secondary, StringIds::allow_building_locked_vehicles, StringIds::tooltip_build_locked_vehicles),
            Widgets::GroupBox({ 4, 152 }, { kWindowSize.width - 8, 30 }, WindowColour::secondary, StringIds::cheat_vehicle_cargo),
            Widgets::Checkbox({ 10, 166 }, { 200, 12 }, WindowColour::secondary, StringIds::cheat_keep_cargo_modify_pickup, StringIds::tooltip_keep_cargo_modify_pickup));

        static void prepareDraw(Window& self)
        {
            self.activatedWidgets = (1 << Common::Widx::tab_vehicles);

            if (Config::get().displayLockedVehicles)
            {
                self.activatedWidgets |= (1 << Widx::checkbox_display_locked_vehicles);
                self.disabledWidgets &= ~(1 << Widx::checkbox_build_locked_vehicles);
            }
            else
            {
                self.activatedWidgets &= ~(1 << Widx::checkbox_display_locked_vehicles);
                self.disabledWidgets |= (1 << Widx::checkbox_build_locked_vehicles);
            }

            if (Config::get().buildLockedVehicles)
            {
                self.activatedWidgets |= (1 << Widx::checkbox_build_locked_vehicles);
            }
            else
            {
                self.activatedWidgets &= ~(1 << Widx::checkbox_build_locked_vehicles);
            }

            if (Config::get().keepCargoModifyPickup)
            {
                self.activatedWidgets |= (1 << Widx::checkbox_keep_cargo_modify_pickup);
            }
            else
            {
                self.activatedWidgets &= ~(1 << Widx::checkbox_keep_cargo_modify_pickup);
            }
        }

        static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets and tabs.
            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);
        }

        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self.type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::reliablity_all_to_zero:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::vehicleReliability;
                    args.param1 = 0;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::vehicle);
                    WindowManager::invalidate(WindowType::vehicleList);
                    return;
                }

                case Widx::reliablity_all_to_hundred:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::vehicleReliability;
                    args.param1 = 100;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::vehicle);
                    WindowManager::invalidate(WindowType::vehicleList);
                    return;
                }

                case Widx::checkbox_display_locked_vehicles:
                {
                    Config::get().displayLockedVehicles = !Config::get().displayLockedVehicles;

                    // if we don't want to display locked vehicles, there is no reason to allow building them
                    if (Config::get().displayLockedVehicles)
                    {
                        self.disabledWidgets &= ~(1 << Widx::checkbox_build_locked_vehicles);
                    }
                    else
                    {
                        Config::get().buildLockedVehicles = false;
                        self.disabledWidgets |= (1 << Widx::checkbox_build_locked_vehicles);
                    }

                    Config::write();
                    WindowManager::invalidateWidget(self.type, self.number, Widx::checkbox_build_locked_vehicles);
                    WindowManager::invalidateWidget(self.type, self.number, Widx::checkbox_display_locked_vehicles);
                    WindowManager::invalidate(WindowType::buildVehicle);
                    break;
                }

                case Widx::checkbox_build_locked_vehicles:
                {
                    if (Config::get().displayLockedVehicles)
                    {
                        Config::get().buildLockedVehicles = !Config::get().buildLockedVehicles;
                        Config::write();
                        WindowManager::invalidateWidget(self.type, self.number, Widx::checkbox_build_locked_vehicles);
                        WindowManager::invalidate(WindowType::buildVehicle);
                    }
                    break;
                }

                case Widx::checkbox_keep_cargo_modify_pickup:
                {
                    Config::get().keepCargoModifyPickup = !Config::get().keepCargoModifyPickup;
                    Config::write();
                    WindowManager::invalidateWidget(self.type, self.number, Widx::checkbox_keep_cargo_modify_pickup);
                    break;
                }
            }
        }

        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, Common::Widx::tab_vehicles);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Towns
    {
        static constexpr Ui::Size kWindowSize = { 250, 103 };

        namespace Widx
        {
            enum
            {
                ratings_group = Common::Widx::nextWidx,
                ratings_all_min_10pct,
                ratings_all_plus_10pct,
                ratings_all_to_min,
                ratings_all_to_max,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize.width, kWindowSize.height, StringIds::town_cheats),
            Widgets::GroupBox({ 4, 48 }, { kWindowSize.width - 8, 49 }, WindowColour::secondary, StringIds::cheat_set_ratings),
            Widgets::Button({ 10, 62 }, { (kWindowSize.width - 26) / 2, 12 }, WindowColour::secondary, StringIds::cheat_ratings_min_10pct),
            Widgets::Button({ 3 + (kWindowSize.width / 2), 62 }, { (kWindowSize.width - 26) / 2, 12 }, WindowColour::secondary, StringIds::cheat_ratings_plus_10pct),
            Widgets::Button({ 10, 78 }, { (kWindowSize.width - 26) / 2, 12 }, WindowColour::secondary, StringIds::cheat_ratings_to_min),
            Widgets::Button({ 3 + (kWindowSize.width / 2), 78 }, { (kWindowSize.width - 26) / 2, 12 }, WindowColour::secondary, StringIds::cheat_ratings_to_max)

        );

        static void prepareDraw(Window& self)
        {
            self.activatedWidgets = (1 << Common::Widx::tab_towns);
        }

        static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets and tabs.
            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);
        }

        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self.type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::ratings_all_min_10pct:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::companyRatings;
                    args.param1 = false;
                    args.param2 = -10;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }

                case Widx::ratings_all_plus_10pct:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::companyRatings;
                    args.param1 = false;
                    args.param2 = 10;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }

                case Widx::ratings_all_to_min:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::companyRatings;
                    args.param1 = true;
                    args.param2 = -1;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }

                case Widx::ratings_all_to_max:
                {
                    GameCommands::GenericCheatArgs args{};
                    args.subcommand = CheatCommand::companyRatings;
                    args.param1 = true;
                    args.param2 = 1;

                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }
            }
        }

        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, Common::Widx::tab_towns);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::cheats);
        if (window != nullptr)
        {
            return window;
        }

        window = WindowManager::createWindow(
            WindowType::cheats,
            Finances::kWindowSize,
            WindowFlags::none,
            Finances::getEvents());

        Finances::initDate();

        window->setWidgets(Finances::_widgets);
        window->currentTab = Common::Widx::tab_finances - Common::Widx::tab_finances;
        window->holdableWidgets = Finances::holdableWidgets;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->windowTitlebarColour);
        window->setColour(WindowColour::secondary, skin->windowColour);

        return window;
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            WidgetIndex_t widgetIndex;
            const WindowEventList& events;
            const uint64_t* holdableWidgets;
            Ui::Size kWindowSize;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { Finances::_widgets,  Widx::tab_finances,  Finances::getEvents(),  &Finances::holdableWidgets, Finances::kWindowSize  },
            { Companies::_widgets, Widx::tab_companies, Companies::getEvents(), nullptr,                    Companies::kWindowSize },
            { Vehicles::_widgets,  Widx::tab_vehicles,  Vehicles::getEvents(),  nullptr,                    Vehicles::kWindowSize  },
            { Towns::_widgets,     Widx::tab_towns,     Towns::getEvents(),     nullptr,                    Towns::kWindowSize     },
        };
        // clang-format on

        static void switchTab(Window& self, WidgetIndex_t widgetIndex)
        {
            self.currentTab = widgetIndex - Widx::tab_finances;
            self.frameNo = 0;

            auto tabInfo = tabInformationByTabOffset[self.currentTab];

            self.holdableWidgets = tabInfo.holdableWidgets != nullptr ? *tabInfo.holdableWidgets : 0;
            self.eventHandlers = &tabInfo.events;
            self.activatedWidgets = 0;
            self.setWidgets(tabInfo.widgets);
            self.disabledWidgets = 0;

            self.invalidate();

            self.setSize(tabInfo.kWindowSize);
            self.callOnResize();
            self.callPrepareDraw();
            self.initScrollWidgets();
            self.invalidate();
            self.moveInsideScreenEdges();
        }
    }
}
