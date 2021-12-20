#include "../CompanyManager.h"
#include "../GameCommands/Cheat.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include "../Date.h"
#include "../Scenario.h"
#include "../Localisation/StringManager.h"

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
            constexpr uint32_t nextWidx = 9;
        }

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                           \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                          \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                     \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::wt_9, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, frameHeight - 41 }, WidgetType::panel, WindowColour::secondary),                                              \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_8, WindowColour::secondary, ImageIds::tab),                                                 \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::wt_8, WindowColour::secondary, ImageIds::tab),                                                \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::wt_8, WindowColour::secondary, ImageIds::tab),                                                \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::wt_8, WindowColour::secondary, ImageIds::tab)

        // this should be 1 more than the number of widgets defined above in commonWidgets

        constexpr uint64_t enabledWidgets = (1 << Widx::close_button) | (1 << Widx::tab_finances) | (1 << Widx::tab_companies) | (1 << Widx::tab_vehicles) | (1 << Widx::tab_towns);

        static void drawTabs(Ui::Window* const self, Gfx::Context* const context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

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
                if (self->current_tab == Widx::tab_finances - Widx::tab_finances)
                    imageId += financesTabImageIds[(self->frame_no / 2) % std::size(financesTabImageIds)];
                else
                    imageId += financesTabImageIds[0];

                Widget::drawTab(self, context, imageId, Widx::tab_finances);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_company;
                Widget::drawTab(self, context, imageId, Widx::tab_companies);
            }

            // Vehicles tab
            {
                static const uint32_t vehiclesTabImageIds[] = {
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
                if (self->current_tab == Widx::tab_vehicles - Widx::tab_finances)
                    imageId += vehiclesTabImageIds[(self->frame_no / 2) % std::size(vehiclesTabImageIds)];
                else
                    imageId += vehiclesTabImageIds[0];

                auto companyId = CompanyManager::getControllingId();
                auto companyColour = CompanyManager::getCompanyColour(companyId);

                imageId = Gfx::recolour(imageId, companyColour);

                Widget::drawTab(self, context, imageId, Widx::tab_vehicles);
            }

            // Towns tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_towns;
                Widget::drawTab(self, context, imageId, Widx::tab_towns);
            }

            // Time tab
            /*{
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_wrench_frame0;
                Widget::drawTab(self, context, imageId, Widx::tab_time);
            }*/
        }

        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
    }

    namespace Finances
    {
        constexpr Ui::Size windowSize = { 250, 210 };

        static WindowEventList _events;

        namespace Widx
        {
            enum
            {
                cash_step_group = Common::Widx::nextWidx,
                cash_step_value,
                cash_step_decrease,
                cash_step_increase,
                cash_step_apply,
                loan_group,
                loan_value,
                loan_clear,
                year_step_value,
                year_step_decrease,
                year_step_increase,
                month_step_value,
                month_step_decrease,
                month_step_increase,
                day_step_value,
                day_step_decrease,
                day_step_increase,
                date_change_apply,
            };
        }

        static Widget _widgets[] = {
            commonWidgets(windowSize.width, windowSize.height, StringIds::financial_cheats),
            // money
            makeWidget({ 4, 48 }, { windowSize.width - 8, 33 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_increase_funds),
            makeStepperWidgets({ 80, 62 }, { 95, 12 }, WidgetType::wt_17, WindowColour::secondary, StringIds::empty),
            makeWidget({ 180, 62 }, { 60, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_add),
            // loan
            makeWidget({ 4, 86 }, { windowSize.width - 8, 33 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_clear_loan),
            makeWidget({ 80, 100 }, { 95, 12 }, WidgetType::wt_17, WindowColour::secondary),
            makeWidget({ 180, 100 }, { 60, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_clear),
            // date/time
            makeWidget({ 4, 124 }, { windowSize.width - 8, 80 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_date_change_apply),
            makeStepperWidgets({ 80, 138 }, { 95, 12 }, WidgetType::wt_17, WindowColour::secondary, StringIds::empty),
            makeStepperWidgets({ 80, 154 }, { 95, 12 }, WidgetType::wt_17, WindowColour::secondary, StringIds::empty),
            makeStepperWidgets({ 80, 170 }, { 95, 12 }, WidgetType::wt_17, WindowColour::secondary, StringIds::empty),
            makeWidget({ 10, 186 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_date_change_apply),
            widgetEnd(),
        };

        static uint64_t enabledWidgets
            = Common::enabledWidgets
            | (1 << Widx::loan_clear)
            | (1 << Widx::cash_step_decrease)
            | (1 << Widx::cash_step_increase)
            | (1 << Widx::cash_step_apply)
            | (1 << Widx::year_step_decrease)
            | (1 << Widx::year_step_increase)
            | (1 << Widx::month_step_decrease)
            | (1 << Widx::month_step_increase)
            | (1 << Widx::day_step_decrease)
            | (1 << Widx::day_step_increase)
            | (1 << Widx::date_change_apply);

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
        static uint32_t _dateIncreaseStep = 1;
        static Date _date;

        static void prepareDraw(Window* self)
        {
            self->activated_widgets = (1 << Common::Widx::tab_finances);
        }

        static void draw(Ui::Window* const self, Gfx::Context* const context)
        {
            // Draw widgets and tabs.
            self->draw(context);
            Common::drawTabs(self, context);

            // Add cash step label and value
            {
                auto& widget = self->widgets[Widx::cash_step_value];
                Gfx::drawString_494B3F(
                    *context,
                    self->x + 10,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_amount);

                auto args = FormatArguments::common();
                args.push(_cashIncreaseStep);
                Gfx::drawString_494B3F(
                    *context,
                    self->x + widget.left + 1,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_loan_value,
                    &args);
            }

            // Loan label and value
            {
                auto& widget = self->widgets[Widx::loan_value];
                Gfx::drawString_494B3F(
                    *context,
                    self->x + 10,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::company_current_loan);

                auto company = CompanyManager::getPlayerCompany();
                auto args = FormatArguments::common();
                args.push(company->current_loan);

                Gfx::drawString_494B3F(
                    *context,
                    self->x + widget.left + 1,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_loan_value,
                    &args);
            }

            // Add year label and value
            {
                auto& widget = self->widgets[Widx::year_step_value];
                Gfx::drawString_494B3F(
                    *context,
                    self->x + 10,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_year);

                auto args = FormatArguments::common();
                args.push(_date.year);
                Gfx::drawString_494B3F(
                    *context,
                    self->x + widget.left + 1,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_year_value,
                    &args);
            }

            // Add month label and value
            {
                auto& widget = self->widgets[Widx::month_step_value];
                Gfx::drawString_494B3F(
                    *context,
                    self->x + 10,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_month);

                auto args = FormatArguments::common();
                args.push((string_id)OpenLoco::StringManager::monthToString(_date.month).second);
                Gfx::drawString_494B3F(
                    *context,
                    self->x + widget.left + 1,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::black_stringid,
                    &args);
            }

            // Add day label and value
            {
                auto& widget = self->widgets[Widx::day_step_value];
                Gfx::drawString_494B3F(
                    *context,
                    self->x + 10,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_day);

                auto args = FormatArguments::common();
                args.push(_date.day + 1);
                Gfx::drawString_494B3F(
                    *context,
                    self->x + widget.left + 1,
                    self->y + widget.top,
                    Colour::black,
                    StringIds::cheat_day_value,
                    &args);
            }
        }

        static void onMouseUp(Ui::Window* const self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self->type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::cash_step_apply:
                    GameCommands::do_81(CheatCommand::addCash, _cashIncreaseStep);
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    break;

                case Widx::loan_clear:
                    GameCommands::do_81(CheatCommand::clearLoan);
                    WindowManager::invalidateWidget(self->type, self->number, Widx::loan_value);
                    break;

                case Widx::date_change_apply:
                {
                    GameCommands::do_81(CheatCommand::modifyDate, _date.year, (int32_t)_date.month, _date.day);
                    Windows::TimePanel::invalidateFrame();
                    break;
                }
            }
        }

        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;

            currency32_t cashStepSize{};
            int32_t timeStepSize{};

            if (*_clickRepeatTicks < 100)
            {
                cashStepSize = 1'000;
                timeStepSize = 1;
            }
            else if (*_clickRepeatTicks < 200)
            {
                cashStepSize = 10'000;
                timeStepSize = 10;
            }
            else if (*_clickRepeatTicks < 300)
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
                    WindowManager::invalidateWidget(self->type, self->number, Widx::cash_step_value);
                    break;

                case Widx::cash_step_increase:
                    _cashIncreaseStep = std::max<currency32_t>(_cashIncreaseStep + cashStepSize, 0);
                    WindowManager::invalidateWidget(self->type, self->number, Widx::cash_step_value);
                    break;

                case Widx::year_step_decrease:
                    _date.year = std::max<int32_t>(OpenLoco::Scenario::min_year, _date.year - timeStepSize);
                    break;

                case Widx::year_step_increase:
                    _date.year = std::min<int32_t>(std::numeric_limits<int32_t>().max(), _date.year + timeStepSize);
                    break;

                case Widx::month_step_decrease:
                    _date.month = (MonthId)(std::max<int8_t>(0, ((int8_t)_date.month - timeStepSize)));
                    break;

                case Widx::month_step_increase:
                    _date.month = (MonthId)(std::min<int8_t>(11, ((int8_t)_date.month + timeStepSize)));
                    break;

                case Widx::day_step_decrease:
                    _date.day = std::max<int32_t>(0, _date.day - timeStepSize);
                    break;

                case Widx::day_step_increase:
                    _date.day = std::min<int32_t>(getMonthTotalDay(_date.year, _date.month) - 1, _date.day + timeStepSize);
                    break;
            }

             WindowManager::invalidate(WindowType::cheats);
        }

        static void onUpdate(Window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(self->type, self->number, Common::Widx::tab_finances);
        }

        static void initEvents()
        {
            _date = getCurrentDate();
            _events.draw = draw;
            _events.on_mouse_up = onMouseUp;
            _events.on_mouse_down = onMouseDown;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
        }
    }

    namespace Companies
    {
        constexpr Ui::Size windowSize = { 250, 172 };

        static WindowEventList _events;

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
            };
        }

        static Widget _widgets[] = {
            commonWidgets(windowSize.width, windowSize.height, StringIds::company_cheats),
            makeWidget({ 4, 48 }, { windowSize.width - 8, 33 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_select_target_company),
            makeDropdownWidgets({ 10, 62 }, { windowSize.width - 20, 12 }, WidgetType::wt_17, WindowColour::secondary),
            makeWidget({ 4, 86 }, { windowSize.width - 8, 80 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_select_cheat_to_apply),
            makeWidget({ 10, 100 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_switch_to_company),
            makeWidget({ 10, 116 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_acquire_company_assets),
            makeWidget({ 10, 132 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_toggle_bankruptcy),
            makeWidget({ 10, 148 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_toggle_jail_status),
            widgetEnd(),
        };

        static uint64_t enabledWidgets = Common::enabledWidgets | (1 << Widx::target_company_dropdown) | (1 << Widx::target_company_dropdown_btn) | (1 << Widx::switch_company_button) | (1 << Widx::acquire_company_assets_button) | (1 << Widx::toggle_bankruptcy_button) | (1 << Widx::toggle_jail_status_button);

        static CompanyId _targetCompanyId{};

        static void prepareDraw(Window* self)
        {
            self->activated_widgets = (1 << Common::Widx::tab_companies);

            if (_targetCompanyId == CompanyManager::getControllingId())
            {
                self->disabled_widgets |= (1 << Widx::switch_company_button) | (1 << Widx::acquire_company_assets_button);
            }
            else
            {
                self->disabled_widgets &= ~((1 << Widx::switch_company_button) | (1 << Widx::acquire_company_assets_button));
            }
        }

        static void draw(Ui::Window* const self, Gfx::Context* const context)
        {
            // Draw widgets and tabs.
            self->draw(context);
            Common::drawTabs(self, context);

            // Draw current company name
            auto company = CompanyManager::get(_targetCompanyId);
            auto& widget = self->widgets[Widx::target_company_dropdown];
            Gfx::drawString_494B3F(
                *context,
                self->x + widget.left,
                self->y + widget.top,
                Colour::black,
                StringIds::black_stringid,
                &company->name);
        }

        static void onMouseUp(Ui::Window* const self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self->type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::acquire_company_assets_button:
                {
                    GameCommands::do_81(CheatCommand::acquireAssets, enumValue(_targetCompanyId));
                    Gfx::invalidateScreen();
                    return;
                }

                case Widx::switch_company_button:
                {
                    GameCommands::do_81(CheatCommand::switchCompany, enumValue(_targetCompanyId));
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }

                case Widx::toggle_bankruptcy_button:
                {
                    GameCommands::do_81(CheatCommand::toggleBankruptcy, enumValue(_targetCompanyId));
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }

                case Widx::toggle_jail_status_button:
                {
                    GameCommands::do_81(CheatCommand::toggleJail, enumValue(_targetCompanyId));
                    WindowManager::invalidate(WindowType::playerInfoToolbar);
                    return;
                }
            }
        }

        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Widx::target_company_dropdown)
                Dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
        }

        static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            if (widgetIndex == Widx::target_company_dropdown)
            {
                _targetCompanyId = Dropdown::getCompanyIdFromSelection(itemIndex);
                self->invalidate();
            }
        }

        static void onUpdate(Window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(self->type, self->number, Common::Widx::tab_finances);
        }

        static void initEvents()
        {
            _events.draw = draw;
            _events.on_dropdown = onDropdown;
            _events.on_mouse_up = onMouseUp;
            _events.on_mouse_down = onMouseDown;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
        }
    }

    namespace Vehicles
    {
        constexpr Ui::Size windowSize = { 250, 103 };

        static WindowEventList _events;

        namespace Widx
        {
            enum
            {
                reliability_group = Common::Widx::nextWidx,
                reliablity_all_to_zero,
                reliablity_all_to_hundred,
            };
        }

        static Widget _widgets[] = {
            commonWidgets(windowSize.width, windowSize.height, StringIds::vehicle_cheats),
            makeWidget({ 4, 48 }, { windowSize.width - 8, 49 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_set_vehicle_reliability),
            makeWidget({ 10, 62 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_reliability_zero),
            makeWidget({ 10, 78 }, { windowSize.width - 20, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_reliability_hundred),
            widgetEnd(),
        };

        static uint64_t enabledWidgets = Common::enabledWidgets | (1 << Widx::reliablity_all_to_zero) | (1 << Widx::reliablity_all_to_hundred);

        static void prepareDraw(Window* self)
        {
            self->activated_widgets = (1 << Common::Widx::tab_vehicles);
        }

        static void draw(Ui::Window* const self, Gfx::Context* const context)
        {
            // Draw widgets and tabs.
            self->draw(context);
            Common::drawTabs(self, context);
        }

        static void onMouseUp(Ui::Window* const self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self->type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::reliablity_all_to_zero:
                {
                    GameCommands::do_81(CheatCommand::vehicleReliability, 0);
                    WindowManager::invalidate(WindowType::vehicle);
                    WindowManager::invalidate(WindowType::vehicleList);
                    return;
                }

                case Widx::reliablity_all_to_hundred:
                {
                    GameCommands::do_81(CheatCommand::vehicleReliability, 100);
                    WindowManager::invalidate(WindowType::vehicle);
                    WindowManager::invalidate(WindowType::vehicleList);
                    return;
                }
            }
        }

        static void onUpdate(Window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(self->type, self->number, Common::Widx::tab_vehicles);
        }

        static void initEvents()
        {
            _events.draw = draw;
            _events.on_mouse_up = onMouseUp;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
        }
    }

    namespace Towns
    {
        constexpr Ui::Size windowSize = { 250, 103 };

        static WindowEventList _events;

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

        static Widget _widgets[] = {
            commonWidgets(windowSize.width, windowSize.height, StringIds::town_cheats),
            makeWidget({ 4, 48 }, { windowSize.width - 8, 49 }, WidgetType::groupbox, WindowColour::secondary, StringIds::cheat_set_ratings),
            makeWidget({ 10, 62 }, { (windowSize.width - 26) / 2, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_ratings_min_10pct),
            makeWidget({ 3 + (windowSize.width / 2), 62 }, { (windowSize.width - 26) / 2, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_ratings_plus_10pct),
            makeWidget({ 10, 78 }, { (windowSize.width - 26) / 2, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_ratings_to_min),
            makeWidget({ 3 + (windowSize.width / 2), 78 }, { (windowSize.width - 26) / 2, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::cheat_ratings_to_max),
            widgetEnd(),
        };

        static uint64_t enabledWidgets = Common::enabledWidgets | (1 << Widx::ratings_all_min_10pct) | (1 << Widx::ratings_all_plus_10pct) | (1 << Widx::ratings_all_to_min) | (1 << Widx::ratings_all_to_max);

        static void prepareDraw(Window* self)
        {
            self->activated_widgets = (1 << Common::Widx::tab_towns);
        }

        static void draw(Ui::Window* const self, Gfx::Context* const context)
        {
            // Draw widgets and tabs.
            self->draw(context);
            Common::drawTabs(self, context);
        }

        static void onMouseUp(Ui::Window* const self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self->type);
                    break;

                case Common::Widx::tab_finances:
                case Common::Widx::tab_companies:
                case Common::Widx::tab_vehicles:
                case Common::Widx::tab_towns:
                    Common::switchTab(self, widgetIndex);
                    break;

                case Widx::ratings_all_min_10pct:
                {
                    GameCommands::do_81(CheatCommand::companyRatings, false, -10);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }

                case Widx::ratings_all_plus_10pct:
                {
                    GameCommands::do_81(CheatCommand::companyRatings, false, 10);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }

                case Widx::ratings_all_to_min:
                {
                    GameCommands::do_81(CheatCommand::companyRatings, true, -1);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }

                case Widx::ratings_all_to_max:
                {
                    GameCommands::do_81(CheatCommand::companyRatings, true, 1);
                    WindowManager::invalidate(WindowType::town);
                    return;
                }
            }
        }

        static void onUpdate(Window* const self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(self->type, self->number, Common::Widx::tab_towns);
        }

        static void initEvents()
        {
            _events.draw = draw;
            _events.on_mouse_up = onMouseUp;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
        }
    }

    static void initEvents();

    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::cheats);
        if (window != nullptr)
            return window;

        initEvents();

        window = WindowManager::createWindow(
            WindowType::cheats,
            Finances::windowSize,
            0,
            &Finances::_events);

        window->widgets = Finances::_widgets;
        window->current_tab = Common::Widx::tab_finances - Common::Widx::tab_finances;
        window->enabled_widgets = Finances::enabledWidgets;
        window->holdable_widgets = Finances::holdableWidgets;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_0C);

        return window;
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            WidgetIndex_t widgetIndex;
            WindowEventList* events;
            const uint64_t* enabledWidgets;
            const uint64_t* holdableWidgets;
            Ui::Size windowSize;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { Finances::_widgets,  Widx::tab_finances,  &Finances::_events,  &Finances::enabledWidgets,  &Finances::holdableWidgets, Finances::windowSize  },
            { Companies::_widgets, Widx::tab_companies, &Companies::_events, &Companies::enabledWidgets, nullptr,                    Companies::windowSize },
            { Vehicles::_widgets,  Widx::tab_vehicles,  &Vehicles::_events,  &Vehicles::enabledWidgets,  nullptr,                    Vehicles::windowSize  },
            { Towns::_widgets,     Widx::tab_towns,     &Towns::_events,     &Towns::enabledWidgets,     nullptr,                    Towns::windowSize     },
        };
        // clang-format on

        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            self->current_tab = widgetIndex - Widx::tab_finances;
            self->frame_no = 0;

            auto tabInfo = tabInformationByTabOffset[self->current_tab];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = tabInfo.holdableWidgets != nullptr ? *tabInfo.holdableWidgets : 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabled_widgets = 0;

            self->invalidate();

            self->setSize(tabInfo.windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }
    }

    static void initEvents()
    {
        Finances::initEvents();
        Companies::initEvents();
        Vehicles::initEvents();
        Towns::initEvents();
    }
}
