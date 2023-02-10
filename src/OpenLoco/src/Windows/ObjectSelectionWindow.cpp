#include "Audio/Audio.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/AirportObject.h"
#include "Objects/BridgeObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/CliffEdgeObject.h"
#include "Objects/CompetitorObject.h"
#include "Objects/CurrencyObject.h"
#include "Objects/DockObject.h"
#include "Objects/HillShapesObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Objects/RegionObject.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/ScaffoldingObject.h"
#include "Objects/SnowObject.h"
#include "Objects/StreetLightObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Objects/TrainStationObject.h"
#include "Objects/TreeObject.h"
#include "Objects/TunnelObject.h"
#include "Objects/VehicleObject.h"
#include "Objects/WallObject.h"
#include "Objects/WaterObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "Window.h"
#include <OpenLoco/Console/Console.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ObjectSelectionWindow
{
    static constexpr int kRowHeight = 12;
    static constexpr Ui::Size kWindowSize = { 600, 398 };

    static loco_global<uint8_t[999], 0x004FE384> _4FE384;

#pragma pack(push, 1)
    struct tabPosition
    {
        uint8_t index;
        uint8_t row;
    };
#pragma pack(pop)

    static loco_global<char[2], 0x005045F8> _strCheckmark;
    static loco_global<uint8_t*, 0x50D144> _50D144;

    static loco_global<uint16_t, 0x0052334A> _52334A;
    static loco_global<uint16_t, 0x0052334C> _52334C;

    static loco_global<uint16_t[33], 0x00112C181> _tabObjectCounts;
    static loco_global<tabPosition[36], 0x0112C21C> _tabInformation;

    static void initEvents();

    enum widx
    {
        frame,
        caption,
        closeButton,
        panel,
        tabArea,
        advancedButton,
        scrollview,
        objectImage,
    };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 600, 398 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 598, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::title_object_selection),
        makeWidget({ 585, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 65 }, { 600, 333 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 3, 15 }, { 589, 50 }, WidgetType::wt_6, WindowColour::secondary),
        makeWidget({ 470, 20 }, { 122, 12 }, WidgetType::button, WindowColour::primary, StringIds::object_selection_advanced, StringIds::object_selection_advanced_tooltip),
        makeWidget({ 4, 68 }, { 288, 317 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        makeWidget({ 391, 68 }, { 114, 114 }, WidgetType::buttonWithImage, WindowColour::secondary),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void sub_4731EE(Window* self, ObjectType eax);

    // 0x00473154
    static void assignTabPositions(Window* self)
    {
        uint8_t currentRow = 0;
        uint8_t currentPos = 0;
        uint8_t rowCapacity = 19;
        uint8_t tabPos = 0;

        for (int8_t currentType = ObjectManager::maxObjectTypes - 1; currentType >= 0; currentType--)
        {
            if ((_4FE384[currentType] & (1 << 0)) != 0)
                continue;

            // Skip all types that don't have any objects
            if (_tabObjectCounts[currentType] == 0)
                continue;

            // Skip certain object types that only have one entry in game
            if ((_4FE384[currentType] & (1 << 4)) == 0 && _tabObjectCounts[currentType] == 1)
                continue;

            // Hide advanced object types as needed
            if ((self->var_856 & (1 << 0)) == 0 && (_4FE384[currentType] & (1 << 1)) != 0)
                continue;

            if (isEditorMode() && (_4FE384[currentType] & (1 << 3)) != 0)
                continue;

            if ((_4FE384[currentType] & (1 << 2)) != 0)
                continue;

            // Assign tab position
            _tabInformation[tabPos].index = static_cast<uint8_t>(currentType);
            _tabInformation[tabPos].row = currentRow;
            tabPos++;

            // Distribute tabs over two rows -- ensure there's capacity left in current row
            currentPos++;
            if (currentPos >= rowCapacity)
            {
                currentPos = 0;
                rowCapacity--;
                currentRow++;
            }
        }

        // Add a marker to denote the last tab
        _tabInformation[tabPos].index = 0xFF;

        const auto firstTabIndex = ObjectType(_tabInformation[0].index);
        sub_4731EE(self, firstTabIndex);
    }

    // 0x004731EE
    static void sub_4731EE(Window* self, ObjectType eax)
    {
        registers regs;
        regs.eax = static_cast<uint32_t>(eax);
        regs.esi = X86Pointer(self);
        call(0x004731EE, regs);
    }

    // 0x00472BBC
    static ObjectManager::ObjIndexPair sub_472BBC(Window* self)
    {
        const auto objects = ObjectManager::getAvailableObjects(static_cast<ObjectType>(self->currentTab));

        for (auto [index, object] : objects)
        {
            if (_50D144[index] & (1 << 0))
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        if (objects.size() > 0)
        {
            return { static_cast<int16_t>(objects[0].first), objects[0].second };
        }

        return { -1, ObjectManager::ObjectIndexEntry{} };
    }

    // 0x00473A95
    static void sub_473A95()
    {
        // Title::sub_473A95
        registers regs;
        regs.eax = 0;
        call(0x00473A95, regs);
    }

    // 0x00472A20
    Ui::Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::objectSelection);

        if (window != nullptr)
            return window;

        sub_473A95();

        window = WindowManager::createWindowCentred(WindowType::objectSelection, { kWindowSize }, 0, &_events);
        window->widgets = widgets;
        window->enabledWidgets = (1ULL << widx::closeButton) | (1ULL << widx::tabArea) | (1ULL << widx::advancedButton);
        window->initScrollWidgets();
        window->frameNo = 0;
        window->rowHover = -1;
        window->var_856 = isEditorMode() ? 0 : 1;

        initEvents();

        window->object = nullptr;

        assignTabPositions(window);
        sub_4731EE(window, ObjectType::region);
        ObjectManager::freeTemporaryObject();

        auto objIndex = sub_472BBC(window);
        if (objIndex.index != -1)
        {
            window->rowHover = objIndex.index;
            window->object = reinterpret_cast<std::byte*>(objIndex.object._header);
            ObjectManager::loadTemporaryObject(*objIndex.object._header);
        }

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_0C);

        return window;
    }

    struct TabDisplayInfo
    {
        string_id name;
        uint32_t image;
    };

    static const TabDisplayInfo _tabDisplayInfo[] = {
        { StringIds::object_interface_styles, ImageIds::tab_object_settings },
        { StringIds::object_sounds, ImageIds::tab_object_audio },
        { StringIds::object_currency, ImageIds::tab_object_currency },
        { StringIds::object_animation_effects, ImageIds::tab_object_smoke },
        { StringIds::object_cliffs, ImageIds::tab_object_cliff },
        { StringIds::object_water, ImageIds::tab_object_water },
        { StringIds::object_land, ImageIds::tab_object_landscape },
        { StringIds::object_town_names, ImageIds::tab_object_town_names },
        { StringIds::object_cargo, ImageIds::tab_object_cargo },
        { StringIds::object_walls, ImageIds::tab_object_walls },
        { StringIds::object_signals, ImageIds::tab_object_signals },
        { StringIds::object_level_crossing, ImageIds::tab_object_level_crossings },
        { StringIds::object_street_lights, ImageIds::tab_object_streetlights },
        { StringIds::object_tunnels, ImageIds::tab_object_tunnels },
        { StringIds::object_bridges, ImageIds::tab_object_bridges },
        { StringIds::object_track_stations, ImageIds::tab_object_track_stations },
        { StringIds::object_track_extras, ImageIds::tab_object_track_mods },
        { StringIds::object_tracks, ImageIds::tab_object_track },
        { StringIds::object_road_stations, ImageIds::tab_object_road_stations },
        { StringIds::object_road_extras, ImageIds::tab_object_road_mods },
        { StringIds::object_roads, ImageIds::tab_object_road },
        { StringIds::object_airports, ImageIds::tab_object_airports },
        { StringIds::object_docks, ImageIds::tab_object_docks },
        { StringIds::object_vehicles, ImageIds::tab_object_vehicles },
        { StringIds::object_trees, ImageIds::tab_object_trees },
        { StringIds::object_snow, ImageIds::tab_object_snow },
        { StringIds::object_climate, ImageIds::tab_object_climate },
        { StringIds::object_map_generation_data, ImageIds::tab_object_map },
        { StringIds::object_buildings, ImageIds::tab_object_buildings },
        { StringIds::object_scaffolding, ImageIds::tab_object_construction },
        { StringIds::object_industries, ImageIds::tab_object_industries },
        { StringIds::object_world_region, ImageIds::tab_object_world },
        { StringIds::object_company_owners, ImageIds::tab_object_companies },
        { StringIds::object_scenario_descriptions, ImageIds::tab_object_scenarios },
    };

    // 0x004733AC
    static void prepareDraw(Ui::Window& self)
    {
        self.activatedWidgets |= (1 << widx::objectImage);
        widgets[widx::closeButton].type = WidgetType::buttonWithImage;

        if (isEditorMode())
        {
            widgets[widx::closeButton].type = WidgetType::none;
        }

        self.activatedWidgets &= ~(1 << widx::advancedButton);

        if (self.var_856 & (1 << 0))
        {
            self.activatedWidgets |= (1 << widx::advancedButton);
        }

        auto args = FormatArguments();
        args.push(_tabDisplayInfo[self.currentTab].name);
    }

    static loco_global<uint16_t[40], 0x0112C1C5> _112C1C5;

    static constexpr uint8_t kRowOffsetX = 10;
    static constexpr uint8_t kRowOffsetY = 24;

    // 0x0047328D
    static void drawTabs(Window* self, Gfx::RenderTarget* rt)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto y = self->widgets[widx::panel].top + self->y - 26;
        auto x = self->x + 3;

        for (auto row = 1; row >= 0; row--)
        {
            auto xPos = x + (row * kRowOffsetX);
            auto yPos = y - (row * kRowOffsetY);
            for (auto index = 0; _tabInformation[index].index != 0xFF; index++)
            {
                if (_tabInformation[index].row != row)
                    continue;

                auto image = Gfx::recolour(ImageIds::tab, self->getColour(WindowColour::secondary).c());
                if (_tabInformation[index].index == self->currentTab)
                {
                    image = Gfx::recolour(ImageIds::selected_tab, self->getColour(WindowColour::secondary).c());
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    image = Gfx::recolour(_tabDisplayInfo[_tabInformation[index].index].image, Colour::mutedSeaGreen);
                    drawingCtx.drawImage(rt, xPos, yPos, image);
                }
                else
                {
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    image = Gfx::recolour(_tabDisplayInfo[_tabInformation[index].index].image, Colour::mutedSeaGreen);
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    image = Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33);
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    if (row < 1)
                    {
                        auto colour = Colours::getShade(self->getColour(WindowColour::secondary).c(), 7);
                        drawingCtx.drawRect(*rt, xPos, yPos + 26, 31, 1, colour, Drawing::RectFlags::none);
                    }
                }
                xPos += 31;
            }
        }
    }

    static constexpr Ui::Point kObjectPreviewOffset = { 56, 56 };
    static constexpr Ui::Size kObjectPreviewSize = { 112, 112 };
    static constexpr uint8_t kDescriptionRowHeight = 10;

    template<typename T>
    static void callDrawPreviewImage(Gfx::RenderTarget& rt, const Ui::Point& drawingOffset, Object* objectPtr)
    {
        auto object = reinterpret_cast<T*>(objectPtr);
        object->drawPreviewImage(rt, drawingOffset.x, drawingOffset.y);
    }

    // 0x00473579
    static void drawPreviewImage(ObjectHeader* header, Gfx::RenderTarget* rt, int16_t x, int16_t y, Object* objectPtr)
    {
        auto type = header->getType();

        // Clip the draw area to simplify image draw
        Ui::Point drawAreaPos = Ui::Point{ x, y } - kObjectPreviewOffset;
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(drawAreaPos.x, drawAreaPos.y, kObjectPreviewSize.width, kObjectPreviewSize.height));
        if (!clipped)
            return;

        switch (type)
        {
            case ObjectType::interfaceSkin:
                callDrawPreviewImage<InterfaceSkinObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::currency:
                callDrawPreviewImage<CurrencyObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::cliffEdge:
                callDrawPreviewImage<CliffEdgeObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::water:
                callDrawPreviewImage<WaterObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::land:
                callDrawPreviewImage<LandObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::wall:
                callDrawPreviewImage<WallObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackSignal:
                callDrawPreviewImage<TrainSignalObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::levelCrossing:
                callDrawPreviewImage<LevelCrossingObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::streetLight:
                callDrawPreviewImage<StreetLightObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::tunnel:
                callDrawPreviewImage<TunnelObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::bridge:
                callDrawPreviewImage<BridgeObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackStation:
                callDrawPreviewImage<TrainStationObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackExtra:
                callDrawPreviewImage<TrackExtraObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::track:
                callDrawPreviewImage<TrackObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::roadStation:
                callDrawPreviewImage<RoadStationObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::roadExtra:
                callDrawPreviewImage<RoadExtraObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::road:
                callDrawPreviewImage<RoadObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::airport:
                callDrawPreviewImage<AirportObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::dock:
                callDrawPreviewImage<DockObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::vehicle:
                callDrawPreviewImage<VehicleObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::tree:
                callDrawPreviewImage<TreeObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::snow:
                callDrawPreviewImage<SnowObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::hillShapes:
                callDrawPreviewImage<HillShapesObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::building:
                callDrawPreviewImage<BuildingObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::scaffolding:
                callDrawPreviewImage<ScaffoldingObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::industry:
                callDrawPreviewImage<IndustryObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::region:
                callDrawPreviewImage<RegionObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::competitor:
                callDrawPreviewImage<CompetitorObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            default:
                // null
                break;
        }
    }

    template<typename T>
    static void callDrawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, const int16_t width, Object* objectPtr)
    {
        auto object = reinterpret_cast<T*>(objectPtr);
        object->drawDescription(rt, x, y, width);
    }

    static void drawDescription(ObjectHeader* header, Window* self, Gfx::RenderTarget* rt, int16_t x, int16_t y, Object* objectPtr)
    {
        int16_t width = self->x + self->width - x;
        int16_t height = self->y + self->height - y;
        // Clip the draw area to simplify image draw
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(x, y, width, height));
        if (!clipped)
            return;

        switch (header->getType())
        {
            case ObjectType::levelCrossing:
                callDrawDescription<LevelCrossingObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::trackStation:
                callDrawDescription<TrainStationObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::roadStation:
                callDrawDescription<RoadStationObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::airport:
                callDrawDescription<AirportObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::dock:
                callDrawDescription<DockObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::vehicle:
                callDrawDescription<VehicleObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::building:
                callDrawDescription<BuildingObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::competitor:
                callDrawDescription<CompetitorObject>(*clipped, 0, 0, width, objectPtr);
                break;

            default:
                // null
                break;
        }
    }

    static void drawDatDetails(const ObjectManager::ObjectIndexEntry& indexEntry, Window* self, Gfx::RenderTarget* rt, int16_t x, int16_t y)
    {
        int16_t width = self->x + self->width - x;
        int16_t height = self->y + self->height - y;
        // Clip the draw area to simplify image draw
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(x, y, width, height));
        if (!clipped)
            return;

        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            strncpy(buffer, indexEntry._filename, strlen(indexEntry._filename) + 1);
            FormatArguments args{};
            args.push<string_id>(StringIds::buffer_1250);
            drawingCtx.drawStringLeft(*clipped, 18, height - kDescriptionRowHeight * 3 - 4, Colour::black, StringIds::object_selection_filename, &args);
        }
    }

    // 0x004733F5
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.fillRectInset(*rt, self.x, self.y + 20, self.x + self.width - 1, self.y + 20 + 60, self.getColour(WindowColour::primary), Drawing::RectInsetFlags::none);
        self.draw(rt);

        drawTabs(&self, rt);

        bool doDefault = true;
        if (self.object != nullptr)
        {
            auto objectPtr = self.object;
            auto var = ObjectManager::ObjectIndexEntry::read(&objectPtr)._header;
            if (var->getType() != ObjectType::townNames && var->getType() != ObjectType::climate)
            {
                doDefault = false;
            }
        }

        if (doDefault)
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 5);
            drawingCtx.drawRect(*rt, self.x + widget.left, self.y + widget.top, widget.width(), widget.height(), colour, Drawing::RectFlags::none);
        }
        else
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 0);
            drawingCtx.drawRect(*rt, self.x + widget.left + 1, self.y + widget.top + 1, widget.width() - 2, widget.height() - 2, colour, Drawing::RectFlags::none);
        }

        auto type = self.currentTab;

        auto args = FormatArguments();
        args.push(_112C1C5[type]);
        args.push(ObjectManager::getMaxObjects(static_cast<ObjectType>(type)));

        drawingCtx.drawStringLeft(*rt, self.x + 3, self.y + self.height - 12, Colour::black, 2038, &args);

        if (self.rowHover == -1)
            return;

        auto* temporaryObject = ObjectManager::getTemporaryObject();

        if (temporaryObject == nullptr)
            return;

        {
            auto objectPtr = self.object;

            drawPreviewImage(
                ObjectManager::ObjectIndexEntry::read(&objectPtr)._header,
                rt,
                widgets[widx::objectImage].midX() + 1 + self.x,
                widgets[widx::objectImage].midY() + 1 + self.y,
                temporaryObject);
        }

        auto x = self.widgets[widx::objectImage].midX() + self.x;
        auto y = self.widgets[widx::objectImage].bottom + 3 + self.y;
        auto width = self.width - self.widgets[widx::scrollview].right - 6;

        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer++ = ControlCodes::windowColour2;
            auto objectPtr = self.object;

            strncpy(buffer, ObjectManager::ObjectIndexEntry::read(&objectPtr)._name, 510);

            drawingCtx.drawStringCentredClipped(*rt, x, y, width, Colour::black, StringIds::buffer_2039);
        }

        {
            auto objectPtr = self.object;

            drawDescription(
                ObjectManager::ObjectIndexEntry::read(&objectPtr)._header,
                &self,
                rt,
                self.widgets[widx::scrollview].right + self.x + 4,
                y + kDescriptionRowHeight,
                temporaryObject);
        }

        {
            auto objectPtr = self.object;

            drawDatDetails(
                ObjectManager::ObjectIndexEntry::read(&objectPtr),
                &self,
                rt,
                self.widgets[widx::scrollview].right + self.x + 4,
                y + kDescriptionRowHeight);
        }
    }

    // 0x0047361D
    static void drawScroll(Window& self, Gfx::RenderTarget& rt, const uint32_t)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.clearSingle(rt, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

        if (ObjectManager::getNumInstalledObjects() == 0)
            return;

        int y = 0;
        auto objects = ObjectManager::getAvailableObjects(static_cast<ObjectType>(self.currentTab));
        for (auto [i, object] : objects)
        {
            Drawing::RectInsetFlags flags = Drawing::RectInsetFlags::colourLight | Drawing::RectInsetFlags::fillDarker | Drawing::RectInsetFlags::borderInset;
            drawingCtx.fillRectInset(rt, 2, y, 11, y + 10, self.getColour(WindowColour::secondary), flags);

            uint8_t textColour = ControlCodes::Colour::black;

            auto objectPtr = self.object;
            if (objectPtr != nullptr)
            {
                auto windowObjectName = ObjectManager::ObjectIndexEntry::read(&objectPtr)._name;
                if (object._name == windowObjectName)
                {
                    drawingCtx.fillRect(rt, 0, y, self.width, y + kRowHeight - 1, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                    textColour = ControlCodes::windowColour2;
                }
            }

            if (_50D144[i] & (1 << 0))
            {
                auto x = 2;
                drawingCtx.setCurrentFontSpriteBase(Font::m2);

                if (textColour != ControlCodes::windowColour2)
                {
                    drawingCtx.setCurrentFontSpriteBase(Font::m1);
                }

                auto checkColour = self.getColour(WindowColour::secondary).opaque();

                if (_50D144[i] & 0x1C)
                {
                    checkColour = checkColour.inset();
                }

                drawingCtx.drawString(rt, x, y, checkColour, _strCheckmark);
            }

            char buffer[512]{};
            buffer[0] = textColour;
            strncpy(&buffer[1], object._name, 510);
            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);

            drawingCtx.drawString(rt, 15, y, Colour::black, buffer);
            y += kRowHeight;
        }
    }

    // 0x00473A13
    bool tryCloseWindow()
    {
        return !(call(0x00473A13) & X86_FLAG_CARRY);
    }

    // 0x004737BA
    static void onMouseUp(Window& self, WidgetIndex_t w)
    {
        switch (w)
        {
            case widx::closeButton:
                tryCloseWindow();
                break;

            case widx::tabArea:
            {
                int clickedTab = -1;
                int y = widgets[widx::panel].top + self.y - 26;
                int x = self.x + 3;

                for (int row = 0; row < 2; row++)
                {
                    auto xPos = x + (row * kRowOffsetX);
                    auto yPos = y - (row * kRowOffsetY);

                    for (int i = 0; _tabInformation[i].index != 0xFF; i++)
                    {
                        if (_tabInformation[i].row != row)
                            continue;

                        if (_52334A >= xPos && _52334C >= yPos)
                        {
                            if (_52334A < xPos + 31 && yPos + 27 > _52334C)
                            {
                                clickedTab = _tabInformation[i].index;
                                break;
                            }
                        }

                        xPos += 31;
                    }
                }

                if (clickedTab != -1 && self.currentTab != clickedTab)
                {
                    sub_4731EE(&self, static_cast<ObjectType>(clickedTab));
                    self.rowHover = -1;
                    self.object = nullptr;
                    self.scrollAreas[0].contentWidth = 0;
                    ObjectManager::freeTemporaryObject();
                    auto objIndex = sub_472BBC(&self);

                    if (objIndex.index != -1)
                    {
                        self.rowHover = objIndex.index;
                        self.object = reinterpret_cast<std::byte*>(objIndex.object._header);

                        ObjectManager::loadTemporaryObject(*objIndex.object._header);
                    }

                    self.invalidate();
                }

                break;
            }

            case widx::advancedButton:
            {
                self.var_856 ^= 1;
                int currentTab = self.currentTab;
                assignTabPositions(&self);

                if ((self.var_856 & 1) == 0)
                {
                    if (_4FE384[currentTab] & 1 << 1)
                    {
                        currentTab = _tabInformation[0].index;
                    }
                }
                sub_4731EE(&self, static_cast<ObjectType>(currentTab));
                self.invalidate();

                break;
            }
        }
    }

    // 0x004738ED
    static void getScrollSize(Window& self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = _tabObjectCounts[self.currentTab] * kRowHeight;
    }

    // 0x00473900
    static std::optional<FormatArguments> tooltip(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_object_list);
        return args;
    }

    // 0x00472B54
    static ObjectManager::ObjIndexPair getObjectFromSelection(Window* self, int16_t& y)
    {
        const auto objects = ObjectManager::getAvailableObjects(static_cast<ObjectType>(self->currentTab));

        for (auto [index, object] : objects)
        {
            y -= kRowHeight;
            if (y < 0)
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        return { -1, ObjectManager::ObjectIndexEntry{} };
    }

    // 0x0047390A
    static void onScrollMouseOver(Ui::Window& self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(&self, y);

        if (objIndex.index == self.rowHover || objIndex.index == -1)
            return;

        self.rowHover = objIndex.index;
        self.object = reinterpret_cast<std::byte*>(objIndex.object._header);
        ObjectManager::freeTemporaryObject();

        if (objIndex.index != -1)
        {
            ObjectManager::loadTemporaryObject(*objIndex.object._header);
        }

        self.invalidate();
    }

    // 0x00473D1D
    static bool windowEditorObjectSelectionSelectObject(uint16_t bx, void* ebp)
    {
        registers regs;
        regs.bx = bx;
        regs.ebp = X86Pointer(ebp);

        return call(0x00473D1D, regs) & X86_FLAG_CARRY;
    }

    // 0x00473948
    static void onScrollMouseDown(Ui::Window& self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(&self, y);
        auto index = objIndex.index;
        auto object = objIndex.object._header;

        if (index == -1)
            return;

        self.invalidate();
        Audio::playSound(Audio::SoundId::clickDown, Input::getMouseLocation().x);

        auto type = objIndex.object._header->getType();

        if (ObjectManager::getMaxObjects(type) == 1)
        {
            if (!(_50D144[index] & (1 << 0)))
            {
                auto [oldIndex, oldObject] = ObjectManager::getActiveObject(type, _50D144);

                if (oldIndex != -1)
                {
                    windowEditorObjectSelectionSelectObject(6, oldObject._header);
                }
            }
        }

        auto bx = 0;

        if (!(_50D144[index] & (1 << 0)))
        {
            bx |= (1 << 0);
        }

        bx |= 6;

        if (!windowEditorObjectSelectionSelectObject(bx, object))
            return;

        auto errorTitle = StringIds::error_unable_to_select_object;

        if (bx & (1 << 0))
        {
            errorTitle = StringIds::error_unable_to_deselect_object;
        }

        Ui::Windows::Error::open(errorTitle, GameCommands::getErrorText());
    }

    // 0x00474821
    static void unloadUnselectedObjects()
    {
        for (ObjectType type = ObjectType::interfaceSkin; enumValue(type) <= enumValue(ObjectType::scenarioText); type = static_cast<ObjectType>(enumValue(type) + 1))
        {
            auto objects = ObjectManager::getAvailableObjects(type);
            for (auto [i, object] : objects)
            {
                if (!(_50D144[i] & (1 << 0)))
                {
                    ObjectManager::unload(*object._header);
                }
            }
        }
    }

    // 0x00474874
    static void editorLoadSelectedObjects()
    {
        for (ObjectType type = ObjectType::interfaceSkin; enumValue(type) <= enumValue(ObjectType::scenarioText); type = static_cast<ObjectType>(enumValue(type) + 1))
        {
            auto objects = ObjectManager::getAvailableObjects(type);
            for (auto [i, object] : objects)
            {
                if (_50D144[i] & (1 << 0))
                {
                    if (!ObjectManager::findObjectHandle(*object._header))
                    {
                        ObjectManager::load(*object._header);
                    }
                }
            }
        }
    }

    // 0x00473B91
    static void editorObjectFlagsFree0()
    {
        call(0x00473B91); // editor_object_flags_free_0
    }

    // 0x004739DD
    static void onClose(Window& self)
    {
        unloadUnselectedObjects();
        editorLoadSelectedObjects();
        ObjectManager::reloadAll();
        ObjectManager::freeTemporaryObject();

        if (!isEditorMode())
        {
            // Make new selection available in-game.
            ObjectManager::updateYearly2();
            ObjectManager::sub_4748FA();
            Gfx::loadCurrency();
            Gfx::loadPalette();
            Gfx::invalidateScreen();
        }
        else
        {
            editorObjectFlagsFree0();
        }
    }

    // 0x00473A04
    static void onUpdate(Window& self)
    {
        WindowManager::invalidateWidget(WindowType::objectSelection, self.number, widx::objectImage);
    }

    static void initEvents()
    {
        _events.onClose = onClose;
        _events.onMouseUp = onMouseUp;
        _events.onUpdate = onUpdate;
        _events.getScrollSize = getScrollSize;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
    }
}
