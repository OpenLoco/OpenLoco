#include "Date.h"
#include "Economy/Economy.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/CargoObject.h"
#include "Objects/CompetitorObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Chart.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"
#include "World/Company.h"
#include "World/CompanyManager.h"
#include "World/CompanyRecords.h"
#include <OpenLoco/Core/Numerics.hpp>

using namespace OpenLoco::Literals;

namespace OpenLoco::Ui::Windows::CompanyList
{
    static uint16_t _hoverItemTicks;     // 0x009C68C7
    static GraphSettings _graphSettings; // 0x0113DC7A

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

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { frameWidth, 231 }, WindowColour::secondary),
                makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_compare_companies),
                makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_company_performance),
                makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_cargo_graphs),
                makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_cargo_distance_graphs),
                makeRemapWidget({ 127, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_company_values),
                makeRemapWidget({ 158, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_cargo_payment_rates),
                makeRemapWidget({ 189, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_speed_records));
        }

        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex);
        static void onUpdate(Window& self);
        static void prepareDraw(Window& self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void refreshCompanyList(Window* self);
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx);
        static void drawGraphAndLegend(Window* self, Gfx::DrawingContext& drawingCtx);
    }

    namespace CompanyList
    {
        static constexpr Ui::Size32 kMaxWindowSize = { 640, 470 };
        static constexpr Ui::Size32 kMinWindowSize = { 300, 272 };
        static constexpr Ui::Size32 kWindowSize = { 640, 272 };

        static constexpr uint8_t kRowHeight = 25;

        enum widx
        {
            sort_name = 11,
            sort_status,
            sort_performance,
            sort_value,
            scrollview,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << sort_name) | (1 << sort_status) | (1 << sort_performance) | (1 << sort_value) | (1 << scrollview);

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(640, 272, StringIds::title_company_list),
            makeWidget({ 4, 43 }, { 175, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_name),
            makeWidget({ 179, 43 }, { 210, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_status),
            makeWidget({ 389, 43 }, { 145, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_performance),
            makeWidget({ 534, 43 }, { 100, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_company_value),
            makeWidget({ 3, 56 }, { 634, 201 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical)

        );

        enum SortMode : uint16_t
        {
            Name,
            Status,
            Performance,
            Value,
        };

        // 0x004360A2
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_company_list:
                case Common::widx::tab_performance:
                case Common::widx::tab_cargo_units:
                case Common::widx::tab_cargo_distance:
                case Common::widx::tab_values:
                case Common::widx::tab_payment_rates:
                case Common::widx::tab_speed_records:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case sort_name:
                case sort_status:
                case sort_performance:
                case sort_value:
                {
                    auto sortMode = widgetIndex - widx::sort_name;
                    if (self.sortMode == sortMode)
                        return;

                    self.sortMode = sortMode;
                    self.invalidate();
                    self.var_83C = 0;
                    self.rowHover = -1;

                    Common::refreshCompanyList(&self);
                    break;
                }
            }
        }

        // 0x004363CB
        static void onResize(Window& self)
        {
            self.setSize(kMinWindowSize, kMaxWindowSize);
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
                FormatArguments args{};
                auto statusString = CompanyManager::getOwnerStatus(lhs.id(), args);
                StringManager::formatString(lhsString, statusString, args);
            }

            char rhsString[256] = { 0 };
            {
                FormatArguments args{};
                auto statusString = CompanyManager::getOwnerStatus(rhs.id(), args);
                StringManager::formatString(rhsString, statusString, args);
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
                if ((company.challengeFlags & CompanyFlags::sorted) != CompanyFlags::none)
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
        static void onUpdate(Window& self)
        {
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::companyList, self.number, self.currentTab + Common::widx::tab_company_list);

            _hoverItemTicks++;

            // Add three companies every tick.
            updateCompanyList(&self);
            updateCompanyList(&self);
            updateCompanyList(&self);
        }

        // 0x004362F7
        static void event_08(Window& self)
        {
            self.flags |= WindowFlags::notScrollView;
        }

        // 0x004362FF
        static void event_09(Window& self)
        {
            if (!self.hasFlags(WindowFlags::notScrollView))
                return;

            if (self.rowHover == -1)
                return;

            self.rowHover = -1;
            self.invalidate();
        }

        // 0x00436321
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = self.var_83C * kRowHeight;
        }

        // 0x004363A0
        static void onScrollMouseDown(Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            uint16_t currentRow = y / kRowHeight;
            if (currentRow > self.var_83C)
                return;

            CompanyId currentCompany = CompanyId(self.rowInfo[currentRow]);
            if (currentCompany == CompanyId::null)
                return;

            CompanyWindow::open(currentCompany);
        }

        // 0x00436361
        static void onScrollMouseOver(Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            self.flags &= ~(WindowFlags::notScrollView);

            uint16_t currentRow = y / kRowHeight;
            int16_t currentCompany = -1;

            if (currentRow < self.var_83C)
                currentCompany = self.rowInfo[currentRow];

            if (self.rowHover == currentCompany)
                return;

            self.rowHover = currentCompany;
            self.invalidate();
        }

        // 0x004362B6
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_company_list);
            return args;
        }

        // 0x0043632C
        static Ui::CursorId cursor(Window& self, WidgetIndex_t widgetIdx, [[maybe_unused]] int16_t xPos, int16_t yPos, Ui::CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / kRowHeight;
            if (currentIndex < self.var_83C && self.rowInfo[currentIndex] != -1)
                return CursorId::handPointer;

            return fallback;
        }

        // 0x00435D07
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            // Reposition header buttons
            self.widgets[widx::sort_name].right = std::min(178, self.width - 8);

            self.widgets[widx::sort_status].left = std::min(179, self.width - 8);
            self.widgets[widx::sort_status].right = std::min(388, self.width - 8);

            self.widgets[widx::sort_performance].left = std::min(389, self.width - 8);
            self.widgets[widx::sort_performance].right = std::min(533, self.width - 8);

            self.widgets[widx::sort_value].left = std::min(534, self.width - 8);
            self.widgets[widx::sort_value].right = std::min(633, self.width - 8);

            // Set header button captions
            self.widgets[widx::sort_name].text = self.sortMode == SortMode::Name ? StringIds::table_header_company_name_desc : StringIds::table_header_company_name;
            self.widgets[widx::sort_status].text = self.sortMode == SortMode::Status ? StringIds::table_header_company_status_desc : StringIds::table_header_company_status;
            self.widgets[widx::sort_performance].text = self.sortMode == SortMode::Performance ? StringIds::table_header_company_performance_desc : StringIds::table_header_company_performance;
            self.widgets[widx::sort_value].text = self.sortMode == SortMode::Value ? StringIds::table_header_company_value_desc : StringIds::table_header_company_value;
        }

        // 0x00435E56
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            FormatArguments args{};
            if (self.var_83C == 1)
                args.push(StringIds::company_singular);
            else
                args.push(StringIds::companies_plural);

            args.push(self.var_83C);

            auto point = Point(self.x + 3, self.y + self.height - 13);
            tr.drawStringLeft(point, Colour::black, StringIds::black_stringid, args);
        }

        // 0x00435EA7
        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            const auto& rt = drawingCtx.currentRenderTarget();
            auto tr = Gfx::TextRenderer(drawingCtx);

            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(colour);

            auto yBottom = 0;
            for (auto i = 0; i < self.var_83C; i++, yBottom += 25)
            {
                auto yTop = yBottom + 25;
                if (yTop <= rt.y)
                    continue;

                yTop = rt.y + rt.height;
                if (yBottom >= yTop)
                    break;

                auto rowItem = self.rowInfo[i];
                if (rowItem == -1)
                    continue;

                auto stringId = StringIds::black_stringid;

                if (rowItem == self.rowHover)
                {
                    drawingCtx.drawRect(0, yBottom, self.width, 24, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);

                    stringId = StringIds::wcolour2_stringid;
                }

                auto company = CompanyManager::get(CompanyId(rowItem));
                auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);
                auto imageId = Gfx::recolour(competitorObj->images[enumValue(company->ownerEmotion)], company->mainColours.primary);

                {
                    FormatArguments args{};
                    args.push(StringIds::table_item_company);
                    args.push(imageId);
                    args.push(company->name);

                    auto point = Point(0, yBottom - 1);
                    tr.drawStringLeftClipped(point, 173, Colour::black, stringId, args);
                }

                {
                    FormatArguments args{};
                    args.skip(sizeof(StringId));
                    StringId ownerStatus = CompanyManager::getOwnerStatus(company->id(), args);
                    args.rewind();
                    args.push(ownerStatus);

                    auto point = Point(175, yBottom + 7);
                    tr.drawStringLeftClipped(point, 208, Colour::black, stringId, args);
                }

                auto performanceStringId = StringIds::performance_index;

                if ((company->challengeFlags & CompanyFlags::increasedPerformance) != CompanyFlags::none && (company->challengeFlags & CompanyFlags::decreasedPerformance) != CompanyFlags::none)
                {
                    performanceStringId = StringIds::performance_index_decrease;

                    if ((company->challengeFlags & CompanyFlags::increasedPerformance) != CompanyFlags::none)
                    {
                        performanceStringId = StringIds::performance_index_increase;
                    }
                }

                {
                    FormatArguments args{};

                    args.push(performanceStringId);
                    formatPerformanceIndex(company->performanceIndex, args);

                    auto point = Point(385, yBottom - 1);
                    tr.drawStringLeftClipped(point, 143, Colour::black, stringId, args);
                }

                {
                    FormatArguments args{};

                    args.push(StringIds::company_value_currency);
                    args.push(company->companyValueHistory[0]);

                    auto point = Point(530, yBottom - 1);
                    tr.drawStringLeftClipped(point, 98, Colour::black, stringId, args);
                }
            }
        }

        // 0x00436198
        static void tabReset(Window* self)
        {
            self->minWidth = kMinWindowSize.width;
            self->minHeight = kMinWindowSize.height;
            self->maxWidth = kMaxWindowSize.width;
            self->maxHeight = kMaxWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
            self->var_83C = 0;
            self->rowHover = -1;
            Common::refreshCompanyList(self);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = onUpdate,
            .event_08 = event_08,
            .event_09 = event_09,
            .getScrollSize = getScrollSize,
            .scrollMouseDown = onScrollMouseDown,
            .scrollMouseOver = onScrollMouseOver,
            .tooltip = tooltip,
            .cursor = cursor,
            .prepareDraw = prepareDraw,
            .draw = draw,
            .drawScroll = drawScroll,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x00435BC8
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::companyList);

        if (window != nullptr)
        {
            if (ToolManager::isToolActive(ToolManager::getToolWindowType(), ToolManager::getToolWindowNumber()))
            {
                ToolManager::toolCancel();
                window = WindowManager::bringToFront(WindowType::companyList);
            }
        }

        if (window == nullptr)
        {
            static constexpr Ui::Size32 kWindowSize = { 640, 272 };

            window = WindowManager::createWindow(WindowType::companyList, kWindowSize, WindowFlags::none, CompanyList::getEvents());

            window->frameNo = 0;
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
        window->minWidth = CompanyList::kMinWindowSize.width;
        window->minHeight = CompanyList::kMinWindowSize.height;
        window->maxWidth = CompanyList::kMaxWindowSize.width;
        window->maxHeight = CompanyList::kMaxWindowSize.height;

        window->invalidate();

        window->setWidgets(CompanyList::widgets);
        window->enabledWidgets = CompanyList::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &CompanyList::getEvents();
        window->activatedWidgets = 0;
        window->initScrollWidgets();

        return window;
    }

    void removeCompany(CompanyId id)
    {
        auto* w = WindowManager::find(WindowType::companyList);
        if (w != nullptr)
        {
            for (auto i = 0; i < w->var_83C; i++)
            {
                if (static_cast<CompanyId>(w->rowInfo[i]) == id)
                {
                    w->rowInfo[i] = -1;
                }
            }
        }
        WindowManager::invalidate(WindowType::companyList);
    }

    // 0x00435C69
    void openPerformanceIndexes()
    {
        auto window = open();
        window->callOnMouseUp(Common::widx::tab_performance);
    }

    namespace CompanyPerformance
    {
        static constexpr Ui::Size32 kWindowSize = { 635, 322 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(635, 322, StringIds::title_company_performance)

        );

        // 0x004366D7
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x00436490
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            _graphSettings.left = self.x + 4;
            _graphSettings.top = self.y + self.widgets[Common::widx::panel].top + 4;
            _graphSettings.width = 520;
            _graphSettings.height = self.height - self.widgets[Common::widx::panel].top - 8;
            _graphSettings.yOffset = 17;
            _graphSettings.xOffset = 40;
            _graphSettings.yAxisLabelIncrement = 20;
            _graphSettings.linesToExclude = 0;

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

                _graphSettings.yData[count] = reinterpret_cast<std::byte*>(&company.performanceIndexHistory[0]);
                _graphSettings.dataStart[count] = maxHistorySize - company.historySize;
                _graphSettings.lineColour[count] = Colours::getShade(companyColour, 6);
                _graphSettings.itemId[count] = enumValue(companyId);
                count++;
            }

            _graphSettings.lineCount = count;
            _graphSettings.dataEnd = maxHistorySize;
            _graphSettings.dataTypeSize = 2;
            _graphSettings.xLabel = StringIds::rawdate_short;
            _graphSettings.yLabel = StringIds::percentage_one_decimal_place;
            _graphSettings.xAxisTickIncrement = 4;
            _graphSettings.xAxisLabelIncrement = 12;
            _graphSettings.dword_113DD86 = 0;
            _graphSettings.yAxisStepSize = 100;
            _graphSettings.flags = GraphFlags::dataFrontToBack;

            Common::drawGraphAndLegend(&self, drawingCtx);
        }

        // 0x004361D8
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxHeight = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .prepareDraw = Common::prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace CargoUnits
    {
        static constexpr Ui::Size32 kWindowSize = { 640, 272 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(635, 322, StringIds::title_company_cargo_units)

        );

        // 0x004369FB
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x004367B4
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            _graphSettings.left = self.x + 4;
            _graphSettings.top = self.y + self.widgets[Common::widx::panel].top + 4;
            _graphSettings.width = 525;
            _graphSettings.height = self.height - self.widgets[Common::widx::panel].top - 8;
            _graphSettings.yOffset = 17;
            _graphSettings.xOffset = 45;
            _graphSettings.yAxisLabelIncrement = 25;
            _graphSettings.linesToExclude = 0;

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

                _graphSettings.yData[count] = reinterpret_cast<std::byte*>(&company.cargoUnitsDeliveredHistory[0]);
                _graphSettings.dataStart[count] = maxHistorySize - company.historySize;
                _graphSettings.lineColour[count] = Colours::getShade(companyColour, 6);
                _graphSettings.itemId[count] = enumValue(companyId);
                count++;
            }

            _graphSettings.lineCount = count;
            _graphSettings.dataEnd = maxHistorySize;
            _graphSettings.dataTypeSize = 4;
            _graphSettings.xLabel = StringIds::rawdate_short;
            _graphSettings.yLabel = StringIds::cargo_units_delivered;
            _graphSettings.xAxisTickIncrement = 4;
            _graphSettings.xAxisLabelIncrement = 12;
            _graphSettings.dword_113DD86 = 0;
            _graphSettings.yAxisStepSize = 1000;
            _graphSettings.flags = GraphFlags::dataFrontToBack;

            Common::drawGraphAndLegend(&self, drawingCtx);
        }

        // 0x00436201
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxHeight = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .prepareDraw = Common::prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace CargoDistance
    {
        static constexpr Ui::Size32 kWindowSize = { 660, 272 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(635, 322, StringIds::title_cargo_distance_graphs)

        );

        // 0x00436D1F
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x00436AD8
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            _graphSettings.left = self.x + 4;
            _graphSettings.top = self.y + self.widgets[Common::widx::panel].top + 4;
            _graphSettings.width = 545;
            _graphSettings.height = self.height - self.widgets[Common::widx::panel].top - 8;
            _graphSettings.yOffset = 17;
            _graphSettings.xOffset = 65;
            _graphSettings.yAxisLabelIncrement = 25;
            _graphSettings.linesToExclude = 0;

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

                _graphSettings.yData[count] = reinterpret_cast<std::byte*>(&company.cargoUnitsDistanceHistory[0]);
                _graphSettings.dataStart[count] = maxHistorySize - company.historySize;
                _graphSettings.lineColour[count] = Colours::getShade(companyColour, 6);
                _graphSettings.itemId[count] = enumValue(companyId);
                count++;
            }

            _graphSettings.lineCount = count;
            _graphSettings.dataEnd = maxHistorySize;
            _graphSettings.dataTypeSize = 4;
            _graphSettings.xLabel = StringIds::rawdate_short;
            _graphSettings.yLabel = StringIds::cargo_units_delivered;
            _graphSettings.xAxisTickIncrement = 4;
            _graphSettings.xAxisLabelIncrement = 12;
            _graphSettings.dword_113DD86 = 0;
            _graphSettings.yAxisStepSize = 1000;
            _graphSettings.flags = GraphFlags::dataFrontToBack;

            Common::drawGraphAndLegend(&self, drawingCtx);
        }

        // 0x00436227
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxHeight = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .prepareDraw = Common::prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace CompanyValues
    {
        static constexpr Ui::Size32 kWindowSize = { 685, 322 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(685, 322, StringIds::title_company_values)

        );

        // 0x00437043
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x00436DFC
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            _graphSettings.left = self.x + 4;
            _graphSettings.top = self.y + self.widgets[Common::widx::panel].top + 4;
            _graphSettings.width = 570;
            _graphSettings.height = self.height - self.widgets[Common::widx::panel].top - 8;
            _graphSettings.yOffset = 17;
            _graphSettings.xOffset = 90;
            _graphSettings.yAxisLabelIncrement = 25;
            _graphSettings.linesToExclude = 0;

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

                _graphSettings.yData[count] = reinterpret_cast<std::byte*>(&company.companyValueHistory[0]);
                _graphSettings.dataStart[count] = maxHistorySize - company.historySize;
                _graphSettings.lineColour[count] = Colours::getShade(companyColour, 6);
                _graphSettings.itemId[count] = enumValue(companyId);
                count++;
            }

            _graphSettings.lineCount = count;
            _graphSettings.dataEnd = maxHistorySize;
            _graphSettings.dataTypeSize = 6;
            _graphSettings.xLabel = StringIds::rawdate_short;
            _graphSettings.yLabel = StringIds::small_company_value_currency;
            _graphSettings.xAxisTickIncrement = 4;
            _graphSettings.xAxisLabelIncrement = 12;
            _graphSettings.dword_113DD86 = 0;
            _graphSettings.yAxisStepSize = 10000;
            _graphSettings.flags = GraphFlags::dataFrontToBack;

            Common::drawGraphAndLegend(&self, drawingCtx);
        }

        // 0x0043624D
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxHeight = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .prepareDraw = Common::prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace CargoPaymentRates
    {
        static constexpr Ui::Size32 kWindowSize = { 495, 342 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(495, 342, StringIds::title_cargo_payment_rates)

        );

        // 0x0043737D
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x004F9442
        static constexpr Colour _cargoLineColour[32] = {
            Colour::red,
            Colour::mutedPurple,
            Colour::yellow,
            Colour::blue,
            Colour::orange,
            Colour::green,
            Colour::mutedDarkRed,
            Colour::mutedDarkTeal,
            Colour::mutedDarkYellow,
            Colour::black,
            Colour::white,
            Colour::mutedDarkPurple,
            Colour::purple,
            Colour::darkBlue,
            Colour::mutedTeal,
            Colour::darkGreen,
            Colour::mutedSeaGreen,
            Colour::mutedGrassGreen,
            Colour::mutedAvocadoGreen,
            Colour::mutedOliveGreen,
            Colour::darkYellow,
            Colour::amber,
            Colour::grey,
            Colour::darkOrange,
            Colour::mutedYellow,
            Colour::brown,
            Colour::mutedOrange,
            Colour::darkRed,
            Colour::darkPink,
            Colour::pink,
            Colour::mutedRed,
            Colour::grey,
        };

        // 0x00437949
        static void drawGraphLegend(Window* self, Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

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

                if (!(self->var_854 & (1 << cargoCount)) || !(_hoverItemTicks & (1 << 2)))
                {
                    drawingCtx.fillRect(x, y + 3, x + 4, y + 7, palette, Gfx::RectFlags::none);
                }

                auto args = FormatArguments();
                args.push(cargo->name);

                auto point = Point(x + 6, y);
                tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

                y += 10;
                cargoCount++;
            }
        }

        // 0x00437120
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            _graphSettings.left = self.x + 4;
            _graphSettings.top = self.y + self.widgets[Common::widx::panel].top + 14;
            _graphSettings.width = 380;
            _graphSettings.height = self.height - self.widgets[Common::widx::panel].top - 28;
            _graphSettings.yOffset = 17;
            _graphSettings.xOffset = 80;
            _graphSettings.yAxisLabelIncrement = 25;
            _graphSettings.linesToExclude = 0;

            auto count = 0;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                auto cargo = ObjectManager::get<CargoObject>(i);
                if (cargo == nullptr)
                    continue;

                auto colour = _cargoLineColour[i];
                auto* deliveredCargoPayment = Economy::getDeliveryCargoPaymentsTable(i);

                _graphSettings.yData[count] = reinterpret_cast<std::byte*>(deliveredCargoPayment);
                _graphSettings.dataStart[count] = 0;
                _graphSettings.lineColour[count] = Colours::getShade(colour, 6);
                _graphSettings.itemId[count] = i;
                count++;
            }

            _graphSettings.lineCount = count;
            _graphSettings.dataEnd = 60;
            _graphSettings.dataTypeSize = 4;
            _graphSettings.xLabel = StringIds::cargo_delivered_days;
            _graphSettings.yLabel = StringIds::cargo_delivered_currency;
            _graphSettings.xAxisTickIncrement = 5;
            _graphSettings.xAxisLabelIncrement = 20;
            _graphSettings.dword_113DD86 = 0;
            _graphSettings.yAxisStepSize = 0;
            _graphSettings.flags = GraphFlags::none;

            _graphSettings.xAxisRange = 2;
            _graphSettings.xAxisStepSize = 2;
            _graphSettings.pointFlags = GraphPointFlags::drawLines;

            Ui::drawGraph(_graphSettings, &self, drawingCtx);

            if (self.var_854 != 0)
            {
                auto i = 0;
                while (Numerics::bitScanForward(self.var_854) != _graphSettings.itemId[i])
                {
                    i++;
                }

                // Exclude all lines except highlighted data
                _graphSettings.linesToExclude = 0xFFFFFFFF & ~(1 << i);

                if (_hoverItemTicks & 4)
                    _graphSettings.lineColour[i] = PaletteIndex::black0;

                _graphSettings.flags |= GraphFlags::hideAxesAndLabels;

                Ui::drawGraph(_graphSettings, &self, drawingCtx);
            }

            {
                auto x = self.width + self.x - 104;
                auto y = self.y + 52;

                drawGraphLegend(&self, drawingCtx, x, y);
            }

            {
                auto point = Point(self.x + 8, self.widgets[Common::widx::panel].top + self.y + 1);

                FormatArguments args{};
                args.push<uint16_t>(100);
                args.push<uint16_t>(10);

                tr.drawStringLeft(point, Colour::black, StringIds::cargo_deliver_graph_title, args);
            }

            {
                auto point = Point(self.x + 160, self.height + self.y - 13);

                tr.drawStringLeft(point, Colour::black, StringIds::cargo_transit_time);
            }
        }

        // 0x004379F2
        static void setLegendHover(Window* self, int16_t x, int16_t y)
        {
            uint32_t selectedCargo = 0;
            if (!Input::hasFlag(Input::Flags::rightMousePressed))
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
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxHeight = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
            Economy::buildDeliveredCargoPaymentsTable();
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .prepareDraw = Common::prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace CompanySpeedRecords
    {
        static constexpr Ui::Size32 kWindowSize = { 495, 169 };

        const uint64_t enabledWidgets = Common::enabledWidgets;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(495, 169, StringIds::title_speed_records)

        );

        // 0x00437591
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x0043745A
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            auto y = self.y + 47;

            for (auto i = 0; i < 3; i++)
            {
                auto recordSpeed = CompanyManager::getRecords().speed[i];
                if (recordSpeed == 0_mph)
                    continue;
                {
                    FormatArguments args{};
                    args.push(recordSpeed);

                    const StringId string[] = {
                        StringIds::land_speed_record,
                        StringIds::air_speed_record,
                        StringIds::water_speed_record,
                    };

                    auto point = Point(self.x + 4, y);
                    tr.drawStringLeft(point, Colour::black, string[i], args);
                }
                y += 11;

                auto companyId = CompanyManager::getRecords().company[i];

                if (companyId != CompanyId::null)
                {
                    auto company = CompanyManager::get(companyId);
                    auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);

                    auto imageId = competitorObj->images[enumValue(company->ownerEmotion)];
                    imageId = Gfx::recolour(imageId, company->mainColours.primary);

                    auto x = self.x + 4;
                    drawingCtx.drawImage(x, y, imageId);

                    y += 7;
                    auto point = Point(self.x + 33, y);

                    FormatArguments args{};
                    args.push(company->name);
                    args.push<uint16_t>(0);
                    args.push(CompanyManager::getRecords().date[i]);

                    tr.drawStringLeft(point, Colour::black, StringIds::record_date_achieved, args);
                    y += 17;
                }

                y += 5;
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .prepareDraw = Common::prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t enabledWidgets;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { CompanyList::widgets,         widx::tab_company_list,   CompanyList::getEvents(),         CompanyList::enabledWidgets },
            { CompanyPerformance::widgets,  widx::tab_performance,    CompanyPerformance::getEvents(),  CompanyPerformance::enabledWidgets },
            { CargoUnits::widgets,          widx::tab_cargo_units,    CargoUnits::getEvents(),          CargoUnits::enabledWidgets },
            { CargoDistance::widgets,       widx::tab_cargo_distance, CargoDistance::getEvents(),       CargoDistance::enabledWidgets },
            { CompanyValues::widgets,       widx::tab_values,         CompanyValues::getEvents(),       CompanyValues::enabledWidgets },
            { CargoPaymentRates::widgets,   widx::tab_payment_rates,  CargoPaymentRates::getEvents(),   CargoPaymentRates::enabledWidgets },
            { CompanySpeedRecords::widgets, widx::tab_speed_records,  CompanySpeedRecords::getEvents(), CompanySpeedRecords::enabledWidgets },
        };
        // clang-format on

        // 0x0043667B
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_company_list:
                case Common::widx::tab_performance:
                case Common::widx::tab_cargo_units:
                case Common::widx::tab_cargo_distance:
                case Common::widx::tab_values:
                case Common::widx::tab_payment_rates:
                case Common::widx::tab_speed_records:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x004378BA
        static void setLegendHover(Window* self, int16_t x, int16_t y)
        {
            uint32_t selectedCompany = 0;
            if (!Input::hasFlag(Input::Flags::rightMousePressed))
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
        static void onUpdate(Window& self)
        {
            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self.number, self.currentTab + Common::widx::tab_company_list);

            auto x = self.width - 104 + self.x;
            auto y = self.y + 52;

            switch (self.currentTab + widx::tab_company_list)
            {
                case widx::tab_cargo_distance:
                case widx::tab_cargo_units:
                case widx::tab_performance:
                case widx::tab_values:
                {
                    _hoverItemTicks++;
                    setLegendHover(&self, x, y);
                    break;
                }
                case widx::tab_payment_rates:
                {
                    _hoverItemTicks++;
                    CargoPaymentRates::setLegendHover(&self, x, y);
                    break;
                }
                case widx::tab_speed_records:
                    break;
            }
        }

        // 0x00436419
        static void prepareDraw(Window& self)
        {
            // Activate the current tab
            self.activatedWidgets &= ~((1ULL << tab_cargo_distance) | (1ULL << tab_cargo_units) | (1ULL << tab_company_list) | (1ULL << tab_payment_rates) | (1ULL << tab_performance) | (1ULL << tab_speed_records) | (1ULL << tab_values));
            self.activatedWidgets |= (1ULL << Common::tabInformationByTabOffset[self.currentTab].widgetIndex);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;
        }

        // 0x004360FA
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self->type, self->number))
                ToolManager::toolCancel();

            self->currentTab = widgetIndex - widx::tab_company_list;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_company_list];

            self->enabledWidgets = tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = &tabInfo.events;
            self->activatedWidgets = 0;
            self->setWidgets(tabInfo.widgets);

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
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Company List Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_companies;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_company_list);
            }

            // Performance Index Tab
            {
                static constexpr uint32_t performanceImageIds[] = {
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
                    imageId += performanceImageIds[(self->frameNo / 4) % std::size(performanceImageIds)];
                else
                    imageId += performanceImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_performance);
            }

            // Cargo Unit Tab
            {
                static constexpr uint32_t cargoUnitsImageIds[] = {
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
                    imageId += cargoUnitsImageIds[(self->frameNo / 4) % std::size(cargoUnitsImageIds)];
                else
                    imageId += cargoUnitsImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo_units);
            }

            // Cargo Distance Tab
            {
                static constexpr uint32_t cargoDistanceImageIds[] = {
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
                    imageId += cargoDistanceImageIds[(self->frameNo / 4) % std::size(cargoDistanceImageIds)];
                else
                    imageId += cargoDistanceImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo_distance);
            }

            // Company Values Tab
            {
                static constexpr uint32_t companyValuesImageIds[] = {
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
                    imageId += companyValuesImageIds[(self->frameNo / 4) % std::size(companyValuesImageIds)];
                else
                    imageId += companyValuesImageIds[0];

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_values);

                if (!(self->isDisabled(widx::tab_values)))
                {
                    auto& widget = self->widgets[widx::tab_values];
                    auto point = Point(widget.left + self->x + 28, widget.top + self->y + 14 + 1);
                    tr.drawStringRight(point, Colour::black, StringIds::currency_symbol);
                }
            }

            // Payment Rates Tab
            {

                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_cargo_payment_rates;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_payment_rates);

                if (!(self->isDisabled(widx::tab_payment_rates)))
                {
                    auto& widget = self->widgets[widx::tab_payment_rates];
                    auto point = Point(widget.left + self->x + 28, widget.top + self->y + 14 + 1);
                    tr.drawStringRight(point, Colour::black, StringIds::currency_symbol);
                }
            }

            // Speed Records Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_awards;

                imageId = Gfx::recolour(imageId, self->getColour(WindowColour::secondary).c());

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_speed_records);
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

        // 0x00437810
        static void drawGraphLegend(Window* self, Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

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

                if (!(self->var_854 & (1 << companyCount)) || !(_hoverItemTicks & (1 << 2)))
                {
                    drawingCtx.fillRect(x, y + 3, x + 4, y + 7, colour, Gfx::RectFlags::none);
                }

                FormatArguments args{};
                args.push(company.name);

                auto point = Point(x + 6, y);
                tr.drawStringLeftClipped(point, 94, Colour::black, stringId, args);

                y += 10;
                companyCount++;
            }
        }

        // 0x004365E4
        static void drawGraphAndLegend(Window* self, Gfx::DrawingContext& drawingCtx)
        {
            auto totalMonths = (getCurrentYear() * 12) + static_cast<uint16_t>(getCurrentMonth());

            _graphSettings.xAxisRange = totalMonths;
            _graphSettings.xAxisStepSize = 1;
            _graphSettings.pointFlags = GraphPointFlags::drawLines;

            Ui::drawGraph(_graphSettings, self, drawingCtx);

            if (self->var_854 != 0)
            {
                auto i = 0;
                auto bitScan = Numerics::bitScanForward(self->var_854);
                while (bitScan != _graphSettings.itemId[i] && bitScan != -1)
                {
                    i++;
                }

                // Exclude all except hilighted data
                _graphSettings.linesToExclude = 0xFFFFFFFF & ~(1 << i);

                if (_hoverItemTicks & (1 << 2))
                    _graphSettings.lineColour[i] = 10;

                _graphSettings.flags |= GraphFlags::hideAxesAndLabels;

                Ui::drawGraph(_graphSettings, self, drawingCtx);
            }

            auto x = self->width + self->x - 104;
            auto y = self->y + 52;

            Common::drawGraphLegend(self, drawingCtx, x, y);
        }
    }
}
