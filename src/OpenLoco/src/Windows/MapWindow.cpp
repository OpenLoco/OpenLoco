#include "Drawing/SoftwareDrawingEngine.h"
#include "Engine/Limits.h"
#include "Entities/Entity.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameStateFlags.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Objects/IndustryObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "SceneManager.h"
#include "Types.hpp"
#include "Ui/LastMapWindowAttributes.h"
#include "Ui/ScrollView.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "Vehicles/Orders.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "Widget.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui::WindowManager;
using namespace OpenLoco::World;

namespace OpenLoco::Ui::Windows::MapWindow
{
    static loco_global<int32_t, 0x00523338> _cursorX2;
    static loco_global<int32_t, 0x0052333C> _cursorY2;
    static std::array<int16_t, 4> _4FDC4C = {
        {
            376,
            760,
            376,
            -8,
        }
    };
    static std::array<int16_t, 4> _4FDC4E = {
        {
            0,
            384,
            768,
            384,
        }
    };
    static loco_global<uint8_t[256], 0x004FDC5C> _byte_4FDC5C;
    static loco_global<uint32_t, 0x00F253A4> _dword_F253A4;
    static loco_global<uint8_t*, 0x00F253A8> _dword_F253A8;
    static std::array<uint16_t, 6> _vehicleTypeCounts = {
        {
            0,
            0,
            0,
            0,
            0,
            0,
        }
    };
    static loco_global<uint8_t[16], 0x00F253CE> _assignedIndustryColours;
    static loco_global<uint8_t[19], 0x00F253DF> _routeToObjectIdMap;
    static loco_global<uint8_t[19], 0x00F253F2> _routeColours;
    static loco_global<uint8_t[8], 0x00F25404> _trackColours;
    static loco_global<uint8_t[8], 0x00F2540C> _roadColours;
    static loco_global<Colour[Limits::kMaxCompanies + 1], 0x009C645C> _companyColours;
    static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;

    enum widx
    {
        frame = 0,
        caption,
        closeButton,
        panel,
        tabOverall,
        tabVehicles,
        tabIndustries,
        tabRoutes,
        tabOwnership,
        scrollview,
        statusBar,
    };

    const uint64_t enabledWidgets = (1 << closeButton) | (1 << tabOverall) | (1 << tabVehicles) | (1 << tabIndustries) | (1 << tabRoutes) | (1 << tabOwnership);

    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 350, 272 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 348, 13 }, WidgetType::caption_25, WindowColour::primary),
        makeWidget({ 335, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 41 }, { 350, 230 }, WidgetType::panel, WindowColour::secondary),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_overall),
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_vehicles),
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_industries),
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_routes),
        makeRemapWidget({ 158, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tab_map_ownership),
        makeWidget({ 3, 44 }, { 240, 215 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::horizontal | Scrollbars::vertical),
        makeWidget({ 3, 250 }, { 322, 21 }, WidgetType::wt_13, WindowColour::secondary),
        widgetEnd()
    };

    static Pos2 mapWindowPosToLocation(Point pos)
    {
        pos.x = ((pos.x + 8) - kMapColumns) / 2;
        pos.y = ((pos.y + 8)) / 2;
        Pos2 location = { static_cast<coord_t>(pos.y - pos.x), static_cast<coord_t>(pos.x + pos.y) };
        location.x *= kTileSize;
        location.y *= kTileSize;

        switch (getCurrentRotation())
        {
            case 0:
                return location;
            case 1:
                return { static_cast<coord_t>(kMapWidth - 1 - location.y), location.x };
            case 2:
                return { static_cast<coord_t>(kMapWidth - 1 - location.x), static_cast<coord_t>(kMapHeight - 1 - location.y) };
            case 3:
                return { location.y, static_cast<coord_t>(kMapHeight - 1 - location.x) };
        }

        return { 0, 0 }; // unreachable
    }

    static Point locationToMapWindowPos(Pos2 pos)
    {
        int32_t x = pos.x;
        int32_t y = pos.y;

        switch (getCurrentRotation())
        {
            case 3:
                std::swap(x, y);
                x = kMapWidth - 1 - x;
                break;
            case 2:
                x = kMapWidth - 1 - x;
                y = kMapHeight - 1 - y;
                break;
            case 1:
                std::swap(x, y);
                y = kMapHeight - 1 - y;
                break;
            case 0:
                break;
        }

        x /= kTileSize;
        y /= kTileSize;

        return Point(-x + y + kMapColumns - 8, x + y - 8);
    }

    // 0x0046B8E6
    static void onClose(Window& self)
    {
        Ui::getLastMapWindowAttributes().size = Ui::Size(self.width, self.height);
        Ui::getLastMapWindowAttributes().var88A = self.var_88A;
        Ui::getLastMapWindowAttributes().var88C = self.var_88C;
        Ui::getLastMapWindowAttributes().flags = self.flags | WindowFlags::flag_31;

        free(_dword_F253A8);
    }

    // 0x0046B8CF
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
                WindowManager::close(&self);
                break;

            case widx::tabOverall:
            case widx::tabVehicles:
            case widx::tabIndustries:
            case widx::tabRoutes:
            case widx::tabOwnership:
            case widx::scrollview:
            {
                auto tabIndex = widgetIndex - widx::tabOverall;

                if (tabIndex == self.currentTab)
                    return;

                self.currentTab = tabIndex;
                self.frameNo = 0;
                self.var_854 = 0;
                break;
            }
        }
    }

    // 0x0046B9F7
    static void onResize(Window& self)
    {
        self.flags |= WindowFlags::resizable;
        self.minWidth = 350;
        self.maxWidth = 800;
        self.maxHeight = 800;

        Ui::Size kMinWindowSize = { self.minWidth, self.minHeight };
        Ui::Size kMaxWindowSize = { self.maxWidth, self.maxHeight };
        self.setSize(kMinWindowSize, kMaxWindowSize);
    }

    // 0x0046C544
    static void sub_46C544(Window* self)
    {
        registers regs;
        regs.esi = X86Pointer(self);
        call(0x0046C544, regs);
    }

    // 0x0046D34D based on
    static void setHoverItem(Window* self, int16_t y, int index)
    {
        uint32_t itemHover;

        if (y < 0)
        {
            itemHover = (1 << index);
        }
        else
        {
            itemHover = 0;
        }

        if (itemHover != self->var_854)
        {
            self->var_854 = itemHover;
            self->invalidate();
        }

        if (self->var_854 != 0)
        {
            self->invalidate();
        }
    }

    static uint8_t legendWidth = 100;
    static uint8_t legendItemHeight = 10;
    static constexpr uint8_t kOverallGraphKeySize = 6;

    static std::array<size_t, 5> legendLengths = {
        {
            kOverallGraphKeySize,
            std::size(_vehicleTypeCounts),
            ObjectManager::getMaxObjects(ObjectType::industry),
            0,
            Limits::kMaxCompanies,
        }
    };

    static void setHoverItemTab(Window* self, int16_t legendLeft, int16_t legendBottom)
    {
        uint8_t i = 0;
        int16_t y = 0;
        if (!Input::hasFlag(Input::Flags::rightMousePressed))
        {
            uint32_t cursorX = _cursorX2;
            uint32_t cursorY = _cursorY2;
            auto window = WindowManager::findAt(cursorX, cursorY);

            if (window == self)
            {
                cursorX -= legendLeft;
                if (cursorX <= legendWidth)
                {
                    cursorY -= legendBottom;
                    if (self->currentTab == (widx::tabRoutes - widx::tabOverall))
                    {
                        y = cursorY;

                        for (; _routeToObjectIdMap[i] != 0xFF; i++)
                        {
                            y -= legendItemHeight;

                            if (y < 0)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (cursorY < legendLengths[self->currentTab] * legendItemHeight)
                        {
                            y = cursorY;

                            for (; i < legendLengths[self->currentTab]; i++)
                            {
                                if (self->currentTab == (widx::tabIndustries - widx::tabOverall))
                                {
                                    auto industryObj = ObjectManager::get<IndustryObject>(i);

                                    if (industryObj == nullptr)
                                        continue;
                                }
                                else if (self->currentTab == (widx::tabOwnership - widx::tabOverall))
                                {
                                    auto company = CompanyManager::get(CompanyId(i));

                                    if (company->empty())
                                        continue;
                                }

                                y -= legendItemHeight;

                                if (y < 0)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        setHoverItem(self, y, i);
    }

    // 0x0046B69C
    static void clearMap()
    {
        std::fill(static_cast<uint8_t*>(_dword_F253A8), _dword_F253A8 + 0x120000, PaletteIndex::index_0A);
    }

    // 0x00F2541D
    static uint16_t mapFrameNumber = 0;

    // 0x0046BA5B
    static void onUpdate(Window& self)
    {
        self.frameNo++;
        self.callPrepareDraw();

        WindowManager::invalidateWidget(WindowType::map, self.number, self.currentTab + widx::tabOverall);

        mapFrameNumber++;

        if (getCurrentRotation() != self.var_846)
        {
            self.var_846 = getCurrentRotation();
            clearMap();
        }

        auto i = 80;

        while (i > 0)
        {
            sub_46C544(&self);
            i--;
        }

        self.invalidate();

        auto x = self.x + self.width - 104;
        auto y = self.y + 44;

        setHoverItemTab(&self, x, y);
    }

    // 0x0046B9E7
    static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        self.callPrepareDraw();
        *scrollWidth = kMapColumns * 2;
        *scrollHeight = kMapRows * 2;
    }

    // 0x0046B9D4
    static void moveMainViewToMapView(Pos2 pos)
    {
        auto z = TileManager::getHeight(pos).landHeight;
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
            return;

        window->viewportCentreOnTile({ static_cast<coord_t>(pos.x), static_cast<coord_t>(pos.y), static_cast<coord_t>(z) });
    }

    // 0x0046B97C
    static void scrollMouseDown([[maybe_unused]] Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
    {
        auto pos = mapWindowPosToLocation({ x, y });

        moveMainViewToMapView(pos);
    }

    // 0x0046B946
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_map);
        return args;
    }

    // 0x0046B6BF
    static void prepareDraw(Window& self)
    {
        const StringId captionText[] = {
            StringIds::title_map,
            StringIds::title_map_vehicles,
            StringIds::title_map_industries,
            StringIds::title_map_routes,
            StringIds::title_map_companies,
        };

        widgets[widx::caption].text = captionText[self.currentTab];
        auto activatedWidgets = self.activatedWidgets;
        activatedWidgets &= ~((1ULL << widx::statusBar) | (1ULL << widx::scrollview) | (1ULL << widx::tabOwnership) | (1ULL << widx::tabRoutes) | (1ULL << widx::tabIndustries) | (1ULL << widx::tabVehicles) | (1ULL << widx::tabOverall));

        auto currentWidget = self.currentTab + widx::tabOverall;
        activatedWidgets |= (1ULL << currentWidget);
        self.activatedWidgets = activatedWidgets;

        self.widgets[widx::frame].right = self.width - 1;
        self.widgets[widx::frame].bottom = self.height - 1;
        self.widgets[widx::panel].right = self.width - 1;
        self.widgets[widx::panel].bottom = self.height - 1;

        self.widgets[widx::caption].right = self.width - 2;
        self.widgets[widx::closeButton].left = self.width - 15;
        self.widgets[widx::closeButton].right = self.width - 3;
        self.widgets[widx::scrollview].bottom = self.height - 14;
        self.widgets[widx::scrollview].right = self.width - 108;

        self.widgets[widx::statusBar].top = self.height - 12;
        self.widgets[widx::statusBar].bottom = self.height - 3;
        self.widgets[widx::statusBar].right = self.width - 14;

        auto disabledWidgets = 0;

        if (isEditorMode())
        {
            disabledWidgets |= (1 << widx::tabVehicles) | (1 << widx::tabRoutes) | (1 << widx::tabOwnership);
        }

        self.disabledWidgets = disabledWidgets;

        Widget::leftAlignTabs(self, widx::tabOverall, widx::tabOwnership);
    }

    // 0x0046D0E0
    static void drawTabs(Window* self, Gfx::RenderTarget* rt)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();

        // tabOverall
        {
            uint32_t imageId = skin->img;
            imageId += InterfaceSkin::ImageIds::toolbar_menu_map_north;

            Widget::drawTab(self, rt, imageId, widx::tabOverall);
        }

        // tabVehicles,
        {
            if (!(self->disabledWidgets & (1 << widx::tabVehicles)))
            {
                static const uint32_t vehicleImageIds[] = {
                    InterfaceSkin::ImageIds::vehicle_train_frame_0,
                    InterfaceSkin::ImageIds::vehicle_train_frame_1,
                    InterfaceSkin::ImageIds::vehicle_train_frame_2,
                    InterfaceSkin::ImageIds::vehicle_train_frame_3,
                    InterfaceSkin::ImageIds::vehicle_train_frame_4,
                    InterfaceSkin::ImageIds::vehicle_train_frame_5,
                    InterfaceSkin::ImageIds::vehicle_train_frame_6,
                    InterfaceSkin::ImageIds::vehicle_train_frame_7,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tabVehicles - widx::tabOverall)
                    imageId += vehicleImageIds[(self->frameNo / 2) % std::size(vehicleImageIds)];
                else
                    imageId += vehicleImageIds[0];

                auto colour = Colour::black;

                if (!isEditorMode() && !isSandboxMode())
                {
                    auto company = CompanyManager::getPlayerCompany();
                    colour = company->mainColours.primary;
                }

                imageId = Gfx::recolour(imageId, colour);

                Widget::drawTab(self, rt, imageId, widx::tabVehicles);
            }
        }

        // tabIndustries,
        {
            uint32_t imageId = skin->img;
            imageId += InterfaceSkin::ImageIds::toolbar_menu_industries;

            Widget::drawTab(self, rt, imageId, widx::tabIndustries);
        }

        // tabRoutes,
        {
            if (!(self->disabledWidgets & (1 << widx::tabRoutes)))
            {
                static const uint32_t routeImageIds[] = {
                    InterfaceSkin::ImageIds::tab_routes_frame_0,
                    InterfaceSkin::ImageIds::tab_routes_frame_1,
                    InterfaceSkin::ImageIds::tab_routes_frame_2,
                    InterfaceSkin::ImageIds::tab_routes_frame_3,
                };

                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tabRoutes - widx::tabOverall)
                    imageId += routeImageIds[(self->frameNo / 16) % std::size(routeImageIds)];
                else
                    imageId += routeImageIds[0];

                Widget::drawTab(self, rt, imageId, widx::tabRoutes);
            }
        }

        // tabOwnership,
        {
            if (!(self->disabledWidgets & (1 << widx::tabOwnership)))
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_companies;

                Widget::drawTab(self, rt, imageId, widx::tabOwnership);
            }
        }
    }

    // 0x0046D273
    static void drawGraphKeyOverall(Window* self, Gfx::RenderTarget* rt, uint16_t x, uint16_t* y)
    {
        static const PaletteIndex_t overallColours[] = {
            PaletteIndex::index_41,
            PaletteIndex::index_7D,
            PaletteIndex::index_0C,
            PaletteIndex::index_11,
            PaletteIndex::index_BA,
            PaletteIndex::index_64,
        };

        static const StringId lineNames[] = {
            StringIds::map_key_towns,
            StringIds::map_key_industries,
            StringIds::map_key_roads,
            StringIds::map_key_railways,
            StringIds::map_key_stations,
            StringIds::map_key_vegetation,
        };

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (auto i = 0; i < kOverallGraphKeySize; i++)
        {
            auto colour = overallColours[i];
            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                drawingCtx.drawRect(*rt, x, *y + 3, 5, 5, colour, Drawing::RectFlags::none);
            }

            FormatArguments args{};
            args.push(lineNames[i]);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            drawingCtx.drawStringLeftClipped(*rt, x + 6, *y, 94, Colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x004FDD62
    static const PaletteIndex_t vehicleTypeColours[] = {
        PaletteIndex::index_AD,
        PaletteIndex::index_67,
        PaletteIndex::index_A2,
        PaletteIndex::index_BC,
        PaletteIndex::index_15,
        PaletteIndex::index_B8, // changed from 136 to make ships more viewable on the map
    };

    // 0x0046D379
    static void drawGraphKeyVehicles(Window* self, Gfx::RenderTarget* rt, uint16_t x, uint16_t* y)
    {
        static const StringId lineNames[] = {
            StringIds::forbid_trains,
            StringIds::forbid_buses,
            StringIds::forbid_trucks,
            StringIds::forbid_trams,
            StringIds::forbid_aircraft,
            StringIds::forbid_ships,
        };

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (uint8_t i = 0; i < std::size(_vehicleTypeCounts); i++)
        {
            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                auto colour = vehicleTypeColours[i];

                drawingCtx.drawRect(*rt, x, *y + 3, 5, 5, colour, Drawing::RectFlags::none);
            }

            FormatArguments args{};
            args.push(lineNames[i]);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            drawingCtx.drawStringLeftClipped(*rt, x + 6, *y, 94, Colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x004FB464
    // clang-format off
    static constexpr std::array<PaletteIndex_t, 31> industryColours = {
        PaletteIndex::index_0A, PaletteIndex::index_0E, PaletteIndex::index_15, PaletteIndex::index_1F,
        PaletteIndex::index_29, PaletteIndex::index_35, PaletteIndex::index_38, PaletteIndex::index_3F,
        PaletteIndex::index_43, PaletteIndex::index_4B, PaletteIndex::index_50, PaletteIndex::index_58,
        PaletteIndex::index_66, PaletteIndex::index_71, PaletteIndex::index_7D, PaletteIndex::index_85,
        PaletteIndex::index_89, PaletteIndex::index_9D, PaletteIndex::index_A1, PaletteIndex::index_A3,
        PaletteIndex::index_AC, PaletteIndex::index_B8, PaletteIndex::index_BB, PaletteIndex::index_C3,
        PaletteIndex::index_C6, PaletteIndex::index_D0, PaletteIndex::index_D3, PaletteIndex::index_DB,
        PaletteIndex::index_DE, PaletteIndex::index_24, PaletteIndex::index_12,
    };
    // clang-format on

    // 0x0046D47F
    static void drawGraphKeyIndustries(Window* self, Gfx::RenderTarget* rt, uint16_t x, uint16_t* y)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto industry = ObjectManager::get<IndustryObject>(i);

            if (industry == nullptr)
                continue;

            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                auto colour = industryColours[_assignedIndustryColours[i]];

                drawingCtx.drawRect(*rt, x, *y + 3, 5, 5, colour, Drawing::RectFlags::none);
            }

            FormatArguments args{};
            args.push(industry->name);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            drawingCtx.drawStringLeftClipped(*rt, x + 6, *y, 94, Colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D5A4
    static void drawGraphKeyRoutes(Window* self, Gfx::RenderTarget* rt, uint16_t x, uint16_t* y)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (auto i = 0; _routeToObjectIdMap[i] != 0xFF; i++)
        {
            auto index = _routeToObjectIdMap[i];
            auto colour = _routeColours[i];

            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                drawingCtx.drawRect(*rt, x, *y + 3, 5, 5, colour, Drawing::RectFlags::none);
            }

            auto routeType = StringIds::map_routes_aircraft;

            if (index != 0xFE)
            {
                routeType = StringIds::map_routes_ships;

                if (index != 0xFD)
                {
                    if (index & (1 << 7))
                    {
                        auto roadObj = ObjectManager::get<RoadObject>(index & ~(1 << 7));
                        routeType = roadObj->name;
                    }
                    else
                    {
                        auto trackObj = ObjectManager::get<TrackObject>(index);
                        routeType = trackObj->name;
                    }
                }
            }

            FormatArguments args{};
            args.push(routeType);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = StringIds::small_white_string;
            }

            drawingCtx.drawStringLeftClipped(*rt, x + 6, *y, 94, Colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D6E1
    static void drawGraphKeyCompanies(Window* self, Gfx::RenderTarget* rt, uint16_t x, uint16_t* y)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (const auto& company : CompanyManager::companies())
        {
            auto index = company.id();
            auto colour = Colours::getShade(company.mainColours.primary, 6);

            if (!(self->var_854 & (1 << enumValue(index))) || !(mapFrameNumber & (1 << 2)))
            {
                drawingCtx.drawRect(*rt, x, *y + 3, 5, 5, colour, Drawing::RectFlags::none);
            }

            FormatArguments args{};
            args.push(company.name);

            auto stringId = StringIds::small_black_string;

            if (self->var_854 & (1 << enumValue(index)))
            {
                stringId = StringIds::small_white_string;
            }

            drawingCtx.drawStringLeftClipped(*rt, x + 6, *y, 94, Colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D81F
    static void formatVehicleString(Window* self, FormatArguments& args)
    {
        static const StringId vehicleStringSingular[] = {
            StringIds::num_trains_singular,
            StringIds::num_buses_singular,
            StringIds::num_trucks_singular,
            StringIds::num_trams_singular,
            StringIds::num_aircrafts_singular,
            StringIds::num_ships_singular,
        };

        static const StringId vehicleStringPlural[] = {
            StringIds::num_trains_plural,
            StringIds::num_buses_plural,
            StringIds::num_trucks_plural,
            StringIds::num_trams_plural,
            StringIds::num_aircrafts_plural,
            StringIds::num_ships_plural,
        };

        int16_t vehicleIndex = Numerics::bitScanForward(self->var_854);
        uint16_t totalVehicleCount = 0;
        auto stringId = StringIds::status_num_vehicles_plural;

        if (vehicleIndex == -1)
        {
            for (auto i = 0; i < 6; i++)
            {
                totalVehicleCount += _vehicleTypeCounts[i];
            }

            if (totalVehicleCount == 1)
            {
                stringId = StringIds::status_num_vehicles_singular;
            }
        }
        else
        {
            totalVehicleCount = _vehicleTypeCounts[vehicleIndex];
            stringId = vehicleStringPlural[vehicleIndex];

            if (totalVehicleCount == 1)
            {
                stringId = vehicleStringSingular[vehicleIndex];
            }
        }

        args.push(stringId);
        args.push(totalVehicleCount);
    }

    // 0x0046D87C
    static void formatIndustryString(Window* self, FormatArguments& args)
    {
        int16_t industryIndex = Numerics::bitScanForward(self->var_854);

        if (industryIndex == -1)
        {
            auto industries = IndustryManager::industries();
            auto industryCount = std::distance(std::begin(industries), std::end(industries));

            auto stringId = StringIds::status_num_industries_plural;

            if (industryCount == 1)
            {
                stringId = StringIds::status_num_industries_singular;
            }

            args.push(stringId);
            args.push(industryCount);
        }
        else
        {
            auto industryCount = 0;
            for (const auto& industry : IndustryManager::industries())
            {
                if (industry.objectId == industryIndex)
                {
                    industryCount++;
                }
            }

            auto industryObj = ObjectManager::get<IndustryObject>(industryIndex);
            auto stringId = industryObj->namePlural;

            if (industryCount == 1)
            {
                stringId = industryObj->nameSingular;
            }

            auto buffer = StringManager::getString(StringIds::buffer_1250);
            char* ptr = const_cast<char*>(buffer);

            ptr = StringManager::formatString(ptr, stringId, &industryCount);

            *ptr++ = ' ';
            *ptr++ = '(';

            if (industryObj->requiresCargo())
            {
                ptr = StringManager::formatString(ptr, StringIds::industry_require);

                ptr = industryObj->getRequiredCargoString(ptr);

                if (industryObj->producesCargo())
                {
                    ptr = StringManager::formatString(ptr, StringIds::cargo_to_produce);

                    ptr = industryObj->getProducedCargoString(ptr);
                }
            }
            else if (industryObj->producesCargo())
            {
                ptr = StringManager::formatString(ptr, StringIds::industry_produce);

                ptr = industryObj->getProducedCargoString(ptr);
            }

            *ptr++ = ')';
            *ptr = '\0';

            args.push(StringIds::buffer_1250);
            args.push(industryCount);
        }
    }

    // 0x0046B779
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        self.draw(rt);
        drawTabs(&self, rt);

        {
            auto x = self.x + self.width - 104;
            uint16_t y = self.y + 44;

            switch (self.currentTab + widx::tabOverall)
            {
                case widx::tabOverall:
                    drawGraphKeyOverall(&self, rt, x, &y);
                    break;

                case widx::tabVehicles:
                    drawGraphKeyVehicles(&self, rt, x, &y);
                    break;

                case widx::tabIndustries:
                    drawGraphKeyIndustries(&self, rt, x, &y);
                    break;

                case widx::tabRoutes:
                    drawGraphKeyRoutes(&self, rt, x, &y);
                    break;

                case widx::tabOwnership:
                    drawGraphKeyCompanies(&self, rt, x, &y);
                    break;
            }

            y -= self.y;
            y += 14;
            y = std::max(y, static_cast<uint16_t>(92));

            self.minHeight = y;
        }

        FormatArguments args{};

        switch (self.currentTab + widx::tabOverall)
        {
            case widx::tabOverall:
            case widx::tabRoutes:
            case widx::tabOwnership:
                args.push(StringIds::empty);
                break;

            case widx::tabVehicles:
                formatVehicleString(&self, args);
                break;

            case widx::tabIndustries:
                formatIndustryString(&self, args);
                break;
        }

        auto x = self.x + self.widgets[widx::statusBar].left - 1;
        auto y = self.y + self.widgets[widx::statusBar].top - 1;
        auto width = self.widgets[widx::statusBar].width();

        drawingCtx.drawStringLeftClipped(*rt, x, y, width, Colour::black, StringIds::black_stringid, &args);
    }

    // 0x0046BF0F based on
    static void drawVehicleOnMap(Gfx::RenderTarget* rt, Vehicles::VehicleBase* vehicle, uint8_t colour)
    {
        if (vehicle->position.x == Location::null)
            return;

        auto trainPos = locationToMapWindowPos(vehicle->position);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRect(*rt, trainPos.x, trainPos.y, trainPos.x, trainPos.y, colour, Drawing::RectFlags::none);
    }

    // 0x0046C294
    static std::pair<Point, Point> drawRouteLine(Gfx::RenderTarget* rt, Point startPos, Point endPos, Pos2 stationPos, uint8_t colour)
    {
        auto newStartPos = locationToMapWindowPos({ stationPos.x, stationPos.y });

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        if (endPos.x != Location::null)
        {
            drawingCtx.drawLine(*rt, endPos, newStartPos, colour);
        }

        endPos = newStartPos;

        if (startPos.x == Location::null)
        {
            startPos = newStartPos;
        }

        return std::make_pair(startPos, endPos);
    }

    static std::optional<uint8_t> getRouteColour(Vehicles::Vehicle train)
    {
        uint8_t colour;
        if (train.head->vehicleType == VehicleType::aircraft)
        {
            colour = 211;
            auto index = Numerics::bitScanForward(_dword_F253A4);
            if (index != -1)
            {
                if (_routeToObjectIdMap[index] == 0xFE)
                {
                    if (mapFrameNumber & (1 << 2))
                    {
                        colour = _byte_4FDC5C[colour];
                    }
                }
            }
        }
        else if (train.head->vehicleType == VehicleType::ship)
        {
            colour = 139;
            auto index = Numerics::bitScanForward(_dword_F253A4);
            if (index != -1)
            {
                if (_routeToObjectIdMap[index] == 0xFD)
                {
                    if (mapFrameNumber & (1 << 2))
                    {
                        colour = _byte_4FDC5C[colour];
                    }
                }
            }
        }
        else
        {
            return std::nullopt;
        }

        return colour;
    }

    // 0x0046C18D
    static void drawRoutesOnMap(Gfx::RenderTarget* rt, Vehicles::Vehicle train)
    {
        auto colour = getRouteColour(train);

        if (!colour)
            return;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        Point startPos = { Location::null, 0 };
        Point endPos = { Location::null, 0 };
        for (auto& order : Vehicles::OrderRingView(train.head->orderTableOffset))
        {
            if (order.hasFlags(Vehicles::OrderFlags::HasStation))
            {
                auto* stationOrder = static_cast<Vehicles::OrderStation*>(&order);
                auto station = StationManager::get(stationOrder->getStation());
                Pos2 stationPos = { station->x, station->y };

                auto routePos = drawRouteLine(rt, startPos, endPos, stationPos, *colour);
                startPos = routePos.first;
                endPos = routePos.second;
            }
        }

        if (startPos.x == Location::null || endPos.x == Location::null)
            return;

        drawingCtx.drawLine(*rt, startPos, endPos, *colour);
    }

    // 0x0046C426
    static uint8_t getVehicleColour(WidgetIndex_t widgetIndex, Vehicles::Vehicle train, Vehicles::Car car)
    {
        auto colour = PaletteIndex::index_15;

        if (widgetIndex == widx::tabOwnership || widgetIndex == widx::tabVehicles)
        {
            uint8_t index = enumValue(car.front->owner);
            colour = Colours::getShade(_companyColours[index], 7);

            if (widgetIndex == widx::tabVehicles)
            {
                index = enumValue(train.head->vehicleType);
                colour = vehicleTypeColours[index];
            }

            if (_dword_F253A4 & (1 << index))
            {
                if (!(mapFrameNumber & (1 << 2)))
                {
                    colour = _byte_4FDC5C[colour];
                }
            }
        }

        return colour;
    }

    // 0x0046BFAD
    static void countVehiclesOnMap()
    {
        for (auto i = 0; i < 6; i++)
        {
            _vehicleTypeCounts[i] = 0;
        }

        for (auto* vehicle : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*vehicle);

            if (train.head->has38Flags(Vehicles::Flags38::isGhost))
                continue;

            if (train.head->position.x == Location::null)
                continue;

            auto vehicleType = train.head->vehicleType;
            _vehicleTypeCounts[static_cast<uint8_t>(vehicleType)] = _vehicleTypeCounts[static_cast<uint8_t>(vehicleType)] + 1;
        }
    }

    // 0x0046BE6E, 0x0046C35A
    static void drawVehiclesOnMap(Gfx::RenderTarget* rt, WidgetIndex_t widgetIndex)
    {
        for (auto* vehicle : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*vehicle);

            if (train.head->has38Flags(Vehicles::Flags38::isGhost))
                continue;

            if (train.head->position.x == Location::null)
                continue;

            for (auto& car : train.cars)
            {
                auto colour = getVehicleColour(widgetIndex, train, car);
                car.applyToComponents([rt, colour](auto& component) { drawVehicleOnMap(rt, &component, colour); });
            }

            if (widgetIndex == widx::tabRoutes)
            {
                drawRoutesOnMap(rt, train);
            }
        }
    }

    // 0x0046BE51, 0x0046BE34
    static void drawRectOnMap(Gfx::RenderTarget* rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, Drawing::RectFlags flags)
    {
        if (left > right)
        {
            std::swap(left, right);
        }

        if (top > bottom)
        {
            std::swap(top, bottom);
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.fillRect(*rt, left, top, right, bottom, colour, flags);
    }

    // 0x0046BE51
    static void drawViewOnMap(Gfx::RenderTarget* rt, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        left /= 32;
        top /= 16;
        right /= 32;
        bottom /= 16;
        left += _4FDC4C[getCurrentRotation()];
        top += _4FDC4E[getCurrentRotation()];
        right += _4FDC4C[getCurrentRotation()];
        bottom += _4FDC4E[getCurrentRotation()];

        const auto colour = PaletteIndex::index_0A;

        drawRectOnMap(rt, left, top, right, bottom, colour, Drawing::RectFlags::crossHatching);
    }

    // 0x0046BE34
    static void drawViewCornersOnMap(Gfx::RenderTarget* rt, int16_t left, int16_t top, int16_t leftOffset, int16_t topOffset, int16_t rightOffset, int16_t bottomOffset)
    {
        left /= 32;
        top /= 16;
        left += _4FDC4C[getCurrentRotation()];
        top += _4FDC4E[getCurrentRotation()];
        auto right = left;
        auto bottom = top;
        left += leftOffset;
        top += topOffset;
        right += rightOffset;
        bottom += bottomOffset;

        const auto colour = PaletteIndex::index_0A;

        drawRectOnMap(rt, left, top, right, bottom, colour, Drawing::RectFlags::none);
    }

    // 0x0046BAD5
    static void drawViewportPosition(Gfx::RenderTarget* rt)
    {
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
            return;

        auto viewport = window->viewports[0];

        if (viewport == nullptr)
            return;

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            right += viewport->viewWidth;

            drawViewOnMap(rt, left, top, right, bottom);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            top += viewport->viewHeight;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            right += viewport->viewWidth;
            bottom += viewport->viewHeight;

            drawViewOnMap(rt, left, top, right, bottom);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            bottom += viewport->viewHeight;

            drawViewOnMap(rt, left, top, right, bottom);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            left += viewport->viewWidth;
            auto right = viewport->viewX;
            auto bottom = viewport->viewY;
            right += viewport->viewWidth;
            bottom += viewport->viewHeight;

            drawViewOnMap(rt, left, top, right, bottom);
        }

        if (!(mapFrameNumber & (1 << 2)))
            return;

        if (_dword_F253A4 != 0)
            return;

        uint8_t cornerSize = 5;

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;

            drawViewCornersOnMap(rt, left, top, 0, 0, cornerSize, 0);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;

            drawViewCornersOnMap(rt, left, top, -cornerSize, 0, 0, 0);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;

            drawViewCornersOnMap(rt, left, top, 0, 0, 0, cornerSize);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(rt, left, top, 0, -cornerSize, 0, 0);
        }

        {
            auto left = viewport->viewX;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(rt, left, top, 0, 0, cornerSize, 0);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(rt, left, top, -cornerSize, 0, 0, 0);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;

            drawViewCornersOnMap(rt, left, top, 0, 0, 0, cornerSize);
        }

        {
            auto left = viewport->viewX;
            left += viewport->viewWidth;
            auto top = viewport->viewY;
            top += viewport->viewHeight;

            drawViewCornersOnMap(rt, left, top, 0, -cornerSize, 0, 0);
        }
    }

    // 0x0046C481
    static void drawTownNames(Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        for (const auto& town : TownManager::towns())
        {
            auto townPos = locationToMapWindowPos({ town.x, town.y });

            StringManager::formatString(_stringFormatBuffer, town.name);
            drawingCtx.setCurrentFontSpriteBase(Font::small);

            auto strWidth = drawingCtx.getStringWidth(_stringFormatBuffer);

            strWidth /= 2;

            townPos.x -= strWidth;
            townPos.y -= 3;

            drawingCtx.setCurrentFontSpriteBase(Font::small);
            drawingCtx.drawString(*rt, townPos.x, townPos.y, AdvancedColour(Colour::purple).outline(), _stringFormatBuffer);
        }
    }

    // 0x0046B806
    static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
            return;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.clearSingle(rt, PaletteIndex::index_0A);

        auto element = Gfx::getG1Element(0);
        auto backupElement = *element;
        auto offset = *_dword_F253A8;

        if (mapFrameNumber & (1 << 2))
            offset += 0x90000;

        Gfx::getG1Element(0)->offset = offset;
        Gfx::getG1Element(0)->width = kMapColumns * 2;
        Gfx::getG1Element(0)->height = kMapRows * 2;
        Gfx::getG1Element(0)->xOffset = -8;
        Gfx::getG1Element(0)->yOffset = -8;
        Gfx::getG1Element(0)->flags = Gfx::G1ElementFlags::none;

        drawingCtx.drawImage(&rt, 0, 0, 0);

        *element = backupElement;

        if (self.currentTab + widx::tabOverall == widx::tabVehicles)
        {
            countVehiclesOnMap();
        }

        drawVehiclesOnMap(&rt, self.currentTab + widx::tabOverall);

        drawViewportPosition(&rt);

        if (self.savedView.mapX & (1 << 0))
        {
            drawTownNames(&rt);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onResize = onResize,
        .onUpdate = onUpdate,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = scrollMouseDown,
        .scrollMouseDrag = scrollMouseDown,
        .tooltip = tooltip,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }

    // 0x0046D0C3
    static uint32_t checkIndustryColours(PaletteIndex_t colour, uint32_t colourMask)
    {
        for (auto i = 0; i < 31; i++)
        {
            auto industryColour = industryColours[i];
            auto diff = industryColour - colour;
            if (industryColour < colour)
            {
                diff = -diff;
            }

            if (diff <= 2)
            {
                colourMask &= ~(1U << i);
            }
        }

        return colourMask;
    }

    // 0x0046CFF0
    static void assignIndustryColours()
    {
        uint32_t availableColours = 0x7FFFFFFF;

        // First, assign water colour
        {
            auto waterObj = ObjectManager::get<WaterObject>();

            auto waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(waterPixel, availableColours);

            waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(waterPixel, availableColours);
        }

        // Then, assign surface texture colours
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::land); i++)
        {
            auto landObj = ObjectManager::get<LandObject>(i);
            if (landObj == nullptr)
                continue;

            auto landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(landPixel, availableColours);

            landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(landPixel, availableColours);
        }

        availableColours = checkIndustryColours(PaletteIndex::index_3C, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_0C, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_0A, availableColours);

        // Reset assigned industry colours
        for (auto i = 0U; i < std::size(_assignedIndustryColours); i++)
        {
            _assignedIndustryColours[i] = 0xFF;
        }

        // Assign preferred industry colours, if possible
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(i);
            if (industryObj == nullptr)
                continue;

            // Preferred colour still available?
            auto preferredColour = enumValue(industryObj->mapColour);
            if (availableColours & (1U << preferredColour))
            {
                _assignedIndustryColours[i] = preferredColour;
                availableColours &= ~(1U << preferredColour);
            }
        }

        // Assign alternative industry colours if needed
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::industry); i++)
        {
            auto industryObj = ObjectManager::get<IndustryObject>(i);
            if (industryObj == nullptr)
                continue;

            if (_assignedIndustryColours[i] != 0xFF)
                continue;

            auto freeColour = std::max(0, Numerics::bitScanForward(availableColours));
            availableColours &= ~(1U << freeColour);
            _assignedIndustryColours[i] = freeColour;
        }
    }

    // 0x0046CED0
    static void assignRouteColours()
    {
        uint32_t availableColours = 0x7FFFFFFF;

        // First, assign water colour
        {
            auto waterObj = ObjectManager::get<WaterObject>();

            auto waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(waterPixel, availableColours);

            waterPixel = Gfx::getG1Element(waterObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(waterPixel, availableColours);
        }

        // Then, assign surface texture colours
        for (auto i = 0U; i < ObjectManager::getMaxObjects(ObjectType::land); i++)
        {
            auto landObj = ObjectManager::get<LandObject>(i);
            if (landObj == nullptr)
                continue;

            auto landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[0];
            availableColours = checkIndustryColours(landPixel, availableColours);

            landPixel = Gfx::getG1Element(landObj->mapPixelImage)->offset[1];
            availableColours = checkIndustryColours(landPixel, availableColours);
        }

        availableColours = checkIndustryColours(PaletteIndex::index_3C, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_BA, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_D3, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_8B, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_0A, availableColours);
        availableColours = checkIndustryColours(PaletteIndex::index_15, availableColours);

        auto availableTracks = CompanyManager::getPlayerCompany()->getAvailableRailTracks();
        auto availableRoads = CompanyManager::getPlayerCompany()->getAvailableRoads();

        auto i = 0U;
        auto assignColour = [&i, &availableColours](uint8_t id) {
            _routeToObjectIdMap[i] = id;
            auto freeColour = std::max(0, Numerics::bitScanForward(availableColours));
            availableColours &= ~(1U << freeColour);

            auto colour = industryColours[freeColour];
            _routeColours[i] = colour;

            if (id & (1U << 7))
            {
                _roadColours[id & ~(1U << 7)] = colour;
            }
            else
            {
                _trackColours[id] = colour;
            }
            i++;
        };

        for (auto& track : availableTracks)
        {
            assignColour(track);
        }
        for (auto& road : availableRoads)
        {
            assignColour(road);
        }

        // Airplanes
        _routeToObjectIdMap[i] = 0xFE;
        // Ships
        _routeToObjectIdMap[i + 1] = 0xFD;
        // End list
        _routeToObjectIdMap[i + 2] = 0xFF;

        // Airplanes
        _routeColours[i] = 0xD3;
        // Ships
        _routeColours[i + 1] = 0x8B;
    }

    static const WindowEventList& getEvents();

    // 0x0046B490
    void open()
    {
        auto window = WindowManager::bringToFront(WindowType::map, 0);

        if (window != nullptr)
            return;

        auto ptr = malloc(kMapSize * 8);

        if (ptr == nullptr)
            return;

        _dword_F253A8 = static_cast<uint8_t*>(ptr);
        Ui::Size size = { 350, 272 };

        if (Ui::getLastMapWindowAttributes().flags != WindowFlags::none)
        {
            size = Ui::getLastMapWindowAttributes().size;
            size.width = std::clamp<uint16_t>(size.width, 350, Ui::width());
            size.height = std::clamp<uint16_t>(size.height, 272, Ui::height() - 56);
        }

        window = WindowManager::createWindow(WindowType::map, size, WindowFlags::none, getEvents());
        window->widgets = widgets;
        window->enabledWidgets |= enabledWidgets;

        window->initScrollWidgets();
        window->frameNo = 0;

        if (Ui::getLastMapWindowAttributes().flags != WindowFlags::none)
        {
            window->var_88A = Ui::getLastMapWindowAttributes().var88A;
            window->var_88C = Ui::getLastMapWindowAttributes().var88C;
            window->flags |= (Ui::getLastMapWindowAttributes().flags & WindowFlags::flag_16);
        }

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_0F);

        window->var_846 = getCurrentRotation();

        clearMap();

        centerOnViewPoint();

        window->currentTab = 0;
        window->savedView.mapX = 1;
        window->var_854 = 0;
        window->var_856 = 0;

        assignIndustryColours();
        assignRouteColours();

        mapFrameNumber = 0;
    }

    // 0x0046B5C0
    void centerOnViewPoint()
    {
        auto mainWindow = WindowManager::getMainWindow();

        if (mainWindow == nullptr)
            return;

        auto viewport = mainWindow->viewports[0];

        if (viewport == nullptr)
            return;

        auto window = WindowManager::find(WindowType::map, 0);

        if (window == nullptr)
            return;

        auto x = viewport->viewWidth / 2;
        auto y = viewport->viewHeight / 2;
        x += viewport->viewX;
        y += viewport->viewY;
        x /= 32;
        y /= 16;
        x += _4FDC4C[getCurrentRotation()];
        y += _4FDC4E[getCurrentRotation()];

        auto width = widgets[widx::scrollview].width() - 10;
        auto height = widgets[widx::scrollview].height() - 10;
        x -= width / 2;
        x = std::max(x, 0);
        y -= height / 2;
        y = std::max(y, 0);

        width = -width;
        height = -height;
        width += window->scrollAreas[0].contentWidth;
        height += window->scrollAreas[0].contentHeight;

        width -= x;
        if (width < 0)
        {
            x += width;
            x = std::max(x, 0);
        }

        height -= y;
        if (height < 0)
        {
            y += height;
            y = std::max(y, 0);
        }

        window->scrollAreas[0].contentOffsetX = x;
        window->scrollAreas[0].contentOffsetY = y;

        Ui::ScrollView::updateThumbs(window, widx::scrollview);
    }
}
