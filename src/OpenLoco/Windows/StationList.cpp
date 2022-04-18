#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CargoObject.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::StationList
{
    static const Ui::Size window_size = { 600, 197 };
    static const Ui::Size max_dimensions = { 640, 1200 };
    static const Ui::Size min_dimensions = { 192, 100 };

    static const uint8_t rowHeight = 10; // CJK: 13

    enum widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_all_stations,
        tab_rail_stations,
        tab_road_stations,
        tab_airports,
        tab_ship_ports,
        company_select,
        sort_name,
        sort_status,
        sort_total_waiting,
        sort_accepts,
        scrollview,
    };

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 600, 197 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 598, 13 }, WidgetType::caption_24, WindowColour::primary, StringIds::stringid_all_stations),
        makeWidget({ 585, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 41 }, { 600, 155 }, WidgetType::panel, WindowColour::secondary),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_all_stations),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_rail_stations),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_road_stations),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_airports),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_ship_ports),
        makeWidget({ 0, 14 }, { 26, 26 }, WidgetType::buttonWithImage, WindowColour::primary, StringIds::null, StringIds::tooltip_select_company),
        makeWidget({ 4, 43 }, { 200, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_name),
        makeWidget({ 204, 43 }, { 200, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_station_status),
        makeWidget({ 404, 43 }, { 90, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_total_units_waiting),
        makeWidget({ 494, 43 }, { 120, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_cargo_accepted),
        makeWidget({ 3, 56 }, { 594, 126 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        widgetEnd(),
    };

    static WindowEventList _events;

    struct TabDetails
    {
        widx widgetIndex;
        string_id windowTitleId;
        uint32_t imageId;
        uint16_t stationMask;
    };

    static TabDetails tabInformationByType[] = {
        { tab_all_stations, StringIds::stringid_all_stations, InterfaceSkin::ImageIds::all_stations, StationFlags::allModes },
        { tab_rail_stations, StringIds::stringid_rail_stations, InterfaceSkin::ImageIds::rail_stations, StationFlags::transportModeRail },
        { tab_road_stations, StringIds::stringid_road_stations, InterfaceSkin::ImageIds::road_stations, StationFlags::transportModeRoad },
        { tab_airports, StringIds::stringid_airports, InterfaceSkin::ImageIds::airports, StationFlags::transportModeAir },
        { tab_ship_ports, StringIds::stringid_ship_ports, InterfaceSkin::ImageIds::ship_ports, StationFlags::transportModeWater }
    };

    enum SortMode : uint16_t
    {
        Name,
        Status,
        TotalUnitsWaiting,
        CargoAccepted,
    };

    loco_global<uint16_t[4], 0x112C826> _common_format_args;

    static Ui::CursorId cursor(Window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback);
    static void draw(Ui::Window* window, Gfx::Context* context);
    static void drawScroll(Ui::Window& window, Gfx::Context& context, const uint32_t scrollIndex);
    static void event_08(Window* window);
    static void event_09(Window* window);
    static void getScrollSize(Ui::Window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    static void onMouseDown(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onScrollMouseDown(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseOver(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void onUpdate(Window* window);
    static void prepareDraw(Ui::Window* window);
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex);

    static void initEvents()
    {
        _events.cursor = cursor;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
        _events.event_08 = event_08;
        _events.event_09 = event_09;
        _events.getScrollSize = getScrollSize;
        _events.onDropdown = onDropdown;
        _events.onMouseDown = onMouseDown;
        _events.onMouseUp = onMouseUp;
        _events.onUpdate = onUpdate;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.prepareDraw = prepareDraw;
        _events.tooltip = tooltip;
    }

    // 0x004910E8
    static void refreshStationList(Window* window)
    {
        window->rowCount = 0;

        for (auto& station : StationManager::stations())
        {
            if (station.owner == CompanyId(window->number))
            {
                station.flags &= ~StationFlags::flag_4;
            }
        }
    }

    // 0x004911FD
    static bool orderByName(const OpenLoco::Station& lhs, const OpenLoco::Station& rhs)
    {
        char lhsString[256] = { 0 };
        StringManager::formatString(lhsString, lhs.name, (void*)&lhs.town);

        char rhsString[256] = { 0 };
        StringManager::formatString(rhsString, rhs.name, (void*)&rhs.town);

        return strcmp(lhsString, rhsString) < 0;
    }

    // 0x00491281, 0x00491247
    static bool orderByQuantity(const OpenLoco::Station& lhs, const OpenLoco::Station& rhs)
    {
        uint32_t lhsSum = 0;
        for (const auto& cargo : lhs.cargoStats)
        {
            lhsSum += cargo.quantity;
        }

        uint32_t rhsSum = 0;
        for (const auto& cargo : rhs.cargoStats)
        {
            rhsSum += cargo.quantity;
        }

        return rhsSum < lhsSum;
    }

    // 0x004912BB
    static bool orderByAccepts(const OpenLoco::Station& lhs, const OpenLoco::Station& rhs)
    {
        char* ptr;

        char lhsString[256] = { 0 };
        ptr = &lhsString[0];
        for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            if (lhs.cargoStats[cargoId].isAccepted())
            {
                ptr = StringManager::formatString(ptr, ObjectManager::get<CargoObject>(cargoId)->name);
            }
        }

        char rhsString[256] = { 0 };
        ptr = &rhsString[0];
        for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
        {
            if (rhs.cargoStats[cargoId].isAccepted())
            {
                ptr = StringManager::formatString(ptr, ObjectManager::get<CargoObject>(cargoId)->name);
            }
        }

        return strcmp(lhsString, rhsString) < 0;
    }

    // 0x004911FD, 0x00491247, 0x00491281, 0x004912BB
    static bool getOrder(const SortMode mode, const OpenLoco::Station& lhs, const OpenLoco::Station& rhs)
    {
        switch (mode)
        {
            case SortMode::Name:
                return orderByName(lhs, rhs);

            case SortMode::Status:
            case SortMode::TotalUnitsWaiting:
                return orderByQuantity(lhs, rhs);

            case SortMode::CargoAccepted:
                return orderByAccepts(lhs, rhs);
        }

        return false;
    }

    // 0x0049111A
    static void updateStationList(Window* window)
    {
        StationId edi = StationId::null;

        for (auto& station : StationManager::stations())
        {
            if (station.owner != CompanyId(window->number))
                continue;

            if ((station.flags & StationFlags::flag_5) != 0)
                continue;

            const uint16_t mask = tabInformationByType[window->currentTab].stationMask;
            if ((station.flags & mask) == 0)
                continue;

            if ((station.flags & StationFlags::flag_4) != 0)
                continue;

            if (edi == StationId::null)
            {
                edi = station.id();
                continue;
            }

            if (getOrder(SortMode(window->sortMode), station, *StationManager::get(edi)))
            {
                edi = station.id();
            }
        }

        if (edi != StationId::null)
        {
            bool dl = false;

            StationManager::get(edi)->flags |= StationFlags::flag_4;

            auto ebp = window->rowCount;
            if (edi != StationId(window->rowInfo[ebp]))
            {
                window->rowInfo[ebp] = enumValue(edi);
                dl = true;
            }

            window->rowCount += 1;
            if (window->rowCount > window->var_83C)
            {
                window->var_83C = window->rowCount;
                dl = true;
            }

            if (dl)
            {
                window->invalidate();
            }
        }
        else
        {
            if (window->var_83C != window->rowCount)
            {
                window->var_83C = window->rowCount;
                window->invalidate();
            }

            refreshStationList(window);
        }
    }

    // 0x00490F6C
    Window* open(CompanyId companyId)
    {
        Window* window = WindowManager::bringToFront(WindowType::stationList, enumValue(companyId));
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
                Input::toolCancel();

            // Still active?
            window = WindowManager::bringToFront(WindowType::stationList, enumValue(companyId));
        }

        if (window == nullptr)
        {
            // 0x00491010
            window = WindowManager::createWindow(
                WindowType::stationList,
                window_size,
                WindowFlags::flag_11,
                &_events);

            window->number = enumValue(companyId);
            window->owner = companyId;
            window->currentTab = 0;
            window->frame_no = 0;
            window->sortMode = 0;
            window->var_83C = 0;
            window->rowHover = -1;

            refreshStationList(window);

            window->minWidth = min_dimensions.width;
            window->minHeight = min_dimensions.height;
            window->maxWidth = max_dimensions.width;
            window->maxHeight = max_dimensions.height;
            window->flags |= WindowFlags::resizable;

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, interface->colour_0A);
        }

        // TODO: only needs to be called once.
        initEvents();

        window->currentTab = 0;
        window->invalidate();

        window->widgets = _widgets;
        window->enabledWidgets = (1 << close_button) | (1 << tab_all_stations) | (1 << tab_rail_stations) | (1 << tab_road_stations) | (1 << tab_airports) | (1 << tab_ship_ports) | (1 << company_select) | (1 << sort_name) | (1 << sort_status) | (1 << sort_total_waiting) | (1 << sort_accepts) | (1 << scrollview);

        window->activatedWidgets = 0;
        window->holdableWidgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }

    Window* open(CompanyId companyId, uint8_t type)
    {
        if (type > 4)
            throw std::domain_error("Unexpected station type");

        Window* station_list = open(companyId);
        widx target = tabInformationByType[type].widgetIndex;
        station_list->callOnMouseUp(target);

        return station_list;
    }

    // 0x004919A4
    static Ui::CursorId cursor(Window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (widgetIdx != widx::scrollview)
            return fallback;

        uint16_t currentIndex = yPos / rowHeight;
        if (currentIndex < window->var_83C && window->rowInfo[currentIndex] != -1)
            return CursorId::handPointer;

        return fallback;
    }

    // 0x0049196F
    static void event_08(Window* window)
    {
        window->flags |= WindowFlags::notScrollView;
    }

    // 0x00491977
    static void event_09(Window* window)
    {
        if ((window->flags & WindowFlags::notScrollView) == 0)
            return;

        if (window->rowHover == -1)
            return;

        window->rowHover = -1;
        window->invalidate();
    }

    // 0x00491344
    static void prepareDraw(Ui::Window* window)
    {
        // Reset active tab.
        window->activatedWidgets &= ~((1 << tab_all_stations) | (1 << tab_rail_stations) | (1 << tab_road_stations) | (1 << tab_airports) | (1 << tab_ship_ports));
        window->activatedWidgets |= (1ULL << tabInformationByType[window->currentTab].widgetIndex);

        // Set company name.
        auto company = CompanyManager::get(CompanyId(window->number));
        *_common_format_args = company->name;

        // Set window title.
        window->widgets[widx::caption].text = tabInformationByType[window->currentTab].windowTitleId;

        // Resize general window widgets.
        window->widgets[widx::frame].right = window->width - 1;
        window->widgets[widx::frame].bottom = window->height - 1;

        window->widgets[widx::panel].right = window->width - 1;
        window->widgets[widx::panel].bottom = window->height - 1;

        window->widgets[widx::caption].right = window->width - 2;

        window->widgets[widx::close_button].left = window->width - 15;
        window->widgets[widx::close_button].right = window->width - 3;

        window->widgets[widx::scrollview].right = window->width - 4;
        window->widgets[widx::scrollview].bottom = window->height - 14;

        // Reposition header buttons.
        window->widgets[widx::sort_name].right = std::min(203, window->width - 4);

        window->widgets[widx::sort_status].left = std::min(204, window->width - 4);
        window->widgets[widx::sort_status].right = std::min(403, window->width - 4);

        window->widgets[widx::sort_total_waiting].left = std::min(404, window->width - 4);
        window->widgets[widx::sort_total_waiting].right = std::min(493, window->width - 4);

        window->widgets[widx::sort_accepts].left = std::min(494, window->width - 4);
        window->widgets[widx::sort_accepts].right = std::min(613, window->width - 4);

        // Reposition company selection.
        window->widgets[widx::company_select].left = window->width - 28;
        window->widgets[widx::company_select].right = window->width - 3;

        // Set header button captions.
        window->widgets[widx::sort_name].text = window->sortMode == SortMode::Name ? StringIds::table_header_name_desc : StringIds::table_header_name;
        window->widgets[widx::sort_status].text = window->sortMode == SortMode::Status ? StringIds::table_header_status_desc : StringIds::table_header_status;
        window->widgets[widx::sort_total_waiting].text = window->sortMode == SortMode::TotalUnitsWaiting ? StringIds::table_header_total_waiting_desc : StringIds::table_header_total_waiting;
        window->widgets[widx::sort_accepts].text = window->sortMode == SortMode::CargoAccepted ? StringIds::table_header_accepts_desc : StringIds::table_header_accepts;

        // Reposition tabs (0x00491A39 / 0x00491A3F)
        int16_t new_tab_x = window->widgets[widx::tab_all_stations].left;
        int16_t tab_width = window->widgets[widx::tab_all_stations].right - new_tab_x;

        for (auto& tabInfo : tabInformationByType)
        {
            if (window->isDisabled(tabInfo.widgetIndex))
                continue;

            Widget& tab = window->widgets[tabInfo.widgetIndex];

            tab.left = new_tab_x;
            new_tab_x += tab_width;
            tab.right = new_tab_x++;
        }
    }

    // 0x0049157F
    static void drawScroll(Ui::Window& window, Gfx::Context& context, const uint32_t scrollIndex)
    {
        auto shade = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
        Gfx::clearSingle(context, shade);

        uint16_t yPos = 0;
        for (uint16_t i = 0; i < window.var_83C; i++)
        {
            auto stationId = StationId(window.rowInfo[i]);

            // Skip items outside of view, or irrelevant to the current filter.
            if (yPos + rowHeight < context.y || yPos >= yPos + rowHeight + context.height || stationId == StationId::null)
            {
                yPos += rowHeight;
                continue;
            }

            string_id text_colour_id = StringIds::black_stringid;

            // Highlight selection.
            if (stationId == StationId(window.rowHover))
            {
                Gfx::drawRect(context, 0, yPos, window.width, rowHeight, 0x2000030);
                text_colour_id = StringIds::wcolour2_stringid;
            }

            auto station = StationManager::get(stationId);

            // First, draw the town name.
            _common_format_args[0] = StringIds::stringid_stringid;
            _common_format_args[1] = station->name;
            _common_format_args[2] = enumValue(station->town);
            _common_format_args[3] = getTransportIconsFromStationFlags(station->flags);

            Gfx::drawString_494BBF(context, 0, yPos, 198, Colour::black, text_colour_id, &*_common_format_args);

            // Then the station's current status.
            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            station->getStatusString(buffer);

            _common_format_args[0] = StringIds::buffer_1250;
            Gfx::drawString_494BBF(context, 200, yPos, 198, Colour::black, text_colour_id, &*_common_format_args);

            // Total units waiting.
            uint16_t totalUnits = 0;
            for (const auto& stats : station->cargoStats)
                totalUnits += stats.quantity;

            _common_format_args[0] = StringIds::num_units;
            *(uint32_t*)&_common_format_args[1] = totalUnits;
            Gfx::drawString_494BBF(context, 400, yPos, 88, Colour::black, text_colour_id, &*_common_format_args);

            // And, finally, what goods the station accepts.
            char* ptr = buffer;
            *ptr = '\0';

            for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
            {
                auto& stats = station->cargoStats[cargoId];

                if (!stats.isAccepted())
                    continue;

                if (*buffer != '\0')
                    ptr = StringManager::formatString(ptr, StringIds::unit_separator);

                ptr = StringManager::formatString(ptr, ObjectManager::get<CargoObject>(cargoId)->name);
            }

            _common_format_args[0] = StringIds::buffer_1250;
            Gfx::drawString_494BBF(context, 490, yPos, 118, Colour::black, text_colour_id, &*_common_format_args);

            yPos += rowHeight;
        }
    }

    // 00491A76
    static void drawTabs(Ui::Window* window, Gfx::Context* context)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(CompanyId(window->number));

        for (const auto& tab : tabInformationByType)
        {
            uint32_t image = Gfx::recolour(skin->img + tab.imageId, companyColour);
            Widget::drawTab(window, context, image, tab.widgetIndex);
        }
    }

    // 0x004914D8
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        // Draw widgets and tabs.
        window->draw(context);
        drawTabs(window, context);

        // Draw company owner image.
        auto company = CompanyManager::get(CompanyId(window->number));
        auto competitor = ObjectManager::get<CompetitorObject>(company->competitorId);
        uint32_t image = Gfx::recolour(competitor->images[company->ownerEmotion], company->mainColours.primary);
        uint16_t x = window->x + window->widgets[widx::company_select].left + 1;
        uint16_t y = window->y + window->widgets[widx::company_select].top + 1;
        Gfx::drawImage(context, x, y, image);

        // TODO: locale-based pluralisation.
        _common_format_args[0] = window->var_83C == 1 ? StringIds::status_num_stations_singular : StringIds::status_num_stations_plural;
        _common_format_args[1] = window->var_83C;

        // Draw number of stations.
        auto origin = Ui::Point(window->x + 4, window->y + window->height - 12);
        Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::black_stringid, &*_common_format_args);
    }

    // 0x004917BB
    static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::company_select)
            return;

        if (itemIndex == -1)
            return;

        CompanyId companyId = Dropdown::getCompanyIdFromSelection(itemIndex);

        // Try to find an open station list for this company.
        auto companyWindow = WindowManager::bringToFront(WindowType::stationList, enumValue(companyId));
        if (companyWindow != nullptr)
            return;

        // If not, we'll turn this window into a window for the company selected.
        auto company = CompanyManager::get(companyId);
        if (company->name == StringIds::empty)
            return;

        window->number = enumValue(companyId);
        window->owner = companyId;
        window->sortMode = 0;
        window->rowCount = 0;

        refreshStationList(window);

        window->var_83C = 0;
        window->rowHover = -1;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();
        window->invalidate();
    }

    // 0x004917B0
    static void onMouseDown(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        if (widgetIndex == widx::company_select)
            Dropdown::populateCompanySelect(window, &window->widgets[widgetIndex]);
    }

    // 0x00491785
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(window);
                break;

            case tab_all_stations:
            case tab_rail_stations:
            case tab_road_stations:
            case tab_airports:
            case tab_ship_ports:
            {
                if (Input::isToolActive(window->type, window->number))
                    Input::toolCancel();

                window->currentTab = widgetIndex - widx::tab_all_stations;
                window->frame_no = 0;

                window->invalidate();

                window->var_83C = 0;
                window->rowHover = -1;

                refreshStationList(window);

                window->callOnResize();
                window->callPrepareDraw();
                window->initScrollWidgets();
                window->moveInsideScreenEdges();
                break;
            }

            case sort_name:
            case sort_status:
            case sort_total_waiting:
            case sort_accepts:
            {
                auto sort_mode = widgetIndex - widx::sort_name;
                if (window->sortMode == sort_mode)
                    return;

                window->sortMode = sort_mode;
                window->invalidate();
                window->var_83C = 0;
                window->rowHover = -1;

                refreshStationList(window);
                break;
            }
        }
    }

    // 0x00491A0C
    static void onScrollMouseDown(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentRow = y / rowHeight;
        if (currentRow > window->var_83C)
            return;

        const auto currentStation = StationId(window->rowInfo[currentRow]);
        if (currentStation == StationId::null)
            return;

        Station::open(currentStation);
    }

    // 0x004919D1
    static void onScrollMouseOver(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        window->flags &= ~(WindowFlags::notScrollView);

        uint16_t currentRow = y / rowHeight;
        int16_t currentStation = -1;

        if (currentRow < window->var_83C)
            currentStation = window->rowInfo[currentRow];

        if (currentStation == window->rowHover)
            return;

        window->rowHover = currentStation;
        window->invalidate();
    }

    // 0x0049193F
    static void onUpdate(Window* window)
    {
        window->frame_no++;

        window->callPrepareDraw();
        WindowManager::invalidateWidget(WindowType::stationList, window->number, window->currentTab + 4);

        // Add three stations every tick.
        updateStationList(window);
        updateStationList(window);
        updateStationList(window);
    }

    // 0x00491999
    static void getScrollSize(Ui::Window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = rowHeight * window->var_83C;
    }

    // 0x00491841
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_station_list);
        return args;
    }
}
