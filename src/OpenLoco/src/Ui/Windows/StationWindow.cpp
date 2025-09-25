#include "Config.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/RenameStation.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
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
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/Widgets/ViewportWidget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleDraw.h"
#include "Vehicles/VehicleManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/String.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::Ui::Windows::Station
{
    static loco_global<uint8_t[TileManager::getMapSize()], 0x00F00484> _byte_F00484;
    static loco_global<StationId, 0x00112C786> _lastSelectedStation;

    using Vehicles::VehicleHead;

    namespace Common
    {
        static constexpr Ui::Size32 kMinWindowSize = { 192, 136 };

        static constexpr Ui::Size32 kMaxWindowSize = { 600, 440 };

        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_station,
            tab_cargo,
            tab_cargo_ratings,
            tab_vehicles_trains,
            tab_vehicles_buses,
            tab_vehicles_trucks,
            tab_vehicles_trams,
            tab_vehicles_aircraft,
            tab_vehicles_ships,
            content_begin,
        };

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                Widgets::Caption({ 1, 1 }, { frameWidth - 2, 13 }, Widgets::Caption::Style::blackText, WindowColour::primary, StringIds::title_station),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { frameWidth, 95 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_station),
                Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_station_cargo),
                Widgets::Tab({ 65, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_station_cargo_ratings),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_trains),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_buses),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_trucks),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_trams),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_aircraft),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_ships));
        }

        static bool isVehicleTypeAvailable(Window& self, VehicleType vehicleType)
        {
            return (self.var_846 & (1U << enumValue(vehicleType))) != 0;
        }

        static void setVehicleTypeAvailable(Window& self, VehicleType vehicleType)
        {
            self.var_846 |= (1U << enumValue(vehicleType));
        }

        // Defined at the bottom of this file.
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, const WidgetId id);
        static void prepareDraw(Window& self);
        static void textInput(Window& self, WidgetIndex_t callingWidget, [[maybe_unused]] const WidgetId id, const char* input);
        static void update(Window& self);
        static void renameStationPrompt(Window* self, WidgetIndex_t widgetIndex);
        static void switchTab(Window& self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx);
        static void enableRenameByCaption(Window* self);
    }

    namespace Station
    {
        static constexpr Ui::Size32 kWindowSize = { 223, 136 };

        enum widx
        {
            viewport = Common::widx::content_begin,
            status_bar,
            centre_on_viewport,
        };

        static constexpr auto widgets = makeWidgets(
            // commonWidgets(kWindowSize.width, kWindowSize.height),
            Common::makeCommonWidgets(223, 136),
            Widgets::Viewport({ 3, 44 }, { 195, 80 }, WindowColour::secondary, Widget::kContentUnk),
            Widgets::Label({ 3, 115 }, { 195, 21 }, WindowColour::secondary, ContentAlign::center),
            Widgets::ImageButton({ 0, 0 }, { 24, 24 }, WindowColour::secondary, ImageIds::centre_viewport, StringIds::move_main_view_to_show_this)

        );

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
            Common::drawTabs(self, drawingCtx);

            auto station = StationManager::get(StationId(self.number));
            const char* buffer = StringManager::getString(StringIds::buffer_1250);
            station->getStatusString((char*)buffer);

            FormatArguments args{};
            args.push(StringIds::buffer_1250);

            const auto& widget = self.widgets[widx::status_bar];
            const auto width = widget.width() - 1;
            auto point = Point(widget.left - 1, widget.top - 1);
            tr.drawStringLeftClipped(point, width, Colour::black, StringIds::black_stringid, args);
        }

        // 0x0048E4D4
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, const WidgetId id)
        {
            switch (widgetIndex)
            {
                // 0x0049932D
                case widx::centre_on_viewport:
                    self.viewportCentreMain();
                    return;
            }

            Common::onMouseUp(self, widgetIndex, id);
        }

        static void initViewport(Window& self);

        // 0x0048E70B
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(kWindowSize, Common::kMaxWindowSize);

            if (self.viewports[0] != nullptr)
            {
                uint16_t newWidth = self.width - 8;
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
            {
                return;
            }

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
                {
                    return;
                }

                flags = self.viewports[0]->flags;
                self.viewportRemove(0);
            }
            else
            {
                if (Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
                {
                    flags |= ViewportFlags::gridlines_on_landscape;
                }
            }
            // Remove station names from viewport
            flags |= ViewportFlags::station_names_displayed;

            self.savedView = view;

            // 0x0048F1CB start
            if (self.viewports[0] == nullptr)
            {
                auto widget = &self.widgets[widx::viewport];
                auto tile = World::Pos3({ station->x, station->y, station->z });
                auto origin = Ui::Point(widget->left + 1, widget->top + 1);
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

    namespace VehiclesStopping
    {
        static void refreshVehicleList(Window* self);
    }

    // 0x0048F210
    Window* open(StationId stationId)
    {
        auto window = WindowManager::bringToFront(WindowType::station, enumValue(stationId));
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
            {
                ToolManager::toolCancel();
            }

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
            window->setColour(WindowColour::secondary, skin->windowPlayerColor);
            // 0x0048F29F end
        }

        window->currentTab = Common::widx::tab_station - Common::widx::tab_station;
        window->invalidate();

        // We'll need the vehicle list to determine what vehicle tabs to show
        VehiclesStopping::refreshVehicleList(window);

        window->setWidgets(Station::widgets);
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
            scrollview = Common::widx::content_begin,
            status_bar,
            station_catchment,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(223, 136),
            Widgets::ScrollView({ 3, 44 }, { 217, 80 }, WindowColour::secondary, 2),
            Widgets::Label({ 3, 125 }, { 195, 10 }, WindowColour::secondary, ContentAlign::center),
            Widgets::ImageButton({ 198, 44 }, { 24, 24 }, WindowColour::secondary, ImageIds::show_station_catchment, StringIds::station_catchment)

        );

        // 0x0048E7C0
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 26;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            self.widgets[widx::status_bar].top = self.height - 12;
            self.widgets[widx::status_bar].bottom = self.height - 3;
            self.widgets[widx::status_bar].right = self.width - 14;

            self.widgets[widx::station_catchment].right = self.width - 2;
            self.widgets[widx::station_catchment].left = self.width - 25;

            Widget::leftAlignTabs(self, Common::widx::tab_station, Common::widx::tab_cargo_ratings);

            self.activatedWidgets &= ~(1 << widx::station_catchment);
            if (StationId(self.number) == _lastSelectedStation)
            {
                self.activatedWidgets |= (1 << widx::station_catchment);
            }
        }

        // 0x0048E8DE
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);

            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            buffer = StringManager::formatString(buffer, StringIds::accepted_cargo_separator);

            auto station = StationManager::get(StationId(self.number));
            uint8_t cargoTypeCount = 0;

            for (uint32_t cargoId = 0; cargoId < kMaxCargoStats; cargoId++)
            {
                auto& stats = station->cargoStats[cargoId];

                if (!stats.isAccepted())
                {
                    continue;
                }

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
            auto point = Point(widget.left - 1, widget.top - 1);

            tr.drawStringLeftClipped(point, width, Colour::black, StringIds::buffer_1250);
        }

        // 0x0048EB0B
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, const WidgetId id)
        {
            switch (widgetIndex)
            {
                case widx::station_catchment:
                {
                    StationId windowNumber = StationId(self.number);
                    if (windowNumber == _lastSelectedStation)
                    {
                        windowNumber = StationId::null;
                    }

                    showStationCatchment(windowNumber);
                    return;
                }
            }

            Common::onMouseUp(self, widgetIndex, id);
        }

        // 0x0048EBB7
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(Common::kMinWindowSize, Common::kMaxWindowSize);
        }

        // 0x0048EB64
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            auto station = StationManager::get(StationId(self.number));
            scrollHeight = 0;
            for (const auto& cargoStats : station->cargoStats)
            {
                if (cargoStats.quantity != 0)
                {
                    scrollHeight += 12;
                    if (cargoStats.origin != StationId(self.number))
                    {
                        scrollHeight += 10;
                    }
                }
            }
        }

        // 0x0048EB4F
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
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
                {
                    cargoName = cargoObj->unitNamePlural;
                }

                const auto& widget = self.widgets[widx::scrollview];
                auto xPos = widget.width() - 14;

                {
                    FormatArguments args{};
                    args.push(cargoName);
                    args.push<uint32_t>(cargo.quantity);

                    auto cargoStr = StringIds::station_cargo;
                    if (cargo.origin != StationId(self.number))
                    {
                        cargoStr = StringIds::station_cargo_en_route_start;
                    }

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
            {
                totalUnits += stats.quantity;
            }

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
        static constexpr Ui::Size32 kWindowSize = { 249, 136 };

        static constexpr Ui::Size32 kMaxWindowSize = { 249, 440 };

        enum widx
        {
            scrollview = Common::widx::content_begin,
            status_bar,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(249, 136),
            Widgets::ScrollView({ 3, 44 }, { 244, 80 }, WindowColour::secondary, 2),
            Widgets::Label({ 3, 125 }, { 221, 11 }, WindowColour::secondary, ContentAlign::center)

        );

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
            Common::drawTabs(self, drawingCtx);
        }

        // 0x0048EE97
        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(kWindowSize, kMaxWindowSize);
        }

        // 0x0048EE4A
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            auto station = StationManager::get(StationId(self.number));
            scrollHeight = 0;
            for (uint8_t i = 0; i < 32; i++)
            {
                if (station->cargoStats[i].origin != StationId::null)
                {
                    scrollHeight += 10;
                }
            }
        }

        // 0x0048EE73
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
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
            .onMouseUp = Common::onMouseUp,
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

    // NB: This namespace shares a fair bit of code with the VehicleList window.
    // We should look into sharing some of these functions.
    namespace VehiclesStopping
    {
        static constexpr Ui::Size32 kWindowSize = { 400, 200 };

        static constexpr Ui::Size32 kMaxWindowSize = { 600, 800 };

        enum widx
        {
            scrollview = Common::widx::content_begin,
            status_bar,
        };

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(223, 136),
            Widgets::ScrollView({ 3, 44 }, { 544, 138 }, WindowColour::secondary, Scrollbars::vertical),
            Widgets::Label({ 3, kWindowSize.height - 13 }, { kWindowSize.width, 10 }, WindowColour::secondary, ContentAlign::left, StringIds::black_stringid)

        );

        static bool vehicleStopsAtActiveStation(const VehicleHead* head, StationId filterStationId)
        {
            auto orders = Vehicles::OrderRingView(head->orderTableOffset);
            for (auto& order : orders)
            {
                auto* stationOrder = order.as<Vehicles::OrderStation>();
                if (stationOrder == nullptr)
                {
                    continue;
                }

                const auto stationId = stationOrder->getStation();
                if (stationId == filterStationId)
                {
                    return true;
                }
            }
            return false;
        }

        static VehicleType getCurrentVehicleType(Window* self)
        {
            return static_cast<VehicleType>(self->currentTab - (Common::widx::tab_vehicles_trains - Common::widx::tab_station));
        }

        static void refreshVehicleList(Window* self)
        {
            auto currentVehicleType = getCurrentVehicleType(self);
            self->rowCount = 0;

            for (auto* vehicle : VehicleManager::VehicleList())
            {
                if (!vehicleStopsAtActiveStation(vehicle, StationId(self->number)))
                {
                    continue;
                }

                Common::setVehicleTypeAvailable(*self, vehicle->vehicleType);

                if (vehicle->vehicleType != currentVehicleType)
                {
                    continue;
                }

                vehicle->vehicleFlags &= ~VehicleFlags::sorted;
            }
        }

        static bool orderByName(const VehicleHead& lhs, const VehicleHead& rhs)
        {
            char lhsString[256] = { 0 };
            {
                FormatArguments lhsArgs{};
                lhsArgs.push(lhs.ordinalNumber);
                StringManager::formatString(lhsString, lhs.name, lhsArgs);
            }

            char rhsString[256] = { 0 };
            {
                FormatArguments rhsArgs{};
                rhsArgs.push(rhs.ordinalNumber);
                StringManager::formatString(rhsString, rhs.name, rhsArgs);
            }

            return Utility::strlogicalcmp(lhsString, rhsString) < 0;
        }

        static void updateVehicleList(Window* self)
        {
            auto currentVehicleType = getCurrentVehicleType(self);
            EntityId insertId = EntityId::null;

            for (auto* vehicle : VehicleManager::VehicleList())
            {
                if (vehicle->vehicleType != currentVehicleType)
                {
                    continue;
                }

                if (vehicle->hasVehicleFlags(VehicleFlags::sorted))
                {
                    continue;
                }

                if (!vehicleStopsAtActiveStation(vehicle, StationId(self->number)))
                {
                    continue;
                }

                if (insertId == EntityId::null)
                {
                    insertId = vehicle->id;
                    continue;
                }

                auto* insertVehicle = EntityManager::get<VehicleHead>(insertId);
                if (insertVehicle == nullptr)
                {
                    continue;
                }
                if (orderByName(*vehicle, *insertVehicle))
                {
                    insertId = vehicle->id;
                    continue;
                }
            }

            if (insertId != EntityId::null)
            {
                auto vehicle = EntityManager::get<VehicleHead>(insertId);
                if (vehicle == nullptr)
                {
                    self->var_83C = self->rowCount;
                    refreshVehicleList(self);
                    return;
                }
                vehicle->vehicleFlags |= VehicleFlags::sorted;

                if (vehicle->id != EntityId(self->rowInfo[self->rowCount]))
                {
                    self->rowInfo[self->rowCount] = enumValue(vehicle->id);
                }

                self->rowCount++;

                if (self->rowCount > self->var_83C)
                {
                    self->var_83C = self->rowCount;
                }
            }
            else
            {
                if (self->var_83C != self->rowCount)
                {
                    self->var_83C = self->rowCount;
                }

                refreshVehicleList(self);
            }
        }

        void removeTrainFromList(Window& self, EntityId head)
        {
            for (auto i = 0; i < self.var_83C; ++i)
            {
                auto& entry = self.rowInfo[i];
                if (entry == enumValue(head))
                {
                    entry = enumValue(EntityId::null);
                }
            }
        }

        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            static constexpr StringId kTypeToCaption[] = {
                StringIds::stringid_trains,
                StringIds::stringid_buses,
                StringIds::stringid_trucks,
                StringIds::stringid_trams,
                StringIds::stringid_aircraft,
                StringIds::stringid_ships,
            };

            auto currentVehicleType = getCurrentVehicleType(&self);
            self.widgets[Common::widx::caption].text = kTypeToCaption[enumValue(currentVehicleType)];

            // Basic frame widget dimensions
            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            static constexpr std::pair<StringId, StringId> kTypeToFooterStringIds[]{
                { StringIds::num_trains_singular, StringIds::num_trains_plural },
                { StringIds::num_buses_singular, StringIds::num_buses_plural },
                { StringIds::num_trucks_singular, StringIds::num_trucks_plural },
                { StringIds::num_trams_singular, StringIds::num_trams_plural },
                { StringIds::num_aircrafts_singular, StringIds::num_aircrafts_plural },
                { StringIds::num_ships_singular, StringIds::num_ships_plural },
            };

            {
                // Reposition status bar
                auto& widget = self.widgets[widx::status_bar];
                widget.top = self.height - 13;
                widget.bottom = self.height - 3;

                // Set status bar
                FormatArguments args{ widget.textArgs };
                auto& footerStringPair = kTypeToFooterStringIds[enumValue(currentVehicleType)];
                args.push(self.var_83C == 1 ? footerStringPair.first : footerStringPair.second);
                args.push(self.var_83C);
            }
        }

        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);
        }

        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            const auto& rt = drawingCtx.currentRenderTarget();

            auto tr = Gfx::TextRenderer(drawingCtx);

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 1);
            drawingCtx.clearSingle(shade);

            auto yPos = 0;
            for (auto i = 0; i < self.var_83C; i++)
            {
                const auto vehicleId = EntityId(self.rowInfo[i]);

                // Item not in rendering context, or no vehicle available for this slot?
                if (yPos + self.rowHeight < rt.y || vehicleId == EntityId::null)
                {
                    yPos += self.rowHeight;
                    continue;
                }
                else if (yPos >= rt.y + rt.height + self.rowHeight)
                {
                    break;
                }

                auto head = EntityManager::get<VehicleHead>(vehicleId);
                if (head == nullptr)
                {
                    continue;
                }

                // Highlight selection.
                if (head->id == EntityId(self.rowHover))
                {
                    drawingCtx.drawRect(0, yPos, self.width, self.rowHeight, Colours::getShade(self.getColour(WindowColour::secondary).c(), 0), Gfx::RectFlags::none);
                }

                auto vehicle = Vehicles::Vehicle(*head);

                // Draw vehicle at the bottom of the row
                drawTrainInline(drawingCtx, vehicle, Ui::Point(0, yPos + (self.rowHeight - 28) / 2 + 6));

                // Draw vehicle status
                {
                    // Prepare status for drawing
                    auto status = head->getStatus();
                    auto args = FormatArguments::common();
                    args.push(head->name);
                    args.push(head->ordinalNumber);
                    args.push(status.status1);
                    args.push(status.status1Args);
                    args.push(status.status2);
                    args.push(status.status2Args);

                    StringId format = StringIds::vehicle_list_status_2pos;
                    if (status.status2 != StringIds::null)
                    {
                        format = StringIds::vehicle_list_status_3pos;
                    }

                    // Draw status
                    yPos += 2;
                    auto point = Point(1, yPos);
                    tr.drawStringLeftClipped(point, 308, AdvancedColour(Colour::black).outline(), format, args);
                }

                yPos += self.rowHeight - 2;
            }
        }

        static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_vehicle_list);
            return args;
        }

        static void onUpdate(Window& self)
        {
            self.frameNo++;
            self.callPrepareDraw();

            updateVehicleList(&self);
            updateVehicleList(&self);
            updateVehicleList(&self);

            self.invalidate();
        }

        static void event_08(Window& self)
        {
            self.flags |= WindowFlags::notScrollView;
        }

        static void event_09(Window& self)
        {
            if (self.hasFlags(WindowFlags::notScrollView))
            {
                self.rowHover = -1;
            }
        }

        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            scrollHeight = self.var_83C * self.rowHeight;
        }

        static CursorId cursor(Window& self, WidgetIndex_t widgetIdx, [[maybe_unused]] const WidgetId id, [[maybe_unused]] int16_t xPos, int16_t yPos, CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
            {
                return fallback;
            }

            uint16_t currentIndex = yPos / self.rowHeight;
            if (currentIndex < self.var_83C && self.rowInfo[currentIndex] != -1)
            {
                return CursorId::handPointer;
            }

            return fallback;
        }

        static void onScrollMouseOver(Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            self.flags &= ~WindowFlags::notScrollView;

            uint16_t currentRow = y / self.rowHeight;
            if (currentRow < self.var_83C)
            {
                self.rowHover = self.rowInfo[currentRow];
            }
            else
            {
                self.rowHover = -1;
            }
        }

        static void onScrollMouseDown(Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            uint16_t currentRow = y / self.rowHeight;
            if (currentRow >= self.var_83C)
            {
                return;
            }

            EntityId currentVehicleId = EntityId(self.rowInfo[currentRow]);
            if (currentVehicleId == EntityId::null)
            {
                return;
            }

            auto* head = EntityManager::get<VehicleHead>(currentVehicleId);
            if (head == nullptr)
            {
                return;
            }

            if (head->isPlaced())
            {
                Ui::Windows::Vehicle::Main::open(head);
            }
            else
            {
                Ui::Windows::Vehicle::Details::open(head);
            }
        }

        static void onResize(Window& self)
        {
            Common::enableRenameByCaption(&self);

            self.setSize(kWindowSize, kMaxWindowSize);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = Common::onMouseUp,
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

    // 0x00491BC6
    void sub_491BC6()
    {
        TileLoop tileLoop;

        for (uint32_t posId = 0; posId < TileManager::getMapSize(); posId++)
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
        {
            return;
        }

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
            const widx widgetIndex;
            std::span<const Widget> widgets;
            const WindowEventList& events;
            const uint8_t rowHeight;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { widx::tab_station,           Station::widgets,         Station::getEvents(),          0 },
            { widx::tab_cargo,             Cargo::widgets,           Cargo::getEvents(),            0 },
            { widx::tab_cargo_ratings,     CargoRatings::widgets,    CargoRatings::getEvents(),     0 },
            { widx::tab_vehicles_trains,   VehiclesStopping::widgets, VehiclesStopping::getEvents(), 28 },
            { widx::tab_vehicles_buses,    VehiclesStopping::widgets, VehiclesStopping::getEvents(), 28 },
            { widx::tab_vehicles_trucks,   VehiclesStopping::widgets, VehiclesStopping::getEvents(), 28 },
            { widx::tab_vehicles_trams,    VehiclesStopping::widgets, VehiclesStopping::getEvents(), 28 },
            { widx::tab_vehicles_aircraft, VehiclesStopping::widgets, VehiclesStopping::getEvents(), 48 },
            { widx::tab_vehicles_ships,    VehiclesStopping::widgets, VehiclesStopping::getEvents(), 36 },
        };
        // clang-format on

        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case widx::caption:
                    renameStationPrompt(&self, widgetIndex);
                    break;

                case widx::close_button:
                    WindowManager::close(&self);
                    break;

                case widx::tab_station:
                case widx::tab_cargo:
                case widx::tab_cargo_ratings:
                case widx::tab_vehicles_trains:
                case widx::tab_vehicles_buses:
                case widx::tab_vehicles_trucks:
                case widx::tab_vehicles_trams:
                case widx::tab_vehicles_aircraft:
                case widx::tab_vehicles_ships:
                    switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0048E352, 0x0048E7C0 and 0x0048EC3B
        static void prepareDraw(Window& self)
        {
            // Hide vehicle types without known vehicles calling at this station
            for (auto i = enumValue(VehicleType::train); i <= enumValue(VehicleType::ship); i++)
            {
                if (isVehicleTypeAvailable(self, VehicleType(i)))
                {
                    self.disabledWidgets &= ~(1ULL << (widx::tab_vehicles_trains + i));
                }
                else
                {
                    self.disabledWidgets |= (1ULL << (widx::tab_vehicles_trains + i));
                }
            }

            Widget::leftAlignTabs(self, widx::tab_station, widx::tab_vehicles_ships);

            // Activate the current tab.
            self.activatedWidgets &= ~((1ULL << widx::tab_station) | (1ULL << widx::tab_cargo) | (1ULL << widx::tab_cargo_ratings) | (1ULL << widx::tab_vehicles_trains) | (1ULL << widx::tab_vehicles_buses) | (1ULL << widx::tab_vehicles_trucks) | (1ULL << widx::tab_vehicles_trams) | (1ULL << widx::tab_vehicles_aircraft) | (1ULL << widx::tab_vehicles_ships));
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
        static void textInput(Window& self, WidgetIndex_t callingWidget, [[maybe_unused]] const WidgetId id, const char* input)
        {
            if (callingWidget != Common::widx::caption)
            {
                return;
            }

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
        static void switchTab(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex != widx::tab_cargo)
            {
                if (StationId(self.number) == _lastSelectedStation)
                {
                    showStationCatchment(StationId::null);
                }
            }

            if (ToolManager::isToolActive(self.type, self.number))
            {
                ToolManager::toolCancel();
            }

            TextInput::sub_4CE6C9(self.type, self.number);

            self.currentTab = widgetIndex - widx::tab_station;
            self.frameNo = 0;
            self.flags &= ~(WindowFlags::flag_16);
            self.var_85C = -1;

            self.viewportRemove(0);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_station];

            self.holdableWidgets = 0;
            self.eventHandlers = &tabInfo.events;
            self.activatedWidgets = 0;
            self.setWidgets(tabInfo.widgets);
            self.disabledWidgets = 0;
            self.rowHeight = tabInfo.rowHeight;

            // We'll need the vehicle list to determine what vehicle tabs to show
            VehiclesStopping::refreshVehicleList(&self);
            self.rowCount = 0;
            self.var_83C = 0;
            self.rowHover = -1;

            self.invalidate();
            self.callOnResize();
            self.callPrepareDraw();
            self.initScrollWidgets();
            self.invalidate();
            self.moveInsideScreenEdges();
        }

        // 0x0048EFBC
        void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            auto station = StationManager::get(StationId(self.number));
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
                if (self.currentTab == widx::tab_cargo - widx::tab_station)
                {
                    imageId += cargoTabImageIds[(self.frameNo / 8) % std::size(cargoTabImageIds)];
                }
                else
                {
                    imageId += cargoTabImageIds[0];
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo);
            }

            // Cargo ratings tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_cargo_ratings;
                Widget::drawTab(self, drawingCtx, imageId, widx::tab_cargo_ratings);

                auto widget = self.widgets[widx::tab_cargo_ratings];
                auto yOffset = widget.top + 14;
                auto xOffset = widget.left + 4;
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
                            {
                                ratingColour = Colour::red;
                            }
                        }

                        auto ratingBarLength = (cargo.rating * 30) / 256;
                        drawingCtx.fillRect(xOffset, yOffset, xOffset - 1 + ratingBarLength, yOffset + 1, Colours::getShade(ratingColour, 6), Gfx::RectFlags::none);

                        yOffset += 3;
                        totalRatingBars++;
                        if (totalRatingBars >= 4)
                        {
                            break;
                        }
                    }
                }
            }

            // clang-format off
            static constexpr std::pair<WidgetIndex_t, std::array<uint32_t, 8>> kTabAnimations[] = {
                { Common::widx::tab_vehicles_trains, {
                    InterfaceSkin::ImageIds::vehicle_train_frame_0,
                    InterfaceSkin::ImageIds::vehicle_train_frame_1,
                    InterfaceSkin::ImageIds::vehicle_train_frame_2,
                    InterfaceSkin::ImageIds::vehicle_train_frame_3,
                    InterfaceSkin::ImageIds::vehicle_train_frame_4,
                    InterfaceSkin::ImageIds::vehicle_train_frame_5,
                    InterfaceSkin::ImageIds::vehicle_train_frame_6,
                    InterfaceSkin::ImageIds::vehicle_train_frame_7,
                } },
                { Common::widx::tab_vehicles_aircraft, {
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_0,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_1,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_2,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_3,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_4,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_5,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_6,
                    InterfaceSkin::ImageIds::vehicle_aircraft_frame_7,
                } },
                { Common::widx::tab_vehicles_buses, {
                    InterfaceSkin::ImageIds::vehicle_buses_frame_0,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_1,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_2,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_3,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_4,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_5,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_6,
                    InterfaceSkin::ImageIds::vehicle_buses_frame_7,
                } },
                { Common::widx::tab_vehicles_trams, {
                    InterfaceSkin::ImageIds::vehicle_trams_frame_0,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_1,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_2,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_3,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_4,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_5,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_6,
                    InterfaceSkin::ImageIds::vehicle_trams_frame_7,
                } },
                { Common::widx::tab_vehicles_trucks, {
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_0,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_1,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_2,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_3,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_4,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_5,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_6,
                    InterfaceSkin::ImageIds::vehicle_trucks_frame_7,
                } },
                { Common::widx::tab_vehicles_ships, {
                    InterfaceSkin::ImageIds::vehicle_ships_frame_0,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_1,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_2,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_3,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_4,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_5,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_6,
                    InterfaceSkin::ImageIds::vehicle_ships_frame_7,
                } },
            };
            // clang-format on

            for (auto [tab, frames] : kTabAnimations)
            {
                if (self.isDisabled(tab))
                {
                    continue;
                }

                auto isActive = tab == self.currentTab + Common::widx::tab_station;
                auto imageId = isActive ? frames[self.frameNo / 2 % 8] : frames[0];

                uint32_t image = Gfx::recolour(skin->img + imageId, companyColour);
                Widget::drawTab(self, drawingCtx, image, tab);
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
                    self->disabledWidgets &= ~(1 << Common::widx::caption);
                }
                else
                {
                    self->disabledWidgets |= (1 << Common::widx::caption);
                }
            }
        }
    }
}
