#include "../CompanyManager.h"
#include "../Config.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileLoop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/CargoObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../StationManager.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui::Windows::Station
{
    static loco_global<uint8_t[map_size], 0x00F00484> _byte_F00484;
    static loco_global<StationId, 0x00112C786> _lastSelectedStation;

    namespace Common
    {
        static const Ui::Size minWindowSize = { 192, 136 };

        static const Ui::Size maxWindowSize = { 600, 440 };

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

#define commonWidgets(frameWidth, frameHeight)                                                                                                                       \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_23, WindowColour::primary, StringIds::title_station),                                       \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 95 }, WidgetType::panel, WindowColour::secondary),                                                                       \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_station),                                 \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_station_cargo),                          \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_station_cargo_ratings)

        // Defined at the bottom of this file.
        static void prepareDraw(Window* self);
        static void textInput(Window* self, WidgetIndex_t callingWidget, const char* input);
        static void update(Window* self);
        static void renameStationPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void repositionTabs(Window* self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void enableRenameByCaption(Window* self);
        static void initEvents();
    }

    namespace Station
    {
        static const Ui::Size windowSize = { 223, 136 };

        enum widx
        {
            viewport = 7,
            status_bar,
            centre_on_viewport,
        };

        Widget widgets[] = {
            // commonWidgets(windowSize.width, windowSize.height),
            commonWidgets(223, 136),
            makeWidget({ 3, 44 }, { 195, 80 }, WidgetType::viewport, WindowColour::secondary, 0xFFFFFFFE),
            makeWidget({ 3, 115 }, { 195, 21 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::null, StringIds::move_main_view_to_show_this),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << centre_on_viewport);

        static WindowEventList events;

        // 0x0048E352
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::viewport].right = self->width - 4;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            Common::repositionTabs(self);
        }

        // 0x0048E470
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
            self->drawViewports(context);
            Widget::drawViewportCentreButton(context, self, widx::centre_on_viewport);

            auto station = StationManager::get(StationId(self->number));
            const char* buffer = StringManager::getString(StringIds::buffer_1250);
            station->getStatusString((char*)buffer);

            auto args = FormatArguments();
            args.push(StringIds::buffer_1250);

            const auto& widget = self->widgets[widx::status_bar];
            const auto x = self->x + widget.left - 1;
            const auto y = self->y + widget.top - 1;
            const auto width = widget.width() - 1;
            Gfx::drawString_494BBF(*context, x, y, width, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x0048E4D4
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameStationPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_station:
                case Common::widx::tab_cargo:
                case Common::widx::tab_cargo_ratings:
                    Common::switchTab(self, widgetIndex);
                    break;

                // 0x0049932D
                case widx::centre_on_viewport:
                    self->viewportCentreMain();
                    break;
            }
        }

        static void initViewport(Window* self);

        // 0x0048E70B
        static void onResize(Window* self)
        {
            Common::enableRenameByCaption(self);

            self->setSize(windowSize, Common::maxWindowSize);

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
                    self->savedView.clear();
                }
            }

            initViewport(self);
        }

        // 0x0048F11B
        static void initViewport(Window* self)
        {
            if (self->currentTab != 0)
                return;

            self->callPrepareDraw();

            // Figure out the station's position on the map.
            auto station = StationManager::get(StationId(self->number));

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
                if (self->savedView == view)
                    return;

                flags = self->viewports[0]->flags;
                self->viewportRemove(0);
                ViewportManager::collectGarbage();
            }
            else
            {
                if ((Config::get().flags & Config::Flags::gridlinesOnLandscape) != 0)
                    flags |= ViewportFlags::gridlines_on_landscape;
            }
            // Remove station names from viewport
            flags |= ViewportFlags::station_names_displayed;

            self->savedView = view;

            // 0x0048F1CB start
            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = Map::Pos3({ station->x, station->y, station->z });
                auto origin = Ui::Point(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = Ui::Size(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(self, 0, origin, size, self->savedView.zoomLevel, tile);
                self->invalidate();
                self->flags |= WindowFlags::viewportNoScrolling;
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
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
            events.viewportRotate = initViewport;
        }
    }

    // 0x0048F210
    Window* open(StationId stationId)
    {
        auto window = WindowManager::bringToFront(WindowType::station, enumValue(stationId));
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
                Input::toolCancel();

            window = WindowManager::bringToFront(WindowType::station, enumValue(stationId));
        }

        if (window == nullptr)
        {
            // 0x0048F29F start
            const uint32_t newFlags = WindowFlags::resizable | WindowFlags::flag_11;
            window = WindowManager::createWindow(WindowType::station, Station::windowSize, newFlags, &Station::events);
            window->number = enumValue(stationId);
            auto station = StationManager::get(stationId);
            window->owner = station->owner;
            window->minWidth = Common::minWindowSize.width;
            window->minHeight = Common::minWindowSize.height;
            window->maxWidth = Common::maxWindowSize.width;
            window->maxHeight = Common::maxWindowSize.height;

            window->savedView.clear();

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_0A);
            // 0x0048F29F end
        }
        // TODO(avgeffen): only needs to be called once.
        Common::initEvents();

        window->currentTab = Common::widx::tab_station - Common::widx::tab_station;
        window->invalidate();

        window->widgets = Station::widgets;
        window->enabledWidgets = Station::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Station::events;
        window->activatedWidgets = 0;
        window->disabledWidgets = 0;
        window->initScrollWidgets();
        Station::initViewport(window);

        return window;
    }

    void reset()
    {
        _lastSelectedStation = StationId::null;
    }

    namespace Cargo
    {
        enum widx
        {
            scrollview = 7,
            status_bar,
            station_catchment,
        };

        static Widget widgets[] = {
            commonWidgets(223, 136),
            makeWidget({ 3, 44 }, { 217, 80 }, WidgetType::scrollview, WindowColour::secondary, 2),
            makeWidget({ 3, 125 }, { 195, 10 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 198, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::show_station_catchment, StringIds::station_catchment),
            widgetEnd(),
        };

        static WindowEventList events;

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << station_catchment);

        // 0x0048E7C0
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 24;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::station_catchment].right = self->width - 2;
            self->widgets[widx::station_catchment].left = self->width - 25;

            Common::repositionTabs(self);

            self->activatedWidgets &= ~(1 << widx::station_catchment);
            if (StationId(self->number) == _lastSelectedStation)
                self->activatedWidgets |= (1 << widx::station_catchment);
        }

        // 0x0048E8DE
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            buffer = StringManager::formatString(buffer, StringIds::accepted_cargo_separator);

            auto station = StationManager::get(StationId(self->number));
            uint8_t cargoTypeCount = 0;

            for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
            {
                auto& stats = station->cargoStats[cargoId];

                if (!stats.isAccepted())
                    continue;

                *buffer++ = ' ';
                *buffer++ = ControlCodes::inline_sprite_str;
                *(reinterpret_cast<uint32_t*>(buffer)) = ObjectManager::get<CargoObject>(cargoId)->unit_inline_sprite;
                buffer += 4;

                cargoTypeCount++;
            }

            if (cargoTypeCount == 0)
            {
                buffer = StringManager::formatString(buffer, StringIds::cargo_nothing_accepted);
            }

            *buffer++ = '\0';

            const auto& widget = self->widgets[widx::status_bar];
            const auto x = self->x + widget.left - 1;
            const auto y = self->y + widget.top - 1;
            const auto width = widget.width();

            Gfx::drawString_494BBF(*context, x, y, width, Colour::black, StringIds::buffer_1250);
        }

        // 0x0048EB0B
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameStationPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_station:
                case Common::widx::tab_cargo:
                case Common::widx::tab_cargo_ratings:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::station_catchment:
                {
                    StationId windowNumber = StationId(self->number);
                    if (windowNumber == _lastSelectedStation)
                        windowNumber = StationId::null;

                    showStationCatchment(windowNumber);
                    break;
                }
            }
        }

        // 0x0048EBB7
        static void onResize(Window* self)
        {
            Common::enableRenameByCaption(self);

            self->setSize(Common::minWindowSize, Common::maxWindowSize);
        }

        // 0x0048EB64
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            auto station = StationManager::get(StationId(self->number));
            *scrollHeight = 0;
            for (const auto& cargoStats : station->cargoStats)
            {
                if (cargoStats.quantity != 0)
                {
                    *scrollHeight += 12;
                    if (cargoStats.origin != StationId(self->number))
                        *scrollHeight += 10;
                }
            }
        }

        // 0x0048EB4F
        static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_cargo_list);
            return args;
        }

        // 0x0048E986
        static void drawScroll(Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            Gfx::clearSingle(context, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

            const auto station = StationManager::get(StationId(self.number));
            int16_t y = 1;
            auto cargoId = 0;
            for (const auto& cargoStats : station->cargoStats)
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

                auto cargoObj = ObjectManager::get<CargoObject>(cargoId);
                if (units != 0)
                {
                    uint16_t xPos = 1;
                    for (; units > 0; units--)
                    {
                        {
                            Gfx::drawImage(&context, xPos, y, cargoObj->unit_inline_sprite);
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
                auto cargoStr = StringIds::station_cargo;

                if (cargo.origin != StationId(self.number))
                    cargoStr = StringIds::station_cargo_en_route_start;
                const auto& widget = self.widgets[widx::scrollview];
                auto xPos = widget.width() - 14;

                Gfx::drawString_494C78(context, xPos, y, AdvancedColour(Colour::black).outline(), cargoStr, &args);
                y += 10;
                if (cargo.origin != StationId(self.number))
                {
                    auto originStation = StationManager::get(cargo.origin);
                    auto args2 = FormatArguments();
                    args2.push(originStation->name);
                    args2.push(originStation->town);

                    Gfx::drawString_494C78(context, xPos, y, AdvancedColour(Colour::black).outline(), StringIds::station_cargo_en_route_end, &args2);
                    y += 10;
                }
                y += 2;
                cargoId++;
            }

            uint16_t totalUnits = 0;
            for (const auto& stats : station->cargoStats)
                totalUnits += stats.quantity;

            if (totalUnits == 0)
            {
                auto args = FormatArguments();
                args.push(StringIds::nothing_waiting);
                Gfx::drawString_494B3F(context, 1, 0, Colour::black, StringIds::black_stringid, &args);
            }
        }

        // 0x0048EC21
        static void onClose(Window* self)
        {
            if (StationId(self->number) == _lastSelectedStation)
            {
                showStationCatchment(StationId::null);
            }
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.draw = draw;
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
            events.getScrollSize = getScrollSize;
            events.tooltip = tooltip;
            events.drawScroll = drawScroll;
        }
    }

    namespace CargoRatings
    {
        static const Ui::Size windowSize = { 249, 136 };

        static const Ui::Size maxWindowSize = { 249, 440 };

        enum widx
        {
            scrollview = 7,
            status_bar,
        };

        static Widget widgets[] = {
            commonWidgets(249, 136),
            makeWidget({ 3, 44 }, { 244, 80 }, WidgetType::scrollview, WindowColour::secondary, 2),
            makeWidget({ 3, 125 }, { 221, 11 }, WidgetType::wt_13, WindowColour::secondary),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0048EC3B
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            Common::repositionTabs(self);
        }

        // 0x0048ED24
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
        }

        // 0x0048EE1A
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameStationPrompt(self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_station:
                case Common::widx::tab_cargo:
                case Common::widx::tab_cargo_ratings:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0048EE97
        static void onResize(Window* self)
        {
            Common::enableRenameByCaption(self);

            self->setSize(windowSize, maxWindowSize);
        }

        // 0x0048EE4A
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            auto station = StationManager::get(StationId(self->number));
            *scrollHeight = 0;
            for (uint8_t i = 0; i < 32; i++)
            {
                if (station->cargoStats[i].origin != StationId::null)
                    *scrollHeight += 10;
            }
        }

        // 0x0048EE73
        static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_ratings_list);
            return args;
        }

        // 0x0048EF02
        static void drawRatingBar(Window* self, Gfx::Context* context, int16_t x, int16_t y, uint8_t amount, Colour colour)
        {
            Gfx::fillRectInset(*context, x, y, x + 99, y + 9, self->getColour(WindowColour::secondary).u8(), 48);

            uint16_t rating = (amount * 96) / 256;
            if (rating > 2)
            {
                Gfx::fillRectInset(*context, x + 2, y + 2, x + 1 + rating, y + 8, enumValue(colour), 0);
            }
        }

        // 0x0048ED2F
        static void drawScroll(Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            Gfx::clearSingle(context, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

            const auto station = StationManager::get(StationId(self.number));
            int16_t y = 0;
            auto cargoId = 0;
            for (const auto& cargoStats : station->cargoStats)
            {
                auto& cargo = cargoStats;
                if (cargo.empty())
                {
                    cargoId++;
                    continue;
                }

                auto cargoObj = ObjectManager::get<CargoObject>(cargoId);
                Gfx::drawString_494BBF(context, 1, y, 98, Colour::black, StringIds::wcolour2_stringid, &cargoObj->name);

                auto rating = cargo.rating;
                auto colour = Colour::green;
                if (rating < 100)
                {
                    colour = Colour::yellow;
                    if (rating < 50)
                    {
                        colour = Colour::red;
                    }
                }

                uint8_t amount = (rating * 327) / 256;
                drawRatingBar(&self, &context, 100, y, amount, colour);

                uint16_t percent = rating / 2;
                Gfx::drawString_494B3F(context, 201, y, Colour::black, StringIds::station_cargo_rating_percent, &percent);
                y += 10;
                cargoId++;
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onUpdate = Common::update;
            events.prepareDraw = prepareDraw;
            events.textInput = Common::textInput;
            events.getScrollSize = getScrollSize;
            events.tooltip = tooltip;
            events.drawScroll = drawScroll;
        }
    }

    // 0x00491BC6
    void sub_491BC6()
    {
        TileLoop tileLoop;

        for (uint32_t posId = 0; posId < map_size; posId++)
        {
            if (_byte_F00484[posId] & (1 << 0))
            {
                TileManager::mapInvalidateTileFull(tileLoop.current());
            }
            tileLoop.next();
        }
    }

    // 0x0049271A
    void showStationCatchment(StationId stationId)
    {
        if (stationId == _lastSelectedStation)
            return;

        const StationId oldStationId = _lastSelectedStation;
        _lastSelectedStation = stationId;

        if (oldStationId != StationId::null)
        {
            if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::catchmentArea))
            {
                WindowManager::invalidate(WindowType::station, enumValue(oldStationId));
                sub_491BC6();
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::catchmentArea);
            }
        }

        const StationId newStationId = _lastSelectedStation;

        if (newStationId != StationId::null)
        {
            Ui::Windows::Construction::sub_4A6FAC();
            auto station = StationManager::get(_lastSelectedStation);

            station->setCatchmentDisplay(0);
            Input::setMapSelectionFlags(Input::MapSelectionFlags::catchmentArea);

            WindowManager::invalidate(WindowType::station, enumValue(newStationId));

            sub_491BC6();
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t* enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { Station::widgets, widx::tab_station, &Station::events, &Station::enabledWidgets },
            { Cargo::widgets, widx::tab_cargo, &Cargo::events, &Cargo::enabledWidgets },
            { CargoRatings::widgets, widx::tab_cargo_ratings, &CargoRatings::events, &Common::enabledWidgets }
        };

        // 0x0048E352, 0x0048E7C0 and 0x0048EC3B
        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->currentTab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab.
            self->activatedWidgets &= ~((1ULL << widx::tab_station) | (1ULL << widx::tab_cargo) | (1ULL << widx::tab_cargo_ratings));
            widx widgetIndex = tabInformationByTabOffset[self->currentTab].widgetIndex;
            self->activatedWidgets |= (1ULL << widgetIndex);

            // Put station and town name in place.
            auto station = StationManager::get(StationId(self->number));

            uint32_t stationTypeImages[16] = {
                StringIds::label_icons_none,
                StringIds::label_icons_rail,
                StringIds::label_icons_road,
                StringIds::label_icons_rail_road,
                StringIds::label_icons_air,
                StringIds::label_icons_rail_air,
                StringIds::label_icons_road_air,
                StringIds::label_icons_rail_road_air,
                StringIds::label_icons_water,
                StringIds::label_icons_rail_water,
                StringIds::label_icons_road_water,
                StringIds::label_icons_rail_road_water,
                StringIds::label_icons_air_water,
                StringIds::label_icons_rail_air_water,
                StringIds::label_icons_road_air_water,
                StringIds::label_icons_rail_road_air_water
            };
            auto args = FormatArguments();
            args.push(station->name);
            args.push(station->town);
            args.push(stationTypeImages[(station->flags & 0xF)]);

            // Resize common widgets.
            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;
        }

        // 0x0048E5DF
        static void textInput(Window* self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget != Common::widx::caption)
                return;

            GameCommands::setErrorTitle(StringIds::error_cant_rename_station);

            uint32_t* buffer = (uint32_t*)input;
            GameCommands::do_11(self->number, 1, buffer[0], buffer[1], buffer[2]);
            GameCommands::do_11(0, 2, buffer[3], buffer[4], buffer[5]);
            GameCommands::do_11(0, 0, buffer[6], buffer[7], buffer[8]);
        }

        // 0x0048E6F1
        static void update(Window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::station, self->number);
        }

        // 0x0048E5E7
        static void renameStationPrompt(Window* self, WidgetIndex_t widgetIndex)
        {
            auto station = StationManager::get(StationId(self->number));
            auto args = FormatArguments();
            args.push<int64_t>(0);
            args.push(station->name);
            args.push(station->town);

            TextInput::openTextInput(self, StringIds::title_station_name, StringIds::prompt_type_new_station_name, station->name, widgetIndex, &station->town);
        }

        // 0x0048EF82, 0x0048EF88
        static void repositionTabs(Window* self)
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
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex != widx::tab_cargo)
            {
                if (StationId(self->number) == _lastSelectedStation)
                {
                    showStationCatchment(StationId::null);
                }
            }

            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_station;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->var_85C = -1;

            self->viewportRemove(0);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_station];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;
            self->disabledWidgets = 0;

            self->invalidate();

            self->setSize(Station::windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x0048EFBC
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            auto station = StationManager::get(StationId(self->number));
            auto companyColour = CompanyManager::getCompanyColour(station->owner);

            // Station tab
            {
                uint32_t imageId = Gfx::recolour(skin->img, companyColour);
                imageId += InterfaceSkin::ImageIds::toolbar_menu_stations;
                Widget::drawTab(self, context, imageId, widx::tab_station);
            }

            // Cargo tab
            {
                static const uint32_t cargoTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame0,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame1,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame2,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame3,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_cargo - widx::tab_station)
                    imageId += cargoTabImageIds[(self->frame_no / 8) % std::size(cargoTabImageIds)];
                else
                    imageId += cargoTabImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_cargo);
            }

            // Cargo ratings tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_cargo_ratings;
                Widget::drawTab(self, context, imageId, widx::tab_cargo_ratings);

                auto widget = self->widgets[widx::tab_cargo_ratings];
                auto yOffset = widget.top + self->y + 14;
                auto xOffset = widget.left + self->x + 4;
                auto totalRatingBars = 0;

                for (const auto& cargoStats : station->cargoStats)
                {
                    auto& cargo = cargoStats;
                    if (!cargo.empty())
                    {
                        Gfx::fillRect(*context, xOffset, yOffset, xOffset + 22, yOffset + 1, (1 << 25) | PaletteIndex::index_30);

                        auto ratingColour = Colour::green;
                        if (cargo.rating < 100)
                        {
                            ratingColour = Colour::yellow;
                            if (cargo.rating < 50)
                                ratingColour = Colour::red;
                        }

                        auto ratingBarLength = (cargo.rating * 30) / 256;
                        Gfx::fillRect(*context, xOffset, yOffset, xOffset - 1 + ratingBarLength, yOffset + 1, Colours::getShade(ratingColour, 6));

                        yOffset += 3;
                        totalRatingBars++;
                        if (totalRatingBars >= 4)
                            break;
                    }
                }
            }
        }

        // 0x0048E32C
        static void enableRenameByCaption(Window* self)
        {
            auto station = StationManager::get(StationId(self->number));
            if (station->owner != CompanyId::null)
            {
                if (CompanyManager::isPlayerCompany(station->owner))
                {
                    self->enabledWidgets |= (1 << Common::widx::caption);
                }
                else
                {
                    self->enabledWidgets &= ~(1 << Common::widx::caption);
                }
            }
        }

        static void initEvents()
        {
            Station::initEvents();
            Cargo::initEvents();
            CargoRatings::initEvents();
        }
    }
}
