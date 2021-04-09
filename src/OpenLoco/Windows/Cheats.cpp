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
        }

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                        \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                          \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                     \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, frameHeight - 41 }, widget_type::panel, 1),                                                \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab),                                                   \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab),                                                  \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab),                                                  \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab)

        constexpr uint64_t enabledWidgets = (1 << Widx::close_button) | (1 << Widx::tab_finances) | (1 << Widx::tab_companies) | (1 << Widx::tab_vehicles) | (1 << Widx::tab_towns);

        static void drawTabs(Ui::window* const self, Gfx::drawpixelinfo_t* const context)
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

                Widget::draw_tab(self, context, imageId, Widx::tab_finances);
            }

            // Companies tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_company;
                Widget::draw_tab(self, context, imageId, Widx::tab_companies);
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

                Widget::draw_tab(self, context, imageId, Widx::tab_vehicles);
            }

            // Towns tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_towns;
                Widget::draw_tab(self, context, imageId, Widx::tab_towns);
            }
        }
    }

    namespace Finances
    {
        constexpr Gfx::ui_size_t windowSize = { 250, 182 };

        static window_event_list _events;

        namespace Widx
        {
            enum
            {
                loan_group = 8,
                loan_value,
                loan_clear,
                switch_company_group,
                switch_company_dropdown,
                switch_company_dropdown_btn,
                switch_company_apply,
            };
        }

        static widget_t _widgets[] = {
            commonWidgets(windowSize.width, windowSize.height, StringIds::financial_cheats),
            makeWidget({ 4, 48 }, { windowSize.width - 8, 30 }, widget_type::groupbox, 1, StringIds::cheat_current_loan),
            makeWidget({ 10, 60 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::company_current_loan_value),
            makeWidget({ 115, 60 }, { 80, 12 }, widget_type::wt_11, 1, StringIds::cheat_clear_loan),
            makeWidget({ 4, 80 }, { windowSize.width - 8, 30 }, widget_type::groupbox, 1, StringIds::cheat_current_loan),
            makeDropdownWidgets({ 10, 92 }, { 100, 12 }, widget_type::wt_17, 1),
            makeWidget({ 115, 92 }, { 80, 12 }, widget_type::wt_11, 1, StringIds::cheat_clear_loan),
            widgetEnd(),
        };

        static uint64_t enabledWidgets = Common::enabledWidgets | (1 << Widx::loan_clear) | (1 << Widx::switch_company_dropdown) | (1 << Widx::switch_company_dropdown_btn) | (1 << Widx::switch_company_apply);

        static void prepareDraw(window* self)
        {
            self->activated_widgets = (1 << Common::Widx::tab_finances);

            auto company = CompanyManager::getPlayerCompany();
            auto args = FormatArguments::common();
            args.skip(4);
            args.push(company->current_loan);
        }

        static void draw(Ui::window* const self, Gfx::drawpixelinfo_t* const context)
        {
            // Draw widgets and tabs.
            self->draw(context);
            Common::drawTabs(self, context);

            // Draw current company name
            auto company = CompanyManager::getPlayerCompany();
            auto& widget = self->widgets[Widx::switch_company_dropdown];
            Gfx::drawString_494B3F(
                *context,
                self->x + widget.left,
                self->y + widget.top,
                Colour::black,
                company->name);
        }

        static void onMouseUp(Ui::window* const self, const widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::Widx::close_button:
                    WindowManager::close(self->type);
                    break;

                case Widx::loan_clear:
                    GameCommands::do_81(CheatCommand::clearLoan);
                    WindowManager::invalidateWidget(self->type, self->number, Widx::loan_value);
                    break;
            }
        }

        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == Widx::switch_company_dropdown)
                Dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
        }

        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            CompanyId_t targetCompanyId = Dropdown::getCompanyIdFromSelection(itemIndex);

            if (widgetIndex == Widx::switch_company_dropdown)
            {
                GameCommands::do_81(CheatCommand::switchCompany, targetCompanyId);
                WindowManager::invalidate(WindowType::playerInfoToolbar);
                return;
            }
        }

        static void onUpdate(window* const self)
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

    static void initEvents();

    window* open()
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
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0C;

        return window;
    }

    static void initEvents()
    {
        Finances::initEvents();
    }
}
