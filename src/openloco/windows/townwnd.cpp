#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../map/tile.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../viewportmgr.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town
{
    static const gfx::ui_size_t windowSize = { 223, 161 };

    static loco_global<uint16_t[10], 0x0112C826> commonFormatArgs;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_town,
            tab_population,
            tab_company_ratings,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 120 }, widget_type::panel, 1),                                                               \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_town),                         \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_population_graph),            \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_town_ratings_each_company)

        // Defined at the bottom of this file.
        static void prepare_draw(window* self);
        static void on_resize(window* self);
        static void text_input(window* self, widget_index callingWidget, char* input);
        static void update(window* self);
        static void renameTownPrompt(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void initEvents();
    }

    namespace town
    {
        enum widx
        {
            viewport = 7,
            unk_8,
            centre_on_viewport,
            expand_town,
            demolish_town,
        };

        static widget_t widgets[] = {
            commonWidgets(223, 161, string_ids::title_town),
            make_widget({ 3, 44 }, { 195, 104 }, widget_type::viewport, 1, 0xFFFFFFFE),
            make_widget({ 3, 139 }, { 195, 21 }, widget_type::wt_13, 1),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::town_expand, string_ids::expand_this_town),
            make_widget({ 198, 68 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rubbish_bin, string_ids::demolish_this_town),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << centre_on_viewport) | (1 << expand_town) | (1 << demolish_town);

        static window_event_list events;

        // 0x00498EAF
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::unk_8].top = self->height - 12;
            self->widgets[widx::unk_8].bottom = self->height - 3;
            self->widgets[widx::unk_8].right = self->width - 14;

            self->widgets[widx::expand_town].right = self->width - 2;
            self->widgets[widx::expand_town].left = self->width - 25;

            self->widgets[widx::demolish_town].right = self->width - 2;
            self->widgets[widx::demolish_town].left = self->width - 25;

            if (is_editor_mode())
            {
                self->widgets[widx::expand_town].type = widget_type::wt_9;
                self->widgets[widx::demolish_town].type = widget_type::wt_9;
            }
            else
            {
                self->widgets[widx::expand_town].type = widget_type::none;
                self->widgets[widx::demolish_town].type = widget_type::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x00498FFE
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            self->drawViewports(dpi);
            widget::drawViewportCentreButton(dpi, self, (widget_index)widx::centre_on_viewport);

            static string_id townSizeLabelIds[] = {
                string_ids::town_size_hamlet,
                string_ids::town_size_village,
                string_ids::town_size_town,
                string_ids::town_size_city,
                string_ids::town_size_metropolis,
            };

            auto town = townmgr::get(self->number);
            commonFormatArgs[0] = townSizeLabelIds[(uint8_t)town->size];
            *(int32_t*)&commonFormatArgs[1] = town->population;

            const uint16_t xPos = self->x + self->widgets[widx::unk_8].left - 1;
            const uint16_t yPos = self->y + self->widgets[widx::unk_8].top - 1;
            const uint16_t width = self->widgets[widx::unk_8].width() - 1;
            gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::status_town_population, &*commonFormatArgs);
        }

        // 0x00499079
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameTownPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_town:
                case common::widx::tab_population:
                case common::widx::tab_company_ratings:
                    common::switchTab(self, widgetIndex);
                    break;

                // 0x0049932D
                case widx::centre_on_viewport:
                {
                    if (self->viewports[0] == nullptr || self->saved_view.isEmpty())
                        break;

                    registers regs;
                    regs.ax = self->saved_view.mapX;
                    regs.cx = self->saved_view.mapY & 0x3FFF;
                    regs.dx = self->saved_view.surfaceZ;
                    regs.esi = (int32_t)WindowManager::getMainWindow();
                    call(0x004C6827, regs);
                    break;
                }

                // 0x004990B9
                case widx::expand_town:
                {
                    auto town = townmgr::get(self->number);

                    const uint32_t ebx = (town->var_38 >> 3) + 5;
                    const int16_t currentYear = current_year();
                    int16_t tempYear = currentYear - 51;
                    set_current_year(tempYear);

                    for (uint8_t i = 8; i > 0; i--)
                    {
                        for (uint32_t j = ebx; j > 0; j--)
                        {
                            registers regs;
                            regs.esi = (int32_t)town;
                            regs.eax = 0xFF;

                            call(0x00498116, regs);
                            call(0x004975E0, regs);
                        }

                        tempYear += 7;
                        set_current_year(tempYear);
                    }

                    set_current_year(currentYear);

                    // Set new history entry.
                    uint8_t historyEntry = std::min<uint8_t>(town->population / 50, 255);
                    town->history[town->history_size - 1] = historyEntry;

                    // Play construction sound at the town centre.
                    int16_t tileZ = openloco::map::tile_element_height(town->x, town->y) & 0xFFFF;
                    audio::play_sound(audio::sound_id::construct, loc16(town->x + 16, town->y + 16, tileZ));
                    break;
                }

                // 0x0049916A
                case widx::demolish_town:
                {
                    loco_global<string_id, 0x009C68E8> gameErrorString;
                    gameErrorString = string_ids::cant_remove_town;
                    bool success = game_commands::do_50(1, self->number);
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

        // 0x004993A5
        static void on_resize(window* self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self->set_size(gfx::ui_size_t(192, 161), gfx::ui_size_t(600, 440));

            common::on_resize(self);

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

        // 0x00499A87
        static void initViewport(window* self)
        {
            if (self->current_tab != 0)
                return;

            self->call_prepare_draw();

            // Figure out the town's position on the map.
            auto town = townmgr::get(self->number);
            int16_t tileZ = openloco::map::tile_element_height(town->x, town->y) & 0xFFFF;

            // Compute views.
            SavedView view = {
                town->x,
                town->y,
                2,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                tileZ,
            };

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

            // 0x00499B39 start
            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = openloco::map::map_pos3({ town->x, town->y, tileZ });
                auto zoomLevel = openloco::ui::viewportmgr::ZoomLevel::half;
                viewportmgr::create(self, 0, gfx::point_t(widget->left + self->x + 1, widget->top + self->y + 1), gfx::ui_size_t(widget->width() - 2, widget->height() - 2), zoomLevel, tile);
                self->invalidate();
                self->flags |= window_flags::viewport_no_scrolling;
            }
            // 0x00499B39 end

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

    // 0x00499B7E
    window* open(uint16_t townId)
    {
        auto window = WindowManager::bringToFront(WindowType::town, townId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::town, townId);
        }

        if (window == nullptr)
        {
            // 0x00499C0D start
            const uint32_t newFlags = window_flags::flag_8 | window_flags::resizable;
            window = WindowManager::createWindow(WindowType::town, windowSize, newFlags, &town::events);
            window->number = townId;
            window->min_width = 192;
            window->min_height = 161;
            window->max_width = 600;
            window->max_height = 440;

            auto skin = objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_0B;
                window->colours[1] = skin->colour_0C;
            }
            // 0x00499C0D end

            window->saved_view.clear();
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = 0;
        window->invalidate();

        window->widgets = town::widgets;
        window->enabled_widgets = town::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &town::events;
        window->activated_widgets = 0;
        window->disabled_widgets = 0;
        window->init_scroll_widgets();
        town::initViewport(window);

        return window;
    }

    namespace population
    {
        static widget_t widgets[] = {
            commonWidgets(223, 161, string_ids::title_town_population),
            widget_end(),
        };

        static window_event_list events;

        // 0x00499469
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);
        }

        // 0x004994F9
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            gfx::drawpixelinfo_t* clipped = nullptr;
            if (!gfx::clip_drawpixelinfo(&clipped, dpi, self->x, self->y + 44, self->width, self->height - 44))
                return;

            auto town = townmgr::get(self->number);

            // Draw Y label and grid lines.
            int32_t yTick = town->history_min_population;
            for (int16_t yPos = self->height - 57; yPos >= 14; yPos -= 20)
            {
                const uint16_t xPos = 39;
                gfx::draw_rect(clipped, xPos, yPos, 241, 1, colour::get_shade(self->colours[1], 4));

                *(int32_t*)&*commonFormatArgs = yTick;
                gfx::draw_string_494C78(*clipped, xPos, yPos - 6, colour::black, string_ids::population_graph_people, &*commonFormatArgs);

                yTick += 1000;
            }

            int8_t month = static_cast<int8_t>(current_month());
            int16_t year = current_year();
            int8_t yearSkip = 0;

            for (uint8_t i = town->history_size - 1; i > 0; i--)
            {
                const uint16_t xPos = 41 + i;
                const uint16_t yPos = 2;

                // Draw horizontal year and vertical grid lines.
                if (month == 0)
                {
                    if (yearSkip == 0)
                    {
                        commonFormatArgs[0] = year;
                        gfx::draw_string_centred(*clipped, xPos, yPos, colour::black, string_ids::population_graph_year, &*commonFormatArgs);
                    }

                    gfx::draw_rect(clipped, xPos, 11, 1, self->height - 66, colour::get_shade(self->colours[1], 4));
                }

                // Draw population graph
                uint16_t yPos1 = -town->history[i] + (self->height - 57);
                uint16_t yPos2 = -town->history[i + 1] + (self->height - 57);
                gfx::draw_line(clipped, xPos, yPos1, xPos + 1, yPos2, colour::get_shade(self->colours[1], 7));

                month--;
                if (month < 0)
                {
                    month = 11;
                    year--;

                    yearSkip++;
                    if (yearSkip >= 3)
                        yearSkip = 0;
                }
            }
        }

        // 0x004996AC
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameTownPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_town:
                case common::widx::tab_population:
                case common::widx::tab_company_ratings:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x004996F6
        static void on_resize(window* self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self->set_size(gfx::ui_size_t(299, 172), gfx::ui_size_t(299, 327));

            common::on_resize(self);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = common::text_input;
        }
    }

    namespace company_ratings
    {
        static widget_t widgets[] = {
            commonWidgets(340, 208, string_ids::title_town_local_authority),
            widget_end(),
        };

        static window_event_list events;

        // 0x00499761
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);
        }

        // 0x004997F1
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            uint16_t xPos = self->x + 4;
            uint16_t yPos = self->y + 46;
            gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::local_authority_ratings_transport_companies);

            xPos += 4;
            yPos += 14;
            auto town = townmgr::get(self->number);
            for (company_id_t i = 0; i < std::size(town->company_ratings); i++)
            {
                if ((town->companies_with_rating & (1 << i)) == 0)
                    continue;

                int16_t rating = (std::clamp<int16_t>(town->company_ratings[i], -1000, 1000) + 1000) / 20;
                string_id rank{};
                if (rating >= 70)
                    rank = string_ids::town_rating_excellent;
                else if (rating >= 60)
                    rank = string_ids::town_rating_good;
                else if (rating >= 50)
                    rank = string_ids::town_rating_average;
                else if (rating >= 25)
                    rank = string_ids::town_rating_poor;
                else
                    rank = string_ids::town_rating_appalling;

                commonFormatArgs[0] = companymgr::get(i)->name;
                commonFormatArgs[2] = rating;
                commonFormatArgs[3] = rank;
                gfx::draw_string_494BBF(*dpi, xPos, yPos, self->width - 12, colour::black, string_ids::town_rating_company_percentage_rank, &*commonFormatArgs);

                yPos += 10;
            }
        }

        // 0x004998E7
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameTownPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_town:
                case common::widx::tab_population:
                case common::widx::tab_company_ratings:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x00499936
        static void on_resize(window* self)
        {
            // Call to sub_498E9B has been deliberately omitted.

            self->set_size(gfx::ui_size_t(340, 208), gfx::ui_size_t(340, 208));

            common::on_resize(self);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
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
            { town::widgets, widx::tab_town, &town::events, &town::enabledWidgets },
            { population::widgets, widx::tab_population, &population::events, &common::enabledWidgets },
            { company_ratings::widgets, widx::tab_company_ratings, &company_ratings::events, &common::enabledWidgets }
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

            // Activate the current tab.
            self->activated_widgets &= ~((1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Put town name in place.
            commonFormatArgs[0] = townmgr::get(self->number)->name;

            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        static void on_resize(window* self)
        {
            if (self->width < self->min_width)
            {
                self->width = self->min_width;
                self->invalidate();
            }
            else if (self->width > self->max_width)
            {
                self->width = self->max_width;
                self->invalidate();
            }

            if (self->height < self->min_height)
            {
                self->height = self->min_height;
                self->invalidate();
            }
            else if (self->height > self->max_height)
            {
                self->height = self->max_height;
                self->invalidate();
            }
        }

        // 0x00499287
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget != common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            addr<0x009C68E8, string_id>() = string_ids::error_cant_rename_town;

            uint32_t* buffer = (uint32_t*)input;
            game_commands::do_46(1, self->number, 1, buffer[0], buffer[1], buffer[2]);
            game_commands::do_46(1, 0, 2, buffer[3], buffer[4], buffer[5]);
            game_commands::do_46(1, 0, 0, buffer[6], buffer[7], buffer[8]);
        }

        static void update(window* self)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::station, self->number);
        }

        static void renameTownPrompt(window* self, widget_index widgetIndex)
        {
            auto town = townmgr::get(self->number);
            commonFormatArgs[2] = town->name;
            textinput::open_textinput(self, string_ids::title_town_name, string_ids::prompt_type_new_town_name, town->name, widgetIndex, &commonFormatArgs[2]);
        }

        // 0x004999A7, 0x004999AD
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_town].left;
            const int16_t tabWidth = self->widgets[widx::tab_town].right - xPos;

            for (uint8_t i = widx::tab_town; i <= widx::tab_company_ratings; i++)
            {
                if (self->is_disabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x004991BC
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            textinput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_town;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);
            self->var_85C = -1;

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_town];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabled_widgets = 0;

            self->invalidate();

            self->set_size(windowSize);
            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x004999E1
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Town tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::toolbar_menu_towns;
                widget::draw_tab(self, dpi, imageId, widx::tab_town);
            }

            // Population tab
            {
                static const uint32_t populationTabImageIds[] = {
                    interface_skin::image_ids::tab_population_frame0,
                    interface_skin::image_ids::tab_population_frame1,
                    interface_skin::image_ids::tab_population_frame2,
                    interface_skin::image_ids::tab_population_frame3,
                    interface_skin::image_ids::tab_population_frame4,
                    interface_skin::image_ids::tab_population_frame5,
                    interface_skin::image_ids::tab_population_frame6,
                    interface_skin::image_ids::tab_population_frame7,
                };

                uint32_t imageId = gfx::recolour(skin->img, self->colours[1]);
                if (self->current_tab == widx::tab_population - widx::tab_town)
                    imageId += populationTabImageIds[(self->frame_no / 4) % std::size(populationTabImageIds)];
                else
                    imageId += populationTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_population);
            }

            // Company ratings tab
            {
                static const uint32_t ratingsTabImageIds[] = {
                    interface_skin::image_ids::tab_ratings_frame0,
                    interface_skin::image_ids::tab_ratings_frame1,
                    interface_skin::image_ids::tab_ratings_frame2,
                    interface_skin::image_ids::tab_ratings_frame3,
                    interface_skin::image_ids::tab_ratings_frame4,
                    interface_skin::image_ids::tab_ratings_frame5,
                    interface_skin::image_ids::tab_ratings_frame6,
                    interface_skin::image_ids::tab_ratings_frame7,
                    interface_skin::image_ids::tab_ratings_frame8,
                    interface_skin::image_ids::tab_ratings_frame9,
                    interface_skin::image_ids::tab_ratings_frame10,
                    interface_skin::image_ids::tab_ratings_frame11,
                    interface_skin::image_ids::tab_ratings_frame12,
                    interface_skin::image_ids::tab_ratings_frame13,
                    interface_skin::image_ids::tab_ratings_frame14,
                    interface_skin::image_ids::tab_ratings_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_company_ratings - widx::tab_town)
                    imageId += ratingsTabImageIds[(self->frame_no / 4) % std::size(ratingsTabImageIds)];
                else
                    imageId += ratingsTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_company_ratings);
            }
        }

        static void initEvents()
        {
            town::initEvents();
            population::initEvents();
            company_ratings::initEvents();
        }
    }
}
