#include "Config.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/RenameStation.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/MapSelection.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Objects/CargoObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::Ui::Windows::Station
{
    static loco_global<uint8_t[kMapSize], 0x00F00484> _byte_F00484;
    static loco_global<StationId, 0x00112C786> _lastSelectedStation;

    namespace Common
    {
        static constexpr Ui::Size kMinWindowSize = { 192, 136 };

        static constexpr Ui::Size kMaxWindowSize = { 600, 440 };

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
        static void prepareDraw(Window& self);
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input);
        static void update(Window& self);
        static void renameStationPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx);
        static void enableRenameByCaption(Window* self);
    }

    namespace Station
    {
        static constexpr Ui::Size kWindowSize = { 223, 136 };

        enum widx
        {
            viewport = 7,
            status_bar,
            centre_on_viewport,
        };

        static constexpr Widget widgets[] = {
            // commonWidgets(kWindowSize.width, kWindowSize.height),
            commonWidgets(223, 136),
            makeWidget({ 3, 44 }, { 195, 80 }, WidgetType::viewport, WindowColour::secondary, Widget::kContentUnk),
            makeWidget({ 3, 115 }, { 195, 21 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 0, 0 }, { 24, 24 }, WidgetType::viewportCentreButton, WindowColour::secondary, Widget::kContentNull, StringIds::move_main_view_to_show_this),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << centre_on_viewport);

        // 0x0048E352
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::viewport].right = self.width - 4;
            self.widgets[widx::viewport].bottom = self.height - 14;

            self.widgets[widx::status_bar].top = self.height - 12;
            self.widgets[widx::status_bar].bottom = self.height - 3;
            self.widgets[widx::status_bar].right = self.width - 14;

            self.widgets[widx::centre_on_viewport].right = self.widgets[widx::viewport].right - 1;
            self.widgets[widx::centre_on_viewport].bottom = self.widgets[widx::viewport].bottom - 1;
            self.widgets[widx::centre_on_viewport].left = self.widgets[widx::viewport].right - 24;
            self.widgets[widx::centre_on_viewport].top = self.widgets[widx::viewport].bottom - 24;

            Widget::leftAlignTabs(self, Common::widx::tab_station, Common::widx::tab_cargo_ratings);
        }

        // 0x0048E470
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            auto station = StationManager::get(StationId(self.number));
            const char* buffer = StringManager::getString(StringIds::buffer_1250);
            station->getStatusString((char*)buffer);

            FormatArguments args{};
            args.push(StringIds::buffer_1250);

            const auto& widget = self.widgets[widx::status_bar];
            const auto width = widget.width() - 1;
            auto point = Point(self.x + widget.left - 1, self.y + widget.top - 1);
            tr.drawStringLeftClipped(point, width, Colour::black, StringIds::black_stringid, args);
        }

        // 0x0048E4D4
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameStationPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_station:
                case Common::widx::tab_cargo:
                case Common::widx::tab_cargo_ratings:
                    Common::switchTab(&self, widgetIndex);
                    break;

                // 0x0049932D
                case widx::centre_on_viewport:
                    self.viewportCentreMain();
                    break;
            }
        }

        static void initViewport(Window& self);

        // 0x0048E70B
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(kWindowSize, Common::kMaxWindowSize);

            if (self.viewports[0] != nullptr)
            {
                uint16_t newWidth = self.height - 8;
                uint16_t newHeight = self.height - 59;

                auto& viewport = self.viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->viewWidth = newWidth << viewport->zoom;
                    viewport->viewHeight = newHeight << viewport->zoom;
                    self.savedView.clear();
                }
            }

            initViewport(self);
        }

        // 0x0048F11B
        static void initViewport(Window& self)
        {
            if (self.currentTab != 0)
                return;

            self.callPrepareDraw();

            // Figure out the station's position on the map.
            auto station = StationManager::get(StationId(self.number));

            // Compute views.

            SavedView view = {
                station->x,
                station->y,
                ZoomLevel::half,
                static_cast<int8_t>(self.viewports[0]->getRotation()),
                station->z,
            };
            view.flags |= (1 << 14);

            ViewportFlags flags = ViewportFlags::none;
            if (self.viewports[0] != nullptr)
            {
                if (self.savedView == view)
                    return;

                flags = self.viewports[0]->flags;
                self.viewportRemove(0);
                ViewportManager::collectGarbage();
            }
            else
            {
                if (Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
                    flags |= ViewportFlags::gridlines_on_landscape;
            }
            // Remove station names from viewport
            flags |= ViewportFlags::station_names_displayed;

            self.savedView = view;

            // 0x0048F1CB start
            if (self.viewports[0] == nullptr)
            {
                auto widget = &self.widgets[widx::viewport];
                auto tile = World::Pos3({ station->x, station->y, station->z });
                auto origin = Ui::Point(widget->left + self.x + 1, widget->top + self.y + 1);
                auto size = Ui::Size(widget->width() - 2, widget->height() - 2);
                ViewportManager::create(&self, 0, origin, size, self.savedView.zoomLevel, tile);
                self.invalidate();
                self.flags |= WindowFlags::viewportNoScrolling;
            }
            // 0x0048F1CB end

            if (self.viewports[0] != nullptr)
            {
                self.viewports[0]->flags = flags;
                self.invalidate();
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::update,
            .textInput = Common::textInput,
            .viewportRotate = initViewport,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x0048F210
    Window* open(StationId stationId)
    {
        auto window = WindowManager::bringToFront(WindowType::station, enumValue(stationId));
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
                ToolManager::toolCancel();

            window = WindowManager::bringToFront(WindowType::station, enumValue(stationId));
        }

        if (window == nullptr)
        {
            // 0x0048F29F start
            const WindowFlags newFlags = WindowFlags::resizable | WindowFlags::flag_11;
            window = WindowManager::createWindow(WindowType::station, Station::kWindowSize, newFlags, Station::getEvents());
            window->number = enumValue(stationId);
            auto station = StationManager::get(stationId);
            window->owner = station->owner;
            window->minWidth = Common::kMinWindowSize.width;
            window->minHeight = Common::kMinWindowSize.height;
            window->maxWidth = Common::kMaxWindowSize.width;
            window->maxHeight = Common::kMaxWindowSize.height;

            window->savedView.clear();

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_0A);
            // 0x0048F29F end
        }

        window->currentTab = Common::widx::tab_station - Common::widx::tab_station;
        window->invalidate();

        window->setWidgets(Station::widgets);
        window->enabledWidgets = Station::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Station::getEvents();
        window->activatedWidgets = 0;
        window->disabledWidgets = 0;
        window->initScrollWidgets();
        Station::initViewport(*window);

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

        static constexpr Widget widgets[] = {
            commonWidgets(223, 136),
            makeWidget({ 3, 44 }, { 217, 80 }, WidgetType::scrollview, WindowColour::secondary, 2),
            makeWidget({ 3, 125 }, { 195, 10 }, WidgetType::wt_13, WindowColour::secondary),
            makeWidget({ 198, 44 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::show_station_catchment, StringIds::station_catchment),
            widgetEnd(),
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << station_catchment);

        // 0x0048E7C0
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 24;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            self.widgets[widx::status_bar].top = self.height - 12;
            self.widgets[widx::status_bar].bottom = self.height - 3;
            self.widgets[widx::status_bar].right = self.width - 14;

            self.widgets[widx::station_catchment].right = self.width - 2;
            self.widgets[widx::station_catchment].left = self.width - 25;

            Widget::leftAlignTabs(self, Common::widx::tab_station, Common::widx::tab_cargo_ratings);

            self.activatedWidgets &= ~(1 << widx::station_catchment);
            if (StationId(self.number) == _lastSelectedStation)
                self.activatedWidgets |= (1 << widx::station_catchment);
        }

        // 0x0048E8DE
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);

            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            buffer = StringManager::formatString(buffer, StringIds::accepted_cargo_separator);

            auto station = StationManager::get(StationId(self.number));
            uint8_t cargoTypeCount = 0;

            for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
            {
                auto& stats = station->cargoStats[cargoId];

                if (!stats.isAccepted())
                    continue;

                *buffer++ = ' ';
                *buffer++ = ControlCodes::inlineSpriteStr;
                *(reinterpret_cast<uint32_t*>(buffer)) = ObjectManager::get<CargoObject>(cargoId)->unitInlineSprite;
                buffer += 4;

                cargoTypeCount++;
            }

            if (cargoTypeCount == 0)
            {
                buffer = StringManager::formatString(buffer, StringIds::cargo_nothing_accepted);
            }

            *buffer++ = '\0';

            const auto& widget = self.widgets[widx::status_bar];
            const auto width = widget.width();
            auto point = Point(self.x + widget.left - 1, self.y + widget.top - 1);

            tr.drawStringLeftClipped(point, width, Colour::black, StringIds::buffer_1250);
        }

        // 0x0048EB0B
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameStationPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_station:
                case Common::widx::tab_cargo:
                case Common::widx::tab_cargo_ratings:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::station_catchment:
                {
                    StationId windowNumber = StationId(self.number);
                    if (windowNumber == _lastSelectedStation)
                        windowNumber = StationId::null;

                    showStationCatchment(windowNumber);
                    break;
                }
            }
        }

        // 0x0048EBB7
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(Common::kMinWindowSize, Common::kMaxWindowSize);
        }

        // 0x0048EB64
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            auto station = StationManager::get(StationId(self.number));
            *scrollHeight = 0;
            for (const auto& cargoStats : station->cargoStats)
            {
                if (cargoStats.quantity != 0)
                {
                    *scrollHeight += 12;
                    if (cargoStats.origin != StationId(self.number))
                        *scrollHeight += 10;
                }
            }
        }

        // 0x0048EB4F
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_cargo_list);
            return args;
        }

        // 0x0048E986
        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            drawingCtx.clearSingle(Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

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
                            drawingCtx.drawImage(xPos, y, cargoObj->unitInlineSprite);
                            xPos += 10;
                        }
                    }
                }
                auto cargoName = cargoObj->unitNameSingular;

                if (cargo.quantity != 1)
                    cargoName = cargoObj->unitNamePlural;

                const auto& widget = self.widgets[widx::scrollview];
                auto xPos = widget.width() - 14;

                {
                    FormatArguments args{};
                    args.push(cargoName);
                    args.push<uint32_t>(cargo.quantity);

                    auto cargoStr = StringIds::station_cargo;
                    if (cargo.origin != StationId(self.number))
                        cargoStr = StringIds::station_cargo_en_route_start;

                    auto point = Point(xPos, y);
                    tr.drawStringRight(point, AdvancedColour(Colour::black).outline(), cargoStr, args);
                    y += 10;
                }

                if (cargo.origin != StationId(self.number))
                {
                    auto originStation = StationManager::get(cargo.origin);
                    FormatArguments args{};
                    args.push(originStation->name);
                    args.push(originStation->town);

                    auto point = Point(xPos, y);
                    tr.drawStringRight(point, AdvancedColour(Colour::black).outline(), StringIds::station_cargo_en_route_end, args);
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
                FormatArguments args{};
                args.push(StringIds::nothing_waiting);

                auto point = Point(1, 0);
                tr.drawStringLeft(point, Colour::black, StringIds::black_stringid, args);
            }
        }

        // 0x0048EC21
        static void onClose(Window& self)
        {
            if (StationId(self.number) == _lastSelectedStation)
            {
                showStationCatchment(StationId::null);
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::update,
            .getScrollSize = getScrollSize,
            .textInput = Common::textInput,
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

    namespace CargoRatings
    {
        static constexpr Ui::Size kWindowSize = { 249, 136 };

        static constexpr Ui::Size kMaxWindowSize = { 249, 440 };

        enum widx
        {
            scrollview = 7,
            status_bar,
        };

        static constexpr Widget widgets[] = {
            commonWidgets(249, 136),
            makeWidget({ 3, 44 }, { 244, 80 }, WidgetType::scrollview, WindowColour::secondary, 2),
            makeWidget({ 3, 125 }, { 221, 11 }, WidgetType::wt_13, WindowColour::secondary),
            widgetEnd(),
        };

        // 0x0048EC3B
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            self.widgets[widx::status_bar].top = self.height - 12;
            self.widgets[widx::status_bar].bottom = self.height - 3;
            self.widgets[widx::status_bar].right = self.width - 14;

            Widget::leftAlignTabs(self, Common::widx::tab_station, Common::widx::tab_cargo_ratings);
        }

        // 0x0048ED24
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
        }

        // 0x0048EE1A
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::caption:
                    Common::renameStationPrompt(&self, widgetIndex);
                    break;

                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_station:
                case Common::widx::tab_cargo:
                case Common::widx::tab_cargo_ratings:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x0048EE97
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(kWindowSize, kMaxWindowSize);
        }

        // 0x0048EE4A
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            auto station = StationManager::get(StationId(self.number));
            *scrollHeight = 0;
            for (uint8_t i = 0; i < 32; i++)
            {
                if (station->cargoStats[i].origin != StationId::null)
                    *scrollHeight += 10;
            }
        }

        // 0x0048EE73
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_ratings_list);
            return args;
        }

        // 0x0048EF02
        static void drawRatingBar(Window* self, Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y, uint8_t amount, Colour colour)
        {
            drawingCtx.fillRectInset(x, y, x + 99, y + 9, self->getColour(WindowColour::secondary), Gfx::RectInsetFlags::borderInset | Gfx::RectInsetFlags::fillNone);

            uint16_t rating = (amount * 96) / 256;
            if (rating > 2)
            {
                drawingCtx.fillRectInset(x + 2, y + 2, x + 1 + rating, y + 8, colour, Gfx::RectInsetFlags::none);
            }
        }

        // 0x0048ED2F
        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            drawingCtx.clearSingle(Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

            const auto station = StationManager::get(StationId(self.number));
            auto point = Point(0, 0);
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
                point.x = 1;
                {
                    auto argsBuf = FormatArgumentsBuffer{};
                    auto args = FormatArguments{ argsBuf };
                    args.push(cargoObj->name);

                    tr.drawStringLeftClipped(point, 98, Colour::black, StringIds::wcolour2_stringid, args);
                }

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
                drawRatingBar(&self, drawingCtx, 100, point.y, amount, colour);

                uint16_t percent = rating / 2;
                point.x = 201;
                {
                    auto argsBuf = FormatArgumentsBuffer{};
                    auto args = FormatArguments{ argsBuf };
                    args.push(percent);

                    tr.drawStringLeft(point, Colour::black, StringIds::station_cargo_rating_percent, args);
                }

                point.y += 10;
                cargoId++;
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::update,
            .getScrollSize = getScrollSize,
            .textInput = Common::textInput,
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

    // 0x00491BC6
    void sub_491BC6()
    {
        TileLoop tileLoop;

        for (uint32_t posId = 0; posId < kMapSize; posId++)
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
            if (World::hasMapSelectionFlag(World::MapSelectionFlags::catchmentArea))
            {
                WindowManager::invalidate(WindowType::station, enumValue(oldStationId));
                sub_491BC6();
                World::resetMapSelectionFlag(World::MapSelectionFlags::catchmentArea);
            }
        }

        const StationId newStationId = _lastSelectedStation;

        if (newStationId != StationId::null)
        {
            Ui::Windows::Construction::sub_4A6FAC();
            auto* station = StationManager::get(_lastSelectedStation);

            setCatchmentDisplay(station, CatchmentFlags::flag_0);
            World::setMapSelectionFlags(World::MapSelectionFlags::catchmentArea);

            WindowManager::invalidate(WindowType::station, enumValue(newStationId));

            sub_491BC6();
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t* enabledWidgets;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { Station::widgets,      widx::tab_station,       Station::getEvents(),      &Station::enabledWidgets },
            { Cargo::widgets,        widx::tab_cargo,         Cargo::getEvents(),        &Cargo::enabledWidgets },
            { CargoRatings::widgets, widx::tab_cargo_ratings, CargoRatings::getEvents(), &Common::enabledWidgets }
        };
        // clang-format on

        // 0x0048E352, 0x0048E7C0 and 0x0048EC3B
        static void prepareDraw(Window& self)
        {
            // Activate the current tab.
            self.activatedWidgets &= ~((1ULL << widx::tab_station) | (1ULL << widx::tab_cargo) | (1ULL << widx::tab_cargo_ratings));
            widx widgetIndex = tabInformationByTabOffset[self.currentTab].widgetIndex;
            self.activatedWidgets |= (1ULL << widgetIndex);

            // Put station and town name in place.
            auto* station = StationManager::get(StationId(self.number));

            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(station->name);
            args.push(station->town);
            args.push(getTransportIconsFromStationFlags(station->flags));

            // Resize common widgets.
            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;
        }

        // 0x0048E5DF
        static void textInput(Window& self, WidgetIndex_t callingWidget, const char* input)
        {
            if (callingWidget != Common::widx::caption)
                return;

            GameCommands::setErrorTitle(StringIds::error_cant_rename_station);

            GameCommands::RenameStationArgs args{};

            args.stationId = StationId(self.number);
            args.nameBufferIndex = 1;
            std::memcpy(args.buffer, input, 36);

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.nameBufferIndex = 2;

            GameCommands::doCommand(args, GameCommands::Flags::apply);

            args.nameBufferIndex = 0;

            GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        // 0x0048E6F1
        static void update(Window& self)
        {
            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidate(WindowType::station, self.number);
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

            if (ToolManager::isToolActive(self->type, self->number))
                ToolManager::toolCancel();

            TextInput::sub_4CE6C9(self->type, self->number);

            self->currentTab = widgetIndex - widx::tab_station;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);
            self->var_85C = -1;

            self->viewportRemove(0);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_station];

            self->enabledWidgets = *tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = &tabInfo.events;
            self->activatedWidgets = 0;
            self->setWidgets(tabInfo.widgets);
            self->disabledWidgets = 0;

            self->invalidate();

            self->setSize(Station::kWindowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x0048EFBC
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            auto station = StationManager::get(StationId(self->number));
            auto companyColour = CompanyManager::getCompanyColour(station->owner);

            // Station tab
            {
                uint32_t imageId = Gfx::recolour(skin->img, companyColour);
                imageId += InterfaceSkin::ImageIds::toolbar_menu_stations;
                Widget::drawTab(self, drawingCtx, imageId, widx::tab_station);
            }

            // Cargo tab
            {
                static constexpr uint32_t cargoTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame0,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame1,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame2,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame3,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_cargo - widx::tab_station)
                    imageId += cargoTabImageIds[(self->frameNo / 8) % std::size(cargoTabImageIds)];
                else
                    imageId += cargoTabImageIds[0];

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo);
            }

            // Cargo ratings tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_cargo_ratings;
                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo_ratings);

                auto widget = self->widgets[widx::tab_cargo_ratings];
                auto yOffset = widget.top + self->y + 14;
                auto xOffset = widget.left + self->x + 4;
                auto totalRatingBars = 0;

                for (const auto& cargoStats : station->cargoStats)
                {
                    auto& cargo = cargoStats;
                    if (!cargo.empty())
                    {
                        drawingCtx.fillRect(xOffset, yOffset, xOffset + 22, yOffset + 1, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);

                        auto ratingColour = Colour::green;
                        if (cargo.rating < 100)
                        {
                            ratingColour = Colour::yellow;
                            if (cargo.rating < 50)
                                ratingColour = Colour::red;
                        }

                        auto ratingBarLength = (cargo.rating * 30) / 256;
                        drawingCtx.fillRect(xOffset, yOffset, xOffset - 1 + ratingBarLength, yOffset + 1, Colours::getShade(ratingColour, 6), Gfx::RectFlags::none);

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
    }
}
