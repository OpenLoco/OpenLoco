#include "../CompanyManager.h"
#include "../Config.h"
#include "../GameCommands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../Input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../map/tile_loop.hpp"
#include "../map/tilemgr.h"
#include "../objects/cargo_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"
#include "../viewportmgr.h"
#include "../widget.h"

using namespace openloco::interop;
using namespace openloco::map;

namespace openloco::ui::windows::station
{
    static loco_global<uint8_t[map_size], 0x00F00484> _byte_F00484;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    static loco_global<uint16_t, 0x00112C786> _lastSelectedStation;

    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;

    namespace common
    {
        static const gfx::ui_size_t minWindowSize = { 192, 136 };

        static const gfx::ui_size_t maxWindowSize = { 600, 440 };

        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_station,
            tab_cargo,
            tab_cargo_ratings,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_station) | (1 << widx::tab_cargo) | (1 << widx::tab_cargo_ratings);

#define commonWidgets(frameWidth, frameHeight)                                                                                           \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_23, 0, string_ids::title_station),                             \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 95 }, widget_type::panel, 1),                                                                \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_station),                       \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_station_cargo),                \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_station_cargo_ratings)

        // Defined at the bottom of this file.
        static void prepareDraw(window* self);
        static void textInput(window* self, widget_index callingWidget, char* input);
        static void update(window* self);
        static void renameStationPrompt(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void enableRenameByCaption(window* self);
        static void initEvents();
    }

    namespace station
    {
        static const gfx::ui_size_t windowSize = { 223, 136 };

        enum widx
        {
            viewport = 7,
            status_bar,
            centre_on_viewport,
        };

        widget_t widgets[] = {
            // commonWidgets(windowSize.width, windowSize.height),
            commonWidgets(223, 136),
            makeWidget({ 3, 44 }, { 195, 80 }, widget_type::viewport, 1, 0xFFFFFFFE),
            makeWidget({ 3, 115 }, { 195, 21 }, widget_type::wt_13, 1),
            makeWidget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << centre_on_viewport);

        static window_event_list events;

        // 0x0048E352
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->widgets[widx::viewport].right = self->width - 4;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x0048E470
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            self->drawViewports(dpi);
            widget::drawViewportCentreButton(dpi, self, widx::centre_on_viewport);

            auto station = stationmgr::get(self->number);
            const char* buffer = stringmgr::getString(string_ids::buffer_1250);
            station->getStatusString((char*)buffer);

            auto args = FormatArguments();
            args.push(string_ids::buffer_1250);

            const auto& widget = self->widgets[widx::status_bar];
            const auto x = self->x + widget.left - 1;
            const auto y = self->y + widget.top - 1;
            const auto width = widget.width() - 1;
            gfx::drawString_494BBF(*dpi, x, y, width, colour::black, string_ids::black_stringid, &args);
        }

        // 0x0048E4D4
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameStationPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_station:
                case common::widx::tab_cargo:
                case common::widx::tab_cargo_ratings:
                    common::switchTab(self, widgetIndex);
                    break;

                // 0x0049932D
                case widx::centre_on_viewport:
                {
                    if (self->viewports[0] == nullptr || self->saved_view.isEmpty())
                        break;
                    auto main = WindowManager::getMainWindow();
                    if (self->saved_view.isThingView())
                    {
                        auto thing = thingmgr::get<Thing>(self->saved_view.thingId);
                        main->viewportCentreOnTile({ thing->x, thing->y, thing->z });
                    }
                    else
                    {
                        main->viewportCentreOnTile(self->saved_view.getPos());
                    }
                    break;
                }
            }
        }

        static void initViewport(window* self);

        // 0x0048E70B
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);

            self->setSize(windowSize, common::maxWindowSize);

            if (self->viewports[0] != nullptr)
            {
                uint16_t newWidth = self->height - 8;
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

        // 0x0048F11B
        static void initViewport(window* self)
        {
            if (self->current_tab != 0)
                return;

            self->callPrepareDraw();

            // Figure out the station's position on the map.
            auto station = stationmgr::get(self->number);

            // Compute views.

            SavedView view = {
                station->x,
                station->y,
                ZoomLevel::half,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                station->z,
            };
            view.flags |= (1 << 14);

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
            // Remove station names from viewport
            flags |= viewport_flags::station_names_displayed;

            self->saved_view = view;

            // 0x0048F1CB start
            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = openloco::map::map_pos3({ station->x, station->y, station->z });
                auto origin = gfx::point_t(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = gfx::ui_size_t(widget->width() - 2, widget->height() - 2);
                viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, tile);
                self->invalidate();
                self->flags |= window_flags::viewport_no_scrolling;
            }
            // 0x0048F1CB end

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->flags = flags;
                self->invalidate();
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
            events.viewport_rotate = initViewport;
        }
    }

    // 0x0048F210
    window* open(uint16_t stationId)
    {
        auto window = WindowManager::bringToFront(WindowType::station, stationId);
        if (window != nullptr)
        {
            if (input::isToolActive(window->type, window->number))
                input::toolCancel();

            window = WindowManager::bringToFront(WindowType::station, stationId);
        }

        if (window == nullptr)
        {
            // 0x0048F29F start
            const uint32_t newFlags = window_flags::resizable | window_flags::flag_11;
            window = WindowManager::createWindow(WindowType::station, station::windowSize, newFlags, &station::events);
            window->number = stationId;
            auto station = stationmgr::get(stationId);
            window->owner = station->owner;
            window->min_width = common::minWindowSize.width;
            window->min_height = common::minWindowSize.height;
            window->max_width = common::maxWindowSize.width;
            window->max_height = common::maxWindowSize.height;

            window->saved_view.clear();

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0A;
            // 0x0048F29F end
        }
        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = common::widx::tab_station - common::widx::tab_station;
        window->invalidate();

        window->widgets = station::widgets;
        window->enabled_widgets = station::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &station::events;
        window->activated_widgets = 0;
        window->disabled_widgets = 0;
        window->initScrollWidgets();
        station::initViewport(window);

        return window;
    }

    namespace cargo
    {
        enum widx
        {
            scrollview = 7,
            status_bar,
            station_catchment,
        };

        static widget_t widgets[] = {
            commonWidgets(223, 136),
            makeWidget({ 3, 44 }, { 217, 80 }, widget_type::scrollview, 1, 2),
            makeWidget({ 3, 125 }, { 195, 10 }, widget_type::wt_13, 1),
            makeWidget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::show_station_catchment, string_ids::station_catchment),
            widgetEnd(),
        };

        static window_event_list events;

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << station_catchment);

        // 0x0048E7C0
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 24;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::station_catchment].right = self->width - 2;
            self->widgets[widx::station_catchment].left = self->width - 25;

            common::repositionTabs(self);

            self->activated_widgets &= ~(1 << widx::station_catchment);
            if (self->number == _lastSelectedStation)
                self->activated_widgets |= (1 << widx::station_catchment);
        }

        // 0x0048E8DE
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto buffer = const_cast<char*>(stringmgr::getString(string_ids::buffer_1250));
            buffer = stringmgr::formatString(buffer, string_ids::accepted_cargo_separator);

            auto station = stationmgr::get(self->number);
            uint8_t cargoTypeCount = 0;

            for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
            {
                auto& stats = station->cargo_stats[cargoId];

                if (!stats.isAccepted())
                    continue;

                *buffer++ = ' ';
                *buffer++ = control_codes::inline_sprite_str;
                *(reinterpret_cast<uint32_t*>(buffer)) = objectmgr::get<cargo_object>(cargoId)->unit_inline_sprite;
                buffer += 4;

                cargoTypeCount++;
            }

            if (cargoTypeCount == 0)
            {
                buffer = stringmgr::formatString(buffer, string_ids::cargo_nothing_accepted);
            }

            *buffer++ = '\0';

            const auto& widget = self->widgets[widx::status_bar];
            const auto x = self->x + widget.left - 1;
            const auto y = self->y + widget.top - 1;
            const auto width = widget.width();

            gfx::drawString_494BBF(*dpi, x, y, width, colour::black, string_ids::buffer_1250);
        }

        // 0x0048EB0B
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameStationPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_station:
                case common::widx::tab_cargo:
                case common::widx::tab_cargo_ratings:
                    common::switchTab(self, widgetIndex);
                    break;

                case widx::station_catchment:
                {
                    station_id_t windowNumber = self->number;
                    if (windowNumber == _lastSelectedStation)
                        windowNumber = station_id::null;

                    showStationCatchment(windowNumber);
                    break;
                }
            }
        }

        // 0x0048EBB7
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);

            self->setSize(common::minWindowSize, common::maxWindowSize);
        }

        // 0x0048EB64
        static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            auto station = stationmgr::get(self->number);
            *scrollHeight = 0;
            for (const auto& cargoStats : station->cargo_stats)
            {
                if (cargoStats.quantity != 0)
                {
                    *scrollHeight += 12;
                    if (cargoStats.origin != self->number)
                        *scrollHeight += 10;
                }
            }
        }

        // 0x0048EB4F
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_cargo_list);
        }

        // 0x0048E986
        static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            gfx::clearSingle(*dpi, colour::getShade(self->colours[1], 4));

            const auto station = stationmgr::get(self->number);
            int16_t y = 1;
            auto cargoId = 0;
            for (const auto& cargoStats : station->cargo_stats)
            {
                // auto& cargo = station->cargo_stats[i];
                auto& cargo = cargoStats;
                auto quantity = cargo.quantity;
                if (quantity == 0)
                {
                    cargoId++;
                    continue;
                }

                quantity = std::min(int(quantity), 400);

                auto units = (quantity + 9) / 10;

                auto cargoObj = objectmgr::get<cargo_object>(cargoId);
                if (units != 0)
                {
                    uint16_t xPos = 1;
                    for (; units > 0; units--)
                    {
                        {
                            gfx::drawImage(dpi, xPos, y, cargoObj->unit_inline_sprite);
                            xPos += 10;
                        }
                    }
                }
                auto cargoName = cargoObj->unit_name_singular;

                if (cargo.quantity != 1)
                    cargoName = cargoObj->unit_name_plural;

                auto args = FormatArguments();
                args.push(cargoName);
                args.push<uint32_t>(cargo.quantity);
                auto cargoStr = string_ids::station_cargo;

                if (cargo.origin != self->number)
                    cargoStr = string_ids::station_cargo_en_route_start;
                const auto& widget = self->widgets[widx::scrollview];
                auto xPos = widget.width() - 14;

                gfx::drawString_494C78(*dpi, xPos, y, colour::outline(colour::black), cargoStr, &args);
                y += 10;
                if (cargo.origin != self->number)
                {
                    auto originStation = stationmgr::get(cargo.origin);
                    auto args2 = FormatArguments();
                    args2.push(originStation->name);
                    args2.push(originStation->town);

                    gfx::drawString_494C78(*dpi, xPos, y, colour::outline(colour::black), string_ids::station_cargo_en_route_end, &args2);
                    y += 10;
                }
                y += 2;
                cargoId++;
            }

            uint16_t totalUnits = 0;
            for (const auto& stats : station->cargo_stats)
                totalUnits += stats.quantity;

            if (totalUnits == 0)
            {
                auto args = FormatArguments();
                args.push(string_ids::nothing_waiting);
                gfx::drawString_494B3F(*dpi, 1, 0, colour::black, string_ids::black_stringid, &args);
            }
        }

        // 0x0048EC21
        static void onClose(window* self)
        {
            if (self->number == _lastSelectedStation)
            {
                showStationCatchment(station_id::null);
            }
        }

        static void initEvents()
        {
            events.on_close = onClose;
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
            events.get_scroll_size = getScrollSize;
            events.tooltip = tooltip;
            events.draw_scroll = drawScroll;
        }
    }

    namespace cargo_ratings
    {
        static const gfx::ui_size_t windowSize = { 249, 136 };

        static const gfx::ui_size_t maxWindowSize = { 249, 440 };

        enum widx
        {
            scrollview = 7,
            status_bar,
        };

        static widget_t widgets[] = {
            commonWidgets(249, 136),
            makeWidget({ 3, 44 }, { 244, 80 }, widget_type::scrollview, 1, 2),
            makeWidget({ 3, 125 }, { 221, 11 }, widget_type::wt_13, 1),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0048EC3B
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            common::repositionTabs(self);
        }

        // 0x0048ED24
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
        }

        // 0x0048EE1A
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameStationPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_station:
                case common::widx::tab_cargo:
                case common::widx::tab_cargo_ratings:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0048EE97
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);

            self->setSize(windowSize, maxWindowSize);
        }

        // 0x0048EE4A
        static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            auto station = stationmgr::get(self->number);
            *scrollHeight = 0;
            for (uint8_t i = 0; i < 32; i++)
            {
                if (station->cargo_stats[i].origin != station_id::null)
                    *scrollHeight += 10;
            }
        }

        // 0x0048EE73
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_ratings_list);
        }

        // 0x0048EF02
        static void drawRatingBar(window* self, gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, uint8_t amount, colour_t colour)
        {
            gfx::fillRectInset(dpi, x, y, x + 99, y + 9, self->colours[1], 48);

            uint16_t rating = (amount * 96) / 256;
            if (rating > 2)
            {
                gfx::fillRectInset(dpi, x + 2, y + 2, x + 1 + rating, y + 8, colour, 0);
            }
        }

        // 0x0048ED2F
        static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            gfx::clearSingle(*dpi, colour::getShade(self->colours[1], 4));

            const auto station = stationmgr::get(self->number);
            int16_t y = 0;
            auto cargoId = 0;
            for (const auto& cargoStats : station->cargo_stats)
            {
                auto& cargo = cargoStats;
                if (cargo.empty())
                {
                    cargoId++;
                    continue;
                }

                auto cargoObj = objectmgr::get<cargo_object>(cargoId);
                gfx::drawString_494BBF(*dpi, 1, y, 98, 0, string_ids::wcolour2_stringid, &cargoObj->name);

                auto rating = cargo.rating;
                auto colour = colour::moss_green;
                if (rating < 100)
                {
                    colour = colour::dark_olive_green;
                    if (rating < 50)
                    {
                        colour = colour::saturated_red;
                    }
                }

                uint8_t amount = (rating * 327) / 256;
                drawRatingBar(self, dpi, 100, y, amount, colour);

                uint16_t percent = rating / 2;
                gfx::drawString_494B3F(*dpi, 201, y, 0, string_ids::station_cargo_rating_percent, &percent);
                y += 10;
                cargoId++;
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_update = common::update;
            events.prepare_draw = prepareDraw;
            events.text_input = common::textInput;
            events.get_scroll_size = getScrollSize;
            events.tooltip = tooltip;
            events.draw_scroll = drawScroll;
        }
    }

    // 0x00491BC6
    static void sub_491BC6()
    {
        tile_loop tileLoop;

        for (uint32_t posId = 0; posId < map_size; posId++)
        {
            if (_byte_F00484[posId] & (1 << 0))
            {
                tilemgr::mapInvalidateTileFull(tileLoop.current());
            }
            tileLoop.next();
        }
    }

    // 0x0049271A
    void showStationCatchment(uint16_t stationId)
    {
        if (stationId == _lastSelectedStation)
            return;

        uint16_t oldStationId = _lastSelectedStation;
        _lastSelectedStation = stationId;

        if (oldStationId != station_id::null)
        {
            if (input::hasMapSelectionFlag(input::map_selection_flags::catchment_area))
            {
                WindowManager::invalidate(WindowType::station, oldStationId);
                sub_491BC6();
                input::resetMapSelectionFlag(input::map_selection_flags::catchment_area);
            }
        }

        auto newStationId = _lastSelectedStation;

        if (newStationId != station_id::null)
        {
            ui::windows::construction::sub_4A6FAC();
            auto station = stationmgr::get(_lastSelectedStation);

            station->setCatchmentDisplay(0);
            input::setMapSelectionFlags(input::map_selection_flags::catchment_area);

            WindowManager::invalidate(WindowType::station, newStationId);

            sub_491BC6();
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
            { station::widgets, widx::tab_station, &station::events, &station::enabledWidgets },
            { cargo::widgets, widx::tab_cargo, &cargo::events, &cargo::enabledWidgets },
            { cargo_ratings::widgets, widx::tab_cargo_ratings, &cargo_ratings::events, &common::enabledWidgets }
        };

        // 0x0048E352, 0x0048E7C0 and 0x0048EC3B
        static void prepareDraw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1ULL << widx::tab_station) | (1ULL << widx::tab_cargo) | (1ULL << widx::tab_cargo_ratings));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Put station and town name in place.
            auto station = stationmgr::get(self->number);

            uint32_t stationTypeImages[16] = {
                string_ids::label_icons_none,
                string_ids::label_icons_rail,
                string_ids::label_icons_road,
                string_ids::label_icons_rail_road,
                string_ids::label_icons_air,
                string_ids::label_icons_rail_air,
                string_ids::label_icons_road_air,
                string_ids::label_icons_rail_road_air,
                string_ids::label_icons_water,
                string_ids::label_icons_rail_water,
                string_ids::label_icons_road_water,
                string_ids::label_icons_rail_road_water,
                string_ids::label_icons_air_water,
                string_ids::label_icons_rail_air_water,
                string_ids::label_icons_road_air_water,
                string_ids::label_icons_rail_road_air_water
            };
            auto args = FormatArguments();
            args.push(station->name);
            args.push(station->town);
            args.push(stationTypeImages[(station->flags & 0xF)]);

            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        // 0x0048E5DF
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget != common::widx::caption)
                return;

            if (strlen(input) == 0)
                return;

            gGameCommandErrorTitle = string_ids::error_cant_rename_station;

            uint32_t* buffer = (uint32_t*)input;
            game_commands::do_11(self->number, 1, buffer[0], buffer[1], buffer[2]);
            game_commands::do_11(0, 2, buffer[3], buffer[4], buffer[5]);
            game_commands::do_11(0, 0, buffer[6], buffer[7], buffer[8]);
        }

        // 0x0048E6F1
        static void update(window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::station, self->number);
        }

        // 0x0048E5E7
        static void renameStationPrompt(window* self, widget_index widgetIndex)
        {
            auto station = stationmgr::get(self->number);
            auto args = FormatArguments();
            args.push<int64_t>(0);
            args.push(station->name);
            args.push(station->town);

            textinput::openTextinput(self, string_ids::title_station_name, string_ids::prompt_type_new_station_name, station->name, widgetIndex, &station->town);
        }

        // 0x0048EF82, 0x0048EF88
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_station].left;
            const int16_t tabWidth = self->widgets[widx::tab_station].right - xPos;

            for (uint8_t i = widx::tab_station; i <= widx::tab_cargo_ratings; i++)
            {
                if (self->isDisabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x0048E520
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (widgetIndex != widx::tab_cargo)
            {
                if (self->number == _lastSelectedStation)
                {
                    showStationCatchment(station_id::null);
                }
            }

            if (input::isToolActive(self->type, self->number))
                input::toolCancel();

            textinput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_station;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);
            self->var_85C = -1;

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_station];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabled_widgets = 0;

            self->invalidate();

            self->setSize(station::windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x0048EFBC
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();
            auto station = stationmgr::get(self->number);
            auto companyColour = companymgr::getCompanyColour(station->owner);

            // Station tab
            {
                uint32_t imageId = gfx::recolour(skin->img, companyColour);
                imageId += interface_skin::image_ids::toolbar_menu_stations;
                widget::draw_tab(self, dpi, imageId, widx::tab_station);
            }

            // Cargo tab
            {
                static const uint32_t cargoTabImageIds[] = {
                    interface_skin::image_ids::tab_cargo_delivered_frame0,
                    interface_skin::image_ids::tab_cargo_delivered_frame1,
                    interface_skin::image_ids::tab_cargo_delivered_frame2,
                    interface_skin::image_ids::tab_cargo_delivered_frame3,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_cargo - widx::tab_station)
                    imageId += cargoTabImageIds[(self->frame_no / 8) % std::size(cargoTabImageIds)];
                else
                    imageId += cargoTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_cargo);
            }

            // Cargo ratings tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::tab_cargo_ratings;
                widget::draw_tab(self, dpi, imageId, widx::tab_cargo_ratings);

                auto widget = self->widgets[widx::tab_cargo_ratings];
                auto yOffset = widget.top + self->y + 14;
                auto xOffset = widget.left + self->x + 4;
                auto totalRatingBars = 0;

                for (const auto& cargoStats : station->cargo_stats)
                {
                    auto& cargo = cargoStats;
                    if (!cargo.empty())
                    {
                        gfx::fillRect(dpi, xOffset, yOffset, xOffset + 22, yOffset + 1, (1 << 25) | palette_index::index_30);

                        auto ratingColour = colour::moss_green;
                        if (cargo.rating < 100)
                        {
                            ratingColour = colour::dark_olive_green;
                            if (cargo.rating < 50)
                                ratingColour = colour::saturated_red;
                        }

                        auto ratingBarLength = (cargo.rating * 30) / 256;
                        gfx::fillRect(dpi, xOffset, yOffset, xOffset - 1 + ratingBarLength, yOffset + 1, colour::getShade(ratingColour, 6));

                        yOffset += 3;
                        totalRatingBars++;
                        if (totalRatingBars >= 4)
                            break;
                    }
                }
            }
        }

        // 0x0048E32C
        static void enableRenameByCaption(window* self)
        {
            auto station = stationmgr::get(self->number);
            if (station->owner != 255)
            {
                if (isPlayerCompany(station->owner))
                {
                    self->enabled_widgets |= (1 << common::widx::caption);
                }
                else
                {
                    self->enabled_widgets &= ~(1 << common::widx::caption);
                }
            }
        }

        static void initEvents()
        {
            station::initEvents();
            cargo::initEvents();
            cargo_ratings::initEvents();
        }
    }
}
