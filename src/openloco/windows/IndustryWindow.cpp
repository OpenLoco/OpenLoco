#include "../audio/audio.h"
#include "../config.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../graphics/types.h"
#include "../industrymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/industry_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"
#include "../viewportmgr.h"
#include "../widget.h"

using namespace openloco::interop;
using namespace openloco::game_commands;

namespace openloco::ui::windows::industry
{
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_industry,
            tab_production,
            tab_production_2,
            tab_transported,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_industry) | (1 << widx::tab_production) | (1 << widx::tab_production_2) | (1 << widx::tab_transported);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 95 }, widget_type::panel, 1),                                                                \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_industry),                     \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_production_graph),            \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_production_graph),            \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_statistics)

        // Defined at the bottom of this file.
        static void prepare_draw(window* self);
        static void text_input(window* self, widget_index callingWidget, char* input);
        static void update(window* self);
        static void renameIndustryPrompt(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void setDisabledWidgets(window* self);
        static void draw(window* self, gfx::drawpixelinfo_t* dpi);
        static void on_mouse_up(window* self, widget_index widgetIndex);
        static void initEvents();
    }

    namespace industry
    {
        static const gfx::ui_size_t windowSize = { 223, 137 };

        static const gfx::ui_size_t minWindowSize = { 192, 137 };

        static const gfx::ui_size_t maxWindowSize = { 600, 440 };

        enum widx
        {
            viewport = 8,
            status_bar,
            centre_on_viewport,
            demolish_industry,
        };

        static widget_t widgets[] = {
            commonWidgets(223, 137, string_ids::title_town),
            make_widget({ 3, 44 }, { 195, 80 }, widget_type::viewport, 1, 0xFFFFFFFE),
            make_widget({ 3, 115 }, { 195, 21 }, widget_type::wt_13, 1),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rubbish_bin, string_ids::demolish_this_industry),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << centre_on_viewport) | (1 << demolish_industry);

        static window_event_list events;

        // 0x00455ADD
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::demolish_industry].right = self->width - 2;
            self->widgets[widx::demolish_industry].left = self->width - 25;

            if (is_editor_mode())
            {
                self->widgets[widx::demolish_industry].type = widget_type::wt_9;
            }
            else
            {
                self->widgets[widx::demolish_industry].type = widget_type::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x00455C22
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            self->drawViewports(dpi);
            widget::drawViewportCentreButton(dpi, self, widx::centre_on_viewport);

            const char* buffer = stringmgr::get_string(string_ids::buffer_1250);
            auto industry = industrymgr::get(self->number);
            industry->getStatusString(const_cast<char*>(buffer));

            auto args = FormatArguments();
            args.push(string_ids::buffer_1250);

            auto widget = &self->widgets[widx::status_bar];
            auto x = self->x + widget->left - 1;
            auto y = self->y + widget->top - 1;
            auto width = widget->width();
            gfx::draw_string_494BBF(*dpi, x, y, width, colour::black, string_ids::black_stringid, &args);
        }

        // 0x00455C86
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameIndustryPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_industry:
                case common::widx::tab_production:
                case common::widx::tab_production_2:
                case common::widx::tab_transported:
                    common::switchTab(self, widgetIndex);
                    break;

                // 0x00455EA2
                case widx::centre_on_viewport:
                {
                    if (self->viewports[0] == nullptr || self->saved_view.isEmpty())
                        break;

                    auto main = WindowManager::getMainWindow();
                    main->viewportCentreOnTile(self->saved_view.getPos());
                    break;
                }

                // 0x00455E59
                case widx::demolish_industry:
                {
                    bool success = game_commands::do_48(self->number);
                    if (!success)
                        break;

                    loco_global<uint16_t, 0x009C68E0> gameCommandMapX;
                    loco_global<uint16_t, 0x009C68E2> gameCommandMapY;
                    loco_global<uint16_t, 0x009C68E4> gameCommandMapZ;

                    audio::play_sound(audio::sound_id::demolish, loc16(gameCommandMapX, gameCommandMapY, gameCommandMapZ));
                    break;
                }
            }
        }

        static void initViewport(window* self);

        // 0x00455F1A
        static void on_resize(window* self)
        {
            self->set_size(minWindowSize, maxWindowSize);

            if (self->viewports[0] != nullptr)
            {
                uint16_t newWidth = self->width - 30;
                if (!is_editor_mode())
                    newWidth += 22;

                uint16_t newHeight = self->height - 59;

                auto& viewport = self->viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->view_width = newWidth << viewport->zoom;
                    viewport->view_height = newHeight << viewport->zoom;
                    self->saved_view.clear();
                }
            }

            initViewport(self);
        }

        // 0x00456C36
        static void initViewport(window* self)
        {
            if (self->current_tab != common::widx::tab_industry - common::widx::tab_industry)
                return;

            self->call_prepare_draw();

            // Figure out the industry's position on the map.
            auto industry = industrymgr::get(self->number);
            int16_t tileZ = openloco::map::tile_element_height(industry->x, industry->y) & 0xFFFF;

            // Compute views.
            SavedView view = {
                industry->x,
                industry->y,
                ZoomLevel::quarter,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                tileZ,
            };
            //view.flags |= (1 << 14);

            uint16_t flags = 0;
            if (self->viewports[0] != nullptr)
            {
                if (self->saved_view == view)
                    return;

                flags = self->viewports[0]->flags;
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                viewportmgr::collectGarbage();
            }
            else
            {
                if ((config::get().flags & config::flags::gridlines_on_landscape) != 0)
                    flags |= viewport_flags::gridlines_on_landscape;
            }

            self->saved_view = view;

            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = openloco::map::map_pos3({ industry->x, industry->y, tileZ });
                auto origin = gfx::point_t(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = gfx::ui_size_t(widget->width() - 2, widget->height() - 2);
                viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, tile);
                self->invalidate();
                self->flags |= window_flags::viewport_no_scrolling;
            }

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->flags = flags;
                self->invalidate();
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = common::text_input;
            events.viewport_rotate = initViewport;
        }
    }

    // 0x00456D2D
    window* open(industry_id_t industryId)
    {
        auto window = WindowManager::bringToFront(WindowType::industry, industryId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::industry, industryId);
        }

        if (window == nullptr)
        {
            // 0x00456DBC start
            const uint32_t newFlags = window_flags::flag_8 | window_flags::resizable;
            window = WindowManager::createWindow(WindowType::industry, industry::windowSize, newFlags, &industry::events);
            window->number = industryId;
            window->min_width = 192;
            window->min_height = 137;
            window->max_width = 600;
            window->max_height = 440;

            auto skin = objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_0B;
                window->colours[1] = skin->colour_0C;
            }
            // 0x00456DBC end

            window->saved_view.clear();
        }

        // TODO: only needs to be called once.
        common::initEvents();

        window->current_tab = common::widx::tab_industry - common::widx::tab_industry;
        window->invalidate();

        window->widgets = industry::widgets;
        window->enabled_widgets = industry::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &industry::events;
        window->activated_widgets = 0;

        common::setDisabledWidgets(window);

        window->init_scroll_widgets();
        industry::initViewport(window);

        return window;
    }

    namespace production
    {

        static const gfx::ui_size_t minWindowSize = { 299, 282 };

        static const gfx::ui_size_t maxWindowSize = { 299, 337 };

        static window_event_list events;

        // 0x00455FD9
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            common::repositionTabs(self);
        }

        // 0x0045654F
        static void on_resize(window* self)
        {
            {
                self->set_size(minWindowSize, maxWindowSize);
            }
        }

        static void initEvents()
        {
            events.draw = common::draw;
            events.on_mouse_up = common::on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = common::text_input;
        }
    }

    namespace production_2
    {
        static const gfx::ui_size_t minWindowSize = { 299, 282 };

        static const gfx::ui_size_t maxWindowSize = { 299, 337 };

        static widget_t widgets[] = {
            commonWidgets(222, 136, string_ids::title_industry_monthly_production),
            widget_end(),
        };

        static window_event_list events;

        // 0x0045626F
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            common::repositionTabs(self);
        }

        // 0x004565FF
        static void on_resize(window* self)
        {
            {
                self->set_size(minWindowSize, maxWindowSize);
            }
        }

        static void initEvents()
        {
            events.draw = common::draw;
            events.on_mouse_up = common::on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = common::text_input;
        }
    }

    namespace transported
    {
        static const gfx::ui_size_t windowSize = { 300, 127 };

        static widget_t widgets[] = {
            commonWidgets(300, 126, string_ids::title_statistics),
            widget_end(),
        };

        static window_event_list events;

        // 0x00456665
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            common::repositionTabs(self);
        }

        // 0x00456705
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto industry = industrymgr::get(self->number);
            auto industryObj = industry->object();
            int16_t xPos = self->x + 3;
            int16_t yPos = self->y + 45;
            gfx::point_t origin = { xPos, yPos };

            // Draw Last Months received cargo stats
            if (industry->canReceiveCargo())
            {
                origin.x += 4;
                origin.y += 10;
                gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::received_cargo);

                auto cargoNumber = 0;
                for (const auto& receivedCargoType : industryObj->required_cargo_type)
                {
                    if (receivedCargoType != 0xFF)
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(receivedCargoType);
                        auto args = FormatArguments();

                        if (industry->required_cargo_quantity[cargoNumber] == 1)
                        {
                            args.push(cargoObj->unit_name_singular);
                        }
                        else
                        {
                            args.push(cargoObj->unit_name_plural);
                        }
                        args.push<uint32_t>(industry->required_cargo_quantity[cargoNumber]);

                        origin.y = gfx::draw_string_495224(*dpi, origin.x, origin.y, 290, colour::black, string_ids::black_stringid, &args);
                    }
                    cargoNumber++;
                }
                origin.y += 4;
                origin.x -= 4;
            }

            // Draw Last Months produced cargo stats
            if (industry->canProduceCargo())
            {
                gfx::draw_string_494B3F(*dpi, origin.x, origin.y, colour::black, string_ids::produced_cargo);
                origin.y += 10;
                origin.x += 4;

                auto cargoNumber = 0;
                for (const auto& producedCargoType : industryObj->produced_cargo_type)
                {
                    if (producedCargoType != 0xFF)
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(producedCargoType);
                        auto args = FormatArguments();

                        if (industry->produced_cargo_quantity[cargoNumber] == 1)
                        {
                            args.push(cargoObj->unit_name_singular);
                        }
                        else
                        {
                            args.push(cargoObj->unit_name_plural);
                        }
                        args.push<uint32_t>(industry->produced_cargo_quantity[cargoNumber]);
                        args.push<uint16_t>(industry->produced_cargo_transported[cargoNumber]);

                        origin.y = gfx::draw_string_495224(*dpi, origin.x, origin.y, 290, colour::black, string_ids::transported_cargo, &args);
                    }
                    cargoNumber++;
                }
            }
        }

        // 0x004569C2
        static void on_resize(window* self)
        {
            {
                self->set_size(windowSize, windowSize);
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = common::on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = common::text_input;
        }
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t* enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { industry::widgets, widx::tab_industry, &industry::events, &industry::enabledWidgets },
            { production_2::widgets, widx::tab_production, &production::events, &common::enabledWidgets },
            { production_2::widgets, widx::tab_production_2, &production_2::events, &common::enabledWidgets },
            { transported::widgets, widx::tab_transported, &transported::events, &common::enabledWidgets }
        };

        static void setDisabledWidgets(window* self)
        {
            auto industryObj = objectmgr::get<industry_object>(industrymgr::get(self->number)->object_id);
            auto disabledWidgets = 0;

            if (industryObj->produced_cargo_type[0] == 0xFF)
                disabledWidgets |= (1 << common::widx::tab_production);

            if (industryObj->produced_cargo_type[1] == 0xFF)
                disabledWidgets |= (1 << common::widx::tab_production_2);

            self->disabled_widgets = disabledWidgets;
        }

        // 0x00456079
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            // Draw Units of Cargo sub title
            const auto industry = industrymgr::get(self->number);
            const auto industryObj = objectmgr::get<industry_object>(industry->object_id);
            const auto cargoObj = objectmgr::get<cargo_object>(industryObj->produced_cargo_type[0]);

            {
                auto args = FormatArguments();
                args.push(cargoObj->units_and_cargo_name);

                int16_t x = self->x + 2;
                int16_t y = self->y - 24 + 68;

                gfx::draw_string_494B3F(*dpi, x, y, colour::black, string_ids::production_graph_label, &args);
            }

            // Draw Y label and grid lines.
            const uint16_t graphBottom = self->y + self->height - 7;
            int32_t yTick = 0;
            for (int16_t yPos = graphBottom; yPos >= self->y + 68; yPos -= 20)
            {
                auto args = FormatArguments();
                args.push(yTick);

                gfx::draw_rect(dpi, self->x + 41, yPos, 239, 1, colour::get_shade(self->colours[1], 4));

                gfx::draw_string_494C78(*dpi, self->x + 39, yPos - 6, colour::black, string_ids::population_graph_people, &args);

                yTick += 1000;
            }

            month_id month = current_month();
            int16_t year = current_year();
            int8_t yearSkip = 0;
            // This is either 0 or 1 depending on selected tab
            // used to select the correct history
            const uint8_t productionTabWidx = self->current_tab + widx::tab_industry;
            const uint8_t productionNum = productionTabWidx - widx::tab_production;
            for (uint8_t i = industry->history_size[productionNum] - 1; i > 0; i--)
            {
                const uint16_t xPos = self->x + 41 + i;
                const uint16_t yPos = self->y + 56;

                // Draw horizontal year and vertical grid lines.
                if (month == month_id::january)
                {
                    if (yearSkip == 0)
                    {
                        auto args = FormatArguments();
                        args.push(year);

                        gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::population_graph_year, &args);
                    }

                    gfx::draw_rect(dpi, xPos, yPos + 11, 1, self->height - 74, colour::get_shade(self->colours[1], 4));
                }

                const auto history = productionTabWidx == widx::tab_production ? industry->history_1 : industry->history_2;
                // Draw production graph
                const uint16_t yPos1 = graphBottom - history[i];
                const uint16_t yPos2 = graphBottom - history[i + 1];

                // Do not draw current segment yet; it may be zeroed.
                if (i < industry->history_size[productionNum] - 1)
                {
                    if (yPos1 <= graphBottom)
                    {
                        if (yPos2 <= graphBottom)
                        {
                            gfx::draw_line(dpi, xPos, yPos1, xPos + 1, yPos2, colour::get_shade(self->colours[1], 7));
                        }
                    }
                }

                if (month == month_id::january)
                {
                    month = month_id::december;
                    year--;

                    yearSkip++;
                    if (yearSkip >= 3)
                        yearSkip = 0;
                }
                else
                {
                    month = month_id(static_cast<int8_t>(month) - 1);
                }
            }
        }

        // 0x004565B5, 0x00456505
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameIndustryPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_industry:
                case common::widx::tab_production:
                case common::widx::tab_production_2:
                case common::widx::tab_transported:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1ULL << widx::tab_industry) | (1ULL << widx::tab_production) | (1ULL << widx::tab_production_2) | (1ULL << widx::tab_transported));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Put industry name in place.
            auto industry = industrymgr::get(self->number);
            auto args = FormatArguments();
            args.push(industry->name);
            args.push(industry->town);

            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        // 0x00455CBC
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget != common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            gGameCommandErrorTitle = string_ids::error_cant_rename_industry;

            uint32_t* buffer = (uint32_t*)input;
            game_commands::do_79(self->number, 1, buffer[0], buffer[1], buffer[2]);
            game_commands::do_79(0, 2, buffer[3], buffer[4], buffer[5]);
            game_commands::do_79(0, 0, buffer[6], buffer[7], buffer[8]);
        }

        static void update(window* self)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::industry, self->number);
        }

        //0x00455D81
        static void renameIndustryPrompt(window* self, widget_index widgetIndex)
        {
            auto industry = industrymgr::get(self->number);
            if (!is_editor_mode())
            {
                if ((industry->flags & industry_flags::flag_04) == 0)
                    return;
                if (!is_player_company(industry->owner))
                    return;
            }

            auto args = FormatArguments();
            args.push<int64_t>(0);
            args.push(industry->name);
            args.push(industry->town);

            textinput::open_textinput(self, string_ids::title_industry_name, string_ids::prompt_enter_new_industry_name, industry->name, widgetIndex, &industry->town);
        }

        // 0x00456A5E, 0x00456A64
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_industry].left;
            const int16_t tabWidth = self->widgets[widx::tab_industry].right - xPos;

            for (uint8_t i = widx::tab_industry; i <= widx::tab_transported; i++)
            {
                if (self->is_disabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x00455CC7
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            ui::textinput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_industry;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);
            self->var_85C = -1;

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_industry];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            common::setDisabledWidgets(self);

            self->invalidate();

            self->set_size(industry::windowSize);
            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        static void drawProductionTab(window* self, gfx::drawpixelinfo_t* dpi, uint8_t productionTabNumber)
        {
            static const uint32_t productionTabImageIds[] = {
                interface_skin::image_ids::tab_production_frame0,
                interface_skin::image_ids::tab_production_frame1,
                interface_skin::image_ids::tab_production_frame2,
                interface_skin::image_ids::tab_production_frame3,
                interface_skin::image_ids::tab_production_frame4,
                interface_skin::image_ids::tab_production_frame5,
                interface_skin::image_ids::tab_production_frame6,
                interface_skin::image_ids::tab_production_frame7,
            };

            auto industry = industrymgr::get(self->number);
            auto industryObj = objectmgr::get<industry_object>(industry->object_id);
            auto skin = objectmgr::get<interface_skin_object>();

            static const uint32_t productionTabIds[] = {
                widx::tab_production,
                widx::tab_production_2,
            };

            auto tab = productionTabIds[productionTabNumber];

            uint32_t imageId = 0xFFFFFFFF;
            auto widget = self->widgets[tab];

            if (industryObj->produced_cargo_type[productionTabNumber] != 0xFF)
            {
                imageId = gfx::recolour(skin->img, self->colours[1]);

                if (self->current_tab == tab - widx::tab_industry)
                    imageId += productionTabImageIds[(self->frame_no / 4) % std::size(productionTabImageIds)];
                else
                    imageId += productionTabImageIds[0];

                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                gfx::draw_image(dpi, xPos, yPos, imageId);

                auto caroObj = objectmgr::get<cargo_object>(industryObj->produced_cargo_type[productionTabNumber]);
                gfx::draw_image(dpi, xPos + 18, yPos + 14, caroObj->unit_inline_sprite);

                widget::draw_tab(self, dpi, -2, tab);
            }
        }

        // 0x00456A98
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Industry tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_industries;
                widget::draw_tab(self, dpi, imageId, widx::tab_industry);
            }

            // Production Tab
            {
                drawProductionTab(self, dpi, 0);
            }

            // 2nd Production Tab
            {
                drawProductionTab(self, dpi, 1);
            }

            // Transported Tab
            {
                static const uint32_t transportedTabImageIds[] = {
                    interface_skin::image_ids::tab_transported_frame0,
                    interface_skin::image_ids::tab_transported_frame1,
                    interface_skin::image_ids::tab_transported_frame2,
                    interface_skin::image_ids::tab_transported_frame3,
                    interface_skin::image_ids::tab_transported_frame4,
                    interface_skin::image_ids::tab_transported_frame5,
                    interface_skin::image_ids::tab_transported_frame6,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_transported - widx::tab_industry)
                    imageId += transportedTabImageIds[(self->frame_no / 4) % std::size(transportedTabImageIds)];
                else
                    imageId += transportedTabImageIds[0];
                widget::draw_tab(self, dpi, imageId, widx::tab_transported);
            }
        }

        static void initEvents()
        {
            industry::initEvents();
            production::initEvents();
            production_2::initEvents();
            transported::initEvents();
        }
    }
}
