#include "../Company.h"
#include "../CompanyManager.h"
#include "../Date.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::PlayerInfoPanel
{
    static const Gfx::ui_size_t window_size = { 140, 27 };

    namespace Widx
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
    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 140, 29 }, WidgetType::wt_3, WindowColour::primary),
        makeWidget({ 2, 2 }, { 136, 25 }, WidgetType::wt_3, WindowColour::primary),
        makeWidget({ 1, 1 }, { 26, 26 }, WidgetType::wt_9, WindowColour::primary),
        makeWidget({ 27, 2 }, { 111, 12 }, WidgetType::wt_9, WindowColour::primary, ImageIds::null, StringIds::tooltip_company_value),
        makeWidget({ 27, 14 }, { 111, 12 }, WidgetType::wt_9, WindowColour::primary, ImageIds::null, StringIds::tooltip_performance_index),
        widgetEnd(),
    };

    static WindowEventList _events;
    std::vector<const Company*> _sortedCompanies;

    static loco_global<uint16_t, 0x0050A004> _50A004;
    static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;
    static loco_global<uint16_t, 0x0113DC78> _113DC78; // Dropdown flags?

    static void prepareDraw(Window* window);
    static void draw(Ui::Window* window, Gfx::Context* context);
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onMouseDown(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onDropdown(Window* w, WidgetIndex_t widgetIndex, int16_t item_index);
    static Ui::CursorId onCursor(Ui::Window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback);
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onUpdate(Window* w);

    // 0x43AA4C
    static void playerMouseDown(Ui::Window* self, WidgetIndex_t widgetIndex)
    {
        _sortedCompanies.clear();

        for (const auto& c : CompanyManager::companies())
        {
            _sortedCompanies.push_back(&c);
        }

        sort(
            _sortedCompanies.begin(),
            _sortedCompanies.end(),
            [](const Company* a, const Company* b) {
                return a->performance_index > b->performance_index;
            });

        const string_id positionArray[15] = {
            StringIds::position_1st,
            StringIds::position_2nd,
            StringIds::position_3rd,
            StringIds::position_4th,
            StringIds::position_5th,
            StringIds::position_6th,
            StringIds::position_7th,
            StringIds::position_8th,
            StringIds::position_9th,
            StringIds::position_10th,
            StringIds::position_11th,
            StringIds::position_12th,
            StringIds::position_13th,
            StringIds::position_14th,
            StringIds::position_15th,
        };

        int index = 0;
        auto highlightIndex = -1;
        for (auto company : _sortedCompanies)
        {
            auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitor_id);

            auto args = FormatArguments();
            args.push(positionArray[index]);
            args.push(Gfx::recolour(competitorObj->images[company->owner_emotion], company->mainColours.primary));
            args.push(company->name);
            args.push<uint16_t>(0); // Needed after a user string id
            formatPerformanceIndex(company->performance_index, args);

            Dropdown::add(index, StringIds::dropdown_company_performance, args);

            if (isPlayerCompany(company->id()))
            {
                highlightIndex = index;
            }

            index++;
        }

        Dropdown::add(index++, StringIds::dropdown_companies_list, ImageIds::company_list_dropdown_icon);
        Dropdown::showBelow(self, widgetIndex, index, 25, (1 << 6));
        if (highlightIndex != -1)
        {
            Dropdown::setHighlightedItem(highlightIndex);
        }
        _113DC78 = _113DC78 | (1 << 1);
    }

    // 0x43AB87
    static void playerDropdownClick(int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        // If its index is bigger than the list then its the company list extra item
        if (static_cast<uint16_t>(itemIndex) >= _sortedCompanies.size())
        {
            CompanyList::open();
        }
        else
        {
            auto company = _sortedCompanies[itemIndex];
            if (!company->empty())
            {
                CompanyWindow::open(company->id());
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
    Window* open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::playerInfoToolbar,
            Gfx::point_t(0, Ui::height() - window_size.height),
            Gfx::ui_size_t(window_size.width, window_size.height),
            Ui::WindowFlags::stick_to_front | Ui::WindowFlags::transparent | Ui::WindowFlags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << Widx::player) | (1 << Widx::company_value) | (1 << Widx::performance_index);
        window->var_854 = 0;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, Colour::translucent(skin->colour_16));
            window->setColour(WindowColour::secondary, Colour::translucent(skin->colour_16));
        }

        return window;
    }

    // 0x004393E7
    static void prepareDraw(Window* window)
    {
        window->widgets[Widx::inner_frame].type = WidgetType::none;
    }

    // 0x43944B
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        Widget& frame = _widgets[Widx::outer_frame];
        Gfx::drawRect(context, window->x + frame.left, window->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        window->draw(context);

        drawRectInset(context, window->x + frame.left + 1, window->y + frame.top + 1, frame.width() - 2, frame.height() - 2, window->getColour(WindowColour::secondary), 0x30);

        auto playerCompany = CompanyManager::get(CompanyManager::getControllingId());
        auto competitor = ObjectManager::get<CompetitorObject>(playerCompany->competitor_id);
        auto image = Gfx::recolour(competitor->images[playerCompany->owner_emotion], playerCompany->mainColours.primary);
        Gfx::drawImage(context, window->x + frame.left + 2, window->y + frame.top + 2, image);

        auto x = window->x + frame.width() / 2 + 12;
        {
            auto companyValueString = StringIds::player_info_bankrupt;
            if (!(playerCompany->challenge_flags & CompanyFlags::bankrupt))
            {
                if (static_cast<int16_t>(playerCompany->cash.var_04) < 0)
                {
                    companyValueString = StringIds::player_info_company_value_negative;
                }
                else
                {
                    companyValueString = StringIds::player_info_company_value;
                }
            }

            auto colour = Colour::opaque(window->getColour(WindowColour::primary));
            if (Input::isHovering(WindowType::playerInfoToolbar, 0, Widx::company_value))
            {
                colour = Colour::white;
            }
            auto args = FormatArguments();
            args.push(playerCompany->cash.var_00);
            args.push(playerCompany->cash.var_04);
            Gfx::drawStringCentred(*context, x, window->y + frame.top + 2, colour, companyValueString, &args);
        }

        {
            auto performanceString = StringIds::player_info_performance;

            if (playerCompany->challenge_flags & CompanyFlags::increasedPerformance)
            {
                performanceString = StringIds::player_info_performance_increase;
            }
            else if (playerCompany->challenge_flags & (CompanyFlags::decreasedPerformance))
            {
                performanceString = StringIds::player_info_performance_decrease;
            }

            auto colour = window->getColour(WindowColour::primary) & 0x7F;
            if (Input::isHovering(WindowType::playerInfoToolbar, 0, Widx::performance_index))
            {
                colour = Colour::white;
            }

            auto args = FormatArguments();
            args.push(playerCompany->performance_index);
            Gfx::drawStringCentred(*context, x, window->y + frame.top + 14, colour, performanceString, &args);
        }
    }

    // 0x004395A4
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::company_value:
                companyValueMouseUp();
                break;
            case Widx::performance_index:
                performanceIndexMouseUp();
                break;
        }
    }

    // 0x004395B1
    static void onMouseDown(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::player:
                playerMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x004395BC
    static void onDropdown(Window* w, WidgetIndex_t widgetIndex, int16_t item_index)
    {
        switch (widgetIndex)
        {
            case Widx::player:
                playerDropdownClick(item_index);
                break;
        }
    }

    // 0x4395C7
    static void companyValueMouseUp()
    {
        CompanyWindow::openFinances(CompanyManager::getControllingId());
    }

    // 0x4395D6
    static void performanceIndexMouseUp()
    {
        CompanyList::openPerformanceIndexes();
    }

    // 0x004395DE
    static Ui::CursorId onCursor(Ui::Window* window, int16_t widgetIndex, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        switch (widgetIndex)
        {
            case Widx::company_value:
            case Widx::performance_index:
                Input::setTooltipTimeout(2000);
                break;
        }
        return fallback;
    }

    // 0x004395F5
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        switch (widgetIndex)
        {
            case Widx::company_value:
                companyValueTooltip(args);
                break;

            case Widx::performance_index:
                performanceIndexTooltip(args);
                break;
        }
        return args;
    }

    static void companyValueTooltip(FormatArguments& args)
    {
        auto playerCompany = CompanyManager::get(CompanyManager::getControllingId());
        args.push(playerCompany->companyValueHistory[0]);
        args.push(playerCompany->vehicleProfit);
    }

    // 0x439643
    static void performanceIndexTooltip(FormatArguments& args)
    {
        auto playerCompany = CompanyManager::get(CompanyManager::getControllingId());

        formatPerformanceIndex(playerCompany->performance_index, args);
    }

    void invalidateFrame()
    {
        _50A004 = _50A004 | (1 << 0);
    }

    // 0x00439670
    static void onUpdate(Window* w)
    {
        w->var_854++;
        if (w->var_854 >= 24)
        {
            w->var_854 = 0;
        }

        if (_50A004 & (1 << 0))
        {
            _50A004 = _50A004 & ~(1 << 0);
            WindowManager::invalidateWidget(WindowType::playerInfoToolbar, 0, Widx::inner_frame);
        }
    }
}
