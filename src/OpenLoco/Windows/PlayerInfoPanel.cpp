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
    static const Ui::Size window_size = { 140, 27 };

    namespace Widx
    {
        enum
        {
            outer_frame,
            inner_frame,
            player,
            company_value,
            performanceIndex
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
        makeWidget({ 1, 1 }, { 26, 26 }, WidgetType::buttonWithImage, WindowColour::primary),
        makeWidget({ 27, 2 }, { 111, 12 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::null, StringIds::tooltip_company_value),
        makeWidget({ 27, 14 }, { 111, 12 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::null, StringIds::tooltip_performance_index),
        widgetEnd(),
    };

    static WindowEventList _events;
    std::vector<const Company*> _sortedCompanies;

    static loco_global<uint16_t, 0x0050A004> _50A004;
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
                return a->performanceIndex > b->performanceIndex;
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
            auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);

            auto args = FormatArguments();
            args.push(positionArray[index]);
            args.push(Gfx::recolour(competitorObj->images[company->ownerEmotion], company->mainColours.primary));
            args.push(company->name);
            args.push<uint16_t>(0); // Needed after a user string id
            formatPerformanceIndex(company->performanceIndex, args);

            Dropdown::add(index, StringIds::dropdown_company_performance, args);

            if (CompanyManager::isPlayerCompany(company->id()))
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
        _events.onMouseUp = onMouseUp;
        _events.event_03 = onMouseDown;
        _events.onMouseDown = onMouseDown;
        _events.onDropdown = onDropdown;
        _events.onUpdate = onUpdate;
        _events.tooltip = tooltip;
        _events.cursor = onCursor;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;
    }

    // 0x00438BC7
    Window* open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::playerInfoToolbar,
            Ui::Point(0, Ui::height() - window_size.height),
            Ui::Size(window_size.width, window_size.height),
            Ui::WindowFlags::stickToFront | Ui::WindowFlags::transparent | Ui::WindowFlags::noBackground,
            &_events);
        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::player) | (1 << Widx::company_value) | (1 << Widx::performanceIndex);
        window->var_854 = 0;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, AdvancedColour(skin->colour_16).translucent());
            window->setColour(WindowColour::secondary, AdvancedColour(skin->colour_16).translucent());
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
        Gfx::drawRect(*context, window->x + frame.left, window->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        window->draw(context);

        drawRectInset(*context, window->x + frame.left + 1, window->y + frame.top + 1, frame.width() - 2, frame.height() - 2, window->getColour(WindowColour::secondary).u8(), 0x30);

        auto playerCompany = CompanyManager::get(CompanyManager::getControllingId());
        auto competitor = ObjectManager::get<CompetitorObject>(playerCompany->competitorId);
        auto image = Gfx::recolour(competitor->images[playerCompany->ownerEmotion], playerCompany->mainColours.primary);
        Gfx::drawImage(context, window->x + frame.left + 2, window->y + frame.top + 2, image);

        auto x = window->x + frame.width() / 2 + 12;
        {
            auto companyValueString = StringIds::player_info_bankrupt;
            if (!(playerCompany->challengeFlags & CompanyFlags::bankrupt))
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

            auto colour = window->getColour(WindowColour::primary).opaque();
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

            if (playerCompany->challengeFlags & CompanyFlags::increasedPerformance)
            {
                performanceString = StringIds::player_info_performance_increase;
            }
            else if (playerCompany->challengeFlags & (CompanyFlags::decreasedPerformance))
            {
                performanceString = StringIds::player_info_performance_decrease;
            }

            auto colour = window->getColour(WindowColour::primary).opaque();
            if (Input::isHovering(WindowType::playerInfoToolbar, 0, Widx::performanceIndex))
            {
                colour = Colour::white;
            }

            auto args = FormatArguments();
            args.push(playerCompany->performanceIndex);
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
            case Widx::performanceIndex:
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
            case Widx::performanceIndex:
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

            case Widx::performanceIndex:
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

        formatPerformanceIndex(playerCompany->performanceIndex, args);
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
