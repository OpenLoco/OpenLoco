#include "../audio/audio.h"
#include "../config.h"
#include "../date.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../industrymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/cargo_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::industry_list
{
    static loco_global<uint32_t, 0x00E0C39C> dword_E0C39C;
    static loco_global<uint8_t, 0x00E0C3D9> byte_E0C3D9;
    static loco_global<uint8_t, 0x00525FC7> byte_525FC7;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;
    static loco_global<utility::prng, 0x00525E20> _prng;
    static loco_global<uint32_t, 0x00E0C394> _dword_E0C394;
    static loco_global<uint32_t, 0x00E0C398> _dword_E0C398;
    static loco_global<uint16_t, 0x00E0C3C6> word_E0C3C6;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    loco_global<uint8_t, 0x00508F14> _screen_flags;
    loco_global<int8_t, 0x00F2533F> _gridlines_state;

    static loco_global<uint32_t[32], 0x00525E5E> currencyMultiplicationFactor;

    namespace common
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 154 }, widget_type::panel, 1),                                                               \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_industries_list),              \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_fund_new_industries)

        static window_event_list _events;

        static void init_events();
        static void refreshIndustryList(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void prepare_draw(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void showGridlines();
    }

    namespace industry_list
    {
        static const gfx::ui_size_t windowSize = { 600, 197 };
        static const gfx::ui_size_t maxDimensions = { 600, 900 };
        static const gfx::ui_size_t minDimensions = { 192, 100 };

        static const uint8_t rowHeight = 10;

        enum widx
        {
            sort_industry_name = 6,
            sort_industry_status,
            sort_industry_production_transported,
            scrollview,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << sort_industry_name) | (1 << sort_industry_status) | (1 << sort_industry_production_transported) | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(600, 197, string_ids::title_industries),
            make_widget({ 4, 44 }, { 199, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_industry_name),
            make_widget({ 204, 44 }, { 204, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_industry_status),
            make_widget({ 444, 44 }, { 159, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_industry_production_transported),
            make_widget({ 3, 56 }, { 593, 125 }, widget_type::scrollview, 1, 2),
            widget_end(),
        };

        static window_event_list events;

        enum SortMode : uint16_t
        {
            Name,
            Status,
            ProductionTransported,
        };

        // 0x00457B94
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            // Reposition header buttons.
            self->widgets[widx::sort_industry_name].right = std::min(self->width - 4, 203);

            self->widgets[widx::sort_industry_status].left = std::min(self->width - 4, 204);
            self->widgets[widx::sort_industry_status].right = std::min(self->width - 4, 443);

            self->widgets[widx::sort_industry_production_transported].left = std::min(self->width - 4, 444);
            self->widgets[widx::sort_industry_production_transported].right = std::min(self->width - 4, 603);

            // Set header button captions.
            self->widgets[widx::sort_industry_name].text = self->sort_mode == SortMode::Name ? string_ids::industry_table_header_desc : string_ids::industry_table_header;
            self->widgets[widx::sort_industry_status].text = self->sort_mode == SortMode::Status ? string_ids::industry_table_header_status_desc : string_ids::industry_table_header_status;
            self->widgets[widx::sort_industry_production_transported].text = self->sort_mode == SortMode::ProductionTransported ? string_ids::industry_table_header_production_desc : string_ids::industry_table_header_production;

            if (is_editor_mode())
                self->widgets[common::widx::tab_new_industry].tooltip = string_ids::tooltip_build_new_industries;
            else
                self->widgets[common::widx::tab_new_industry].tooltip = string_ids::tooltip_fund_new_industries;
        }

        // 0x00457CD9
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            auto args = FormatArguments();
            auto xPos = self->x + 4;
            auto yPos = self->y + self->height - 12;

            if (self->var_83C == 1)
                args.push(string_ids::status_num_industries_singular);
            else
                args.push(string_ids::status_num_industries_plural);
            args.push(self->var_83C);

            gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::white_stringid2, &args);
        }

        // 0x00457EC4
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_industry_list:
                case common::widx::tab_new_industry:
                    common::switchTab(self, widgetIndex);
                    break;

                case widx::sort_industry_name:
                case widx::sort_industry_status:
                case widx::sort_industry_production_transported:
                {
                    auto sortMode = widgetIndex - widx::sort_industry_name;
                    if (self->sort_mode == sortMode)
                        return;

                    self->sort_mode = sortMode;
                    self->invalidate();
                    self->var_83C = 0;
                    self->row_hover = -1;

                    common::refreshIndustryList(self);
                    break;
                }
            }
        }

        //0x00458172
        static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / rowHeight;
            if (currentRow > self->var_83C)
                return;

            int16_t currentIndustry = self->row_info[currentRow];
            if (currentIndustry == -1)
                return;

            windows::industry::open(currentIndustry);
        }

        // 0x00458140
        static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(window_flags::flag_14);

            uint16_t currentRow = y / rowHeight;
            int16_t currentIndustry = -1;

            if (currentRow < self->var_83C)
                currentIndustry = self->row_info[currentRow];

            self->row_hover = currentIndustry;
            self->invalidate();
        }

        // 0x00457A52
        static bool orderByName(const openloco::industry& lhs, const openloco::industry& rhs)
        {
            char lhsString[256] = { 0 };
            stringmgr::format_string(lhsString, lhs.name, (void*)&lhs.town);

            char rhsString[256] = { 0 };
            stringmgr::format_string(rhsString, rhs.name, (void*)&rhs.town);

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00457A9F
        static bool orderByStatus(openloco::industry& lhs, openloco::industry& rhs)
        {
            char lhsString[256] = { 0 };
            const char* lhsBuffer = stringmgr::get_string(string_ids::buffer_1250);
            lhs.getStatusString((char*)lhsBuffer);

            stringmgr::format_string(lhsString, string_ids::buffer_1250);

            char rhsString[256] = { 0 };
            const char* rhsBuffer = stringmgr::get_string(string_ids::buffer_1250);
            rhs.getStatusString((char*)rhsBuffer);

            stringmgr::format_string(rhsString, string_ids::buffer_1250);

            return strcmp(lhsString, rhsString) < 0;
        }

        static uint8_t getAverageTransportedCargo(const openloco::industry& industry)
        {
            auto industryObj = objectmgr::get<industry_object>(industry.object_id);
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
        static bool orderByProductionTransported(const openloco::industry& lhs, const openloco::industry& rhs)
        {
            auto lhsVar = getAverageTransportedCargo(lhs);

            auto rhsVar = getAverageTransportedCargo(rhs);

            return rhsVar < lhsVar;
        }

        // 0x00457A52, 0x00457A9F, 0x00457AF3
        static bool getOrder(const SortMode mode, openloco::industry& lhs, openloco::industry& rhs)
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
        static void updateIndustryList(window* self)
        {
            auto chosenIndustry = -1;

            auto i = -1;

            for (auto& industry : industrymgr::industries())
            {
                i++;
                if (industry.empty())
                    continue;

                if ((industry.flags & industry_flags::sorted) != 0)
                    continue;

                if (chosenIndustry == -1)
                {
                    chosenIndustry = i;
                    continue;
                }

                if (getOrder(SortMode(self->sort_mode), industry, *industrymgr::get(chosenIndustry)))
                {
                    chosenIndustry = i;
                }
            }

            if (chosenIndustry != -1)
            {
                bool shouldInvalidate = false;

                industrymgr::get(chosenIndustry)->flags |= industry_flags::sorted;

                auto ebp = self->row_count;
                if (chosenIndustry != self->row_info[ebp])
                {
                    self->row_info[ebp] = chosenIndustry;
                    shouldInvalidate = true;
                }

                self->row_count += 1;
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

                common::refreshIndustryList(self);
            }
        }

        // 0x004580AE
        static void on_update(window* self)
        {
            self->frame_no++;

            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::industryList, self->number, self->current_tab + common::widx::tab_industry_list);

            // Add three industries every tick.
            updateIndustryList(self);
            updateIndustryList(self);
            updateIndustryList(self);
        }

        // 0x00457EE8
        static void tooltip(FormatArguments& args, ui::window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_industry_list);
        }

        // 0x00458108
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = rowHeight * self->var_83C;
        }

        // 0x00457D2A
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::get_shade(self->colours[1], 4);
            gfx::clear_single(*dpi, shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                industry_id_t industryId = self->row_info[i];

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + rowHeight < dpi->y || yPos >= yPos + rowHeight + dpi->height || industryId == (uint16_t)-1)
                {
                    yPos += rowHeight;
                    continue;
                }

                string_id text_colour_id = string_ids::white_stringid2;

                // Highlight selection.
                if (industryId == self->row_hover)
                {
                    gfx::draw_rect(dpi, 0, yPos, self->width, rowHeight, 0x2000030);
                    text_colour_id = string_ids::wcolour2_stringid2;
                }

                if (industryId == 0xFFFF)
                    continue;
                auto industry = industrymgr::get(industryId);

                // Industry Name
                {
                    auto args = FormatArguments();
                    args.push(industry->name);
                    args.push(industry->town);

                    gfx::draw_string_494BBF(*dpi, 0, yPos, 198, colour::black, text_colour_id, &args);
                }
                // Industry Status
                {
                    const char* buffer = stringmgr::get_string(string_ids::buffer_1250);
                    industry->getStatusString((char*)buffer);

                    auto args = FormatArguments();
                    args.push(string_ids::buffer_1250);

                    gfx::draw_string_494BBF(*dpi, 200, yPos, 238, colour::black, text_colour_id, &args);
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

                    gfx::draw_string_494BBF(*dpi, 440, yPos, 138, colour::black, string_ids::production_transported_percent, &args);
                }
                yPos += rowHeight;
            }
        }

        // 0x00458113
        static ui::cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
                return cursor_id::hand_pointer;

            return fallback;
        }

        // 0x004580DE
        static void event_08(window* self)
        {
            self->flags |= window_flags::flag_14;
        }

        // 0x004580E6
        static void event_09(window* self)
        {
            if ((self->flags & window_flags::flag_14) == 0)
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x00457FCA
        static void tabReset(window* self)
        {
            self->min_width = minDimensions.width;
            self->min_height = minDimensions.height;
            self->max_width = maxDimensions.width;
            self->max_height = maxDimensions.height;
            self->var_83C = 0;
            self->row_hover = -1;
            common::refreshIndustryList(self);
        }

        static void init_events()
        {
            events.draw = draw;
            events.cursor = cursor;
            events.draw_scroll = draw_scroll;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = get_scroll_size;
            events.on_mouse_up = on_mouse_up;
            events.on_update = on_update;
            events.scroll_mouse_down = on_scroll_mouse_down;
            events.scroll_mouse_over = on_scroll_mouse_over;
            events.prepare_draw = prepare_draw;
            events.tooltip = tooltip;
        }
    }

    // 0x004577FF
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::industryList, 0);
        if (window != nullptr)
        {
            window->call_on_mouse_up(common::widx::tab_industry_list);
        }
        else
        {
            // 0x00457878
            window = WindowManager::createWindow(
                WindowType::industryList,
                industry_list::windowSize,
                window_flags::flag_8,
                &industry_list::events);

            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->sort_mode = 0;
            window->var_83C = 0;
            window->row_hover = -1;

            common::refreshIndustryList(window);

            WindowManager::sub_4CEE0B(window);

            window->min_width = industry_list::minDimensions.width;
            window->min_height = industry_list::minDimensions.height;
            window->max_width = industry_list::maxDimensions.width;
            window->max_height = industry_list::maxDimensions.height;
            window->flags |= window_flags::resizable;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[0] = skin->colour_0B;
            window->colours[1] = skin->colour_0C;

            // 0x00457878 end

            // TODO: only needs to be called once.
            common::init_events();

            window->invalidate();

            window->widgets = industry_list::widgets;
            window->enabled_widgets = industry_list::enabledWidgets;

            window->activated_widgets = 0;
            window->holdable_widgets = 0;

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
        }
        return window;
    }

    namespace new_industries
    {

        static const gfx::ui_size_t window_size = { 578, 172 };

        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(577, 171, string_ids::title_fund_new_industries),
            make_widget({ 3, 45 }, { 549, 111 }, widget_type::scrollview, 1, 2),
            widget_end(),
        };

        static window_event_list events;

        // 0x0045819F
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            if (is_editor_mode())
            {
                self->widgets[common::widx::caption].text = string_ids::title_build_new_industries;
                self->widgets[common::widx::tab_new_industry].tooltip = string_ids::tooltip_build_new_industries;
            }
            else
            {
                self->widgets[common::widx::caption].text = string_ids::title_fund_new_industries;
                self->widgets[common::widx::tab_new_industry].tooltip = string_ids::tooltip_fund_new_industries;
            }
        }

        // 0x0045826C
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            if (self->var_83C == 0)
            {
                auto xPos = self->x + 3;
                auto yPos = self->y + self->height - 13;
                auto width = self->width - 19;
                gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::no_industry_available);
                return;
            }

            auto industryId = self->var_846;

            if (industryId == 0xFFFF)
            {
                industryId = self->row_hover;

                if (industryId == 0xFFFF)
                    return;
            }

            auto industryObj = objectmgr::get<industry_object>(industryId);
            auto industryCost = 0;

            if (self->var_846 == 0xFFFF)
                industryCost = dword_E0C39C;

            if ((self->var_846 == 0xFFFF && dword_E0C39C == (1ULL << 31)) || self->var_846 != 0xFFFF)
            {
                industryCost = (industryObj->cost_factor * currencyMultiplicationFactor[industryObj->cost_index]) / 8;
            }
            auto args = FormatArguments();
            args.push(industryCost);

            auto widthOffset = 0;

            if (!is_editor_mode())
            {
                auto xPos = self->x + 3 + self->width - 19;
                auto yPos = self->y + self->height - 13;
                widthOffset = 138;

                gfx::draw_string_494C78(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
            }

            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            auto width = self->width - 19 - widthOffset;

            gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::white_stringid2, &industryObj->name);
        }

        // 0x0045843A
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_industry_list:
                case common::widx::tab_new_industry:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 122) + (y / 112) * 5;
        }

        //0x00458966
        static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            int16_t xPos = (x / 122);
            int16_t yPos = (y / 112) * 5;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->row_info[i];
                index--;
                if (index < 0)
                {
                    self->row_hover = rowInfo;
                    byte_525FC7 = static_cast<uint8_t>(rowInfo);

                    int32_t pan = (self->width >> 1) + self->x;
                    loc16 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    audio::play_sound(audio::sound_id::click_down, loc, pan);
                    dword_E0C39C = 0x80000000;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x00458721
        static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = 0xFFFF;

            for (auto i = 0; i < self->var_83C; i++)
            {
                rowInfo = self->row_info[i];
                index--;
                if (index < 0)
                    break;
            }
            self->var_846 = rowInfo;
            self->invalidate();
            auto string = string_ids::buffer_337;

            if (rowInfo == 0xFFFF)
                string = string_ids::null;

            if (stringmgr::get_string(string_ids::buffer_337)[0] != '\0')
            {
                if (string == self->widgets[common::widx::frame].tooltip)
                {
                    if (rowInfo == self->var_85C)
                        return;
                }
            }
            self->widgets[common::widx::frame].tooltip = string;
            self->var_85C = rowInfo;
            tooltip::closeAndReset();

            if (rowInfo == 0xFFFF)
                return;

            auto industryObj = objectmgr::get<industry_object>(rowInfo);
            const char* buffer = stringmgr::get_string(string);
            char* ptr = (char*)buffer;
            *ptr = '\0';

            if (industryObj->producesCargo())
            {
                ptr = stringmgr::format_string(ptr, string_ids::industry_produces);
                ptr = industryObj->getProducedCargoString(ptr);

                if (industryObj->requiresCargo())
                {
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_comma);
                }
            }

            if (industryObj->requiresCargo())
            {
                ptr = stringmgr::format_string(ptr, string_ids::industry_requires);
                ptr = industryObj->getRequiredCargoString(ptr);
            }
        }

        // 0x004585B8
        static void on_update(window* self)
        {
            if (!input::has_flag(input::input_flags::flag5))
            {
                auto cursor = input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        widget_index activeWidget = self->find_widget_at(xPos, yPos);
                        if (activeWidget == common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].v_bottom - 1 + 60, 500);
                                if (ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self->min_width = window_size.width;
                                self->min_height = y;
                                self->max_width = window_size.width;
                                self->max_height = y;
                            }
                            else
                            {
                                if (input::state() != input::input_state::scroll_left)
                                {
                                    self->min_width = window_size.width;
                                    self->min_height = window_size.height;
                                    self->max_width = window_size.width;
                                    self->max_height = window_size.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->saved_view.mapX = 0;
                    if (input::state() != input::input_state::scroll_left)
                    {
                        self->min_width = window_size.width;
                        self->min_height = window_size.height;
                        self->max_width = window_size.width;
                        self->max_height = window_size.height;
                    }
                }
            }
            self->frame_no++;

            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::industryList, self->number, self->current_tab + common::widx::tab_industry_list);
            if ((!input::has_flag(input::input_flags::tool_active)) || self->type != _toolWindowType || self->number != _toolWindowNumber)
            {
                WindowManager::close(self);
            }
        }

        // 0x00458455
        static void tooltip(FormatArguments& args, ui::window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_new_industry_list);
        }

        // 0x004586EA
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 + self->var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= 112;
        }

        // 0x00458C7F
        static void drawIndustryThumb(gfx::drawpixelinfo_t* clipped, const openloco::industry_object* industryObj, int16_t x, int16_t y)
        {
            registers regs;
            regs.cx = x;
            regs.dx = y;
            regs.edi = (uint32_t)clipped;
            regs.ebp = (uint32_t)industryObj;
            call(0x00458C7F, regs);
        }

        // 0x00458352
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::get_shade(self->colours[1], 4);
            gfx::clear_single(*dpi, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                word_E0C3C6 = 0xFFFF;
                if (self->row_info[i] != self->row_hover)
                {
                    if (self->row_info[i] == self->var_846)
                    {
                        word_E0C3C6 = colour::translucent_flag;
                        gfx::draw_rect_inset(dpi, xPos, yPos, 112, 112, self->colours[1], colour::translucent_flag);
                    }
                }
                else
                {
                    word_E0C3C6 = colour::translucent_flag & colour::outline_flag;
                    gfx::draw_rect_inset(dpi, xPos, yPos, 112, 112, self->colours[1], (colour::translucent_flag & colour::outline_flag));
                }

                auto industryObj = objectmgr::get<industry_object>(self->row_info[i]);

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, xPos + 1, yPos + 1, 110, 110))
                    drawIndustryThumb(clipped, industryObj, 56, 96);

                xPos += 112;

                if (xPos >= 112 * 5) // full row
                {
                    xPos = 0;
                    yPos += 112;
                }
            }
        }

        // 0x00458708
        static void event_08(window* self)
        {
            if (self->var_846 != 0xFFFF)
                self->var_846 = 0xFFFF;
            self->invalidate();
        }

        // 0x0045848A
        static void on_tool_update(window& self, const widget_index widgetIndex, int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0045848A, regs);
        }

        // 0x0045851F
        static void on_tool_down(window& self, const widget_index widgetIndex, int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0045851F, regs);
        }

        // 0x00458C09
        static void sub_458C09()
        {
            registers regs;
            call(0x00458C09, regs);
        }

        // 0x00468FFE hide_gridlines
        static void hideGridlines()
        {
            _gridlines_state--;
            if (_gridlines_state == 0)
            {
                auto window = WindowManager::getMainWindow();
                if (window != nullptr)
                {
                    if ((config::get().flags & config::flags::gridlines_on_landscape) == 0)
                    {
                        if ((window->viewports[0]->flags & viewport_flags::gridlines_on_landscape) == 0)
                        {
                            window->invalidate();
                        }
                        window->viewports[0]->flags ^= viewport_flags::gridlines_on_landscape;
                    }
                }
            }
        }

        // 0x004585AD
        static void on_tool_abort(window& self, const widget_index widgetIndex)
        {
            sub_458C09();
            hideGridlines();
        }

        // 0x0045845F
        static void on_close(window* self)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();
        }

        // 0x00458B51
        static void updateActiveThumb(window* self)
        {
            uint16_t scrollHeight = 0;
            self->call_get_scroll_size(0, 0, &scrollHeight);
            self->scroll_areas[0].v_bottom = scrollHeight;

            auto i = 0;
            if (i >= self->var_83C)
            {
                for (; i <= self->var_83C; i++)
                {
                    if (self->row_info[i] == self->row_hover)
                        break;
                }
            }
            self->scroll_areas[0].v_top = i;
            ui::scrollview::update_thumbs(self, new_industries::widx::scrollview);
        }

        // 0x00458AAF
        static void updateBuildableIndustries(window* self)
        {
            auto industryCount = 0;
            for (auto i = 0; objectmgr::get_max_objects(object_type::industry); i++)
            {
                auto industryObj = objectmgr::get<industry_object>(i);
                if (industryObj == nullptr)
                    break;
                if (!is_editor_mode())
                {
                    if (!(industryObj->flags & industry_object_flags::can_be_founded_by_user))
                        continue;
                    if (current_year() < industryObj->designedYear)
                        continue;
                    if (current_year() > industryObj->obsoleteYear)
                        continue;
                }
                self->row_info[industryCount] = i;
                industryCount++;
            }

            self->var_83C = industryCount;
            auto rowHover = -1;

            if (byte_525FC7 != 0xFF)
            {
                for (auto i = 0; i <= self->var_83C; i++)
                {
                    if (i >= self->var_83C)
                    {
                        if (byte_525FC7 == self->row_info[i])
                        {
                            rowHover = byte_525FC7;
                            break;
                        }
                    }
                    else
                    {
                        if (self->var_83C != 0)
                        {
                            rowHover = self->row_info[0];
                        }
                    }
                }
            }
            else
            {
                if (self->var_83C != 0)
                {
                    rowHover = self->row_info[0];
                }
            }
            self->row_hover = rowHover;
            updateActiveThumb(self);
        }

        // 0x00457FFE
        static void tabReset(window* self)
        {
            self->min_width = new_industries::window_size.width;
            self->min_height = new_industries::window_size.height;
            self->max_width = new_industries::window_size.width;
            self->max_height = new_industries::window_size.height;
            input::toolSet(self, common::widx::tab_new_industry, 40);

            input::set_flag(input::input_flags::flag6);
            common::showGridlines();
            byte_E0C3D9 = 0;
            dword_E0C39C = 0x80000000;

            self->var_83C = 0;
            self->row_hover = -1;
            self->var_846 = -1;

            updateBuildableIndustries(self);

            _prng->rand_next();
            _dword_E0C394 = _prng->srand_0();
            _dword_E0C398 = _prng->srand_1();
        }

        static void init_events()
        {
            events.draw = draw;
            events.draw_scroll = draw_scroll;
            events.event_08 = event_08;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.get_scroll_size = get_scroll_size;
            events.on_mouse_up = on_mouse_up;
            events.on_update = on_update;
            events.scroll_mouse_down = on_scroll_mouse_down;
            events.scroll_mouse_over = on_scroll_mouse_over;
            events.prepare_draw = prepare_draw;
            events.tooltip = tooltip;
            events.on_tool_abort = on_tool_abort;
            events.on_close = on_close;
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
            { industry_list::widgets, widx::tab_industry_list, &industry_list::events, industry_list::enabledWidgets },
            { new_industries::widgets, widx::tab_new_industry, &new_industries::events, new_industries::enabledWidgets },
        };

        // 0x00457B94
        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
            }

            // Activate the current tab..
            self->activated_widgets &= ~((1ULL << tab_industry_list) | (1ULL << tab_new_industry));
            self->activated_widgets |= (1ULL << common::tabInformationByTabOffset[self->current_tab].widgetIndex);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;
        }

        // 0x00468FD3
        static void showGridlines()
        {
            if (_gridlines_state == 0)
            {
                auto window = WindowManager::getMainWindow();
                if ((window->viewports[0]->flags & viewport_flags::gridlines_on_landscape) != 0)
                {
                    window->invalidate();
                }
                window->viewports[0]->flags |= viewport_flags::gridlines_on_landscape;
            }
            _gridlines_state++;
        }

        // 0x00457F27
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            self->current_tab = widgetIndex - widx::tab_industry_list;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_industry_list];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            if (self->current_tab == widx::tab_industry_list - widx::tab_industry_list)
                industry_list::tabReset(self);
            if (self->current_tab == widx::tab_new_industry - widx::tab_industry_list)
                new_industries::tabReset(self);

            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x00458A57
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Industry List Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_industries;

                widget::draw_tab(self, dpi, imageId, widx::tab_industry_list);
            }

            // Fund New Industries Tab
            {
                static const uint32_t fundNewIndustriesImageIds[] = {
                    interface_skin::image_ids::build_industry_frame_0,
                    interface_skin::image_ids::build_industry_frame_1,
                    interface_skin::image_ids::build_industry_frame_2,
                    interface_skin::image_ids::build_industry_frame_3,
                    interface_skin::image_ids::build_industry_frame_4,
                    interface_skin::image_ids::build_industry_frame_5,
                    interface_skin::image_ids::build_industry_frame_6,
                    interface_skin::image_ids::build_industry_frame_7,
                    interface_skin::image_ids::build_industry_frame_8,
                    interface_skin::image_ids::build_industry_frame_9,
                    interface_skin::image_ids::build_industry_frame_10,
                    interface_skin::image_ids::build_industry_frame_11,
                    interface_skin::image_ids::build_industry_frame_12,
                    interface_skin::image_ids::build_industry_frame_13,
                    interface_skin::image_ids::build_industry_frame_14,
                    interface_skin::image_ids::build_industry_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_new_industry - widx::tab_industry_list)
                    imageId += fundNewIndustriesImageIds[(self->frame_no / 2) % std::size(fundNewIndustriesImageIds)];
                else
                    imageId += fundNewIndustriesImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_new_industry);
            }
        }

        // 0x00457964
        static void refreshIndustryList(window* window)
        {
            window->row_count = 0;

            for (auto& industry : industrymgr::industries())
            {
                if (industry.empty())
                    continue;

                industry.flags &= ~industry_flags::sorted;
            }
        }

        static void init_events()
        {
            industry_list::init_events();
            new_industries::init_events();
        }
    }
}
