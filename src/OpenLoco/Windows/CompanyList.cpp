#include "../Company.h"
#include "../CompanyManager.h"
#include "../Date.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/CargoObject.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Numeric.hpp"
#include "../Widget.h"

using namespace OpenLoco::interop;

namespace OpenLoco::ui::windows::CompanyList
{
    static loco_global<uint8_t[32], 0x004F9442> _cargoLineColour;
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t[3], 0x0052624E> _word_52624E;
    static loco_global<company_id_t[3], 0x00526254> _byte_526254;
    static loco_global<uint32_t[3], 0x00526258> _dword_526258;
    static loco_global<uint16_t[32][120], 0x009C68F8> _deliveredCargoPayment;
    static loco_global<uint16_t, 0x009C68C7> _word_9C68C7;
    static loco_global<uint16_t, 0x0113DC7A> _graphLeft;
    static loco_global<uint16_t, 0x0113DC7C> _graphTop;
    static loco_global<uint16_t, 0x0113DC7E> _graphRight;
    static loco_global<uint16_t, 0x0113DC80> _graphBottom;
    static loco_global<uint16_t, 0x0113DC82> _graphYOffset;
    static loco_global<uint16_t, 0x0113DC84> _graphXOffset;
    static loco_global<uint32_t, 0x0113DC86> _graphYAxisLabelIncrement;
    static loco_global<uint16_t, 0x0113DC8A> _graphLineCount;
    static loco_global<uint32_t[32], 0x0113DC8C> _graphYData;
    static loco_global<uint32_t, 0x0113DD0C> _dword_113DD0C; //graphType?
    static loco_global<uint16_t[32], 0x0113DD10> _graphDataStart;
    static loco_global<uint32_t, 0x0113DD50> _dword_113DD50;
    static loco_global<uint8_t[32], 0x0113DD54> _graphLineColour;
    static loco_global<uint16_t, 0x0113DD74> _graphDataEnd;
    static loco_global<uint16_t, 0x0113DD76> _graphXLabel;
    static loco_global<uint32_t, 0x0113DD78> _graphXAxisRange;
    static loco_global<uint32_t, 0x0113DD7C> _dword_113DD7C;
    static loco_global<uint16_t, 0x0113DD80> _word_113DD80; //graphXAxisIncrement?
    static loco_global<uint16_t, 0x0113DD82> _graphXAxisLabelIncrement;
    static loco_global<uint16_t, 0x0113DD84> _graphYLabel;
    static loco_global<uint32_t, 0x0113DD86> _dword_113DD86;
    static loco_global<uint32_t, 0x0113DD8A> _dword_113DD8A;
    static loco_global<uint32_t, 0x0113DD8E> _dword_113DD8E;
    static loco_global<uint8_t, 0x0113DD99> _byte_113DD99;
    static loco_global<uint16_t[32], 0x0113DD9A> _graphItemId;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_company_list,
            tab_performance,
            tab_cargo_units,
            tab_cargo_distance,
            tab_values,
            tab_payment_rates,
            tab_speed_records,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_company_list) | (1 << widx::tab_performance) | (1 << widx::tab_cargo_units) | (1 << widx::tab_cargo_distance) | (1 << widx::tab_values) | (1 << widx::tab_payment_rates) | (1 << widx::tab_speed_records);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                         \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                           \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                      \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, string_ids::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 231 }, widget_type::panel, 1),                                                              \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_compare_companies),                 \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_company_performance),              \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_cargo_graphs),                     \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_cargo_distance_graphs),            \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_company_values),                  \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_cargo_payment_rates),             \
        makeRemapWidget({ 189, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, string_ids::tab_speed_records)

        static void onMouseUp(window* self, widget_index widgetIndex);
        static void onUpdate(window* self);
        static void prepareDraw(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void refreshCompanyList(window* self);
        static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi);
        static void drawGraph(window* self, Gfx::drawpixelinfo_t* dpi);
        static void drawGraphAndKey(window* self, Gfx::drawpixelinfo_t* dpi);
        static void initEvents();
    }

    namespace CompanyList
    {
        static const Gfx::ui_size_t maxWindowSize = { 640, 470 };
        static const Gfx::ui_size_t minWindowSize = { 300, 272 };
        static const Gfx::ui_size_t windowSize = { 640, 272 };

        static const uint8_t rowHeight = 25;

        enum widx
        {
            sort_name = 11,
            sort_status,
            sort_performance,
            sort_value,
            scrollview,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << sort_name) | (1 << sort_status) | (1 << sort_performance) | (1 << sort_value) | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(640, 272, string_ids::title_company_list),
            makeWidget({ 4, 43 }, { 175, 12 }, widget_type::wt_14, 1, ImageIds::null, string_ids::tooltip_sort_company_name),
            makeWidget({ 179, 43 }, { 210, 12 }, widget_type::wt_14, 1, ImageIds::null, string_ids::tooltip_sort_company_status),
            makeWidget({ 389, 43 }, { 145, 12 }, widget_type::wt_14, 1, ImageIds::null, string_ids::tooltip_sort_company_performance),
            makeWidget({ 534, 43 }, { 100, 12 }, widget_type::wt_14, 1, ImageIds::null, string_ids::tooltip_sort_company_value),
            makeWidget({ 3, 56 }, { 634, 201 }, widget_type::scrollview, 1, vertical),
            widgetEnd(),
        };

        static window_event_list events;

        enum SortMode : uint16_t
        {
            Name,
            Status,
            Performance,
            Value,
        };

        // 0x004360A2
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_company_list:
                case common::widx::tab_performance:
                case common::widx::tab_cargo_units:
                case common::widx::tab_cargo_distance:
                case common::widx::tab_values:
                case common::widx::tab_payment_rates:
                case common::widx::tab_speed_records:
                    common::switchTab(self, widgetIndex);
                    break;

                case sort_name:
                case sort_status:
                case sort_performance:
                case sort_value:
                {
                    auto sortMode = widgetIndex - widx::sort_name;
                    if (self->sort_mode == sortMode)
                        return;

                    self->sort_mode = sortMode;
                    self->invalidate();
                    self->var_83C = 0;
                    self->row_hover = -1;

                    common::refreshCompanyList(self);
                    break;
                }
            }
        }

        // 0x004363CB
        static void onResize(window* self)
        {
            self->setSize(minWindowSize, maxWindowSize);
        }

        // 0x00437BA0
        static bool orderByName(const OpenLoco::company& lhs, const OpenLoco::company& rhs)
        {
            char lhsString[256] = { 0 };
            stringmgr::formatString(lhsString, lhs.name);

            char rhsString[256] = { 0 };
            stringmgr::formatString(rhsString, rhs.name);

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00437BE1
        static bool orderByStatus(const OpenLoco::company& lhs, const OpenLoco::company& rhs)
        {
            char lhsString[256] = { 0 };
            {
                auto args = FormatArguments();
                auto statusString = companymgr::getOwnerStatus(lhs.id(), args);
                stringmgr::formatString(lhsString, statusString, &args);
            }

            char rhsString[256] = { 0 };
            {
                auto args = FormatArguments();
                auto statusString = companymgr::getOwnerStatus(lhs.id(), args);
                stringmgr::formatString(rhsString, statusString, &args);
            }

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00437C53
        static bool orderByPerformance(const OpenLoco::company& lhs, const OpenLoco::company& rhs)
        {
            auto lhsPerformance = lhs.performance_index;

            auto rhsPerformance = rhs.performance_index;

            return rhsPerformance < lhsPerformance;
        }

        // 0x00437C67
        static bool orderByValue(const OpenLoco::company& lhs, const OpenLoco::company& rhs)
        {
            auto lhsValue = lhs.companyValueHistory[0].var_04;

            auto rhsValue = rhs.companyValueHistory[0].var_04;

            if (lhsValue == rhsValue)
            {
                lhsValue = lhs.companyValueHistory[0].var_00;

                rhsValue = rhs.companyValueHistory[0].var_00;
            }

            return rhsValue < lhsValue;
        }

        // 0x00437BA0, 0x00437BE1, 0x00437C53, 0x00437C67
        static bool getOrder(const SortMode mode, OpenLoco::company& lhs, OpenLoco::company& rhs)
        {
            switch (mode)
            {
                case SortMode::Name:
                    return orderByName(lhs, rhs);

                case SortMode::Status:
                    return orderByStatus(lhs, rhs);

                case SortMode::Performance:
                    return orderByPerformance(lhs, rhs);

                case SortMode::Value:
                    return orderByValue(lhs, rhs);
            }

            return false;
        }

        // 0x00437AE2
        static void updateCompanyList(window* self)
        {
            auto chosenCompany = -1;

            auto i = -1;

            for (auto& company : companymgr::companies())
            {
                i++;
                if (company.empty())
                    continue;

                if ((company.challenge_flags & company_flags::sorted) != 0)
                    continue;

                if (chosenCompany == -1)
                {
                    chosenCompany = i;
                    continue;
                }

                if (getOrder(SortMode(self->sort_mode), company, *companymgr::get(chosenCompany)))
                {
                    chosenCompany = i;
                }
            }

            if (chosenCompany != -1)
            {
                bool shouldInvalidate = false;

                companymgr::get(chosenCompany)->challenge_flags |= company_flags::sorted;

                if (chosenCompany != self->row_info[self->row_count])
                {
                    self->row_info[self->row_count] = chosenCompany;
                    shouldInvalidate = true;
                }

                self->row_count++;
                if (self->row_count > self->var_83C)
                {
                    self->var_83C = self->row_count;
                    shouldInvalidate = true;
                }

                if (shouldInvalidate)
                {
                    self->invalidate();
                }
            }
            else
            {
                if (self->var_83C != self->row_count)
                {
                    self->var_83C = self->row_count;
                    self->invalidate();
                }

                common::refreshCompanyList(self);
            }
        }

        // 0x004362C0
        static void onUpdate(window* self)
        {
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::companyList, self->number, self->current_tab + common::widx::tab_company_list);

            _word_9C68C7++;

            // Add three companies every tick.
            updateCompanyList(self);
            updateCompanyList(self);
            updateCompanyList(self);
        }

        // 0x004362F7
        static void event_08(window* self)
        {
            self->flags |= window_flags::not_scroll_view;
        }

        // 0x004362FF
        static void event_09(window* self)
        {
            if (!(self->flags & window_flags::not_scroll_view))
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x00436321
        static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = self->var_83C * rowHeight;
        }

        // 0x004363A0
        static void onScrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / rowHeight;
            if (currentRow > self->var_83C)
                return;

            int16_t currentCompany = self->row_info[currentRow];
            if (currentCompany == -1)
                return;

            windows::CompanyWindow::open(currentCompany);
        }

        // 0x00436361
        static void onScrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(window_flags::not_scroll_view);

            uint16_t currentRow = y / rowHeight;
            int16_t currentCompany = -1;

            if (currentRow < self->var_83C)
                currentCompany = self->row_info[currentRow];

            if (self->row_hover == currentCompany)
                return;

            self->row_hover = currentCompany;
            self->invalidate();
        }

        // 0x004362B6
        static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_company_list);
        }

        // 0x0043632C
        static ui::cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
                return cursor_id::hand_pointer;

            return fallback;
        }

        // 0x00435D07
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            // Reposition header buttons
            self->widgets[widx::sort_name].right = std::min(178, self->width - 8);

            self->widgets[widx::sort_status].left = std::min(179, self->width - 8);
            self->widgets[widx::sort_status].right = std::min(388, self->width - 8);

            self->widgets[widx::sort_performance].left = std::min(389, self->width - 8);
            self->widgets[widx::sort_performance].right = std::min(533, self->width - 8);

            self->widgets[widx::sort_value].left = std::min(534, self->width - 8);
            self->widgets[widx::sort_value].right = std::min(633, self->width - 8);

            // Set header button captions
            self->widgets[widx::sort_name].text = self->sort_mode == SortMode::Name ? string_ids::table_header_company_name_desc : string_ids::table_header_company_name;
            self->widgets[widx::sort_status].text = self->sort_mode == SortMode::Status ? string_ids::table_header_company_status_desc : string_ids::table_header_company_status;
            self->widgets[widx::sort_performance].text = self->sort_mode == SortMode::Performance ? string_ids::table_header_company_performance_desc : string_ids::table_header_company_performance;
            self->widgets[widx::sort_value].text = self->sort_mode == SortMode::Value ? string_ids::table_header_company_value_desc : string_ids::table_header_company_value;
        }

        // 0x00435E56
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto args = FormatArguments();
            if (self->var_83C == 1)
                args.push(string_ids::company_singular);
            else
                args.push(string_ids::companies_plural);

            args.push(self->var_83C);

            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            Gfx::drawString_494B3F(*dpi, xPos, yPos, Colour::black, string_ids::black_stringid, &args);
        }

        // 0x00435EA7
        static void drawScroll(window* self, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto colour = Colour::getShade(self->colours[1], 3);
            Gfx::clearSingle(*dpi, colour);

            auto yBottom = 0;
            for (auto i = 0; i < self->var_83C; i++, yBottom += 25)
            {
                auto yTop = yBottom + 25;

                if (yTop <= dpi->y)
                    continue;

                yTop = dpi->y + dpi->height;

                if (yBottom >= yTop)
                    continue;

                auto rowItem = self->row_info[i];

                if (rowItem == -1)
                    continue;

                auto stringId = string_ids::black_stringid;

                if (rowItem == self->row_hover)
                {
                    Gfx::drawRect(dpi, 0, yBottom, self->width, 24, (1 << 25) | PaletteIndex::index_30);

                    stringId = string_ids::wcolour2_stringid;
                }

                auto company = companymgr::get(rowItem);
                auto competitorObj = objectmgr::get<competitor_object>(company->competitor_id);
                auto imageId = Gfx::recolour(competitorObj->images[company->owner_emotion], company->mainColours.primary);

                {
                    auto args = FormatArguments();
                    args.push(string_ids::table_item_company);
                    args.push(imageId);
                    args.push(company->name);

                    Gfx::drawString_494BBF(*dpi, 0, yBottom - 1, 173, Colour::black, stringId, &args);
                }

                {

                    auto args = FormatArguments();
                    companymgr::owner_status ownerStatus = companymgr::getOwnerStatus(company->id());
                    args.push(ownerStatus.string);
                    args.push(ownerStatus.argument1);
                    args.push(ownerStatus.argument2);

                    Gfx::drawString_494BBF(*dpi, 175, yBottom + 7, 208, Colour::black, stringId, &args);
                }

                auto performanceStringId = string_ids::performance_index;

                if ((company->challenge_flags & company_flags::increased_performance) && (company->challenge_flags & company_flags::decreased_performance))
                {
                    performanceStringId = string_ids::performance_index_decrease;

                    if (company->challenge_flags & company_flags::increased_performance)
                    {
                        performanceStringId = string_ids::performance_index_increase;
                    }
                }

                {
                    auto args = FormatArguments();

                    args.push(performanceStringId);
                    formatPerformanceIndex(company->performance_index, args);

                    Gfx::drawString_494BBF(*dpi, 385, yBottom - 1, 143, Colour::black, stringId, &args);
                }

                {
                    auto args = FormatArguments();

                    args.push(string_ids::company_value_currency);
                    args.push(company->companyValueHistory[0]);

                    Gfx::drawString_494BBF(*dpi, 530, yBottom - 1, 98, Colour::black, stringId, &args);
                }
            }
        }

        // 0x00436198
        static void tabReset(window* self)
        {
            self->min_width = minWindowSize.width;
            self->min_height = minWindowSize.height;
            self->max_width = maxWindowSize.width;
            self->max_height = maxWindowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            self->var_83C = 0;
            self->row_hover = -1;
            common::refreshCompanyList(self);
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = onUpdate;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = getScrollSize;
            events.scroll_mouse_down = onScrollMouseDown;
            events.scroll_mouse_over = onScrollMouseOver;
            events.tooltip = tooltip;
            events.cursor = cursor;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.draw_scroll = drawScroll;
        }
    }

    // 0x00435BC8
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::companyList);

        if (window != nullptr)
        {
            if (input::isToolActive(_toolWindowType, _toolWindowNumber))
            {
                input::toolCancel();
                window = WindowManager::bringToFront(WindowType::companyList);
            }
        }

        if (window == nullptr)
        {
            Gfx::ui_size_t windowSize = { 640, 272 };

            window = WindowManager::createWindow(WindowType::companyList, windowSize, 0, &CompanyList::events);

            window->frame_no = 0;
            window->saved_view.clear();
            window->flags |= window_flags::resizable;
            window->sort_mode = 2;
            window->var_83C = 0;
            window->row_hover = -1;

            common::refreshCompanyList(window);

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[0] = skin->colour_0B;
            window->colours[1] = skin->colour_0C;

            window->var_854 = 0;
        }

        window->current_tab = 0;
        window->min_width = CompanyList::minWindowSize.width;
        window->min_height = CompanyList::minWindowSize.height;
        window->max_width = CompanyList::maxWindowSize.width;
        window->max_height = CompanyList::maxWindowSize.height;

        window->invalidate();

        common::initEvents();

        window->widgets = CompanyList::widgets;
        window->enabled_widgets = CompanyList::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &CompanyList::events;
        window->activated_widgets = 0;
        window->initScrollWidgets();

        return window;
    }

    // 0x00435C69
    void openPerformanceIndexes()
    {
        auto window = open();
        window->callOnMouseUp(common::widx::tab_performance);
    }

    namespace CompanyPerformance
    {
        static const Gfx::ui_size_t windowSize = { 635, 322 };

        const uint64_t enabledWidgets = common::enabledWidgets;

        widget_t widgets[] = {
            commonWidgets(635, 322, string_ids::title_company_performance),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004366D7
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00436490
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[common::widx::panel].top + 4;
            _graphRight = 520;
            _graphBottom = self->height - self->widgets[common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 40;
            _graphYAxisLabelIncrement = 20;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                if (maxHistorySize < company.history_size)
                    maxHistorySize = company.history_size;
            }

            uint8_t count = 0;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                auto companyId = company.id();
                auto companyColour = companymgr::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.performance_index_history[0]);
                _graphDataStart[count] = maxHistorySize - company.history_size;
                _graphLineColour[count] = Colour::getShade(companyColour, 6);
                _graphItemId[count] = companyId;
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _dword_113DD0C = 2;
            _graphXLabel = string_ids::rawdate_short;
            _graphYLabel = string_ids::percentage_one_decimal_place;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 100;
            _dword_113DD8E = 2;

            common::drawGraphAndKey(self, dpi);
        }

        // 0x004361D8
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_height = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::onUpdate;
            events.prepare_draw = common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CargoUnits
    {
        static const Gfx::ui_size_t windowSize = { 640, 272 };

        const uint64_t enabledWidgets = common::enabledWidgets;

        widget_t widgets[] = {
            commonWidgets(635, 322, string_ids::title_company_cargo_units),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004369FB
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x004367B4
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[common::widx::panel].top + 4;
            _graphRight = 525;
            _graphBottom = self->height - self->widgets[common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 45;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                if (maxHistorySize < company.history_size)
                    maxHistorySize = company.history_size;
            }

            uint8_t count = 0;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                auto companyId = company.id();
                auto companyColour = companymgr::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.cargo_units_delivered_history[0]);
                _graphDataStart[count] = maxHistorySize - company.history_size;
                _graphLineColour[count] = Colour::getShade(companyColour, 6);
                _graphItemId[count] = companyId;
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _dword_113DD0C = 4;
            _graphXLabel = string_ids::rawdate_short;
            _graphYLabel = string_ids::cargo_units_delivered;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 1000;
            _dword_113DD8E = 2;

            common::drawGraphAndKey(self, dpi);
        }

        // 0x00436201
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_height = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::onUpdate;
            events.prepare_draw = common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CargoDistance
    {
        static const Gfx::ui_size_t windowSize = { 660, 272 };

        const uint64_t enabledWidgets = common::enabledWidgets;

        widget_t widgets[] = {
            commonWidgets(635, 322, string_ids::title_cargo_distance_graphs),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x00436D1F
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00436AD8
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[common::widx::panel].top + 4;
            _graphRight = 545;
            _graphBottom = self->height - self->widgets[common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 65;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                if (maxHistorySize < company.history_size)
                    maxHistorySize = company.history_size;
            }

            uint8_t count = 0;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                auto companyId = company.id();
                auto companyColour = companymgr::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.cargo_units_distance_history[0]);
                _graphDataStart[count] = maxHistorySize - company.history_size;
                _graphLineColour[count] = Colour::getShade(companyColour, 6);
                _graphItemId[count] = companyId;
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _dword_113DD0C = 4;
            _graphXLabel = string_ids::rawdate_short;
            _graphYLabel = string_ids::cargo_units_delivered;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 1000;
            _dword_113DD8E = 2;

            common::drawGraphAndKey(self, dpi);
        }

        // 0x00436227
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_height = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::onUpdate;
            events.prepare_draw = common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CompanyValues
    {
        static const Gfx::ui_size_t windowSize = { 685, 322 };

        const uint64_t enabledWidgets = common::enabledWidgets;

        widget_t widgets[] = {
            commonWidgets(685, 322, string_ids::title_company_values),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x00437043
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00436DFC
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[common::widx::panel].top + 4;
            _graphRight = 570;
            _graphBottom = self->height - self->widgets[common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 90;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                if (maxHistorySize < company.history_size)
                    maxHistorySize = company.history_size;
            }

            uint8_t count = 0;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                auto companyId = company.id();
                auto companyColour = companymgr::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.companyValueHistory[0]);
                _graphDataStart[count] = maxHistorySize - company.history_size;
                _graphLineColour[count] = Colour::getShade(companyColour, 6);
                _graphItemId[count] = companyId;
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _dword_113DD0C = 4;
            _graphXLabel = string_ids::rawdate_short;
            _graphYLabel = string_ids::small_company_value_currency;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 10000;
            _dword_113DD8E = 2;

            common::drawGraphAndKey(self, dpi);
        }

        // 0x0043624D
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_height = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::onUpdate;
            events.prepare_draw = common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CargoPaymentRates
    {
        static const Gfx::ui_size_t windowSize = { 495, 342 };

        const uint64_t enabledWidgets = common::enabledWidgets;

        widget_t widgets[] = {
            commonWidgets(495, 342, string_ids::title_cargo_payment_rates),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0043737D
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00437949
        static void drawGraphKey(window* self, Gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y)
        {
            auto cargoCount = 0;
            for (uint8_t i = 0; i < objectmgr::getMaxObjects(object_type::cargo); i++)
            {
                auto cargo = objectmgr::get<cargo_object>(i);
                if (cargo == nullptr)
                    continue;

                auto colour = _cargoLineColour[i];
                colour = Colour::getShade(colour, 6);
                auto stringId = string_ids::small_black_string;

                if (self->var_854 & (1 << cargoCount))
                {
                    stringId = string_ids::small_white_string;
                }

                if (!(self->var_854 & (1 << cargoCount)) || !(_word_9C68C7 & (1 << 2)))
                {
                    Gfx::fillRect(dpi, x, y + 3, x + 4, y + 7, colour);
                }

                auto args = FormatArguments();
                args.push(cargo->name);

                Gfx::drawString_494BBF(*dpi, x + 6, y, 94, Colour::black, stringId, &args);

                y += 10;
                cargoCount++;
            }
        }

        // 0x00437120
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[common::widx::panel].top + 14;
            _graphRight = 380;
            _graphBottom = self->height - self->widgets[common::widx::panel].top - 28;
            _graphYOffset = 17;
            _graphXOffset = 80;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            auto count = 0;
            for (uint8_t i = 0; i < objectmgr::getMaxObjects(object_type::cargo); i++)
            {
                auto cargo = objectmgr::get<cargo_object>(i);
                if (cargo == nullptr)
                    continue;

                auto colour = _cargoLineColour[i];

                _graphYData[count] = reinterpret_cast<uint32_t>(&_deliveredCargoPayment[i][0]);
                _graphDataStart[count] = 0;
                _graphLineColour[count] = Colour::getShade(colour, 6);
                _graphItemId[count] = i;
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = 60;
            _dword_113DD0C = 4;
            _graphXLabel = string_ids::cargo_delivered_days;
            _graphYLabel = string_ids::cargo_delivered_currency;
            _word_113DD80 = 5;
            _graphXAxisLabelIncrement = 20;
            _dword_113DD86 = 0;
            _dword_113DD8A = 0;
            _dword_113DD8E = 0;

            _graphXAxisRange = 2;
            _dword_113DD7C = 2;
            _byte_113DD99 = 1;

            common::drawGraph(self, dpi);

            if (self->var_854 != 0)
            {
                auto i = 0;
                while (utility::bitScanForward(self->var_854) != _graphItemId[i])
                {
                    i++;
                }

                _dword_113DD50 = 0xFFFFFFFF & ~(1 << i);

                if (_word_9C68C7 & (1 << 2))
                    _graphLineColour[i] = 10;

                _dword_113DD8E = _dword_113DD8E | (1 << 2);

                common::drawGraph(self, dpi);
            }

            auto x = self->width + self->x - 104;
            auto y = self->y + 52;

            drawGraphKey(self, dpi, x, y);

            x = self->x + 8;
            y = self->widgets[common::widx::panel].top + self->y + 1;

            auto args = FormatArguments();
            args.push<uint16_t>(100);
            args.push<uint16_t>(10);

            Gfx::drawString_494B3F(*dpi, x, y, Colour::black, string_ids::cargo_deliver_graph_title, &args);

            x = self->x + 160;
            y = self->height + self->y - 13;

            Gfx::drawString_494B3F(*dpi, x, y, Colour::black, string_ids::cargo_transit_time);
        }

        static void sub_4375F7()
        {
            registers regs;
            call(0x004375F7, regs);
        }

        // 0x00436273
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_height = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            sub_4375F7();
        }

        static void initEvents()
        {
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::onUpdate;
            events.prepare_draw = common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CompanySpeedRecords
    {
        static const Gfx::ui_size_t windowSize = { 495, 169 };

        const uint64_t enabledWidgets = common::enabledWidgets;

        widget_t widgets[] = {
            commonWidgets(495, 169, string_ids::title_speed_records),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0043737D
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x0043745A
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto y = self->y + 47;

            for (auto i = 0; i < 3; i++)
            {
                auto recordType = _word_52624E[i];
                if (recordType == 0)
                    continue;
                {
                    auto args = FormatArguments();
                    args.push(recordType);

                    const string_id string[] = {
                        string_ids::land_speed_record,
                        string_ids::air_speed_record,
                        string_ids::water_speed_record,
                    };

                    auto x = self->x + 4;
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, string[i], &args);
                }
                y += 11;

                auto companyId = _byte_526254[i];

                if (companyId != company_id::null)
                {
                    auto company = companymgr::get(companyId);
                    auto competitorObj = objectmgr::get<competitor_object>(company->competitor_id);

                    auto imageId = competitorObj->images[company->owner_emotion];
                    imageId = Gfx::recolour(imageId, company->mainColours.primary);

                    auto x = self->x + 4;
                    Gfx::drawImage(dpi, x, y, imageId);

                    x = self->x + 33;
                    y += 7;

                    auto args = FormatArguments();
                    args.push(company->name);
                    args.push<uint16_t>(0);
                    args.push(_dword_526258[i]);

                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, string_ids::record_date_achieved, &args);
                    y += 17;
                }

                y += 5;
            }
        }

        static void initEvents()
        {
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::onUpdate;
            events.prepare_draw = common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { CompanyList::widgets, widx::tab_company_list, &CompanyList::events, CompanyList::enabledWidgets },
            { CompanyPerformance::widgets, widx::tab_performance, &CompanyPerformance::events, CompanyPerformance::enabledWidgets },
            { CargoUnits::widgets, widx::tab_cargo_units, &CargoUnits::events, CargoUnits::enabledWidgets },
            { CargoDistance::widgets, widx::tab_cargo_distance, &CargoDistance::events, CargoDistance::enabledWidgets },
            { CompanyValues::widgets, widx::tab_values, &CompanyValues::events, CompanyValues::enabledWidgets },
            { CargoPaymentRates::widgets, widx::tab_payment_rates, &CargoPaymentRates::events, CargoPaymentRates::enabledWidgets },
            { CompanySpeedRecords::widgets, widx::tab_speed_records, &CompanySpeedRecords::events, CompanySpeedRecords::enabledWidgets },
        };

        // 0x0043667B
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_company_list:
                case common::widx::tab_performance:
                case common::widx::tab_cargo_units:
                case common::widx::tab_cargo_distance:
                case common::widx::tab_values:
                case common::widx::tab_payment_rates:
                case common::widx::tab_speed_records:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static void sub_4378BA(window* self, int16_t x, int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x004378BA, regs);
        }

        static void sub_4379F2(window* self, int16_t x, int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x004379F2, regs);
        }

        // 0x00437570
        static void onUpdate(window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->current_tab + common::widx::tab_company_list);

            auto x = self->width - 104 + self->x;
            auto y = self->y + 52;

            switch (self->current_tab + widx::tab_company_list)
            {
                case widx::tab_cargo_distance:
                case widx::tab_cargo_units:
                case widx::tab_performance:
                case widx::tab_values:
                {
                    _word_9C68C7++;
                    sub_4378BA(self, x, y);
                    break;
                }
                case widx::tab_payment_rates:
                {
                    _word_9C68C7++;
                    sub_4379F2(self, x, y);
                    break;
                }
                case widx::tab_speed_records:
                    break;
            }
        }

        // 0x00436419
        static void prepareDraw(window* self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab
            self->activated_widgets &= ~((1ULL << tab_cargo_distance) | (1ULL << tab_cargo_units) | (1ULL << tab_company_list) | (1ULL << tab_payment_rates) | (1ULL << tab_performance) | (1ULL << tab_speed_records) | (1ULL << tab_values));
            self->activated_widgets |= (1ULL << common::tabInformationByTabOffset[self->current_tab].widgetIndex);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;
        }

        //0x004360FA
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::isToolActive(self->type, self->number))
                input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_company_list;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_company_list];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            self->invalidate();

            switch (widgetIndex)
            {
                case widx::tab_company_list:
                    CompanyList::tabReset(self);
                    break;
                case widx::tab_performance:
                    CompanyPerformance::tabReset(self);
                    break;
                case widx::tab_cargo_units:
                    CargoUnits::tabReset(self);
                    break;
                case widx::tab_cargo_distance:
                    CargoDistance::tabReset(self);
                    break;
                case widx::tab_values:
                    CompanyValues::tabReset(self);
                    break;
                case widx::tab_payment_rates:
                    CargoPaymentRates::tabReset(self);
                    break;
            }

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x00437637
        static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Company List Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::tab_companies;

                widget::draw_tab(self, dpi, imageId, widx::tab_company_list);
            }

            // Performance Index Tab
            {
                static const uint32_t performanceImageIds[] = {
                    interface_skin::image_ids::tab_performance_index_frame0,
                    interface_skin::image_ids::tab_performance_index_frame1,
                    interface_skin::image_ids::tab_performance_index_frame2,
                    interface_skin::image_ids::tab_performance_index_frame3,
                    interface_skin::image_ids::tab_performance_index_frame4,
                    interface_skin::image_ids::tab_performance_index_frame5,
                    interface_skin::image_ids::tab_performance_index_frame6,
                    interface_skin::image_ids::tab_performance_index_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_performance - widx::tab_company_list)
                    imageId += performanceImageIds[(self->frame_no / 4) % std::size(performanceImageIds)];
                else
                    imageId += performanceImageIds[0];

                imageId = Gfx::recolour(imageId, self->colours[1]);

                widget::draw_tab(self, dpi, imageId, widx::tab_performance);
            }

            // Cargo Unit Tab
            {
                static const uint32_t cargoUnitsImageIds[] = {
                    interface_skin::image_ids::tab_cargo_units_frame0,
                    interface_skin::image_ids::tab_cargo_units_frame1,
                    interface_skin::image_ids::tab_cargo_units_frame2,
                    interface_skin::image_ids::tab_cargo_units_frame3,
                    interface_skin::image_ids::tab_cargo_units_frame4,
                    interface_skin::image_ids::tab_cargo_units_frame5,
                    interface_skin::image_ids::tab_cargo_units_frame6,
                    interface_skin::image_ids::tab_cargo_units_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_cargo_units - widx::tab_company_list)
                    imageId += cargoUnitsImageIds[(self->frame_no / 4) % std::size(cargoUnitsImageIds)];
                else
                    imageId += cargoUnitsImageIds[0];

                imageId = Gfx::recolour(imageId, self->colours[1]);

                widget::draw_tab(self, dpi, imageId, widx::tab_cargo_units);
            }

            // Cargo Distance Tab
            {
                static const uint32_t cargoDistanceImageIds[] = {
                    interface_skin::image_ids::tab_cargo_distance_frame0,
                    interface_skin::image_ids::tab_cargo_distance_frame1,
                    interface_skin::image_ids::tab_cargo_distance_frame2,
                    interface_skin::image_ids::tab_cargo_distance_frame3,
                    interface_skin::image_ids::tab_cargo_distance_frame4,
                    interface_skin::image_ids::tab_cargo_distance_frame5,
                    interface_skin::image_ids::tab_cargo_distance_frame6,
                    interface_skin::image_ids::tab_cargo_distance_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_cargo_distance - widx::tab_company_list)
                    imageId += cargoDistanceImageIds[(self->frame_no / 4) % std::size(cargoDistanceImageIds)];
                else
                    imageId += cargoDistanceImageIds[0];

                imageId = Gfx::recolour(imageId, self->colours[1]);

                widget::draw_tab(self, dpi, imageId, widx::tab_cargo_distance);
            }

            // Company Values Tab
            {
                static const uint32_t companyValuesImageIds[] = {
                    interface_skin::image_ids::tab_production_frame0,
                    interface_skin::image_ids::tab_production_frame1,
                    interface_skin::image_ids::tab_production_frame2,
                    interface_skin::image_ids::tab_production_frame3,
                    interface_skin::image_ids::tab_production_frame4,
                    interface_skin::image_ids::tab_production_frame5,
                    interface_skin::image_ids::tab_production_frame6,
                    interface_skin::image_ids::tab_production_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_values - widx::tab_company_list)
                    imageId += companyValuesImageIds[(self->frame_no / 4) % std::size(companyValuesImageIds)];
                else
                    imageId += companyValuesImageIds[0];

                imageId = Gfx::recolour(imageId, self->colours[1]);

                widget::draw_tab(self, dpi, imageId, widx::tab_values);

                if (!(self->isDisabled(widx::tab_values)))
                {
                    auto x = self->widgets[widx::tab_values].left + self->x + 28;
                    auto y = self->widgets[widx::tab_values].top + self->y + 14 + 1;
                    Gfx::drawString_494C78(*dpi, x, y, Colour::black, string_ids::currency_symbol);
                }
            }

            // Payment Rates Tab
            {

                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::tab_cargo_payment_rates;

                widget::draw_tab(self, dpi, imageId, widx::tab_payment_rates);

                if (!(self->isDisabled(widx::tab_payment_rates)))
                {
                    auto x = self->widgets[widx::tab_payment_rates].left + self->x + 28;
                    auto y = self->widgets[widx::tab_payment_rates].top + self->y + 14 + 1;
                    Gfx::drawString_494C78(*dpi, x, y, Colour::black, string_ids::currency_symbol);
                }
            }

            // Speed Records Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::tab_awards;

                imageId = Gfx::recolour(imageId, self->colours[1]);

                widget::draw_tab(self, dpi, imageId, widx::tab_speed_records);
            }
        }

        // 0x00437AB6
        static void refreshCompanyList(window* self)
        {
            self->row_count = 0;

            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                company.challenge_flags &= ~company_flags::sorted;
            }
        }

        // 0x004CF824
        static void drawGraph(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edi = (uint32_t)dpi;
            call(0x004CF824, regs);
        }

        // 0x00437810
        static void drawGraphKey(window* self, Gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y)
        {
            auto companyCount = 0;
            for (auto& company : companymgr::companies())
            {
                if (company.empty())
                    continue;

                auto companyColour = companymgr::getCompanyColour(company.id());
                auto colour = Colour::getShade(companyColour, 6);
                auto stringId = string_ids::small_black_string;

                if (self->var_854 & (1 << companyCount))
                {
                    stringId = string_ids::small_white_string;
                }

                if (!(self->var_854 & (1 << companyCount)) || !(_word_9C68C7 & (1 << 2)))
                {
                    Gfx::fillRect(dpi, x, y + 3, x + 4, y + 7, colour);
                }

                auto args = FormatArguments();
                args.push(company.name);

                Gfx::drawString_494BBF(*dpi, x + 6, y, 94, Colour::black, stringId, &args);

                y += 10;
                companyCount++;
            }
        }

        // 0x004365E4
        static void drawGraphAndKey(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            auto totalMonths = (getCurrentYear() * 12) + static_cast<uint16_t>(getCurrentMonth());

            _graphXAxisRange = totalMonths;
            _dword_113DD7C = 1;
            _byte_113DD99 = 1;

            common::drawGraph(self, dpi);

            if (self->var_854 != 0)
            {
                auto i = 0;
                auto bitScan = utility::bitScanForward(self->var_854);
                while (bitScan != _graphItemId[i] && bitScan != -1)
                {
                    i++;
                }

                _dword_113DD50 = 0xFFFFFFFF & ~(1 << i);

                if (_word_9C68C7 & (1 << 2))
                    _graphLineColour[i] = 10;

                _dword_113DD8E = _dword_113DD8E | (1 << 2);

                common::drawGraph(self, dpi);
            }

            auto x = self->width + self->x - 104;
            auto y = self->y + 52;

            common::drawGraphKey(self, dpi, x, y);
        }
        static void initEvents()
        {
            CompanyList::initEvents();
            CompanyValues::initEvents();
            CompanyPerformance::initEvents();
            CargoDistance::initEvents();
            CargoUnits::initEvents();
            CargoPaymentRates::initEvents();
            CompanySpeedRecords::initEvents();
        }
    }
}
