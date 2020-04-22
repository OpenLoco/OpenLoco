#include "../company.h"
#include "../companymgr.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include <map>

using namespace openloco::interop;

namespace openloco::ui::PlayerInfoPanel
{
    static const gfx::ui_size_t window_size = { 140, 27 };

    namespace widx
    {
        enum
        {
            w0,
            w1,
            player,
            company_value,
            performance_index
        };
    }
    static void sub_439A1C();

    static void sub_43995C();
    static void sub_439A59();
    static void sub_439A61();
    static void sub_439A70(window* w);
    static void sub_439A93(window* w);
    static void sub_439AB6(window* w);

    static void performanceIndexMouseUp();
    static void sub_4395EB();

    static void companyValueTooltip(FormatArguments& args);
    static void performanceIndexTooltip(FormatArguments& args);
    static void companyValueMouseUp();

    enum class CorporateRating
    {
        platelayer,           // 0 – 9.9%
        engineer,             // 10 – 19.9%
        trafficManager,       // 20 – 29.9%
        transportCoordinator, // 30 – 39.9%
        routeSupervisor,      // 40 – 49.9%
        director,             // 50 – 59.9%
        chiefExecutive,       // 60 – 69.9%
        chairman,             // 70 – 79.9%
        president,            // 80 – 89.9%
        tycoon                // 90 – 100%
    };

    // 0x00509d08
    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 140, 29 }, widget_type::wt_3, 0),
        make_widget({ 2, 2 }, { 136, 25 }, widget_type::wt_3, 0),
        make_widget({ 1, 1 }, { 26, 26 }, widget_type::wt_9, 0),
        make_widget({ 27, 2 }, { 111, 12 }, widget_type::wt_9, 0, string_ids::null, string_ids::tooltip_company_value),
        make_widget({ 27, 14 }, { 111, 12 }, widget_type::wt_9, 0, string_ids::null, string_ids::tooltip_performance_index),
        widget_end(),
    };

    static std::map<CorporateRating, string_id> _ratingNames = {
        { CorporateRating::platelayer, string_ids::str_1772 },
        { CorporateRating::engineer, string_ids::str_1773 },
        { CorporateRating::trafficManager, string_ids::str_1774 },
        { CorporateRating::transportCoordinator, string_ids::str_1775 },
        { CorporateRating::routeSupervisor, string_ids::str_1776 },
        { CorporateRating::director, string_ids::str_1777 },
        { CorporateRating::chiefExecutive, string_ids::str_1778 },
        { CorporateRating::chairman, string_ids::str_1779 },
        { CorporateRating::president, string_ids::str_1780 },
        { CorporateRating::tycoon, string_ids::str_1781 },
    };

    static window_event_list _events;
    std::vector<const company*> _sortedCompanies;

    static loco_global<uint16_t, 0x0050A004> _50A004;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;
    static loco_global<uint8_t, 0x00508F1A> game_speed;
    static loco_global<uint16_t, 0x0113DC78> _113DC78;

    static void prepare_draw(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_mouse_down(ui::window* window, widget_index widgetIndex);
    static void on_dropdown(window* w, widget_index widgetIndex, int16_t item_index);
    static ui::cursor_id on_cursor(ui::window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex);
    static void on_update(window* w);
    static CorporateRating performanceToRating(int16_t ax);

    // 0x43AA4C
    static void playerMouseDown(ui::window* self, widget_index widgetIndex)
    {
        auto args = FormatArguments();
        _sortedCompanies.clear();

        for (const auto& c : companymgr::companies())
        {
            if (!c.empty())
            {
                _sortedCompanies.push_back(&c);
            }
        }

        sort(
            _sortedCompanies.begin(),
            _sortedCompanies.end(),
            [](const company* a, const company* b) {
                return a->performance_index > b->performance_index;
            });

        const string_id positionArray[15] = {
            string_ids::position_1st,
            string_ids::position_2nd,
            string_ids::position_3rd,
            string_ids::position_4th,
            string_ids::position_5th,
            string_ids::position_6th,
            string_ids::position_7th,
            string_ids::position_8th,
            string_ids::position_9th,
            string_ids::position_10th,
            string_ids::position_11th,
            string_ids::position_12th,
            string_ids::position_13th,
            string_ids::position_14th,
            string_ids::position_15th,
        };

        int index = 0;
        auto highlightIndex = -1;
        for (auto company : _sortedCompanies)
        {
            auto competitorObj = objectmgr::get<competitor_object>(company->competitor_id);
            auto rating = performanceToRating(company->performance_index);

            args.push(positionArray[index]);
            args.push(gfx::recolour(competitorObj->images[company->owner_emotion], company->colour.primary << 19));
            args.push(company->name);
            args.push<uint16_t>(0); // Needed after a user string id
            args.push(company->performance_index);
            args.push(_ratingNames[rating]);

            dropdown::add(index, string_ids::dropdown_company_performance, args);

            if (is_player_company(company->id()))
            {
                highlightIndex = index;
            }

            index++;
        }

        dropdown::add(index++, string_ids::dropdown_companies_list, image_ids::company_list_dropdown_icon);
        dropdown::show_below(self, widgetIndex, index, 25);
        if (highlightIndex != -1)
        {
            dropdown::set_highlighted_item(highlightIndex);
        }
        _113DC78 = _113DC78 | (1 << 1);
    }

    // 0x43AB87
    static void playerDropdownClick(int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = dropdown::get_highlighted_item();
        }

        // If its index is bigger than the list then its the company list extra item
        if (static_cast<uint16_t>(itemIndex) >= _sortedCompanies.size())
        {
            windows::CompanyList::OpenUnk();
        }
        else
        {
            auto company = _sortedCompanies[itemIndex];
            if (!company->empty())
            {
                windows::CompanyWindow::OpenUnk(company->id());
            }
        }
    }

    static void initEvents()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.event_03 = on_mouse_down;
        _events.on_mouse_down = on_mouse_down;
        _events.on_dropdown = on_dropdown;
        _events.on_update = on_update;
        _events.tooltip = tooltip;
        _events.cursor = on_cursor;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;
    }

    // 0x00438BC7
    window* open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::playerInfoToolbar,
            gfx::point_t(0, ui::height() - window_size.height),
            gfx::ui_size_t(window_size.width, window_size.height),
            ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::player) | (1 << widx::company_value) | (1 << widx::performance_index);
        window->var_854 = 0;
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = colour::translucent(skin->colour_16);
            window->colours[1] = colour::translucent(skin->colour_16);
        }

        return window;
    }

    // 0x004393E7
    static void prepare_draw(window* window)
    {
        window->widgets[widx::w1].type = widget_type::none;
    }

    // 0x43944B
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        widget_t& frame = _widgets[widx::w0];
        gfx::draw_rect(dpi, window->x + frame.left, window->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        window->draw(dpi);

        draw_rect_inset(dpi, window->x + frame.left + 1, window->y + frame.top + 1, frame.width() - 2, frame.height() - 2, window->colours[1], 0x30);

        auto playerCompany = companymgr::get(companymgr::get_controlling_id());
        auto competitor = objectmgr::get<competitor_object>(playerCompany->competitor_id);
        auto image = gfx::recolour(competitor->images[playerCompany->owner_emotion], playerCompany->colour.primary);
        gfx::draw_image(dpi, window->x + frame.left + 2, window->y + frame.top + 2, image);

        auto x = window->x + frame.width() / 2 + 12;
        {
            auto companyValueString = string_ids::player_info_bankrupt;
            if (!(playerCompany->challenge_flags & company_flags::unk_9))
            {
                if (static_cast<int16_t>(playerCompany->var_08.var_04) < 0)
                {
                    companyValueString = string_ids::player_info_company_value_negative;
                }
                else
                {
                    companyValueString = string_ids::player_info_company_value;
                }
            }

            auto colour = window->colours[0] & 0x7F;
            if (input::is_hovering(WindowType::playerInfoToolbar, 0, widx::company_value))
            {
                colour = colour::white;
            }
            auto args = FormatArguments();
            args.push(playerCompany->var_08.var_00);
            args.push(playerCompany->var_08.var_04);
            gfx::draw_string_centred(*dpi, x, window->y + frame.top + 2, colour, companyValueString, &args);
        }

        {
            auto performanceString = string_ids::player_info_performance;

            if (playerCompany->challenge_flags & company_flags::increased_performance)
            {
                performanceString = string_ids::player_info_performance_increase;
            }
            else if (playerCompany->challenge_flags & (company_flags::decreased_performance))
            {
                performanceString = string_ids::player_info_performance_decrease;
            }

            auto colour = window->colours[0] & 0x7F;
            if (input::is_hovering(WindowType::playerInfoToolbar, 0, widx::performance_index))
            {
                colour = colour::white;
            }

            auto args = FormatArguments();
            args.push(playerCompany->performance_index);
            gfx::draw_string_centred(*dpi, x, window->y + frame.top + 14, colour, performanceString, &args);
        }
    }

    // 0x004395A4
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::company_value:
                companyValueMouseUp();
                break;
            case widx::performance_index:
                performanceIndexMouseUp();
                break;
        }
    }

    // 0x004395B1
    static void on_mouse_down(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::player:
                playerMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x004395BC
    static void on_dropdown(window* w, widget_index widgetIndex, int16_t item_index)
    {
        switch (widgetIndex)
        {
            case widx::player:
                playerDropdownClick(item_index);
                break;
        }
    }

    // 0x4395C7
    static void companyValueMouseUp()
    {
        windows::CompanyWindow::openFinances(companymgr::get_controlling_id());
    }

    // 0x4395D6
    static void performanceIndexMouseUp()
    {
        windows::CompanyList::openPerformanceIndexes();
    }

    // 0x004395DE
    static ui::cursor_id on_cursor(ui::window* window, int16_t widgetIndex, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        switch (widgetIndex)
        {
            case widx::company_value:
                sub_4395EB();
                break;

            case widx::performance_index:
                sub_4395EB();
                break;
        }
        return fallback;
    }

    static void sub_4395EB()
    {
        _tooltipTimeout = 2000;
    }

    // 0x004395F5
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::company_value:
                companyValueTooltip(args);
                break;

            case widx::performance_index:
                performanceIndexTooltip(args);
                break;
        }
    }

    static void companyValueTooltip(FormatArguments& args)
    {
        auto playerCompany = companymgr::get(companymgr::get_controlling_id());
        args.push(playerCompany->companyValue);
        args.push(playerCompany->vehicleProfit);
    }

    // Converts performance index to rating
    // 0x437D60
    static CorporateRating performanceToRating(int16_t ax)
    {
        return static_cast<CorporateRating>(std::min(9, ax / 100));
    }

    static void performanceIndexTooltip(FormatArguments& args)
    {
        auto playerCompany = companymgr::get(companymgr::get_controlling_id());

        args.push(playerCompany->performance_index);

        auto rating = performanceToRating(playerCompany->performance_index);
        args.push(_ratingNames[rating]);
    }

    // 0x00439670
    static void on_update(window* w)
    {
        w->var_854++;
        if (w->var_854 >= 24)
        {
            w->var_854 = 0;
        }

        if (_50A004 & (1 << 0))
        {
            _50A004 = _50A004 & ~(1 << 0);
            WindowManager::invalidateWidget(WindowType::playerInfoToolbar, 0, widx::w1);
        }
    }
}
