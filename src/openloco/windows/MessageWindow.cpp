#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../message.h"
#include "../messagemgr.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../ui/scrollview.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::MessageWindow
{
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint16_t, 0x005271CE> _messageCount;
    static loco_global<uint16_t, 0x005271D0> _activeMessageIndex;

    namespace common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_messages,
            tab_settings,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_messages) | (1 << widx::tab_settings);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { 366, 175 }, widget_type::panel, 1),                                                                      \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_recent_messages),              \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_message_options)

        static window_event_list _events;

        static void prepare_draw(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void on_update(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void initEvents();
    }

    namespace messages
    {
        static const gfx::ui_size_t minWindowSize = { 366, 217 };
        static const gfx::ui_size_t maxWindowSize = { 366, 1200 };
        static int8_t messageHeight = 39;

        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(366, 217, string_ids::title_messages),
            make_widget({ 3, 45 }, { 360, 146 }, widget_type::scrollview, 1, scrollbars::vertical),
            widget_end(),
        };

        static window_event_list events;

        // 0x0042A6F5
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_messages:
                case common::widx::tab_settings:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0042A95A
        static void on_resize(window* self)
        {
            auto scrollview = self->widgets[widx::scrollview];
            auto scrollarea = self->scroll_areas[0];

            auto y = scrollarea.contentHeight - scrollview.height() - 1;
            y = std::max(0, y);

            if (y < scrollarea.contentOffsetY)
            {
                scrollarea.contentOffsetY = y;
                ui::scrollview::update_thumbs(self, widx::scrollview);
                self->invalidate();
            }
        }

        // 0x0042A847
        static void event_08(window* self)
        {
            self->flags |= window_flags::not_scroll_view;
        }

        // 0x0042A84F
        static void event_09(window* self)
        {
            if (!(self->flags & window_flags::not_scroll_view))
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x0042A871
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = _messageCount * messageHeight;
        }

        // 0x0042A8B9
        static void scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            auto messageIndex = y / messageHeight;

            if (messageIndex >= _messageCount)
                return;

            if (_activeMessageIndex != 0xFFFF)
            {
                auto message = messagemgr::get(_activeMessageIndex);
                if (message->var_C8 != 0xFFFF)
                {
                    if (message->var_C8 & (1 << 15))
                        message->var_C8 = 0xFFFF;
                }
            }

            _activeMessageIndex = 0xFFFF;
            WindowManager::close(WindowType::news, 0);

            auto message = messagemgr::get(messageIndex);
            message->var_C8 = (1 << 15) | (1 << 0);

            NewsWindow::open(messageIndex);

            int32_t pan = self->width / 2 + self->x;
            audio::playSound(audio::sound_id::click_down, pan);
        }

        // 0x0042A87C
        static void scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scrollIndex)
        {
            self->flags &= ~(window_flags::not_scroll_view);

            auto messageIndex = y / messageHeight;
            auto messageId = 0xFFFF;

            if (messageIndex < _messageCount)
                messageId = messageIndex;

            if (self->row_hover != messageId)
            {
                self->row_hover = messageId;
                self->invalidate();
            }
        }

        // 0x0042A70C
        static void tooltip(FormatArguments& args, ui::window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_message_list);
        }

        // 0x0042A545
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;
        }

        // 0x0042A5CC
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
        }

        // 0x0042A5D7
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto colour = colour::get_shade(self->colours[1], 4);

            gfx::clear_single(*dpi, colour);

            auto height = 0;
            for (auto i = 0; i < _messageCount; i++)
            {
                if (height + messageHeight <= dpi->y)
                {
                    height += messageHeight;
                    continue;
                }

                if (height >= dpi->y + dpi->height)
                {
                    height += messageHeight;
                    continue;
                }
                auto message = messagemgr::get(i);
                char* buffer = message->messageString;
                auto str = const_cast<char*>(stringmgr::get_string(string_ids::buffer_2039));

                const size_t bufferLength = 512;
                strncpy(str, buffer, bufferLength);

                auto stringId = string_ids::black_stringid;

                if (self->row_hover == i)
                {
                    gfx::draw_rect(dpi, 0, height, self->width, 38, (1 << 25) | palette_index::index_30);
                    stringId = string_ids::wcolour2_stringid;
                }

                {
                    auto args = FormatArguments();
                    args.push(string_ids::tiny_font_date);
                    args.push(message->date);

                    gfx::draw_string_494B3F(*dpi, 0, height, colour::black, stringId, &args);
                }
                {
                    auto args = FormatArguments();
                    args.push(string_ids::buffer_2039);

                    auto width = self->widgets[widx::scrollview].width() - 14;
                    gfx::draw_string_495224(*dpi, 0, height + 6, width, colour::black, stringId, &args);
                    height += messageHeight;
                }
            }
        }

        // 0x0042A7B9
        static void tabReset(window* self)
        {
            self->min_width = minWindowSize.width;
            self->min_height = minWindowSize.height;
            self->max_width = maxWindowSize.width;
            self->max_height = maxWindowSize.height;
            self->width = minWindowSize.width;
            self->height = minWindowSize.height;
            self->row_hover = -1;
        }

        static void initEvents()
        {
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::on_update;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = get_scroll_size;
            events.scroll_mouse_down = scroll_mouse_down;
            events.scroll_mouse_over = scroll_mouse_over;
            events.tooltip = tooltip;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.draw_scroll = draw_scroll;
        }
    }

    // 0x0042A3FF
    void open()
    {
        //call(0x0042A3FF);
        auto window = WindowManager::bringToFront(WindowType::messages);

        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
            {
                input::cancel_tool();
                window = WindowManager::bringToFront(WindowType::messages);
            }
        }

        if (window == nullptr)
        {
            int16_t y = 29;
            int16_t x = ui::width() - 366;
            gfx::point_t origin = { x, y };

            window = WindowManager::createWindow(
                WindowType::messages,
                origin,
                { 366, 217 },
                window_flags::flag_11,
                &messages::events);

            window->enabled_widgets = messages::enabledWidgets;
            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->row_hover = -1;
            window->disabled_widgets = 0;

            WindowManager::sub_4CEE0B(window);

            window->min_width = messages::minWindowSize.width;
            window->min_height = messages::minWindowSize.height;
            window->max_width = messages::maxWindowSize.width;
            window->max_height = messages::maxWindowSize.height;
            window->flags |= window_flags::resizable;

            window->owner = _playerCompany;
            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0A;

            window->width = messages::minWindowSize.width;
            window->height = messages::minWindowSize.height;
        }

        window->current_tab = 0;
        window->invalidate();

        window->widgets = messages::widgets;
        window->enabled_widgets = messages::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &messages::events;
        window->disabled_widgets = 0;

        common::initEvents();

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        uint16_t scrollHeight = 0;
        window->call_get_scroll_size(0, 0, &scrollHeight);

        scrollHeight -= window->widgets[messages::widx::scrollview].height();

        if (static_cast<int16_t>(scrollHeight) < 0)
            scrollHeight = 0;

        window->scroll_areas[0].contentOffsetY = scrollHeight;

        ui::scrollview::update_thumbs(window, messages::widx::scrollview);
    }

    namespace settings
    {
        static const gfx::ui_size_t windowSize = { 366, 139 };

        enum widx
        {
            company_major_news = 6,
            company_major_news_dropdown,
            competitor_major_news,
            competitor_major_news_dropdown,
            company_minor_news,
            company_minor_news_dropdown,
            competitor_minor_news,
            competitor_minor_news_dropdown,
            general_news,
            general_news_dropdown,
            advice,
            advice_dropdown,
        };

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::company_major_news) | (1 << widx::company_major_news_dropdown) | (1 << widx::competitor_major_news) | (1 << widx::competitor_major_news_dropdown) | (1 << widx::company_minor_news) | (1 << widx::company_minor_news_dropdown) | (1 << widx::competitor_minor_news) | (1 << widx::competitor_minor_news_dropdown) | (1 << widx::general_news) | (1 << widx::general_news_dropdown) | (1 << widx::advice) | (1 << widx::advice_dropdown);

        widget_t widgets[] = {
            commonWidgets(366, 217, string_ids::title_messages),
            make_widget({ 236, 47 }, { 124, 12 }, widget_type::wt_18, 1),
            make_widget({ 348, 48 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 236, 62 }, { 124, 12 }, widget_type::wt_18, 1),
            make_widget({ 348, 63 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 236, 77 }, { 124, 12 }, widget_type::wt_18, 1),
            make_widget({ 348, 78 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 236, 92 }, { 124, 12 }, widget_type::wt_18, 1),
            make_widget({ 348, 93 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 236, 107 }, { 124, 12 }, widget_type::wt_18, 1),
            make_widget({ 348, 108 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 236, 122 }, { 124, 12 }, widget_type::wt_18, 1),
            make_widget({ 348, 123 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widget_end(),
        };

        static window_event_list events;

        // 0x0042AA84
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_messages:
                case common::widx::tab_settings:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0042AA9F
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::company_major_news:
                case widx::company_major_news_dropdown:
                case widx::competitor_major_news:
                case widx::competitor_major_news_dropdown:
                case widx::company_minor_news:
                case widx::company_minor_news_dropdown:
                case widx::competitor_minor_news:
                case widx::competitor_minor_news_dropdown:
                case widx::general_news:
                case widx::general_news_dropdown:
                case widx::advice:
                case widx::advice_dropdown:
                {
                    auto widget = self->widgets[widgetIndex - 1];
                    auto xPos = widget.left + self->x;
                    auto yPos = widget.top + self->y;
                    auto width = widget.width() - 2;
                    auto height = widget.height() + 2;
                    auto flags = 1 << 7;

                    dropdown::show(xPos, yPos, width, height, self->colours[1], 3, flags);

                    dropdown::add(0, string_ids::dropdown_stringid, string_ids::message_off);
                    dropdown::add(1, string_ids::dropdown_stringid, string_ids::message_ticker);
                    dropdown::add(2, string_ids::dropdown_stringid, string_ids::message_window);

                    auto dropdownIndex = config::get().news_settings[(widgetIndex - 7) / 2];

                    dropdown::setItemSelected(static_cast<size_t>(dropdownIndex));
                    break;
                }
            }
        }

        // 0x0042AAAC
        static void on_dropdown(window* self, ui::widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::company_major_news:
                case widx::company_major_news_dropdown:
                case widx::competitor_major_news:
                case widx::competitor_major_news_dropdown:
                case widx::company_minor_news:
                case widx::company_minor_news_dropdown:
                case widx::competitor_minor_news:
                case widx::competitor_minor_news_dropdown:
                case widx::general_news:
                case widx::general_news_dropdown:
                case widx::advice:
                case widx::advice_dropdown:
                {
                    if (itemIndex == -1)
                        return;

                    auto dropdownIndex = (widgetIndex - 7) / 2;

                    if (static_cast<config::newsType>(itemIndex) != config::get().news_settings[dropdownIndex])
                    {
                        config::get().news_settings[dropdownIndex] = static_cast<config::newsType>(itemIndex);
                        config::write();
                        gfx::invalidate_screen();
                    }
                    break;
                }
            }
        }

        // 0x0042AA02
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            auto yPos = self->widgets[widx::company_major_news].top + self->y;

            const string_id newsStringIds[] = {
                string_ids::company_major_news,
                string_ids::competitor_major_news,
                string_ids::company_minor_news,
                string_ids::competitor_minor_news,
                string_ids::general_news,
                string_ids::advice,
            };

            const string_id newsDropdownStringIds[] = {
                string_ids::message_off,
                string_ids::message_ticker,
                string_ids::message_window,
            };

            for (auto i = 0; i < 6; i++)
            {
                {
                    auto args = FormatArguments();
                    args.push(newsStringIds[i]);

                    gfx::draw_string_494B3F(*dpi, self->x + 4, yPos, colour::black, string_ids::wcolour2_stringid, &args);
                }

                {
                    auto xPos = self->widgets[widx::company_major_news].left + self->x + 1;
                    auto args = FormatArguments();
                    args.push(newsDropdownStringIds[static_cast<uint8_t>(config::get().news_settings[i])]);

                    gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::black_stringid, &args);
                }
                yPos += 15;
            }
        }

        // 0x0042A7E8
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
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = common::on_update;
            events.prepare_draw = common::prepare_draw;
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
            { messages::widgets, widx::tab_messages, &messages::events, messages::enabledWidgets },
            { settings::widgets, widx::tab_settings, &settings::events, settings::enabledWidgets },
        };

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
            self->activated_widgets &= ~((1ULL << tab_messages) | (1ULL << tab_settings));
            self->activated_widgets |= (1ULL << tabInformationByTabOffset[self->current_tab].widgetIndex);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;
        }

        // 0x0042A716
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            self->current_tab = widgetIndex - widx::tab_messages;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_messages];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabled_widgets = 0;

            self->invalidate();

            if (self->current_tab == widx::tab_messages - widx::tab_messages)
                messages::tabReset(self);
            if (self->current_tab == widx::tab_settings - widx::tab_messages)
                settings::tabReset(self);

            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x0042AB92
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Message Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::tab_messages;

                widget::draw_tab(self, dpi, imageId, widx::tab_messages);
            }

            // Setting Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::tab_message_settings;

                widget::draw_tab(self, dpi, imageId, widx::tab_settings);
            }
        }

        // 0x0042A826 and 0x0042AB6A
        static void on_update(window* self)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::messages, self->number, self->current_tab + common::widx::tab_messages);
        }

        static void initEvents()
        {
            messages::initEvents();
            settings::initEvents();
        }
    }
}
