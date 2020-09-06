#include "../Company.h"
#include "../CompanyManager.h"
#include "../Date.h"
#include "../GameCommands.h"
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

namespace openloco::ui::windows::PlayerInfoPanel
{
    static const gfx::ui_size_t window_size = { 140, 27 };

    namespace widx
    {
        enum
        {
            outer_frame,
            inner_frame,
            player,
            company_value,
            performance_index
        };
    }

    static void performanceIndexMouseUp();
    static void companyValueTooltip(FormatArguments& args);
    static void performanceIndexTooltip(FormatArguments& args);
    static void companyValueMouseUp();

    // 0x00509d08
    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 140, 29 }, widget_type::wt_3, 0),
        makeWidget({ 2, 2 }, { 136, 25 }, widget_type::wt_3, 0),
        makeWidget({ 1, 1 }, { 26, 26 }, widget_type::wt_9, 0),
        makeWidget({ 27, 2 }, { 111, 12 }, widget_type::wt_9, 0, image_ids::null, string_ids::tooltip_company_value),
        makeWidget({ 27, 14 }, { 111, 12 }, widget_type::wt_9, 0, image_ids::null, string_ids::tooltip_performance_index),
        widgetEnd(),
    };

    static window_event_list _events;
    std::vector<const company*> _sortedCompanies;

    static loco_global<uint16_t, 0x0050A004> _50A004; // maybe date related
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;
    static loco_global<uint8_t, 0x00508F1A> game_speed;
    static loco_global<uint16_t, 0x0113DC78> _113DC78; // dropdown flags?

    static void prepareDraw(window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void onMouseUp(ui::window* window, widget_index widgetIndex);
    static void onMouseDown(ui::window* window, widget_index widgetIndex);
    static void onDropdown(window* w, widget_index widgetIndex, int16_t item_index);
    static ui::cursor_id onCursor(ui::window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex);
    static void onUpdate(window* w);

    // 0x43AA4C
    static void playerMouseDown(ui::window* self, widget_index widgetIndex)
    {
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

            auto args = FormatArguments();
            args.push(positionArray[index]);
            args.push(gfx::recolour(competitorObj->images[company->owner_emotion], company->mainColours.primary));
            args.push(company->name);
            args.push<uint16_t>(0); // Needed after a user string id
            formatPerformanceIndex(company->performance_index, args);

            dropdown::add(index, string_ids::dropdown_company_performance, args);

            if (isPlayerCompany(company->id()))
            {
                highlightIndex = index;
            }

            index++;
        }

        dropdown::add(index++, string_ids::dropdown_companies_list, image_ids::company_list_dropdown_icon);
        dropdown::showBelow(self, widgetIndex, index, 25, (1 << 6));
        if (highlightIndex != -1)
        {
            dropdown::setHighlightedItem(highlightIndex);
        }
        _113DC78 = _113DC78 | (1 << 1);
    }

    // 0x43AB87
    static void playerDropdownClick(int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = dropdown::getHighlightedItem();
        }

        // If its index is bigger than the list then its the company list extra item
        if (static_cast<uint16_t>(itemIndex) >= _sortedCompanies.size())
        {
            windows::CompanyList::open();
        }
        else
        {
            auto company = _sortedCompanies[itemIndex];
            if (!company->empty())
            {
                windows::CompanyWindow::open(company->id());
            }
        }
    }

    static void initEvents()
    {
        _events.on_mouse_up = onMouseUp;
        _events.event_03 = onMouseDown;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;
        _events.on_update = onUpdate;
        _events.tooltip = tooltip;
        _events.cursor = onCursor;
        _events.prepare_draw = prepareDraw;
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
        window->initScrollWidgets();

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = colour::translucent(skin->colour_16);
            window->colours[1] = colour::translucent(skin->colour_16);
        }

        return window;
    }

    // 0x004393E7
    static void prepareDraw(window* window)
    {
        window->widgets[widx::inner_frame].type = widget_type::none;
    }

    // 0x43944B
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        widget_t& frame = _widgets[widx::outer_frame];
        gfx::drawRect(dpi, window->x + frame.left, window->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        window->draw(dpi);

        drawRectInset(dpi, window->x + frame.left + 1, window->y + frame.top + 1, frame.width() - 2, frame.height() - 2, window->colours[1], 0x30);

        auto playerCompany = companymgr::get(companymgr::getControllingId());
        auto competitor = objectmgr::get<competitor_object>(playerCompany->competitor_id);
        auto image = gfx::recolour(competitor->images[playerCompany->owner_emotion], playerCompany->mainColours.primary);
        gfx::drawImage(dpi, window->x + frame.left + 2, window->y + frame.top + 2, image);

        auto x = window->x + frame.width() / 2 + 12;
        {
            auto companyValueString = string_ids::player_info_bankrupt;
            if (!(playerCompany->challenge_flags & company_flags::bankrupt))
            {
                if (static_cast<int16_t>(playerCompany->cash.var_04) < 0)
                {
                    companyValueString = string_ids::player_info_company_value_negative;
                }
                else
                {
                    companyValueString = string_ids::player_info_company_value;
                }
            }

            auto colour = colour::opaque(window->colours[0]);
            if (input::isHovering(WindowType::playerInfoToolbar, 0, widx::company_value))
            {
                colour = colour::white;
            }
            auto args = FormatArguments();
            args.push(playerCompany->cash.var_00);
            args.push(playerCompany->cash.var_04);
            gfx::drawStringCentred(*dpi, x, window->y + frame.top + 2, colour, companyValueString, &args);
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
            if (input::isHovering(WindowType::playerInfoToolbar, 0, widx::performance_index))
            {
                colour = colour::white;
            }

            auto args = FormatArguments();
            args.push(playerCompany->performance_index);
            gfx::drawStringCentred(*dpi, x, window->y + frame.top + 14, colour, performanceString, &args);
        }
    }

    // 0x004395A4
    static void onMouseUp(ui::window* window, widget_index widgetIndex)
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
    static void onMouseDown(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::player:
                playerMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x004395BC
    static void onDropdown(window* w, widget_index widgetIndex, int16_t item_index)
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
        windows::CompanyWindow::openFinances(companymgr::getControllingId());
    }

    // 0x4395D6
    static void performanceIndexMouseUp()
    {
        windows::CompanyList::openPerformanceIndexes();
    }

    // 0x004395DE
    static ui::cursor_id onCursor(ui::window* window, int16_t widgetIndex, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        switch (widgetIndex)
        {
            case widx::company_value:
            case widx::performance_index:
                _tooltipTimeout = 2000;
                break;
        }
        return fallback;
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
        auto playerCompany = companymgr::get(companymgr::getControllingId());
        args.push(playerCompany->companyValueHistory[0]);
        args.push(playerCompany->vehicleProfit);
    }

    // 0x439643
    static void performanceIndexTooltip(FormatArguments& args)
    {
        auto playerCompany = companymgr::get(companymgr::getControllingId());

        formatPerformanceIndex(playerCompany->performance_index, args);
    }

    // 0x00439670
    static void onUpdate(window* w)
    {
        w->var_854++;
        if (w->var_854 >= 24)
        {
            w->var_854 = 0;
        }

        if (_50A004 & (1 << 0))
        {
            _50A004 = _50A004 & ~(1 << 0);
            WindowManager::invalidateWidget(WindowType::playerInfoToolbar, 0, widx::inner_frame);
        }
    }
}
