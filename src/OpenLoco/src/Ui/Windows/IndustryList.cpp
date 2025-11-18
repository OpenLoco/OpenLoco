#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Industries/CreateIndustry.h"
#include "GameCommands/Industries/RemoveIndustry.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/MapSelection.h"
#include "Map/TileManager.h"
#include "Objects/CargoObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Random.h"
#include "SceneManager.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/Widgets/TableHeaderWidget.h"
#include "Ui/WindowManager.h"
#include "World/IndustryManager.h"
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco::Ui::Windows::IndustryList
{
    static Core::Prng _placementPrng;           // 0x00E0C394
    static currency32_t _industryPlacementCost; // 0x00E0C39C
    static bool _industryGhostPlaced;           // 0x00E0C3D9
    static World::Pos2 _industryGhostPos;       // 0x00E0C3C2
    static uint8_t _industryGhostType;          // 0x00E0C3DA
    static IndustryId _industryGhostId;         // 0x00E0C3DB

    namespace Common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_industry_list,
            tab_new_industry,
        };

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                Widgets::Caption({ 1, 1 }, { frameWidth - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { frameWidth, 154 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_industries_list),
                Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_fund_new_industries));
        }

        static void refreshIndustryList(Window* self);
        static void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx);
        static void prepareDraw(Window& self);
        static void switchTab(Window& self, WidgetIndex_t widgetIndex);
    }

    namespace IndustryList
    {
        static constexpr Ui::Size32 kWindowSize = { 759, 197 };
        static constexpr Ui::Size kMaxDimensions = { 759, 900 };
        static constexpr Ui::Size kMinDimensions = { 192, 100 };

        static constexpr uint8_t kRowHeight = 10;

        enum widx
        {
            sort_industry_name = 6,
            sort_industry_status,
            sort_industry_production_transported,
            sort_industry_production_last_month,
            scrollview,
            status_bar,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(600, 197, StringIds::title_industries),
            Widgets::TableHeader({ 4, 44 }, { 199, 11 }, WindowColour::secondary, Widget::kContentNull, StringIds::sort_industry_name),
            Widgets::TableHeader({ 204, 44 }, { 204, 11 }, WindowColour::secondary, Widget::kContentNull, StringIds::sort_industry_status),
            Widgets::TableHeader({ 444, 44 }, { 159, 11 }, WindowColour::secondary, Widget::kContentNull, StringIds::sort_industry_production_transported),
            Widgets::TableHeader({ 603, 44 }, { 159, 11 }, WindowColour::secondary, Widget::kContentNull, StringIds::sort_industry_production_last_month),
            Widgets::ScrollView({ 3, 56 }, { 593, 125 }, WindowColour::secondary, Scrollbars::vertical),
            Widgets::Label({ 4, kWindowSize.height - 17 }, { kWindowSize.width, 10 }, WindowColour::secondary, ContentAlign::left, StringIds::black_stringid)

        );

        enum SortMode : uint16_t
        {
            Name,
            Status,
            ProductionTransported,
            ProductionLastMonth,
        };

        // 0x00457B94
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            // Reposition header buttons.
            self.widgets[widx::sort_industry_name].right = std::min(self.width - 4, 203);

            self.widgets[widx::sort_industry_status].left = std::min(self.width - 4, 204);
            self.widgets[widx::sort_industry_status].right = std::min(self.width - 4, 443);

            self.widgets[widx::sort_industry_production_transported].left = std::min(self.width - 4, 444);
            self.widgets[widx::sort_industry_production_transported].right = std::min(self.width - 4, 603);

            self.widgets[widx::sort_industry_production_last_month].left = std::min(self.width - 4, 603);
            self.widgets[widx::sort_industry_production_last_month].right = std::min(self.width - 4, 762);

            // Set header button captions.
            self.widgets[widx::sort_industry_name].text = self.sortMode == SortMode::Name ? StringIds::industry_table_header_desc : StringIds::industry_table_header;
            self.widgets[widx::sort_industry_status].text = self.sortMode == SortMode::Status ? StringIds::industry_table_header_status_desc : StringIds::industry_table_header_status;
            self.widgets[widx::sort_industry_production_transported].text = self.sortMode == SortMode::ProductionTransported ? StringIds::industry_table_header_production_desc : StringIds::industry_table_header_production;
            self.widgets[widx::sort_industry_production_last_month].text = self.sortMode == SortMode::ProductionLastMonth ? StringIds::industry_table_header_production_last_month_desc : StringIds::industry_table_header_production_last_month;

            if (SceneManager::isEditorMode() || SceneManager::isSandboxMode())
            {
                self.widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_build_new_industries;
            }
            else
            {
                self.widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_fund_new_industries;
            }

            // Reposition status bar
            auto& widget = self.widgets[widx::status_bar];
            widget.top = self.height - 12;
            widget.bottom = self.height - 2;

            // Set status bar text
            FormatArguments args{ widget.textArgs };
            args.push(self.var_83C == 1 ? StringIds::status_num_industries_singular : StringIds::status_num_industries_plural);
            args.push(self.var_83C);
        }

        // 0x00457CD9
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);
        }

        // 0x00457EC4
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_industry_list:
                case Common::widx::tab_new_industry:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::sort_industry_name:
                case widx::sort_industry_status:
                case widx::sort_industry_production_transported:
                case widx::sort_industry_production_last_month:
                {
                    auto sortMode = widgetIndex - widx::sort_industry_name;
                    if (self.sortMode == sortMode)
                    {
                        return;
                    }

                    self.sortMode = sortMode;
                    self.invalidate();
                    self.var_83C = 0;
                    self.rowHover = -1;

                    Common::refreshIndustryList(&self);
                    break;
                }
            }
        }

        // 0x00458172
        static void onScrollMouseDown(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            uint16_t currentRow = y / kRowHeight;
            if (currentRow > self.var_83C)
            {
                return;
            }

            const auto currentIndustry = IndustryId(self.rowInfo[currentRow]);
            if (currentIndustry == IndustryId::null)
            {
                return;
            }

            Industry::open(currentIndustry);
        }

        // 0x00458140
        static void onScrollMouseOver(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            self.flags &= ~(WindowFlags::notScrollView);

            uint16_t currentRow = y / kRowHeight;
            int16_t currentIndustry = -1;

            if (currentRow < self.var_83C)
            {
                currentIndustry = self.rowInfo[currentRow];
            }

            self.rowHover = currentIndustry;
            self.invalidate();
        }

        // 0x00457A52
        static bool orderByName(const OpenLoco::Industry& lhs, const OpenLoco::Industry& rhs)
        {
            char lhsString[256] = { 0 };
            {
                FormatArguments args{};
                args.push(lhs.town);

                StringManager::formatString(lhsString, lhs.name, args);
            }

            char rhsString[256] = { 0 };
            {
                FormatArguments args{};
                args.push(rhs.town);

                StringManager::formatString(rhsString, rhs.name, args);
            }

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00457A9F
        static bool orderByStatus(OpenLoco::Industry& lhs, OpenLoco::Industry& rhs)
        {
            char lhsString[256] = { 0 };
            const char* lhsBuffer = StringManager::getString(StringIds::buffer_1250);
            lhs.getStatusString((char*)lhsBuffer);

            StringManager::formatString(lhsString, StringIds::buffer_1250);

            char rhsString[256] = { 0 };
            const char* rhsBuffer = StringManager::getString(StringIds::buffer_1250);
            rhs.getStatusString((char*)rhsBuffer);

            StringManager::formatString(rhsString, StringIds::buffer_1250);

            return strcmp(lhsString, rhsString) < 0;
        }

        static uint8_t getAverageTransportedCargo(const OpenLoco::Industry& industry)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(industry.objectId);
            uint8_t productionTransported = 0xFFU;

            if (industryObj->producesCargo())
            {
                productionTransported = industry.producedCargoPercentTransportedPreviousMonth[0];
                if (industryObj->producedCargoType[1] != 0xFF)
                {
                    productionTransported = industry.producedCargoPercentTransportedPreviousMonth[1];
                    if (industryObj->producedCargoType[0] != 0xFF)
                    {
                        productionTransported += industry.producedCargoPercentTransportedPreviousMonth[0];
                        productionTransported /= 2;
                    }
                }
            }
            return productionTransported;
        }

        // 0x00457AF3
        static bool orderByProductionTransported(const OpenLoco::Industry& lhs, const OpenLoco::Industry& rhs)
        {
            auto lhsVar = getAverageTransportedCargo(lhs);

            auto rhsVar = getAverageTransportedCargo(rhs);

            return rhsVar < lhsVar;
        }

        static std::pair<uint32_t, StringId> getProductionLastMonth(const OpenLoco::Industry& industry)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(industry.objectId);
            auto cargoProduction = std::numeric_limits<uint32_t>::max();
            StringId unitType = StringIds::empty;

            if (industryObj->producesCargo())
            {
                auto cargoNumber = 0;
                for (const auto& producedCargoType : industryObj->producedCargoType)
                {
                    if (producedCargoType != kCargoTypeNull)
                    {
                        cargoProduction = industry.producedCargoQuantityPreviousMonth[cargoNumber];

                        auto cargoObj = ObjectManager::get<CargoObject>(producedCargoType);
                        unitType = cargoProduction > 1 ? cargoObj->unitNamePlural : cargoObj->unitNameSingular;
                        break; // Only support one cargo type for now - it seems most (all?) industries only have a single cargo type
                    }

                    cargoNumber++;
                }
            }

            return std::make_pair(cargoProduction, unitType);
        }

        static bool orderByProductionLastMonth(const OpenLoco::Industry& lhs, const OpenLoco::Industry& rhs)
        {
            return getProductionLastMonth(rhs).first < getProductionLastMonth(lhs).first;
        }

        // 0x00457A52, 0x00457A9F, 0x00457AF3
        static bool getOrder(const SortMode mode, OpenLoco::Industry& lhs, OpenLoco::Industry& rhs)
        {
            switch (mode)
            {
                case SortMode::Name:
                    return orderByName(lhs, rhs);

                case SortMode::Status:
                    return orderByStatus(lhs, rhs);

                case SortMode::ProductionTransported:
                    return orderByProductionTransported(lhs, rhs);

                case SortMode::ProductionLastMonth:
                    return orderByProductionLastMonth(lhs, rhs);
            }

            return false;
        }

        // 0x00457991
        static void updateIndustryList(Window* self)
        {
            auto chosenIndustry = IndustryId::null;

            for (auto& industry : IndustryManager::industries())
            {
                if (industry.hasFlags(IndustryFlags::sorted))
                {
                    continue;
                }

                if (chosenIndustry == IndustryId::null)
                {
                    chosenIndustry = industry.id();
                    continue;
                }

                if (getOrder(SortMode(self->sortMode), industry, *IndustryManager::get(chosenIndustry)))
                {
                    chosenIndustry = industry.id();
                }
            }

            if (chosenIndustry != IndustryId::null)
            {
                bool shouldInvalidate = false;

                IndustryManager::get(chosenIndustry)->flags |= IndustryFlags::sorted;

                auto ebp = self->rowCount;
                if (chosenIndustry != IndustryId(self->rowInfo[ebp]))
                {
                    self->rowInfo[ebp] = enumValue(chosenIndustry);
                    shouldInvalidate = true;
                }

                self->rowCount += 1;
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

                Common::refreshIndustryList(self);
            }
        }

        // 0x004580AE
        static void onUpdate(Window& self)
        {
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::industryList, self.number, self.currentTab + Common::widx::tab_industry_list);

            // Add three industries every tick.
            updateIndustryList(&self);
            updateIndustryList(&self);
            updateIndustryList(&self);
        }

        // 0x00457EE8
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_industry_list);
            return args;
        }

        // 0x00458108
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            scrollHeight = kRowHeight * self.var_83C;
        }

        // 0x00457D2A
        static void drawScroll(Ui::Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            const auto& rt = drawingCtx.currentRenderTarget();
            auto tr = Gfx::TextRenderer(drawingCtx);

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 4);
            drawingCtx.clearSingle(shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                IndustryId industryId = IndustryId(self.rowInfo[i]);

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + kRowHeight < rt.y || yPos >= yPos + kRowHeight + rt.height || industryId == IndustryId::null)
                {
                    yPos += kRowHeight;
                    continue;
                }

                StringId text_colour_id = StringIds::black_stringid;

                // Highlight selection.
                if (industryId == IndustryId(self.rowHover))
                {
                    drawingCtx.drawRect(0, yPos, self.width, kRowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                    text_colour_id = StringIds::wcolour2_stringid;
                }

                if (industryId == IndustryId::null)
                {
                    continue;
                }
                auto industry = IndustryManager::get(industryId);

                // Industry Name
                {
                    FormatArguments args{};
                    args.push(industry->name);
                    args.push(industry->town);

                    auto point = Point(0, yPos);
                    tr.drawStringLeftClipped(point, 198, Colour::black, text_colour_id, args);
                }
                // Industry Status
                {
                    const char* buffer = StringManager::getString(StringIds::buffer_1250);
                    industry->getStatusString((char*)buffer);

                    FormatArguments args{};
                    args.push(StringIds::buffer_1250);

                    auto point = Point(200, yPos);
                    tr.drawStringLeftClipped(point, 238, Colour::black, text_colour_id, args);
                }
                if (industry->canProduceCargo())
                {
                    // Industry Production Delivered
                    {
                        auto productionTransported = getAverageTransportedCargo(*industry);

                        FormatArguments args{};
                        args.push<uint16_t>(productionTransported);

                        auto point = Point(440, yPos);
                        tr.drawStringLeftClipped(point, 138, Colour::black, StringIds::production_transported_percent, args);
                    }
                    // Industry Production Last Month
                    {
                        auto productionTransported = getProductionLastMonth(*industry);

                        FormatArguments args{};
                        args.push(productionTransported.second);
                        args.push<uint32_t>(productionTransported.first);

                        auto point = Point(600, yPos);
                        tr.drawStringLeftClipped(point, 138, Colour::black, StringIds::black_stringid, args);
                    }
                }

                yPos += kRowHeight;
            }
        }

        // 0x00458113
        static Ui::CursorId cursor(Window& self, WidgetIndex_t widgetIdx, [[maybe_unused]] const WidgetId id, [[maybe_unused]] int16_t xPos, int16_t yPos, Ui::CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
            {
                return fallback;
            }

            uint16_t currentIndex = yPos / kRowHeight;
            if (currentIndex < self.var_83C && self.rowInfo[currentIndex] != -1)
            {
                return CursorId::handPointer;
            }

            return fallback;
        }

        // 0x004580DE
        static void event_08(Window& self)
        {
            self.flags |= WindowFlags::notScrollView;
        }

        // 0x004580E6
        static void event_09(Window& self)
        {
            if (!self.hasFlags(WindowFlags::notScrollView))
            {
                return;
            }

            if (self.rowHover == -1)
            {
                return;
            }

            self.rowHover = -1;
            self.invalidate();
        }

        // 0x00457FCA
        static void tabReset(Window& self)
        {
            self.invalidate();
            self.minWidth = kMinDimensions.width;
            self.minHeight = kMinDimensions.height;
            self.maxWidth = kMaxDimensions.width;
            self.maxHeight = kMaxDimensions.height;
            self.width = kWindowSize.width;
            self.height = kWindowSize.height;
            self.var_83C = 0;
            self.rowHover = -1;
            Common::refreshIndustryList(&self);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
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

    // 0x004577FF
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::industryList, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_industry_list, window->widgets[Common::widx::tab_industry_list].id);
        }
        else
        {
            // 0x00457878
            auto origin = Ui::Point32(Ui::width() - IndustryList::kWindowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::industryList,
                origin,
                IndustryList::kWindowSize,
                WindowFlags::flag_8,
                IndustryList::getEvents());

            window->number = 0;
            window->currentTab = 0;
            window->frameNo = 0;
            window->sortMode = 0;
            window->var_83C = 0;
            window->rowHover = -1;

            Common::refreshIndustryList(window);

            WindowManager::moveOtherWindowsDown(*window);

            window->minWidth = IndustryList::kMinDimensions.width;
            window->minHeight = IndustryList::kMinDimensions.height;
            window->maxWidth = IndustryList::kMaxDimensions.width;
            window->maxHeight = IndustryList::kMaxDimensions.height;
            window->flags |= WindowFlags::resizable;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, skin->windowTitlebarColour);
            window->setColour(WindowColour::secondary, skin->windowColour);

            // 0x00457878 end

            window->width = IndustryList::kWindowSize.width;
            window->height = IndustryList::kWindowSize.height;

            window->invalidate();

            window->setWidgets(IndustryList::widgets);
            window->activatedWidgets = 0;
            window->holdableWidgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();
        }
        return window;
    }

    void reset()
    {
        getGameState().lastIndustryOption = 0xFF;
    }

    // 0x0045792A
    void removeIndustry(const IndustryId id)
    {
        auto* wnd = WindowManager::find(WindowType::industryList);
        if (wnd == nullptr)
        {
            return;
        }
        if (wnd->currentTab != Common::widx::tab_industry_list - Common::widx::tab_industry_list)
        {
            return;
        }

        for (auto i = 0; i < wnd->var_83C; ++i)
        {
            if (static_cast<IndustryId>(wnd->rowInfo[i]) == id)
            {
                wnd->rowInfo[i] = enumValue(IndustryId::null);
            }
        }
    }

    namespace NewIndustries
    {
        static constexpr Ui::Size32 kWindowSize = { 578, 172 };

        static constexpr uint8_t kRowHeight = 112;

        enum widx
        {
            scrollview = 6,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(577, 171, StringIds::title_fund_new_industries),
            Widgets::ScrollView({ 3, 45 }, { 549, 111 }, WindowColour::secondary, Scrollbars::vertical)

        );

        // 0x0045819F
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            if (SceneManager::isEditorMode() || SceneManager::isSandboxMode())
            {
                self.widgets[Common::widx::caption].text = StringIds::title_build_new_industries;
                self.widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_build_new_industries;
            }
            else
            {
                self.widgets[Common::widx::caption].text = StringIds::title_fund_new_industries;
                self.widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_fund_new_industries;
            }
        }

        // 0x0045826C
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);

            if (self.var_83C == 0)
            {
                auto point = Point(self.x + 3, self.y + self.height - 13);
                auto width = self.width - 19;
                tr.drawStringLeftClipped(point, width, Colour::black, StringIds::no_industry_available);
                return;
            }

            auto industryObjId = self.var_846;

            if (industryObjId == 0xFFFF)
            {
                industryObjId = self.rowHover;

                if (industryObjId == 0xFFFF)
                {
                    return;
                }
            }

            auto industryObj = ObjectManager::get<IndustryObject>(industryObjId);
            auto industryCost = 0;

            if (self.var_846 == 0xFFFF)
            {
                industryCost = _industryPlacementCost;
            }

            if ((self.var_846 == 0xFFFF && _industryPlacementCost == static_cast<currency32_t>(0x80000000)) || self.var_846 != 0xFFFF)
            {
                industryCost = Economy::getInflationAdjustedCost(industryObj->costFactor, industryObj->costIndex, 3);
            }

            auto widthOffset = 0;

            if (!SceneManager::isEditorMode() && !SceneManager::isSandboxMode())
            {
                FormatArguments args{};
                args.push(industryCost);

                auto point = Point(self.x + 3 + self.width - 19, self.y + self.height - 13);
                widthOffset = 138;

                tr.drawStringRight(point, Colour::black, StringIds::build_cost, args);
            }

            {
                FormatArguments args{};
                args.push(industryObj->name);

                auto point = Point(self.x + 3, self.y + self.height - 13);
                auto width = self.width - 19 - widthOffset;

                tr.drawStringLeftClipped(point, width, Colour::black, StringIds::black_stringid, args);
            }
        }

        // 0x0045843A
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_industry_list:
                case Common::widx::tab_new_industry:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 112) + (y / kRowHeight) * 5;
        }

        // 0x00458966
        static void onScrollMouseDown(Ui::Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
        {
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self.var_83C; i++)
            {
                auto rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    self.rowHover = rowInfo;
                    getGameState().lastIndustryOption = rowInfo;

                    int32_t pan = (self.width >> 1) + self.x;
                    Audio::playSound(Audio::SoundId::clickDown, pan);
                    self.expandContentCounter = -16;
                    _industryPlacementCost = 0x80000000;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x00458721
        static void onScrollMouseOver(Ui::Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = 0xFFFF;
            auto i = 0;

            for (; i < self.var_83C; i++)
            {
                rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    break;
                }
            }
            if (i >= self.var_83C)
            {
                rowInfo = 0xFFFF;
            }
            self.var_846 = rowInfo;
            self.invalidate();
            auto string = StringIds::buffer_337;

            if (rowInfo == 0xFFFF)
            {
                string = StringIds::null;
            }

            if (StringManager::getString(StringIds::buffer_337)[0] != '\0')
            {
                if (string == self.widgets[widx::scrollview].tooltip)
                {
                    if (rowInfo == self.var_85C)
                    {
                        return;
                    }
                }
            }
            self.widgets[widx::scrollview].tooltip = string;
            self.var_85C = rowInfo;
            ToolTip::closeAndReset();

            if (rowInfo == 0xFFFF)
            {
                return;
            }

            auto industryObj = ObjectManager::get<IndustryObject>(rowInfo);
            auto buffer = const_cast<char*>(StringManager::getString(string));
            char* ptr = (char*)buffer;

            *ptr = '\0';
            *ptr++ = ControlCodes::Font::regular;
            *ptr++ = ControlCodes::Colour::black;

            if (industryObj->producesCargo())
            {
                ptr = StringManager::formatString(ptr, StringIds::industry_produces);
                ptr = industryObj->getProducedCargoString(ptr);

                if (industryObj->requiresCargo())
                {
                    ptr = StringManager::formatString(ptr, StringIds::cargo_comma);
                }
            }

            if (industryObj->requiresCargo())
            {
                ptr = StringManager::formatString(ptr, StringIds::industry_requires);
                ptr = industryObj->getRequiredCargoString(ptr);
            }
        }

        // 0x004585B8
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::rightMousePressed))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == &self)
                {
                    xPos -= self.x;
                    xPos += 26;
                    yPos -= self.y;

                    if ((yPos < 42) || (xPos <= self.width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self.findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self.expandContentCounter += 1;
                            if (self.expandContentCounter >= 8)
                            {
                                auto y = std::min(self.scrollAreas[0].contentHeight - 1 + 60, 500);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self.minWidth = kWindowSize.width;
                                self.minHeight = y;
                                self.maxWidth = kWindowSize.width;
                                self.maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self.minWidth = kWindowSize.width;
                                    self.minHeight = kWindowSize.height;
                                    self.maxWidth = kWindowSize.width;
                                    self.maxHeight = kWindowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self.expandContentCounter = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self.minWidth = kWindowSize.width;
                        self.minHeight = kWindowSize.height;
                        self.maxWidth = kWindowSize.width;
                        self.maxHeight = kWindowSize.height;
                    }
                }
            }
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::industryList, self.number, self.currentTab + Common::widx::tab_industry_list);

            if (!ToolManager::isToolActive(self.type, self.number))
            {
                WindowManager::close(&self);
            }
        }

        // 0x00458455
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_new_industry_list);
            return args;
        }

        // 0x004586EA
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            scrollHeight = (4 + self.var_83C) / 5;
            if (scrollHeight == 0)
            {
                scrollHeight += 1;
            }
            scrollHeight *= kRowHeight;
        }

        // 0x00458352
        static void drawScroll(Ui::Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            const auto& rt = drawingCtx.currentRenderTarget();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 4);
            drawingCtx.clearSingle(shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                if (yPos + kRowHeight < rt.y)
                {
                    xPos += kRowHeight;
                    if (xPos >= kRowHeight * 5) // full row
                    {
                        xPos = 0;
                        yPos += kRowHeight;
                    }
                    continue;
                }
                else if (yPos > rt.y + rt.height)
                {
                    break;
                }

                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        drawingCtx.drawRectInset(xPos, yPos, kRowHeight, kRowHeight, self.getColour(WindowColour::secondary), Gfx::RectInsetFlags::colourLight);
                    }
                }
                else
                {
                    drawingCtx.drawRectInset(xPos, yPos, kRowHeight, kRowHeight, self.getColour(WindowColour::secondary), (Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::borderInset));
                }

                auto industryObj = ObjectManager::get<IndustryObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(xPos + 1, yPos + 1, 110, 110));
                if (clipped)
                {
                    drawingCtx.pushRenderTarget(*clipped);
                    industryObj->drawIndustry(drawingCtx, 56, 96);
                    drawingCtx.popRenderTarget();
                }

                xPos += kRowHeight;

                if (xPos >= kRowHeight * 5) // full row
                {
                    xPos = 0;
                    yPos += kRowHeight;
                }
            }
        }

        // 0x00458708
        static void event_08(Window& self)
        {
            if (self.var_846 != 0xFFFF)
            {
                self.var_846 = 0xFFFF;
            }
            self.invalidate();
        }

        // 0x00458C09
        static void removeIndustryGhost()
        {
            if (_industryGhostPlaced)
            {
                _industryGhostPlaced = false;
                GameCommands::IndustryRemovalArgs args;
                args.industryId = _industryGhostId;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
        }

        // 0x00458BB5
        static currency32_t placeIndustryGhost(const GameCommands::IndustryPlacementArgs& placementArgs)
        {
            auto res = GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            if (res == GameCommands::FAILURE)
            {
                return res;
            }
            _industryGhostPos = placementArgs.pos;
            _industryGhostType = placementArgs.type;
            _industryGhostId = GameCommands::getLegacyReturnState().lastPlacedIndustryId;
            _industryGhostPlaced = true;
            return res;
        }

        // 0x0045857B
        static std::optional<GameCommands::IndustryPlacementArgs> getIndustryPlacementArgsFromCursor(const int16_t x, const int16_t y)
        {
            auto* industryListWnd = WindowManager::find(WindowType::industryList);
            if (industryListWnd == nullptr)
            {
                return {};
            }

            if (industryListWnd->currentTab != (Common::widx::tab_new_industry - Common::widx::tab_industry_list))
            {
                return {};
            }

            if (industryListWnd->rowHover == -1)
            {
                return {};
            }

            const auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y }); // ax,cx
            if (!pos)
            {
                return {};
            }

            GameCommands::IndustryPlacementArgs args;
            args.pos = *pos;
            args.type = industryListWnd->rowHover; // dl
            args.srand0 = _placementPrng.srand_0();
            args.srand1 = _placementPrng.srand_1();
            if (SceneManager::isEditorMode())
            {
                args.buildImmediately = true; // bh
            }
            return { args };
        }

        // 0x0045848A
        static void onToolUpdate(Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, const int16_t y)
        {
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
            auto placementArgs = getIndustryPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeIndustryGhost();
                return;
            }

            // Always show buildings, not scaffolding, for ghost placements.
            placementArgs->buildImmediately = true;

            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            World::setMapSelectionCorner(MapSelectionType::full);
            World::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            World::mapInvalidateSelectionRect();

            if (_industryGhostPlaced)
            {
                if (_industryGhostPos == placementArgs->pos && _industryGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeIndustryGhost();
            auto cost = placeIndustryGhost(*placementArgs);
            if (cost != _industryPlacementCost)
            {
                _industryPlacementCost = cost;
                self.invalidate();
            }
        }

        // 0x0045851F
        static void onToolDown([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, const int16_t y)
        {
            removeIndustryGhost();
            auto placementArgs = getIndustryPlacementArgsFromCursor(x, y);
            if (placementArgs)
            {
                GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);
                if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply) != GameCommands::FAILURE)
                {
                    Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
                }
            }

            gPrng2().randNext();
            _placementPrng = gPrng2();
        }

        // 0x004585AD
        static void onToolAbort([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            removeIndustryGhost();
            Ui::Windows::Main::hideGridlines();
        }

        // 0x0045845F
        static void onClose(Window& self)
        {
            if (ToolManager::isToolActive(self.type, self.number))
            {
                ToolManager::toolCancel();
            }
        }

        // 0x00458B51
        static void updateActiveThumb(Window& self)
        {
            int32_t scrollWidth = 0, scrollHeight = 0;
            self.callGetScrollSize(0, scrollWidth, scrollHeight);
            self.scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self.var_83C; i++)
            {
                if (self.rowInfo[i] == self.rowHover)
                {
                    break;
                }
            }

            if (i >= self.var_83C)
            {
                i = 0;
            }

            i = (i / 5) * kRowHeight;

            self.scrollAreas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x00458AAF
        static void updateBuildableIndustries(Window& self)
        {
            auto industryCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
            {
                auto industryObj = ObjectManager::get<IndustryObject>(i);
                if (industryObj == nullptr)
                {
                    continue;
                }

                if (!SceneManager::isEditorMode() && !SceneManager::isSandboxMode())
                {
                    if (!industryObj->hasFlags(IndustryObjectFlags::canBeFoundedByPlayer))
                    {
                        continue;
                    }
                    if (getCurrentYear() < industryObj->designedYear)
                    {
                        continue;
                    }
                    if (getCurrentYear() > industryObj->obsoleteYear)
                    {
                        continue;
                    }
                }
                self.rowInfo[industryCount] = i;
                industryCount++;
            }

            self.var_83C = industryCount;
            auto rowHover = -1;

            auto lastIndustryOption = getGameState().lastIndustryOption;
            if (lastIndustryOption != 0xFF)
            {
                for (auto i = 0; i < self.var_83C; i++)
                {
                    if (lastIndustryOption == self.rowInfo[i])
                    {
                        rowHover = lastIndustryOption;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self.var_83C != 0)
            {
                rowHover = self.rowInfo[0];
            }

            self.rowHover = rowHover;
            updateActiveThumb(self);
        }

        // 0x00457FFE
        static void tabReset(Window& self)
        {
            self.minWidth = NewIndustries::kWindowSize.width;
            self.minHeight = NewIndustries::kWindowSize.height;
            self.maxWidth = NewIndustries::kWindowSize.width;
            self.maxHeight = NewIndustries::kWindowSize.height;
            ToolManager::toolSet(self, Common::widx::tab_new_industry, CursorId::placeFactory);

            Input::setFlag(Input::Flags::flag6);
            Ui::Windows::Main::showGridlines();
            _industryGhostPlaced = false;
            _industryPlacementCost = 0x80000000;

            self.var_83C = 0;
            self.rowHover = -1;
            self.var_846 = 0xFFFFU;

            updateBuildableIndustries(self);

            gPrng2().randNext();
            _placementPrng = gPrng2();
        }

        // 0x004589E8
        static void onResize(Window& self)
        {
            self.invalidate();
            Ui::Size32 kMinWindowSize = { self.minWidth, self.minHeight };
            Ui::Size32 kMaxWindowSize = { self.maxWidth, self.maxHeight };
            bool hasResized = self.setSize(kMinWindowSize, kMaxWindowSize);
            if (hasResized)
            {
                updateActiveThumb(self);
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = onUpdate,
            .event_08 = event_08,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .onToolAbort = onToolAbort,
            .getScrollSize = getScrollSize,
            .scrollMouseDown = onScrollMouseDown,
            .scrollMouseOver = onScrollMouseOver,
            .tooltip = tooltip,
            .prepareDraw = prepareDraw,
            .draw = draw,
            .drawScroll = drawScroll,
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
        };

        static TabInformation tabInformationByTabOffset[] = {
            { IndustryList::widgets, widx::tab_industry_list, IndustryList::getEvents() },
            { NewIndustries::widgets, widx::tab_new_industry, NewIndustries::getEvents() },
        };

        // 0x00457B94
        static void prepareDraw(Window& self)
        {
            // Activate the current tab..
            self.activatedWidgets &= ~((1ULL << tab_industry_list) | (1ULL << tab_new_industry));
            self.activatedWidgets |= (1ULL << tabInformationByTabOffset[self.currentTab].widgetIndex);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;
        }

        // 0x00457F27
        static void switchTab(Window& self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self.type, self.number))
            {
                ToolManager::toolCancel();
            }

            self.currentTab = widgetIndex - widx::tab_industry_list;
            self.frameNo = 0;
            self.flags &= ~(WindowFlags::flag_16);

            self.viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_industry_list];

            self.holdableWidgets = 0;
            self.eventHandlers = &tabInfo.events;
            self.activatedWidgets = 0;
            self.setWidgets(tabInfo.widgets);

            if (self.currentTab == widx::tab_industry_list - widx::tab_industry_list)
            {
                IndustryList::tabReset(self);
            }
            if (self.currentTab == widx::tab_new_industry - widx::tab_industry_list)
            {
                NewIndustries::tabReset(self);
            }

            self.callOnResize();
            self.callPrepareDraw();
            self.initScrollWidgets();
            self.invalidate();
            self.moveInsideScreenEdges();
        }

        // 0x00458A57
        void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Industry List Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_industries;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_industry_list);
            }

            // Fund New Industries Tab
            {
                static constexpr uint32_t fundNewIndustriesImageIds[] = {
                    InterfaceSkin::ImageIds::build_industry_frame_0,
                    InterfaceSkin::ImageIds::build_industry_frame_1,
                    InterfaceSkin::ImageIds::build_industry_frame_2,
                    InterfaceSkin::ImageIds::build_industry_frame_3,
                    InterfaceSkin::ImageIds::build_industry_frame_4,
                    InterfaceSkin::ImageIds::build_industry_frame_5,
                    InterfaceSkin::ImageIds::build_industry_frame_6,
                    InterfaceSkin::ImageIds::build_industry_frame_7,
                    InterfaceSkin::ImageIds::build_industry_frame_8,
                    InterfaceSkin::ImageIds::build_industry_frame_9,
                    InterfaceSkin::ImageIds::build_industry_frame_10,
                    InterfaceSkin::ImageIds::build_industry_frame_11,
                    InterfaceSkin::ImageIds::build_industry_frame_12,
                    InterfaceSkin::ImageIds::build_industry_frame_13,
                    InterfaceSkin::ImageIds::build_industry_frame_14,
                    InterfaceSkin::ImageIds::build_industry_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self.currentTab == widx::tab_new_industry - widx::tab_industry_list)
                {
                    imageId += fundNewIndustriesImageIds[(self.frameNo / 2) % std::size(fundNewIndustriesImageIds)];
                }
                else
                {
                    imageId += fundNewIndustriesImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_new_industry);
            }
        }

        // 0x00457964
        static void refreshIndustryList(Window* window)
        {
            window->rowCount = 0;

            for (auto& industry : IndustryManager::industries())
            {
                industry.flags &= ~IndustryFlags::sorted;
            }
        }
    }
}
