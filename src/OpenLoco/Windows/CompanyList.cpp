#include "../Company.h"
#include "../CompanyManager.h"
#include "../Date.h"
#include "../Economy/Economy.h"
#include "../Graphics/Colour.h"
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
#include "../Ui/WindowManager.h"
#include "../Utility/Numeric.hpp"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::CompanyList
{
    static loco_global<Colour[32], 0x004F9442> _cargoLineColour;
    static loco_global<Ui::WindowNumber_t, 0x00523390> _toolWindowNumber;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t[3], 0x0052624E> _word_52624E;
    static loco_global<CompanyId[3], 0x00526254> _byte_526254;
    static loco_global<uint32_t[3], 0x00526258> _dword_526258;
    static loco_global<currency32_t[32][60], 0x009C68F8> _deliveredCargoPayment;
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
    static loco_global<uint32_t, 0x0113DD0C> _graphDataTypeSize;
    static loco_global<uint16_t[32], 0x0113DD10> _graphDataStart;
    static loco_global<uint32_t, 0x0113DD50> _dword_113DD50;
    static loco_global<PaletteIndex_t[32], 0x0113DD54> _graphLineColour;
    static loco_global<uint16_t, 0x0113DD74> _graphDataEnd;
    static loco_global<uint16_t, 0x0113DD76> _graphXLabel;
    static loco_global<uint32_t, 0x0113DD78> _graphXAxisRange;
    static loco_global<uint32_t, 0x0113DD7C> _dword_113DD7C;
    static loco_global<uint16_t, 0x0113DD80> _word_113DD80; // graphXAxisIncrement?
    static loco_global<uint16_t, 0x0113DD82> _graphXAxisLabelIncrement;
    static loco_global<uint16_t, 0x0113DD84> _graphYLabel;
    static loco_global<uint32_t, 0x0113DD86> _dword_113DD86;
    static loco_global<uint32_t, 0x0113DD8A> _dword_113DD8A;
    static loco_global<uint32_t, 0x0113DD8E> _dword_113DD8E;
    static loco_global<uint8_t, 0x0113DD99> _byte_113DD99;
    static loco_global<uint16_t[32], 0x0113DD9A> _graphItemId;

    namespace Common
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 231 }, WidgetType::panel, WindowColour::secondary),                                                                      \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_compare_companies),                           \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_company_performance),                        \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_cargo_graphs),                               \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_cargo_distance_graphs),                      \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_company_values),                            \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_cargo_payment_rates),                       \
        makeRemapWidget({ 189, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_speed_records)

        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex);
        static void onUpdate(Window* self);
        static void prepareDraw(Window* self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void refreshCompanyList(Window* self);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void drawGraph(Window* self, Gfx::Context* context);
        static void drawGraphAndLegend(Window* self, Gfx::Context* context);
        static void initEvents();
    }

    namespace CompanyList
    {
        static const Ui::Size maxWindowSize = { 640, 470 };
        static const Ui::Size minWindowSize = { 300, 272 };
        static const Ui::Size windowSize = { 640, 272 };

        static const uint8_t rowHeight = 25;

        enum widx
        {
            sort_name = 11,
            sort_status,
            sort_performance,
            sort_value,
            scrollview,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << sort_name) | (1 << sort_status) | (1 << sort_performance) | (1 << sort_value) | (1 << scrollview);

        Widget widgets[] = {
            commonWidgets(640, 272, StringIds::title_company_list),
            makeWidget({ 4, 43 }, { 175, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_name),
            makeWidget({ 179, 43 }, { 210, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_status),
            makeWidget({ 389, 43 }, { 145, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_performance),
            makeWidget({ 534, 43 }, { 100, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_value),
            makeWidget({ 3, 56 }, { 634, 201 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd(),
        };

        static WindowEventList events;

        enum SortMode : uint16_t
        {
            Name,
            Status,
            Performance,
            Value,
        };

        // 0x004360A2
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_company_list:
                case Common::widx::tab_performance:
                case Common::widx::tab_cargo_units:
                case Common::widx::tab_cargo_distance:
                case Common::widx::tab_values:
                case Common::widx::tab_payment_rates:
                case Common::widx::tab_speed_records:
                    Common::switchTab(self, widgetIndex);
                    break;

                case sort_name:
                case sort_status:
                case sort_performance:
                case sort_value:
                {
                    auto sortMode = widgetIndex - widx::sort_name;
                    if (self->sortMode == sortMode)
                        return;

                    self->sortMode = sortMode;
                    self->invalidate();
                    self->var_83C = 0;
                    self->rowHover = -1;

                    Common::refreshCompanyList(self);
                    break;
                }
            }
        }

        // 0x004363CB
        static void onResize(Window* self)
        {
            self->setSize(minWindowSize, maxWindowSize);
        }

        // 0x00437BA0
        static bool orderByName(const OpenLoco::Company& lhs, const OpenLoco::Company& rhs)
        {
            char lhsString[256] = { 0 };
            StringManager::formatString(lhsString, lhs.name);

            char rhsString[256] = { 0 };
            StringManager::formatString(rhsString, rhs.name);

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00437BE1
        static bool orderByStatus(const OpenLoco::Company& lhs, const OpenLoco::Company& rhs)
        {
            char lhsString[256] = { 0 };
            {
                auto args = FormatArguments();
                auto statusString = CompanyManager::getOwnerStatus(lhs.id(), args);
                StringManager::formatString(lhsString, statusString, &args);
            }

            char rhsString[256] = { 0 };
            {
                auto args = FormatArguments();
                auto statusString = CompanyManager::getOwnerStatus(lhs.id(), args);
                StringManager::formatString(rhsString, statusString, &args);
            }

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00437C53
        static bool orderByPerformance(const OpenLoco::Company& lhs, const OpenLoco::Company& rhs)
        {
            auto lhsPerformance = lhs.performanceIndex;

            auto rhsPerformance = rhs.performanceIndex;

            return rhsPerformance < lhsPerformance;
        }

        // 0x00437C67
        static bool orderByValue(const OpenLoco::Company& lhs, const OpenLoco::Company& rhs)
        {
            return rhs.companyValueHistory[0] < lhs.companyValueHistory[0];
        }

        // 0x00437BA0, 0x00437BE1, 0x00437C53, 0x00437C67
        static bool getOrder(const SortMode mode, OpenLoco::Company& lhs, OpenLoco::Company& rhs)
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
        static void updateCompanyList(Window* self)
        {
            CompanyId chosenCompany = CompanyId::null;

            for (auto& company : CompanyManager::companies())
            {
                if ((company.challengeFlags & CompanyFlags::sorted) != 0)
                    continue;

                if (chosenCompany == CompanyId::null)
                {
                    chosenCompany = company.id();
                    continue;
                }

                if (getOrder(SortMode(self->sortMode), company, *CompanyManager::get(chosenCompany)))
                {
                    chosenCompany = company.id();
                }
            }

            if (chosenCompany != CompanyId::null)
            {
                bool shouldInvalidate = false;

                CompanyManager::get(chosenCompany)->challengeFlags |= CompanyFlags::sorted;

                if (chosenCompany != CompanyId(self->rowInfo[self->rowCount]))
                {
                    self->rowInfo[self->rowCount] = enumValue(chosenCompany);
                    shouldInvalidate = true;
                }

                self->rowCount++;
                if (self->rowCount > self->var_83C)
                {
                    self->var_83C = self->rowCount;
                    shouldInvalidate = true;
                }

                if (shouldInvalidate)
                {
                    self->invalidate();
                }
            }
            else
            {
                if (self->var_83C != self->rowCount)
                {
                    self->var_83C = self->rowCount;
                    self->invalidate();
                }

                Common::refreshCompanyList(self);
            }
        }

        // 0x004362C0
        static void onUpdate(Window* self)
        {
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::companyList, self->number, self->currentTab + Common::widx::tab_company_list);

            _word_9C68C7++;

            // Add three companies every tick.
            updateCompanyList(self);
            updateCompanyList(self);
            updateCompanyList(self);
        }

        // 0x004362F7
        static void event_08(Window* self)
        {
            self->flags |= WindowFlags::notScrollView;
        }

        // 0x004362FF
        static void event_09(Window* self)
        {
            if (!(self->flags & WindowFlags::notScrollView))
                return;

            if (self->rowHover == -1)
                return;

            self->rowHover = -1;
            self->invalidate();
        }

        // 0x00436321
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = self->var_83C * rowHeight;
        }

        // 0x004363A0
        static void onScrollMouseDown(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / rowHeight;
            if (currentRow > self->var_83C)
                return;

            CompanyId currentCompany = CompanyId(self->rowInfo[currentRow]);
            if (currentCompany == CompanyId::null)
                return;

            CompanyWindow::open(currentCompany);
        }

        // 0x00436361
        static void onScrollMouseOver(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(WindowFlags::notScrollView);

            uint16_t currentRow = y / rowHeight;
            int16_t currentCompany = -1;

            if (currentRow < self->var_83C)
                currentCompany = self->rowInfo[currentRow];

            if (self->rowHover == currentCompany)
                return;

            self->rowHover = currentCompany;
            self->invalidate();
        }

        // 0x004362B6
        static std::optional<FormatArguments> tooltip(Window* self, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_company_list);
            return args;
        }

        // 0x0043632C
        static Ui::CursorId cursor(Window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->rowInfo[currentIndex] != -1)
                return CursorId::handPointer;

            return fallback;
        }

        // 0x00435D07
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

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
            self->widgets[widx::sort_name].text = self->sortMode == SortMode::Name ? StringIds::table_header_company_name_desc : StringIds::table_header_company_name;
            self->widgets[widx::sort_status].text = self->sortMode == SortMode::Status ? StringIds::table_header_company_status_desc : StringIds::table_header_company_status;
            self->widgets[widx::sort_performance].text = self->sortMode == SortMode::Performance ? StringIds::table_header_company_performance_desc : StringIds::table_header_company_performance;
            self->widgets[widx::sort_value].text = self->sortMode == SortMode::Value ? StringIds::table_header_company_value_desc : StringIds::table_header_company_value;
        }

        // 0x00435E56
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto args = FormatArguments();
            if (self->var_83C == 1)
                args.push(StringIds::company_singular);
            else
                args.push(StringIds::companies_plural);

            args.push(self->var_83C);

            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x00435EA7
        static void drawScroll(Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            Gfx::clearSingle(context, colour);

            auto yBottom = 0;
            for (auto i = 0; i < self.var_83C; i++, yBottom += 25)
            {
                auto yTop = yBottom + 25;

                if (yTop <= context.y)
                    continue;

                yTop = context.y + context.height;

                if (yBottom >= yTop)
                    continue;

                auto rowItem = self.rowInfo[i];

                if (rowItem == -1)
                    continue;

                auto stringId = StringIds::black_stringid;

                if (rowItem == self.rowHover)
                {
                    Gfx::drawRect(context, 0, yBottom, self.width, 24, (1 << 25) | PaletteIndex::index_30);

                    stringId = StringIds::wcolour2_stringid;
                }

                auto company = CompanyManager::get(CompanyId(rowItem));
                auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);
                auto imageId = Gfx::recolour(competitorObj->images[company->ownerEmotion], company->mainColours.primary);

                {
                    auto args = FormatArguments();
                    args.push(StringIds::table_item_company);
                    args.push(imageId);
                    args.push(company->name);

                    Gfx::drawString_494BBF(context, 0, yBottom - 1, 173, Colour::black, stringId, &args);
                }

                {
                    auto args = FormatArguments();
                    args.skip(sizeof(string_id));
                    string_id ownerStatus = CompanyManager::getOwnerStatus(company->id(), args);
                    args.rewind();
                    args.push(ownerStatus);

                    Gfx::drawString_494BBF(context, 175, yBottom + 7, 208, Colour::black, stringId, &args);
                }

                auto performanceStringId = StringIds::performance_index;

                if ((company->challengeFlags & CompanyFlags::increasedPerformance) && (company->challengeFlags & CompanyFlags::decreasedPerformance))
                {
                    performanceStringId = StringIds::performance_index_decrease;

                    if (company->challengeFlags & CompanyFlags::increasedPerformance)
                    {
                        performanceStringId = StringIds::performance_index_increase;
                    }
                }

                {
                    auto args = FormatArguments();

                    args.push(performanceStringId);
                    formatPerformanceIndex(company->performanceIndex, args);

                    Gfx::drawString_494BBF(context, 385, yBottom - 1, 143, Colour::black, stringId, &args);
                }

                {
                    auto args = FormatArguments();

                    args.push(StringIds::company_value_currency);
                    args.push(company->companyValueHistory[0]);

                    Gfx::drawString_494BBF(context, 530, yBottom - 1, 98, Colour::black, stringId, &args);
                }
            }
        }

        // 0x00436198
        static void tabReset(Window* self)
        {
            self->minWidth = minWindowSize.width;
            self->minHeight = minWindowSize.height;
            self->maxWidth = maxWindowSize.width;
            self->maxHeight = maxWindowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            self->var_83C = 0;
            self->rowHover = -1;
            Common::refreshCompanyList(self);
        }

        static void initEvents()
        {
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = onUpdate;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.getScrollSize = getScrollSize;
            events.scrollMouseDown = onScrollMouseDown;
            events.scrollMouseOver = onScrollMouseOver;
            events.tooltip = tooltip;
            events.cursor = cursor;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
            events.drawScroll = drawScroll;
        }
    }

    // 0x00435BC8
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::companyList);

        if (window != nullptr)
        {
            if (Input::isToolActive(_toolWindowType, _toolWindowNumber))
            {
                Input::toolCancel();
                window = WindowManager::bringToFront(WindowType::companyList);
            }
        }

        if (window == nullptr)
        {
            Ui::Size windowSize = { 640, 272 };

            window = WindowManager::createWindow(WindowType::companyList, windowSize, 0, &CompanyList::events);

            window->frame_no = 0;
            window->savedView.clear();
            window->flags |= WindowFlags::resizable;
            window->sortMode = 2;
            window->var_83C = 0;
            window->rowHover = -1;

            Common::refreshCompanyList(window);

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, skin->colour_0B);
            window->setColour(WindowColour::secondary, skin->colour_0C);

            window->var_854 = 0;
        }

        window->currentTab = 0;
        window->minWidth = CompanyList::minWindowSize.width;
        window->minHeight = CompanyList::minWindowSize.height;
        window->maxWidth = CompanyList::maxWindowSize.width;
        window->maxHeight = CompanyList::maxWindowSize.height;

        window->invalidate();

        Common::initEvents();

        window->widgets = CompanyList::widgets;
        window->enabledWidgets = CompanyList::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &CompanyList::events;
        window->activatedWidgets = 0;
        window->initScrollWidgets();

        return window;
    }

    // 0x00435C69
    void openPerformanceIndexes()
    {
        auto window = open();
        window->callOnMouseUp(Common::widx::tab_performance);
    }

    namespace CompanyPerformance
    {
        static const Ui::Size windowSize = { 635, 322 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        Widget widgets[] = {
            commonWidgets(635, 322, StringIds::title_company_performance),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004366D7
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00436490
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[Common::widx::panel].top + 4;
            _graphRight = 520;
            _graphBottom = self->height - self->widgets[Common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 40;
            _graphYAxisLabelIncrement = 20;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : CompanyManager::companies())
            {
                if (maxHistorySize < company.historySize)
                    maxHistorySize = company.historySize;
            }

            uint8_t count = 0;

            for (auto& company : CompanyManager::companies())
            {
                auto companyId = company.id();
                auto companyColour = CompanyManager::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.performanceIndexHistory[0]);
                _graphDataStart[count] = maxHistorySize - company.historySize;
                _graphLineColour[count] = Colours::getShade(companyColour, 6);
                _graphItemId[count] = enumValue(companyId);
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _graphDataTypeSize = 2;
            _graphXLabel = StringIds::rawdate_short;
            _graphYLabel = StringIds::percentage_one_decimal_place;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 100;
            _dword_113DD8E = 2;

            Common::drawGraphAndLegend(self, context);
        }

        // 0x004361D8
        static void tabReset(Window* self)
        {
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxHeight = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::onUpdate;
            events.prepareDraw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CargoUnits
    {
        static const Ui::Size windowSize = { 640, 272 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        Widget widgets[] = {
            commonWidgets(635, 322, StringIds::title_company_cargo_units),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004369FB
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x004367B4
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[Common::widx::panel].top + 4;
            _graphRight = 525;
            _graphBottom = self->height - self->widgets[Common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 45;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : CompanyManager::companies())
            {
                if (maxHistorySize < company.historySize)
                    maxHistorySize = company.historySize;
            }

            uint8_t count = 0;

            for (auto& company : CompanyManager::companies())
            {
                auto companyId = company.id();
                auto companyColour = CompanyManager::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.cargoUnitsDeliveredHistory[0]);
                _graphDataStart[count] = maxHistorySize - company.historySize;
                _graphLineColour[count] = Colours::getShade(companyColour, 6);
                _graphItemId[count] = enumValue(companyId);
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _graphDataTypeSize = 4;
            _graphXLabel = StringIds::rawdate_short;
            _graphYLabel = StringIds::cargo_units_delivered;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 1000;
            _dword_113DD8E = 2;

            Common::drawGraphAndLegend(self, context);
        }

        // 0x00436201
        static void tabReset(Window* self)
        {
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxHeight = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::onUpdate;
            events.prepareDraw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CargoDistance
    {
        static const Ui::Size windowSize = { 660, 272 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        Widget widgets[] = {
            commonWidgets(635, 322, StringIds::title_cargo_distance_graphs),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x00436D1F
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00436AD8
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[Common::widx::panel].top + 4;
            _graphRight = 545;
            _graphBottom = self->height - self->widgets[Common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 65;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : CompanyManager::companies())
            {
                if (maxHistorySize < company.historySize)
                    maxHistorySize = company.historySize;
            }

            uint8_t count = 0;

            for (auto& company : CompanyManager::companies())
            {
                auto companyId = company.id();
                auto companyColour = CompanyManager::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.cargoUnitsDistanceHistory[0]);
                _graphDataStart[count] = maxHistorySize - company.historySize;
                _graphLineColour[count] = Colours::getShade(companyColour, 6);
                _graphItemId[count] = enumValue(companyId);
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _graphDataTypeSize = 4;
            _graphXLabel = StringIds::rawdate_short;
            _graphYLabel = StringIds::cargo_units_delivered;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 1000;
            _dword_113DD8E = 2;

            Common::drawGraphAndLegend(self, context);
        }

        // 0x00436227
        static void tabReset(Window* self)
        {
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxHeight = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::onUpdate;
            events.prepareDraw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CompanyValues
    {
        static const Ui::Size windowSize = { 685, 322 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        Widget widgets[] = {
            commonWidgets(685, 322, StringIds::title_company_values),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x00437043
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00436DFC
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[Common::widx::panel].top + 4;
            _graphRight = 570;
            _graphBottom = self->height - self->widgets[Common::widx::panel].top - 8;
            _graphYOffset = 17;
            _graphXOffset = 90;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            uint16_t maxHistorySize = 1;

            for (auto& company : CompanyManager::companies())
            {
                if (maxHistorySize < company.historySize)
                    maxHistorySize = company.historySize;
            }

            uint8_t count = 0;

            for (auto& company : CompanyManager::companies())
            {
                auto companyId = company.id();
                auto companyColour = CompanyManager::getCompanyColour(companyId);

                _graphYData[count] = reinterpret_cast<uint32_t>(&company.companyValueHistory[0]);
                _graphDataStart[count] = maxHistorySize - company.historySize;
                _graphLineColour[count] = Colours::getShade(companyColour, 6);
                _graphItemId[count] = enumValue(companyId);
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = maxHistorySize;
            _graphDataTypeSize = 6;
            _graphXLabel = StringIds::rawdate_short;
            _graphYLabel = StringIds::small_company_value_currency;
            _word_113DD80 = 4;
            _graphXAxisLabelIncrement = 12;
            _dword_113DD86 = 0;
            _dword_113DD8A = 10000;
            _dword_113DD8E = 2;

            Common::drawGraphAndLegend(self, context);
        }

        // 0x0043624D
        static void tabReset(Window* self)
        {
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxHeight = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
        }

        static void initEvents()
        {
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::onUpdate;
            events.prepareDraw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CargoPaymentRates
    {
        static const Ui::Size windowSize = { 495, 342 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        Widget widgets[] = {
            commonWidgets(495, 342, StringIds::title_cargo_payment_rates),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0043737D
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x00437949
        static void drawGraphLegend(Window* self, Gfx::Context* context, int16_t x, int16_t y)
        {
            auto cargoCount = 0;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                auto cargo = ObjectManager::get<CargoObject>(i);
                if (cargo == nullptr)
                    continue;

                auto colour = _cargoLineColour[i];
                auto palette = Colours::getShade(colour, 6);
                auto stringId = StringIds::small_black_string;

                if (self->var_854 & (1 << cargoCount))
                {
                    stringId = StringIds::small_white_string;
                }

                if (!(self->var_854 & (1 << cargoCount)) || !(_word_9C68C7 & (1 << 2)))
                {
                    Gfx::fillRect(*context, x, y + 3, x + 4, y + 7, palette);
                }

                auto args = FormatArguments();
                args.push(cargo->name);

                Gfx::drawString_494BBF(*context, x + 6, y, 94, Colour::black, stringId, &args);

                y += 10;
                cargoCount++;
            }
        }

        // 0x00437120
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            _graphLeft = self->x + 4;
            _graphTop = self->y + self->widgets[Common::widx::panel].top + 14;
            _graphRight = 380;
            _graphBottom = self->height - self->widgets[Common::widx::panel].top - 28;
            _graphYOffset = 17;
            _graphXOffset = 80;
            _graphYAxisLabelIncrement = 25;
            _dword_113DD50 = 0;

            auto count = 0;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                auto cargo = ObjectManager::get<CargoObject>(i);
                if (cargo == nullptr)
                    continue;

                auto colour = _cargoLineColour[i];

                _graphYData[count] = reinterpret_cast<uint32_t>(&_deliveredCargoPayment[i][0]);
                _graphDataStart[count] = 0;
                _graphLineColour[count] = Colours::getShade(colour, 6);
                _graphItemId[count] = i;
                count++;
            }

            _graphLineCount = count;
            _graphDataEnd = 60;
            _graphDataTypeSize = 4;
            _graphXLabel = StringIds::cargo_delivered_days;
            _graphYLabel = StringIds::cargo_delivered_currency;
            _word_113DD80 = 5;
            _graphXAxisLabelIncrement = 20;
            _dword_113DD86 = 0;
            _dword_113DD8A = 0;
            _dword_113DD8E = 0;

            _graphXAxisRange = 2;
            _dword_113DD7C = 2;
            _byte_113DD99 = 1;

            Common::drawGraph(self, context);

            if (self->var_854 != 0)
            {
                auto i = 0;
                while (Utility::bitScanForward(self->var_854) != _graphItemId[i])
                {
                    i++;
                }

                _dword_113DD50 = 0xFFFFFFFF & ~(1 << i);

                if (_word_9C68C7 & (1 << 2))
                    _graphLineColour[i] = 10;

                _dword_113DD8E = _dword_113DD8E | (1 << 2);

                Common::drawGraph(self, context);
            }

            auto x = self->width + self->x - 104;
            auto y = self->y + 52;

            drawGraphLegend(self, context, x, y);

            x = self->x + 8;
            y = self->widgets[Common::widx::panel].top + self->y + 1;

            auto args = FormatArguments();
            args.push<uint16_t>(100);
            args.push<uint16_t>(10);

            Gfx::drawString_494B3F(*context, x, y, Colour::black, StringIds::cargo_deliver_graph_title, &args);

            x = self->x + 160;
            y = self->height + self->y - 13;

            Gfx::drawString_494B3F(*context, x, y, Colour::black, StringIds::cargo_transit_time);
        }

        // 0x004379F2
        static void setLegendHover(Window* self, int16_t x, int16_t y)
        {
            uint32_t selectedCargo = 0;
            if (!Input::hasFlag(Input::Flags::flag5))
            {
                const auto location = Input::getMouseLocation2();
                auto* frontWindow = WindowManager::findAt(location);
                const auto xDiff = location.x - x;
                const auto yDiff = location.y - y;
                if (frontWindow != nullptr && frontWindow == self && xDiff <= 100 && xDiff >= 0 && yDiff < 320 && yDiff >= 0)
                {
                    auto listY = yDiff;
                    uint8_t cargoItem = 0;
                    for (; cargoItem < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoItem)
                    {
                        auto* cargoObj = ObjectManager::get<CargoObject>(cargoItem);
                        if (cargoObj == nullptr)
                        {
                            continue;
                        }
                        listY -= 10;
                        if (listY <= 0)
                        {
                            selectedCargo = 1ULL << cargoItem;
                            break;
                        }
                    }
                }
            }
            if (self->var_854 != selectedCargo)
            {
                // TODO: var_854 is 16 bits but selectedCargo is 32 bits. Only the first 15 cargo types can be selected.
                self->var_854 = selectedCargo;
                self->invalidate();
            }
            if (self->var_854 != 0)
            {
                self->invalidate();
            }
        }

        // 0x00436273
        static void tabReset(Window* self)
        {
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxHeight = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            Economy::buildDeliveredCargoPaymentsTable();
        }

        static void initEvents()
        {
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::onUpdate;
            events.prepareDraw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace CompanySpeedRecords
    {
        static const Ui::Size windowSize = { 495, 169 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        Widget widgets[] = {
            commonWidgets(495, 169, StringIds::title_speed_records),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x00437591
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x0043745A
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto y = self->y + 47;

            for (auto i = 0; i < 3; i++)
            {
                auto recordSpeed = _word_52624E[i];
                if (recordSpeed == 0)
                    continue;
                {
                    auto args = FormatArguments();
                    args.push(recordSpeed);

                    const string_id string[] = {
                        StringIds::land_speed_record,
                        StringIds::air_speed_record,
                        StringIds::water_speed_record,
                    };

                    auto x = self->x + 4;
                    Gfx::drawString_494B3F(*context, x, y, Colour::black, string[i], &args);
                }
                y += 11;

                auto companyId = _byte_526254[i];

                if (companyId != CompanyId::null)
                {
                    auto company = CompanyManager::get(companyId);
                    auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);

                    auto imageId = competitorObj->images[company->ownerEmotion];
                    imageId = Gfx::recolour(imageId, company->mainColours.primary);

                    auto x = self->x + 4;
                    Gfx::drawImage(context, x, y, imageId);

                    x = self->x + 33;
                    y += 7;

                    auto args = FormatArguments();
                    args.push(company->name);
                    args.push<uint16_t>(0);
                    args.push(_dword_526258[i]);

                    Gfx::drawString_494B3F(*context, x, y, Colour::black, StringIds::record_date_achieved, &args);
                    y += 17;
                }

                y += 5;
            }
        }

        static void initEvents()
        {
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::onUpdate;
            events.prepareDraw = Common::prepareDraw;
            events.draw = draw;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
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
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_company_list:
                case Common::widx::tab_performance:
                case Common::widx::tab_cargo_units:
                case Common::widx::tab_cargo_distance:
                case Common::widx::tab_values:
                case Common::widx::tab_payment_rates:
                case Common::widx::tab_speed_records:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x004378BA
        static void setLegendHover(Window* self, int16_t x, int16_t y)
        {
            uint32_t selectedCompany = 0;
            if (!Input::hasFlag(Input::Flags::flag5))
            {
                const auto location = Input::getMouseLocation2();
                auto* frontWindow = WindowManager::findAt(location);
                const auto xDiff = location.x - x;
                const auto yDiff = location.y - y;
                if (frontWindow != nullptr && frontWindow == self && xDiff <= 100 && xDiff >= 0 && yDiff < 150 && yDiff >= 0)
                {
                    auto listY = yDiff;
                    for (auto& company : CompanyManager::companies())
                    {
                        listY -= 10;
                        if (listY <= 0)
                        {
                            selectedCompany = 1ULL << enumValue(company.id());
                            break;
                        }
                    }
                }
            }
            if (self->var_854 != selectedCompany)
            {
                self->var_854 = selectedCompany;
                self->invalidate();
            }
            if (self->var_854 != 0)
            {
                self->invalidate();
            }
        }

        // 0x00437570
        static void onUpdate(Window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->currentTab + Common::widx::tab_company_list);

            auto x = self->width - 104 + self->x;
            auto y = self->y + 52;

            switch (self->currentTab + widx::tab_company_list)
            {
                case widx::tab_cargo_distance:
                case widx::tab_cargo_units:
                case widx::tab_performance:
                case widx::tab_values:
                {
                    _word_9C68C7++;
                    setLegendHover(self, x, y);
                    break;
                }
                case widx::tab_payment_rates:
                {
                    _word_9C68C7++;
                    CargoPaymentRates::setLegendHover(self, x, y);
                    break;
                }
                case widx::tab_speed_records:
                    break;
            }
        }

        // 0x00436419
        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self->currentTab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab
            self->activatedWidgets &= ~((1ULL << tab_cargo_distance) | (1ULL << tab_cargo_units) | (1ULL << tab_company_list) | (1ULL << tab_payment_rates) | (1ULL << tab_performance) | (1ULL << tab_speed_records) | (1ULL << tab_values));
            self->activatedWidgets |= (1ULL << Common::tabInformationByTabOffset[self->currentTab].widgetIndex);

            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;
        }

        // 0x004360FA
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->currentTab = widgetIndex - widx::tab_company_list;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_company_list];

            self->enabledWidgets = tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
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
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Company List Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_companies;

                Widget::drawTab(self, context, imageId, widx::tab_company_list);
            }

            // Performance Index Tab
            {
                static const uint32_t performanceImageIds[] = {
                    InterfaceSkin::ImageIds::tab_performance_index_frame0,
                    InterfaceSkin::ImageIds::tab_performance_index_frame1,
                    InterfaceSkin::ImageIds::tab_performance_index_frame2,
                    InterfaceSkin::ImageIds::tab_performance_index_frame3,
                    InterfaceSkin::ImageIds::tab_performance_index_frame4,
                    InterfaceSkin::ImageIds::tab_performance_index_frame5,
                    InterfaceSkin::ImageIds::tab_performance_index_frame6,
                    InterfaceSkin::ImageIds::tab_performance_index_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_performance - widx::tab_company_list)
                    imageId += performanceImageIds[(self->frame_no / 4) % std::size(performanceImageIds)];
                else
                    imageId += performanceImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, context, imageId, widx::tab_performance);
            }

            // Cargo Unit Tab
            {
                static const uint32_t cargoUnitsImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cargo_units_frame0,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame1,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame2,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame3,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame4,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame5,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame6,
                    InterfaceSkin::ImageIds::tab_cargo_units_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_cargo_units - widx::tab_company_list)
                    imageId += cargoUnitsImageIds[(self->frame_no / 4) % std::size(cargoUnitsImageIds)];
                else
                    imageId += cargoUnitsImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, context, imageId, widx::tab_cargo_units);
            }

            // Cargo Distance Tab
            {
                static const uint32_t cargoDistanceImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame0,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame1,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame2,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame3,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame4,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame5,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame6,
                    InterfaceSkin::ImageIds::tab_cargo_distance_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_cargo_distance - widx::tab_company_list)
                    imageId += cargoDistanceImageIds[(self->frame_no / 4) % std::size(cargoDistanceImageIds)];
                else
                    imageId += cargoDistanceImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, context, imageId, widx::tab_cargo_distance);
            }

            // Company Values Tab
            {
                static const uint32_t companyValuesImageIds[] = {
                    InterfaceSkin::ImageIds::tab_production_frame0,
                    InterfaceSkin::ImageIds::tab_production_frame1,
                    InterfaceSkin::ImageIds::tab_production_frame2,
                    InterfaceSkin::ImageIds::tab_production_frame3,
                    InterfaceSkin::ImageIds::tab_production_frame4,
                    InterfaceSkin::ImageIds::tab_production_frame5,
                    InterfaceSkin::ImageIds::tab_production_frame6,
                    InterfaceSkin::ImageIds::tab_production_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_values - widx::tab_company_list)
                    imageId += companyValuesImageIds[(self->frame_no / 4) % std::size(companyValuesImageIds)];
                else
                    imageId += companyValuesImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, context, imageId, widx::tab_values);

                if (!(self->isDisabled(widx::tab_values)))
                {
                    auto x = self->widgets[widx::tab_values].left + self->x + 28;
                    auto y = self->widgets[widx::tab_values].top + self->y + 14 + 1;
                    Gfx::drawString_494C78(*context, x, y, Colour::black, StringIds::currency_symbol);
                }
            }

            // Payment Rates Tab
            {

                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_cargo_payment_rates;

                Widget::drawTab(self, context, imageId, widx::tab_payment_rates);

                if (!(self->isDisabled(widx::tab_payment_rates)))
                {
                    auto x = self->widgets[widx::tab_payment_rates].left + self->x + 28;
                    auto y = self->widgets[widx::tab_payment_rates].top + self->y + 14 + 1;
                    Gfx::drawString_494C78(*context, x, y, Colour::black, StringIds::currency_symbol);
                }
            }

            // Speed Records Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_awards;

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, context, imageId, widx::tab_speed_records);
            }
        }

        // 0x00437AB6
        static void refreshCompanyList(Window* self)
        {
            self->rowCount = 0;

            for (auto& company : CompanyManager::companies())
            {
                company.challengeFlags &= ~CompanyFlags::sorted;
            }
        }

        // 0x004CF824
        static void drawGraph(Window* self, Gfx::Context* context)
        {
            registers regs;
            regs.esi = X86Pointer(self);
            regs.edi = X86Pointer(context);
            call(0x004CF824, regs);
        }

        // 0x00437810
        static void drawGraphLegend(Window* self, Gfx::Context* context, int16_t x, int16_t y)
        {
            auto companyCount = 0;
            for (auto& company : CompanyManager::companies())
            {
                auto companyColour = CompanyManager::getCompanyColour(company.id());
                auto colour = Colours::getShade(companyColour, 6);
                auto stringId = StringIds::small_black_string;

                if (self->var_854 & (1 << companyCount))
                {
                    stringId = StringIds::small_white_string;
                }

                if (!(self->var_854 & (1 << companyCount)) || !(_word_9C68C7 & (1 << 2)))
                {
                    Gfx::fillRect(*context, x, y + 3, x + 4, y + 7, colour);
                }

                auto args = FormatArguments();
                args.push(company.name);

                Gfx::drawString_494BBF(*context, x + 6, y, 94, Colour::black, stringId, &args);

                y += 10;
                companyCount++;
            }
        }

        // 0x004365E4
        static void drawGraphAndLegend(Window* self, Gfx::Context* context)
        {
            auto totalMonths = (getCurrentYear() * 12) + static_cast<uint16_t>(getCurrentMonth());

            _graphXAxisRange = totalMonths;
            _dword_113DD7C = 1;
            _byte_113DD99 = 1;

            Common::drawGraph(self, context);

            if (self->var_854 != 0)
            {
                auto i = 0;
                auto bitScan = Utility::bitScanForward(self->var_854);
                while (bitScan != _graphItemId[i] && bitScan != -1)
                {
                    i++;
                }

                _dword_113DD50 = 0xFFFFFFFF & ~(1 << i);

                if (_word_9C68C7 & (1 << 2))
                    _graphLineColour[i] = 10;

                _dword_113DD8E = _dword_113DD8E | (1 << 2);

                Common::drawGraph(self, context);
            }

            auto x = self->width + self->x - 104;
            auto y = self->y + 52;

            Common::drawGraphLegend(self, context, x, y);
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
