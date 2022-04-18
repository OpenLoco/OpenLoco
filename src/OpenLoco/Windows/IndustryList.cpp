#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Date.h"
#include "../Economy/Economy.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Map.hpp"
#include "../Map/TileManager.h"
#include "../Objects/CargoObject.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::IndustryList
{
    static loco_global<currency32_t, 0x00E0C39C> dword_E0C39C;
    static loco_global<bool, 0x00E0C3D9> _industryGhostPlaced;
    static loco_global<Map::Pos2, 0x00E0C3C2> _industryGhostPos;
    static loco_global<uint8_t, 0x00E0C3C9> _industryLastPlacedId;
    static loco_global<uint8_t, 0x00E0C3DA> _industryGhostType;
    static loco_global<uint8_t, 0x00E0C3DB> _industryGhostId;
    static loco_global<uint8_t, 0x00525FC7> _lastSelectedIndustry;
    static loco_global<Utility::prng, 0x00525E20> _prng;
    static loco_global<uint32_t, 0x00E0C394> _dword_E0C394;
    static loco_global<uint32_t, 0x00E0C398> _dword_E0C398;

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

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_industry_list) | (1 << widx::tab_new_industry);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 154 }, WidgetType::panel, WindowColour::secondary),                                                                      \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_industries_list),                         \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_fund_new_industries)

        static WindowEventList _events;

        static void initEvents();
        static void refreshIndustryList(Window* self);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void prepareDraw(Window* self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
    }

    namespace IndustryList
    {
        static const Ui::Size windowSize = { 600, 197 };
        static const Ui::Size maxDimensions = { 600, 900 };
        static const Ui::Size minDimensions = { 192, 100 };

        static const uint8_t rowHeight = 10;

        enum widx
        {
            sort_industry_name = 6,
            sort_industry_status,
            sort_industry_production_transported,
            scrollview,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << sort_industry_name) | (1 << sort_industry_status) | (1 << sort_industry_production_transported) | (1 << scrollview);

        Widget widgets[] = {
            commonWidgets(600, 197, StringIds::title_industries),
            makeWidget({ 4, 44 }, { 199, 11 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::sort_industry_name),
            makeWidget({ 204, 44 }, { 204, 11 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::sort_industry_status),
            makeWidget({ 444, 44 }, { 159, 11 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::sort_industry_production_transported),
            makeWidget({ 3, 56 }, { 593, 125 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd(),
        };

        static WindowEventList events;

        enum SortMode : uint16_t
        {
            Name,
            Status,
            ProductionTransported,
        };

        // 0x00457B94
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            // Reposition header buttons.
            self->widgets[widx::sort_industry_name].right = std::min(self->width - 4, 203);

            self->widgets[widx::sort_industry_status].left = std::min(self->width - 4, 204);
            self->widgets[widx::sort_industry_status].right = std::min(self->width - 4, 443);

            self->widgets[widx::sort_industry_production_transported].left = std::min(self->width - 4, 444);
            self->widgets[widx::sort_industry_production_transported].right = std::min(self->width - 4, 603);

            // Set header button captions.
            self->widgets[widx::sort_industry_name].text = self->sortMode == SortMode::Name ? StringIds::industry_table_header_desc : StringIds::industry_table_header;
            self->widgets[widx::sort_industry_status].text = self->sortMode == SortMode::Status ? StringIds::industry_table_header_status_desc : StringIds::industry_table_header_status;
            self->widgets[widx::sort_industry_production_transported].text = self->sortMode == SortMode::ProductionTransported ? StringIds::industry_table_header_production_desc : StringIds::industry_table_header_production;

            if (isEditorMode() || isSandboxMode())
                self->widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_build_new_industries;
            else
                self->widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_fund_new_industries;
        }

        // 0x00457CD9
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
            auto args = FormatArguments();
            auto xPos = self->x + 4;
            auto yPos = self->y + self->height - 12;

            if (self->var_83C == 1)
                args.push(StringIds::status_num_industries_singular);
            else
                args.push(StringIds::status_num_industries_plural);
            args.push(self->var_83C);

            Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x00457EC4
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_industry_list:
                case Common::widx::tab_new_industry:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::sort_industry_name:
                case widx::sort_industry_status:
                case widx::sort_industry_production_transported:
                {
                    auto sortMode = widgetIndex - widx::sort_industry_name;
                    if (self->sortMode == sortMode)
                        return;

                    self->sortMode = sortMode;
                    self->invalidate();
                    self->var_83C = 0;
                    self->rowHover = -1;

                    Common::refreshIndustryList(self);
                    break;
                }
            }
        }

        // 0x00458172
        static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / rowHeight;
            if (currentRow > self->var_83C)
                return;

            const auto currentIndustry = IndustryId(self->rowInfo[currentRow]);
            if (currentIndustry == IndustryId::null)
                return;

            Industry::open(currentIndustry);
        }

        // 0x00458140
        static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(WindowFlags::notScrollView);

            uint16_t currentRow = y / rowHeight;
            int16_t currentIndustry = -1;

            if (currentRow < self->var_83C)
                currentIndustry = self->rowInfo[currentRow];

            self->rowHover = currentIndustry;
            self->invalidate();
        }

        // 0x00457A52
        static bool orderByName(const OpenLoco::Industry& lhs, const OpenLoco::Industry& rhs)
        {
            char lhsString[256] = { 0 };
            StringManager::formatString(lhsString, lhs.name, (void*)&lhs.town);

            char rhsString[256] = { 0 };
            StringManager::formatString(rhsString, rhs.name, (void*)&rhs.town);

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
            auto industryObj = ObjectManager::get<IndustryObject>(industry.object_id);
            uint8_t productionTransported = -1;

            if (industryObj->producesCargo())
            {
                productionTransported = industry.produced_cargo_transported[0];
                if (industryObj->produced_cargo_type[1] != 0xFF)
                {
                    productionTransported = industry.produced_cargo_transported[1];
                    if (industryObj->produced_cargo_type[0] != 0xFF)
                    {
                        productionTransported += industry.produced_cargo_transported[0];
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
            }

            return false;
        }

        // 0x00457991
        static void updateIndustryList(Window* self)
        {
            auto chosenIndustry = IndustryId::null;

            for (auto& industry : IndustryManager::industries())
            {
                if ((industry.flags & IndustryFlags::sorted) != 0)
                    continue;

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
        static void onUpdate(Window* self)
        {
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::industryList, self->number, self->currentTab + Common::widx::tab_industry_list);

            // Add three industries every tick.
            updateIndustryList(self);
            updateIndustryList(self);
            updateIndustryList(self);
        }

        // 0x00457EE8
        static std::optional<FormatArguments> tooltip(Ui::Window* self, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_industry_list);
            return args;
        }

        // 0x00458108
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = rowHeight * self->var_83C;
        }

        // 0x00457D2A
        static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 4);
            Gfx::clearSingle(context, shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                IndustryId industryId = IndustryId(self.rowInfo[i]);

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + rowHeight < context.y || yPos >= yPos + rowHeight + context.height || industryId == IndustryId::null)
                {
                    yPos += rowHeight;
                    continue;
                }

                string_id text_colour_id = StringIds::black_stringid;

                // Highlight selection.
                if (industryId == IndustryId(self.rowHover))
                {
                    Gfx::drawRect(context, 0, yPos, self.width, rowHeight, 0x2000030);
                    text_colour_id = StringIds::wcolour2_stringid;
                }

                if (industryId == IndustryId::null)
                    continue;
                auto industry = IndustryManager::get(industryId);

                // Industry Name
                {
                    auto args = FormatArguments();
                    args.push(industry->name);
                    args.push(industry->town);

                    Gfx::drawString_494BBF(context, 0, yPos, 198, Colour::black, text_colour_id, &args);
                }
                // Industry Status
                {
                    const char* buffer = StringManager::getString(StringIds::buffer_1250);
                    industry->getStatusString((char*)buffer);

                    auto args = FormatArguments();
                    args.push(StringIds::buffer_1250);

                    Gfx::drawString_494BBF(context, 200, yPos, 238, Colour::black, text_colour_id, &args);
                }
                // Industry Production Delivered
                {
                    if (!industry->canProduceCargo())
                    {
                        yPos += rowHeight;
                        continue;
                    }

                    auto productionTransported = getAverageTransportedCargo(*industry);

                    auto args = FormatArguments();
                    args.push<uint16_t>(productionTransported);

                    Gfx::drawString_494BBF(context, 440, yPos, 138, Colour::black, StringIds::production_transported_percent, &args);
                }
                yPos += rowHeight;
            }
        }

        // 0x00458113
        static Ui::CursorId cursor(Window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->rowInfo[currentIndex] != -1)
                return CursorId::handPointer;

            return fallback;
        }

        // 0x004580DE
        static void event_08(Window* self)
        {
            self->flags |= WindowFlags::notScrollView;
        }

        // 0x004580E6
        static void event_09(Window* self)
        {
            if ((self->flags & WindowFlags::notScrollView) == 0)
                return;

            if (self->rowHover == -1)
                return;

            self->rowHover = -1;
            self->invalidate();
        }

        // 0x00457FCA
        static void tabReset(Window* self)
        {
            self->minWidth = minDimensions.width;
            self->minHeight = minDimensions.height;
            self->maxWidth = maxDimensions.width;
            self->maxHeight = maxDimensions.height;
            self->var_83C = 0;
            self->rowHover = -1;
            Common::refreshIndustryList(self);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.cursor = cursor;
            events.drawScroll = drawScroll;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.getScrollSize = getScrollSize;
            events.onMouseUp = onMouseUp;
            events.onUpdate = onUpdate;
            events.scrollMouseDown = onScrollMouseDown;
            events.scrollMouseOver = onScrollMouseOver;
            events.prepareDraw = prepareDraw;
            events.tooltip = tooltip;
        }
    }

    // 0x004577FF
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::industryList, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_industry_list);
        }
        else
        {
            // 0x00457878
            auto origin = Ui::Point(Ui::width() - IndustryList::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::industryList,
                origin,
                IndustryList::windowSize,
                WindowFlags::flag_8,
                &IndustryList::events);

            window->number = 0;
            window->currentTab = 0;
            window->frame_no = 0;
            window->sortMode = 0;
            window->var_83C = 0;
            window->rowHover = -1;

            Common::refreshIndustryList(window);

            WindowManager::sub_4CEE0B(window);

            window->minWidth = IndustryList::minDimensions.width;
            window->minHeight = IndustryList::minDimensions.height;
            window->maxWidth = IndustryList::maxDimensions.width;
            window->maxHeight = IndustryList::maxDimensions.height;
            window->flags |= WindowFlags::resizable;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, skin->colour_0B);
            window->setColour(WindowColour::secondary, skin->colour_0C);

            // 0x00457878 end

            // TODO: only needs to be called once.
            window->width = IndustryList::windowSize.width;
            window->height = IndustryList::windowSize.height;

            Common::initEvents();

            window->invalidate();

            window->widgets = IndustryList::widgets;
            window->enabledWidgets = IndustryList::enabledWidgets;

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
        _lastSelectedIndustry = 0xFF;
    }

    namespace NewIndustries
    {

        static const Ui::Size window_size = { 578, 172 };

        static const uint8_t rowHeight = 112;

        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview);

        Widget widgets[] = {
            commonWidgets(577, 171, StringIds::title_fund_new_industries),
            makeWidget({ 3, 45 }, { 549, 111 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0045819F
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            if (isEditorMode() || isSandboxMode())
            {
                self->widgets[Common::widx::caption].text = StringIds::title_build_new_industries;
                self->widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_build_new_industries;
            }
            else
            {
                self->widgets[Common::widx::caption].text = StringIds::title_fund_new_industries;
                self->widgets[Common::widx::tab_new_industry].tooltip = StringIds::tooltip_fund_new_industries;
            }
        }

        // 0x0045826C
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            if (self->var_83C == 0)
            {
                auto xPos = self->x + 3;
                auto yPos = self->y + self->height - 13;
                auto width = self->width - 19;
                Gfx::drawString_494BBF(*context, xPos, yPos, width, Colour::black, StringIds::no_industry_available);
                return;
            }

            auto industryObjId = self->var_846;

            if (industryObjId == 0xFFFF)
            {
                industryObjId = self->rowHover;

                if (industryObjId == 0xFFFF)
                    return;
            }

            auto industryObj = ObjectManager::get<IndustryObject>(industryObjId);
            auto industryCost = 0;

            if (self->var_846 == 0xFFFF)
                industryCost = dword_E0C39C;

            if ((self->var_846 == 0xFFFF && dword_E0C39C == static_cast<currency32_t>(0x80000000)) || self->var_846 != 0xFFFF)
            {
                industryCost = Economy::getInflationAdjustedCost(industryObj->cost_factor, industryObj->cost_index, 3);
            }
            auto args = FormatArguments();
            args.push(industryCost);

            auto widthOffset = 0;

            if (!isEditorMode() && !isSandboxMode())
            {
                auto xPos = self->x + 3 + self->width - 19;
                auto yPos = self->y + self->height - 13;
                widthOffset = 138;

                Gfx::drawString_494C78(*context, xPos, yPos, Colour::black, StringIds::build_cost, &args);
            }

            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            auto width = self->width - 19 - widthOffset;

            Gfx::drawString_494BBF(*context, xPos, yPos, width, Colour::black, StringIds::black_stringid, &industryObj->name);
        }

        // 0x0045843A
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_industry_list:
                case Common::widx::tab_new_industry:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 112) + (y / rowHeight) * 5;
        }

        // 0x00458966
        static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            int16_t xPos = (x / rowHeight);
            int16_t yPos = (y / rowHeight) * 5;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                {
                    self->rowHover = rowInfo;
                    _lastSelectedIndustry = static_cast<uint8_t>(rowInfo);

                    int32_t pan = (self->width >> 1) + self->x;
                    Map::Pos3 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    Audio::playSound(Audio::SoundId::clickDown, loc, pan);
                    self->savedView.mapX = -16;
                    dword_E0C39C = 0x80000000;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x00458721
        static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = 0xFFFF;
            auto i = 0;

            for (; i < self->var_83C; i++)
            {
                rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                    break;
            }
            if (i >= self->var_83C)
                rowInfo = 0xFFFF;
            self->var_846 = rowInfo;
            self->invalidate();
            auto string = StringIds::buffer_337;

            if (rowInfo == 0xFFFF)
                string = StringIds::null;

            if (StringManager::getString(StringIds::buffer_337)[0] != '\0')
            {
                if (string == self->widgets[widx::scrollview].tooltip)
                {
                    if (rowInfo == self->var_85C)
                        return;
                }
            }
            self->widgets[widx::scrollview].tooltip = string;
            self->var_85C = rowInfo;
            ToolTip::closeAndReset();

            if (rowInfo == 0xFFFF)
                return;

            auto industryObj = ObjectManager::get<IndustryObject>(rowInfo);
            auto buffer = const_cast<char*>(StringManager::getString(string));
            char* ptr = (char*)buffer;

            *ptr = '\0';
            *ptr++ = ControlCodes::font_regular;
            *ptr++ = ControlCodes::colour_black;

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
        static void onUpdate(Window* self)
        {
            if (!Input::hasFlag(Input::Flags::flag5))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self->findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self->savedView.mapX += 1;
                            if (self->savedView.mapX >= 8)
                            {
                                auto y = std::min(self->scrollAreas[0].contentHeight - 1 + 60, 500);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self->minWidth = window_size.width;
                                self->minHeight = y;
                                self->maxWidth = window_size.width;
                                self->maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self->minWidth = window_size.width;
                                    self->minHeight = window_size.height;
                                    self->maxWidth = window_size.width;
                                    self->maxHeight = window_size.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->savedView.mapX = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self->minWidth = window_size.width;
                        self->minHeight = window_size.height;
                        self->maxWidth = window_size.width;
                        self->maxHeight = window_size.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::industryList, self->number, self->currentTab + Common::widx::tab_industry_list);

            if (!Input::isToolActive(self->type, self->number))
                WindowManager::close(self);
        }

        // 0x00458455
        static std::optional<FormatArguments> tooltip(Ui::Window* self, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_new_industry_list);
            return args;
        }

        // 0x004586EA
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 + self->var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= rowHeight;
        }

        // 0x00458352
        static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 4);
            Gfx::clearSingle(context, shade);

            loco_global<uint16_t, 0x00E0C3C6> word_E0C3C6;
            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                word_E0C3C6 = 0xFFFF;
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        word_E0C3C6 = AdvancedColour::translucent_flag;
                        Gfx::drawRectInset(context, xPos, yPos, rowHeight, rowHeight, self.getColour(WindowColour::secondary).u8(), AdvancedColour::translucent_flag);
                    }
                }
                else
                {
                    word_E0C3C6 = AdvancedColour::translucent_flag | AdvancedColour::outline_flag;
                    Gfx::drawRectInset(context, xPos, yPos, rowHeight, rowHeight, self.getColour(WindowColour::secondary).u8(), (AdvancedColour::translucent_flag | AdvancedColour::outline_flag));
                }

                auto industryObj = ObjectManager::get<IndustryObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipContext(context, Ui::Rect(xPos + 1, yPos + 1, 110, 110));
                if (clipped)
                {
                    industryObj->drawIndustry(&*clipped, 56, 96);
                }

                xPos += rowHeight;

                if (xPos >= rowHeight * 5) // full row
                {
                    xPos = 0;
                    yPos += rowHeight;
                }
            }
        }

        // 0x00458708
        static void event_08(Window* self)
        {
            if (self->var_846 != 0xFFFF)
                self->var_846 = 0xFFFF;
            self->invalidate();
        }

        // 0x00458C09
        static void removeIndustryGhost()
        {
            if (_industryGhostPlaced)
            {
                _industryGhostPlaced = false;
                GameCommands::do_48(GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6, _industryGhostId);
            }
        }

        // 0x00458BB5
        static currency32_t placeIndustryGhost(const GameCommands::IndustryPlacementArgs& placementArgs)
        {
            auto res = GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            if (res == GameCommands::FAILURE)
            {
                return res;
            }
            _industryGhostPos = placementArgs.pos;
            _industryGhostType = placementArgs.type;
            _industryGhostId = _industryLastPlacedId;
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
            args.srand0 = _dword_E0C394;
            args.srand1 = _dword_E0C398;
            if (isEditorMode())
            {
                args.buildImmediately = true; // bh
            }
            return { args };
        }

        // 0x0045848A
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, int16_t x, const int16_t y)
        {
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            auto placementArgs = getIndustryPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeIndustryGhost();
                return;
            }

            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            Map::TileManager::setMapSelectionCorner(4);
            Map::TileManager::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            Map::TileManager::mapInvalidateSelectionRect();

            if (_industryGhostPlaced)
            {
                if (*_industryGhostPos == placementArgs->pos && _industryGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeIndustryGhost();
            auto cost = placeIndustryGhost(*placementArgs);
            if (cost != dword_E0C39C)
            {
                dword_E0C39C = cost;
                self.invalidate();
            }
        }

        // 0x0045851F
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, int16_t x, const int16_t y)
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

            _prng->randNext();
            _dword_E0C394 = _prng->srand_0();
            _dword_E0C398 = _prng->srand_1();
        }

        // 0x004585AD
        static void onToolAbort(Window& self, const WidgetIndex_t widgetIndex)
        {
            removeIndustryGhost();
            Ui::Windows::hideGridlines();
        }

        // 0x0045845F
        static void onClose(Window* self)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();
        }

        // 0x00458B51
        static void updateActiveThumb(Window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->rowInfo[i] == self->rowHover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 5) * rowHeight;

            self->scrollAreas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x00458AAF
        static void updateBuildableIndustries(Window* self)
        {
            auto industryCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
            {
                auto industryObj = ObjectManager::get<IndustryObject>(i);
                if (industryObj == nullptr)
                    continue;

                if (!isEditorMode() && !isSandboxMode())
                {
                    if (!(industryObj->flags & IndustryObjectFlags::can_be_founded_by_user))
                        continue;
                    if (getCurrentYear() < industryObj->designedYear)
                        continue;
                    if (getCurrentYear() > industryObj->obsoleteYear)
                        continue;
                }
                self->rowInfo[industryCount] = i;
                industryCount++;
            }

            self->var_83C = industryCount;
            auto rowHover = -1;

            if (_lastSelectedIndustry != 0xFF)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (_lastSelectedIndustry == self->rowInfo[i])
                    {
                        rowHover = _lastSelectedIndustry;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->rowInfo[0];
            }

            self->rowHover = rowHover;
            updateActiveThumb(self);
        }

        // 0x00457FFE
        static void tabReset(Window* self)
        {
            self->minWidth = NewIndustries::window_size.width;
            self->minHeight = NewIndustries::window_size.height;
            self->maxWidth = NewIndustries::window_size.width;
            self->maxHeight = NewIndustries::window_size.height;
            Input::toolSet(self, Common::widx::tab_new_industry, CursorId::placeFactory);

            Input::setFlag(Input::Flags::flag6);
            Ui::Windows::showGridlines();
            _industryGhostPlaced = false;
            dword_E0C39C = 0x80000000;

            self->var_83C = 0;
            self->rowHover = -1;
            self->var_846 = -1;

            updateBuildableIndustries(self);

            _prng->randNext();
            _dword_E0C394 = _prng->srand_0();
            _dword_E0C398 = _prng->srand_1();
        }

        // 0x004589E8
        static void onResize(Window* self)
        {
            self->invalidate();
            Ui::Size minWindowSize = { self->minWidth, self->minHeight };
            Ui::Size maxWindowSize = { self->maxWidth, self->maxHeight };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.drawScroll = drawScroll;
            events.event_08 = event_08;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.getScrollSize = getScrollSize;
            events.onMouseUp = onMouseUp;
            events.onUpdate = onUpdate;
            events.scrollMouseDown = onScrollMouseDown;
            events.scrollMouseOver = onScrollMouseOver;
            events.prepareDraw = prepareDraw;
            events.tooltip = tooltip;
            events.onToolAbort = onToolAbort;
            events.onClose = onClose;
            events.onResize = onResize;
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
            { IndustryList::widgets, widx::tab_industry_list, &IndustryList::events, IndustryList::enabledWidgets },
            { NewIndustries::widgets, widx::tab_new_industry, &NewIndustries::events, NewIndustries::enabledWidgets },
        };

        // 0x00457B94
        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->currentTab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab..
            self->activatedWidgets &= ~((1ULL << tab_industry_list) | (1ULL << tab_new_industry));
            self->activatedWidgets |= (1ULL << tabInformationByTabOffset[self->currentTab].widgetIndex);

            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;
        }

        // 0x00457F27
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->currentTab = widgetIndex - widx::tab_industry_list;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_industry_list];

            self->enabledWidgets = tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;

            if (self->currentTab == widx::tab_industry_list - widx::tab_industry_list)
                IndustryList::tabReset(self);
            if (self->currentTab == widx::tab_new_industry - widx::tab_industry_list)
                NewIndustries::tabReset(self);

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x00458A57
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Industry List Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_industries;

                Widget::drawTab(self, context, imageId, widx::tab_industry_list);
            }

            // Fund New Industries Tab
            {
                static const uint32_t fundNewIndustriesImageIds[] = {
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
                if (self->currentTab == widx::tab_new_industry - widx::tab_industry_list)
                    imageId += fundNewIndustriesImageIds[(self->frame_no / 2) % std::size(fundNewIndustriesImageIds)];
                else
                    imageId += fundNewIndustriesImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_new_industry);
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

        static void initEvents()
        {
            IndustryList::initEvents();
            NewIndustries::initEvents();
        }
    }
}
