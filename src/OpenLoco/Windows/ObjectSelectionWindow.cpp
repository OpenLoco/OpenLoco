#include "../Audio/Audio.h"
#include "../Console.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/AirportObject.h"
#include "../Objects/BridgeObject.h"
#include "../Objects/BuildingObject.h"
#include "../Objects/CargoObject.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/CurrencyObject.h"
#include "../Objects/DockObject.h"
#include "../Objects/HillShapesObject.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/LevelCrossingObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RegionObject.h"
#include "../Objects/RoadExtraObject.h"
#include "../Objects/RoadObject.h"
#include "../Objects/RoadStationObject.h"
#include "../Objects/RockObject.h"
#include "../Objects/ScaffoldingObject.h"
#include "../Objects/SnowObject.h"
#include "../Objects/StreetLightObject.h"
#include "../Objects/TrackExtraObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/TrainSignalObject.h"
#include "../Objects/TrainStationObject.h"
#include "../Objects/TreeObject.h"
#include "../Objects/TunnelObject.h"
#include "../Objects/VehicleObject.h"
#include "../Objects/WallObject.h"
#include "../Objects/WaterObject.h"
#include "../Ui/WindowManager.h"
#include "../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ObjectSelectionWindow
{
    static constexpr int rowHeight = 12;
    static Gfx::ui_size_t windowSize = { 600, 398 };

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
    static loco_global<uint16_t, 0x0052622E> _52622E; // Tick related

    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<uint16_t[33], 0x00112C181> _tabObjectCounts;
    static loco_global<tabPosition[36], 0x0112C21C> _tabInformation;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;

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

    widget_t widgets[] = {
        makeWidget({ 0, 0 }, { 600, 398 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 598, 13 }, widget_type::caption_25, 0, StringIds::title_object_selection),
        makeWidget({ 585, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 65 }, { 600, 333 }, widget_type::panel, 1),
        makeWidget({ 3, 15 }, { 589, 50 }, widget_type::wt_5, 1),
        makeWidget({ 470, 20 }, { 122, 12 }, widget_type::wt_11, 0, StringIds::object_selection_advanced, StringIds::object_selection_advanced_tooltip),
        makeWidget({ 4, 68 }, { 288, 317 }, widget_type::scrollview, 1, vertical),
        makeWidget({ 391, 68 }, { 114, 114 }, widget_type::wt_9, 1),
        widgetEnd(),
    };

    static window_event_list _events;

    // 0x00473154
    static void sub_473154(window* self)
    {
        registers regs;
        regs.esi = (uintptr_t)self;
        call(0x00473154, regs);
    }

    // 0x004731EE
    static void sub_4731EE(window* self, object_type eax)
    {
        registers regs;
        regs.eax = static_cast<uint32_t>(eax);
        regs.esi = (uintptr_t)self;
        call(0x004731EE, regs);
    }

    // 0x00472BBC
    static ObjectManager::ObjIndexPair sub_472BBC(window* self)
    {
        const auto objects = ObjectManager::getAvailableObjects(static_cast<object_type>(self->current_tab));

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

        return { -1, ObjectManager::object_index_entry{} };
    }

    // 0x00473A95
    static void sub_473A95()
    {
        registers regs;
        regs.eax = 0;
        call(0x00473A95, regs);
    }

    // 0x00472A20
    Ui::window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::objectSelection);

        if (window != nullptr)
            return window;

        sub_473A95();

        window = WindowManager::createWindowCentred(WindowType::objectSelection, { windowSize }, 0, &_events);
        window->widgets = widgets;
        window->enabled_widgets = (1ULL << widx::closeButton) | (1ULL << widx::tabArea) | (1ULL << widx::advancedButton);
        window->initScrollWidgets();
        window->frame_no = 0;
        window->row_hover = -1;
        window->var_856 = 0;

        initEvents();

        window->object = nullptr;

        sub_473154(window);
        sub_4731EE(window, object_type::region);
        ObjectManager::freeScenarioText();

        auto objIndex = sub_472BBC(window);
        if (objIndex.index != -1)
        {
            window->row_hover = objIndex.index;
            window->object = reinterpret_cast<std::byte*>(objIndex.object._header);
            ObjectManager::getScenarioText(*objIndex.object._header);
        }

        auto skin = ObjectManager::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0C;

        return window;
    }

    // 0x004733AC
    static void prepareDraw(Ui::window* self)
    {
        self->activated_widgets |= (1 << widx::objectImage);
        widgets[widx::closeButton].type = widget_type::wt_9;

        if (isEditorMode())
        {
            widgets[widx::closeButton].type = widget_type::none;
        }

        self->activated_widgets &= ~(1 << widx::advancedButton);

        if (self->var_856 & (1 << 0))
        {
            self->activated_widgets |= (1 << widx::advancedButton);
        }

        static string_id tabName[] = {
            StringIds::object_interface_styles,
            StringIds::object_sounds,
            StringIds::object_currency,
            StringIds::object_animation_effects,
            StringIds::object_cliffs,
            StringIds::object_water,
            StringIds::object_land,
            StringIds::object_town_names,
            StringIds::object_cargo,
            StringIds::object_walls,
            StringIds::object_signals,
            StringIds::object_level_crossing,
            StringIds::object_street_lights,
            StringIds::object_tunnels,
            StringIds::object_bridges,
            StringIds::object_track_stations,
            StringIds::object_track_extras,
            StringIds::object_tracks,
            StringIds::object_road_stations,
            StringIds::object_road_extras,
            StringIds::object_roads,
            StringIds::object_airports,
            StringIds::object_docks,
            StringIds::object_vehicles,
            StringIds::object_trees,
            StringIds::object_snow,
            StringIds::object_climate,
            StringIds::object_map_generation_data,
            StringIds::object_buildings,
            StringIds::object_scaffolding,
            StringIds::object_industries,
            StringIds::object_world_region,
            StringIds::object_company_owners,
            StringIds::object_scenario_descriptions,
        };

        auto args = FormatArguments();
        args.push(tabName[self->current_tab]);
    }

    static loco_global<uint16_t[40], 0x0112C1C5> _112C1C5;

    static uint32_t tabImages[] = {
        ImageIds::tab_object_settings,
        ImageIds::tab_object_audio,
        ImageIds::tab_object_currency,
        ImageIds::tab_object_smoke,
        ImageIds::tab_object_cliff,
        ImageIds::tab_object_water,
        ImageIds::tab_object_landscape,
        ImageIds::tab_object_town_names,
        ImageIds::tab_object_cargo,
        ImageIds::tab_object_walls,
        ImageIds::tab_object_signals,
        ImageIds::tab_object_level_crossings,
        ImageIds::tab_object_streetlights,
        ImageIds::tab_object_tunnels,
        ImageIds::tab_object_bridges,
        ImageIds::tab_object_track_stations,
        ImageIds::tab_object_track_mods,
        ImageIds::tab_object_track,
        ImageIds::tab_object_road_stations,
        ImageIds::tab_object_road_mods,
        ImageIds::tab_object_road,
        ImageIds::tab_object_airports,
        ImageIds::tab_object_docks,
        ImageIds::tab_object_vehicles,
        ImageIds::tab_object_trees,
        ImageIds::tab_object_snow,
        ImageIds::tab_object_climate,
        ImageIds::tab_object_map,
        ImageIds::tab_object_buildings,
        ImageIds::tab_object_construction,
        ImageIds::tab_object_industries,
        ImageIds::tab_object_world,
        ImageIds::tab_object_companies,
        ImageIds::tab_object_scenarios,
    };

    static const uint8_t rowOffsetX = 10;
    static const uint8_t rowOffsetY = 24;

    // 0x0047328D
    static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        auto y = self->widgets[widx::panel].top + self->y - 26;
        auto x = self->x + 3;

        for (auto row = 1; row >= 0; row--)
        {
            auto xPos = x + (row * rowOffsetX);
            auto yPos = y - (row * rowOffsetY);
            for (auto index = 0; _tabInformation[index].index != 0xFF; index++)
            {
                if (_tabInformation[index].row != row)
                    continue;

                auto image = Gfx::recolour(ImageIds::tab, self->colours[1]);
                if (_tabInformation[index].index == self->current_tab)
                {
                    image = Gfx::recolour(ImageIds::selected_tab, self->colours[1]);
                    Gfx::drawImage(dpi, xPos, yPos, image);

                    image = Gfx::recolour(tabImages[_tabInformation[index].index], Colour::saturated_green);
                    Gfx::drawImage(dpi, xPos, yPos, image);
                }
                else
                {
                    Gfx::drawImage(dpi, xPos, yPos, image);

                    image = Gfx::recolour(tabImages[_tabInformation[index].index], Colour::saturated_green);
                    Gfx::drawImage(dpi, xPos, yPos, image);

                    image = Gfx::recolourTranslucent(ImageIds::tab, 51);
                    Gfx::drawImage(dpi, xPos, yPos, image);

                    if (row < 1)
                    {
                        auto colour = Colour::getShade(self->colours[1], 7);
                        Gfx::drawRect(dpi, xPos, yPos + 26, 31, 1, colour);
                    }
                }
                xPos += 31;
            }
        }
    }

    // 0x004B7733
    static void drawVehicle(Gfx::drawpixelinfo_t* dpi, vehicle_object* vehicleObject, uint8_t eax, uint8_t esi, Gfx::point_t offset)
    {
        registers regs;
        regs.cx = offset.x;
        regs.dx = offset.y;
        regs.eax = eax;
        regs.esi = esi;
        regs.bl = Colour::saturated_green;
        regs.bh = 2;
        regs.ebp = (uintptr_t)vehicleObject;
        regs.edi = (uintptr_t)dpi;
        call(0x4B7733, regs);
    }

    static const xy32 objectPreviewOffset = { 56, 56 };
    static const Gfx::ui_size_t objectPreviewSize = { 112, 112 };
    static const uint8_t descriptionRowHeight = 10;

    // 0x00473579
    static void drawPreviewImage(ObjectManager::header* header, Gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, void* objectPtr)
    {
        auto type = header->getType();
        switch (type)
        {
            case object_type::sound:
            case object_type::steam:
            case object_type::town_names:
            case object_type::cargo:
            case object_type::climate:
            case object_type::scenario_text:
                // null
                break;

            case object_type::interface_skin:
            {
                auto object = reinterpret_cast<interface_skin_object*>(objectPtr);
                auto image = Gfx::recolour(object->img, Colour::saturated_green);

                Gfx::drawImage(dpi, x - 32, y - 32, image);
                break;
            }

            case object_type::currency:
            {
                auto object = reinterpret_cast<currency_object*>(objectPtr);
                auto currencyIndex = (object->object_icon + 3);

                auto defaultElement = Gfx::getG1Element(ImageIds::currency_symbol);
                auto backupElement = *defaultElement;
                auto currencyElement = Gfx::getG1Element(currencyIndex);

                *defaultElement = *currencyElement;

                auto defaultWidth = _characterWidths[Font::large + 131];
                _characterWidths[Font::large + 131] = currencyElement->width + 1;

                Gfx::drawStringCentred(*dpi, x, y - 9, Colour::black, StringIds::object_currency_big_font);

                _characterWidths[Font::large + 131] = defaultWidth;
                *defaultElement = backupElement;

                break;
            }

            case object_type::rock:
            {
                auto object = reinterpret_cast<rock_object*>(objectPtr);
                auto image = object->image;

                Gfx::drawImage(dpi, x - 30, y, image);

                image += 16;
                Gfx::drawImage(dpi, x - 30, y, image);
                break;
            }

            case object_type::water:
            {
                auto object = ObjectManager::get<water_object>();
                auto image = object->image;
                image += 0x61000023;
                Gfx::drawImage(dpi, x, y, image);

                image = object->image;
                image += 30;
                Gfx::drawImage(dpi, x, y, image);
                break;
            }

            case object_type::land:
            {
                auto object = reinterpret_cast<land_object*>(objectPtr);
                uint32_t imageId = object->image + (object->var_03 - 1) * object->var_0E;
                Gfx::drawImage(dpi, x, y, imageId);
                break;
            }

            case object_type::wall:
            {
                auto object = reinterpret_cast<wall_object*>(objectPtr);
                Gfx::drawpixelinfo_t* clipped = nullptr;

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                if (Gfx::clipDrawpixelinfo(&clipped, dpi, pos, objectPreviewSize))
                {
                    auto image = object->sprite;
                    image = Gfx::recolour(image, Colour::salmon_pink);
                    if (object->flags & (1 << 6))
                    {
                        image |= (1 << 31) | (1 << 28);
                    }

                    Gfx::drawImage(clipped, 70, 72 + (object->var_08 * 2), image);
                    if (object->flags & (1 << 1))
                    {
                        Gfx::drawImage(clipped, 70, 72 + (object->var_08 * 2), Gfx::recolourTranslucent(object->sprite + 6, 140));
                    }
                    else
                    {
                        if (object->flags & (1 << 4))
                        {
                            Gfx::drawImage(clipped, 70, 72 + (object->var_08 * 2), image + 1);
                        }
                    }
                }
                break;
            }

            case object_type::track_signal:
            {
                auto object = reinterpret_cast<train_signal_object*>(objectPtr);

                auto image = object->image;
                auto frames = signalFrames[(((object->num_frames + 2) / 3) - 2)];
                auto frameCount = std::size(frames) - 1;
                auto animationFrame = frameCount & (scenarioTicks() >> object->animationSpeed);

                auto frameIndex = frames[animationFrame];
                frameIndex *= 8;
                image += frameIndex;

                Gfx::drawImage(dpi, x, y + 15, image);

                break;
            }

            case object_type::level_crossing:
            {
                auto object = reinterpret_cast<level_crossing_object*>(objectPtr);

                auto image = (object->closedFrames + 1) * 8;
                auto frameCount = (object->closingFrames - 1);
                auto animationFrame = frameCount & (scenarioTicks() >> object->animationSpeed);
                auto frameIndex = 8 * animationFrame;
                image += frameIndex;
                image += object->image;

                Gfx::drawImage(dpi, x, y, image);
                Gfx::drawImage(dpi, x, y, image + 1);
                Gfx::drawImage(dpi, x, y, image + 2);
                Gfx::drawImage(dpi, x, y, image + 3);

                break;
            }

            case object_type::street_light:
            {
                auto object = reinterpret_cast<street_light_object*>(objectPtr);
                x -= 20;
                y -= -1;
                for (auto i = 0; i < 3; i++)
                {
                    auto image = (i * 4) + object->image;
                    Gfx::drawImage(dpi, x - 14, y, image + 2);
                    Gfx::drawImage(dpi, x, y - 7, image);
                    x += 20;
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::tunnel:
            {
                auto object = reinterpret_cast<tunnel_object*>(objectPtr);

                auto image = object->image;

                Gfx::drawImage(dpi, x - 16, y + 15, image);
                Gfx::drawImage(dpi, x - 16, y + 15, image + 1);

                break;
            }

            case object_type::bridge:
            {
                auto object = reinterpret_cast<bridge_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x - 21, y - 9, image);
                break;
            }

            case object_type::track_station:
            {
                auto object = reinterpret_cast<train_station_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x - 34, y - 34, image);

                auto colour = 59;
                if (!(object->flags & TrainStationFlags::recolourable))
                {
                    colour = 46;
                }

                image = Gfx::recolourTranslucent(object->image + 1, colour);

                Gfx::drawImage(dpi, x - 34, y - 34, image);
                break;
            }

            case object_type::track_extra:
            {
                auto object = reinterpret_cast<track_extra_object*>(objectPtr);
                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                if (object->is_overhead == 0)
                {
                    Gfx::drawImage(dpi, x, y, image);
                }
                else
                {
                    Gfx::drawImage(dpi, x, y, image);
                    Gfx::drawImage(dpi, x, y, image + 97);
                    Gfx::drawImage(dpi, x, y, image + 96);
                }

                break;
            }

            case object_type::track:
            {
                auto object = reinterpret_cast<track_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x, y, image + 18);
                Gfx::drawImage(dpi, x, y, image + 20);
                Gfx::drawImage(dpi, x, y, image + 22);

                break;
            }

            case object_type::road_station:
            {
                auto object = reinterpret_cast<road_station_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x - 34, y - 34, image);

                auto colour = 59;
                if (!(object->flags & RoadStationFlags::recolourable))
                {
                    colour = 46;
                }

                image = Gfx::recolourTranslucent(object->image + 1, colour);

                Gfx::drawImage(dpi, x - 34, y - 34, image);
                break;
            }

            case object_type::road_extra:
            {
                auto object = reinterpret_cast<road_extra_object*>(objectPtr);
                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x, y, image + 36);
                Gfx::drawImage(dpi, x, y, image + 37);
                Gfx::drawImage(dpi, x, y, image);
                Gfx::drawImage(dpi, x, y, image + 33);
                Gfx::drawImage(dpi, x, y, image + 32);

                break;
            }

            case object_type::road:
            {
                auto object = reinterpret_cast<road_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);
                if (object->var_24 == 1)
                {
                    Gfx::drawImage(dpi, x, y, image + 34);
                    Gfx::drawImage(dpi, x, y, image + 36);
                    Gfx::drawImage(dpi, x, y, image + 38);
                }
                else
                {
                    Gfx::drawImage(dpi, x, y, image + 34);
                }

                break;
            }

            case object_type::airport:
            {
                auto object = reinterpret_cast<airport_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x - 34, y - 34, image);

                break;
            }

            case object_type::dock:
            {
                auto object = reinterpret_cast<dock_object*>(objectPtr);

                auto image = Gfx::recolour(object->image, Colour::salmon_pink);

                Gfx::drawImage(dpi, x - 34, y - 34, image);

                break;
            }

            case object_type::vehicle:
            {
                auto object = reinterpret_cast<vehicle_object*>(objectPtr);
                Gfx::drawpixelinfo_t* clipped = nullptr;

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                if (Gfx::clipDrawpixelinfo(&clipped, dpi, pos, objectPreviewSize))
                {
                    uint8_t unk1 = _52622E & 0x3F;
                    uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;
                    drawVehicle(clipped, object, unk1, unk2, { static_cast<int16_t>(objectPreviewOffset.x), 75 });
                }

                break;
            }

            case object_type::tree:
            {
                auto object = reinterpret_cast<tree_object*>(objectPtr);
                Gfx::drawpixelinfo_t* clipped = nullptr;

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                if (Gfx::clipDrawpixelinfo(&clipped, dpi, pos, objectPreviewSize))
                {
                    uint32_t image = treeGrowth[object->growth] * object->num_rotations;
                    auto rotation = (object->num_rotations - 1) & 2;
                    image += rotation;
                    image += object->sprites[object->season_state];

                    auto colourOptions = object->colours;
                    if (colourOptions != 0)
                    {

                        colour_t colour = Utility::bitScanReverse(colourOptions);

                        if (colour == 0xFF)
                        {
                            colour = 0;
                        }

                        image = Gfx::recolour(image, colour);
                    }

                    xy32 treePos = { objectPreviewOffset.x, 104 };
                    if (object->var_08 & (1 << 0))
                    {
                        auto snowImage = treeGrowth[object->growth] * object->num_rotations;
                        snowImage += rotation;
                        snowImage += object->sprites[object->season_state + 6];

                        if (colourOptions != 0)
                        {

                            colour_t colour = Utility::bitScanReverse(colourOptions);

                            if (colour == 0xFF)
                            {
                                colour = 0;
                            }

                            snowImage = Gfx::recolour(snowImage, colour);
                        }
                        treePos.x = 84;
                        Gfx::drawImage(clipped, treePos.x, treePos.y, snowImage);
                        treePos.x = 28;
                    }
                    Gfx::drawImage(clipped, treePos.x, treePos.y, image);
                }

                break;
            }

            case object_type::snow:
            {
                auto object = reinterpret_cast<snow_object*>(objectPtr);
                auto image = object->image;

                Gfx::drawImage(dpi, x, y, image);
                break;
            }

            case object_type::hill_shapes:
            {
                auto object = reinterpret_cast<hill_shapes_object*>(objectPtr);
                auto image = object->image + object->hillHeightMapCount + object->mountainHeightMapCount;

                Gfx::drawImage(dpi, x, y, image);

                break;
            }

            case object_type::building:
            {
                auto object = reinterpret_cast<building_object*>(objectPtr);
                Gfx::drawpixelinfo_t* clipped = nullptr;

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                if (Gfx::clipDrawpixelinfo(&clipped, dpi, pos, objectPreviewSize))
                {
                    colour_t colour = Utility::bitScanReverse(object->colours);

                    if (colour == 0xFF)
                    {
                        colour = 0;
                    }

                    object->drawBuilding(clipped, 1, objectPreviewOffset.x, 96, colour);
                }

                break;
            }

            case object_type::scaffolding:
            {
                auto object = reinterpret_cast<building_object*>(objectPtr);
                Gfx::drawpixelinfo_t* clipped = nullptr;

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                if (Gfx::clipDrawpixelinfo(&clipped, dpi, pos, objectPreviewSize))
                {
                    auto image = Gfx::recolour(object->name, Colour::dark_olive_green);

                    Gfx::drawImage(dpi, objectPreviewOffset.x, 79, image + 24);
                    Gfx::drawImage(dpi, objectPreviewOffset.x, 79, image + 25);
                    Gfx::drawImage(dpi, objectPreviewOffset.x, 79, image + 27);
                }

                break;
            }

            case object_type::industry:
            {
                auto object = reinterpret_cast<industry_object*>(objectPtr);
                Gfx::drawpixelinfo_t* clipped = nullptr;

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                if (Gfx::clipDrawpixelinfo(&clipped, dpi, pos, objectPreviewSize))
                {
                    object->drawIndustry(clipped, objectPreviewOffset.x, 96);
                }

                break;
            }

            case object_type::region:
            {
                auto object = reinterpret_cast<region_object*>(objectPtr);
                auto image = object->image;

                Gfx::drawImage(dpi, x, y, image);
                break;
            }

            case object_type::competitor:
            {
                auto object = reinterpret_cast<competitor_object*>(objectPtr);

                xy32 pos = { x - objectPreviewOffset.x, y - objectPreviewOffset.y };
                Gfx::drawRect(dpi, pos.x, pos.y, objectPreviewSize.width, objectPreviewSize.height, Colour::inset(Colour::dark_brown));

                auto image = Gfx::recolour(object->images[0], Colour::inset(Colour::dark_brown));
                Gfx::drawImage(dpi, x - 32, y - 32, image);

                break;
            }
        }
    }

    static void drawDescription(ObjectManager::header* header, window* self, Gfx::drawpixelinfo_t* dpi, int16_t x, int16_t y, void* objectPtr)
    {
        switch (header->getType())
        {
            case object_type::interface_skin:
            case object_type::sound:
            case object_type::currency:
            case object_type::steam:
            case object_type::rock:
            case object_type::water:
            case object_type::land:
            case object_type::town_names:
            case object_type::cargo:
            case object_type::wall:
            case object_type::track_signal:
            case object_type::street_light:
            case object_type::tunnel:
            case object_type::bridge:
            case object_type::track_extra:
            case object_type::track:
            case object_type::road_extra:
            case object_type::road:
            case object_type::tree:
            case object_type::snow:
            case object_type::climate:
            case object_type::hill_shapes:
            case object_type::scaffolding:
            case object_type::industry:
            case object_type::region:
            case object_type::scenario_text:
                // null
                break;

            case object_type::level_crossing:
            {
                auto object = reinterpret_cast<level_crossing_object*>(objectPtr);

                if (object->designedYear != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_designed, &object->designedYear);
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::track_station:
            {
                auto object = reinterpret_cast<train_station_object*>(objectPtr);

                if (object->designed_year != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_designed, &object->designed_year);
                    y += descriptionRowHeight;
                }

                if (object->obsolete_year != 0xFFFF)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_obsolete, &object->obsolete_year);
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::road_station:
            {
                auto object = reinterpret_cast<road_station_object*>(objectPtr);

                if (object->designed_year != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_designed, &object->designed_year);
                    y += descriptionRowHeight;
                }

                if (object->obsolete_year != 0xFFFF)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_obsolete, &object->obsolete_year);
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::airport:
            {
                auto object = reinterpret_cast<airport_object*>(objectPtr);

                if (object->designed_year != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_designed, &object->designed_year);
                    y += descriptionRowHeight;
                }

                if (object->obsolete_year != 0xFFFF)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_obsolete, &object->obsolete_year);
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::dock:
            {
                auto object = reinterpret_cast<dock_object*>(objectPtr);

                if (object->designed_year != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_designed, &object->designed_year);
                    y += descriptionRowHeight;
                }

                if (object->obsolete_year != 0xFFFF)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_obsolete, &object->obsolete_year);
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::vehicle:
            {
                auto object = reinterpret_cast<vehicle_object*>(objectPtr);

                if (object->designed != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_designed, &object->designed);
                    y += descriptionRowHeight;
                }

                if (object->obsolete != 0xFFFF)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_obsolete, &object->obsolete);
                    y += descriptionRowHeight;
                }

                if (object->power != 0 && (object->mode == TransportMode::road || object->mode == TransportMode::rail))
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_power, &object->power);
                    y += descriptionRowHeight;
                }

                Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_weight, &object->weight);
                y += descriptionRowHeight;

                Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::object_selection_max_speed, &object->speed);

                auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));

                object->getCargoString(buffer);

                auto width = self->width + self->x - x + 4;
                Gfx::drawString_495224(*dpi, x, y, width, Colour::black, StringIds::buffer_1250);

                break;
            }

            case object_type::building:
            {
                auto object = reinterpret_cast<building_object*>(objectPtr);

                if (object->designedYear != 0)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::building_earliest_construction_year, &object->designedYear);
                    y += descriptionRowHeight;
                }

                if (object->obsoleteYear != 0xFFFF)
                {
                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::building_latest_construction_year, &object->obsoleteYear);
                    y += descriptionRowHeight;
                }

                break;
            }

            case object_type::competitor:
            {
                auto object = reinterpret_cast<competitor_object*>(objectPtr);
                {
                    auto args = FormatArguments();
                    args.push<uint16_t>(object->intelligence);
                    args.push(aiRatingToLevel(object->intelligence));

                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::company_details_intelligence, &args);
                    y += descriptionRowHeight;
                }
                {
                    auto args = FormatArguments();
                    args.push<uint16_t>(object->aggressiveness);
                    args.push(aiRatingToLevel(object->aggressiveness));

                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::company_details_aggressiveness, &args);
                    y += descriptionRowHeight;
                }
                {
                    auto args = FormatArguments();
                    args.push<uint16_t>(object->competitiveness);
                    args.push(aiRatingToLevel(object->competitiveness));

                    Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::company_details_competitiveness, &args);
                }

                break;
            }
        }
    }

    // 0x004733F5
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        Gfx::fillRectInset(dpi, self->x, self->y + 20, self->x + self->width - 1, self->y + 20 + 60, self->colours[0], 0);
        self->draw(dpi);

        drawTabs(self, dpi);

        bool doDefault = true;
        if (self->object != nullptr)
        {
            auto objectPtr = self->object;
            auto var = ObjectManager::object_index_entry::read(&objectPtr)._header;
            if (var->getType() != object_type::town_names && var->getType() != object_type::climate)
            {
                doDefault = false;
            }
        }

        if (doDefault)
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colour::getShade(self->colours[1], 5);
            Gfx::drawRect(dpi, self->x + widget.left, self->y + widget.top, widget.width(), widget.height(), colour);
        }
        else
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colour::getShade(self->colours[1], 0);
            Gfx::drawRect(dpi, self->x + widget.left + 1, self->y + widget.top + 1, widget.width() - 2, widget.height() - 2, colour);
        }

        auto type = self->current_tab;

        auto args = FormatArguments();
        args.push(_112C1C5[type]);
        args.push(ObjectManager::getMaxObjects(static_cast<object_type>(type)));

        Gfx::drawString_494B3F(*dpi, self->x + 3, self->y + self->height - 12, 0, 2038, &args);

        if (self->row_hover == -1)
            return;

        loco_global<void*, 0x0050D15C> _50D15C;

        if (_50D15C == reinterpret_cast<void*>(-1))
            return;

        {
            auto objectPtr = self->object;

            drawPreviewImage(
                ObjectManager::object_index_entry::read(&objectPtr)._header,
                dpi,
                widgets[widx::objectImage].mid_x() + 1 + self->x,
                widgets[widx::objectImage].mid_y() + 1 + self->y,
                _50D15C);
        }

        auto x = self->widgets[widx::objectImage].mid_x() + self->x;
        auto y = self->widgets[widx::objectImage].bottom + 3 + self->y;
        auto width = self->width - self->widgets[widx::scrollview].right - 6;

        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer++ = ControlCodes::window_colour_2;
            auto objectPtr = self->object;

            strncpy(buffer, ObjectManager::object_index_entry::read(&objectPtr)._name, 512);

            Gfx::drawStringCentredClipped(*dpi, x, y, width, Colour::black, StringIds::buffer_2039);
        }

        {
            auto objectPtr = self->object;

            drawDescription(
                ObjectManager::object_index_entry::read(&objectPtr)._header,
                self,
                dpi,
                self->widgets[widx::scrollview].right + self->x + 4,
                y + descriptionRowHeight,
                _50D15C);
        }
    }

    // 0x0047361D
    static void drawScroll(window* self, Gfx::drawpixelinfo_t* dpi, uint32_t)
    {
        Gfx::clearSingle(*dpi, Colour::getShade(self->colours[1], 4));

        if (ObjectManager::getNumInstalledObjects() == 0)
            return;

        int y = 0;
        auto objects = ObjectManager::getAvailableObjects(static_cast<object_type>(self->current_tab));
        for (auto [i, object] : objects)
        {
            uint8_t flags = (1 << 7) | (1 << 6) | (1 << 5);
            Gfx::fillRectInset(dpi, 2, y, 11, y + 10, self->colours[1], flags);

            uint8_t textColour = ControlCodes::colour_black;

            auto objectPtr = self->object;
            if (objectPtr != nullptr)
            {
                auto windowObjectName = ObjectManager::object_index_entry::read(&objectPtr)._name;
                if (object._name == windowObjectName)
                {
                    Gfx::fillRect(dpi, 0, y, self->width, y + rowHeight - 1, (1 << 25) | PaletteIndex::index_30);
                    textColour = ControlCodes::window_colour_2;
                }
            }

            if (_50D144[i] & (1 << 0))
            {
                auto x = 2;
                _currentFontSpriteBase = Font::m2;

                if (textColour != ControlCodes::window_colour_2)
                {
                    _currentFontSpriteBase = Font::m1;
                }

                auto checkColour = Colour::opaque(self->colours[1]);

                if (_50D144[i] & 0x1C)
                {
                    checkColour = Colour::inset(checkColour);
                }

                Gfx::drawString(dpi, x, y, checkColour, _strCheckmark);
            }

            char buffer[512]{};
            buffer[0] = textColour;
            strncpy(&buffer[1], object._name, 511);
            _currentFontSpriteBase = Font::medium_bold;

            Gfx::drawString(dpi, 15, y, Colour::black, buffer);
            y += rowHeight;
        }
    }

    // 0x00473A13
    static void closeWindow()
    {
        call(0x00473A13);
    }

    // 0x004737BA
    void onMouseUp(window* self, widget_index w)
    {
        switch (w)
        {
            case widx::closeButton:
                closeWindow();
                break;

            case widx::tabArea:
            {
                int clickedTab = -1;
                int y = widgets[widx::panel].top + self->y - 26;
                int x = self->x + 3;

                for (int row = 0; row < 2; row++)
                {
                    auto xPos = x + (row * rowOffsetX);
                    auto yPos = y - (row * rowOffsetY);

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

                if (clickedTab != -1 && self->current_tab != clickedTab)
                {
                    sub_4731EE(self, static_cast<object_type>(clickedTab));
                    self->row_hover = -1;
                    self->object = nullptr;
                    self->scroll_areas[0].contentWidth = 0;
                    ObjectManager::freeScenarioText();
                    auto objIndex = sub_472BBC(self);

                    if (objIndex.index != -1)
                    {
                        self->row_hover = objIndex.index;
                        self->object = reinterpret_cast<std::byte*>(objIndex.object._header);

                        ObjectManager::getScenarioText(*objIndex.object._header);
                    }

                    self->invalidate();
                }

                break;
            }

            case widx::advancedButton:
            {
                self->var_856 ^= 1;
                int currentTab = self->current_tab;
                sub_473154(self);

                if ((self->var_856 & 1) == 0)
                {
                    if (_4FE384[currentTab] & 1 << 1)
                    {
                        currentTab = _tabInformation[0].index;
                    }
                }
                sub_4731EE(self, static_cast<object_type>(currentTab));
                self->invalidate();

                break;
            }
        }
    }

    // 0x004738ED
    void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = _tabObjectCounts[self->current_tab] * rowHeight;
    }

    // 0x00473900
    static void tooltip(FormatArguments& args, Ui::window* window, widget_index widgetIndex)
    {
        args.push(StringIds::tooltip_object_list);
    }

    // 0x00472B54
    static ObjectManager::ObjIndexPair getObjectFromSelection(window* self, int16_t& y)
    {
        const auto objects = ObjectManager::getAvailableObjects(static_cast<object_type>(self->current_tab));

        for (auto [index, object] : objects)
        {
            y -= rowHeight;
            if (y < 0)
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        return { -1, ObjectManager::object_index_entry{} };
    }

    // 0x0047390A
    static void onScrollMouseOver(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(self, y);

        if (objIndex.index == self->row_hover || objIndex.index == -1)
            return;

        self->row_hover = objIndex.index;
        self->object = reinterpret_cast<std::byte*>(objIndex.object._header);
        ObjectManager::freeScenarioText();

        if (objIndex.index != -1)
        {
            ObjectManager::getScenarioText(*objIndex.object._header);
        }

        self->invalidate();
    }

    // 0x00473D1D
    static bool windowEditorObjectSelectionSelectObject(uint16_t bx, void* ebp)
    {
        registers regs;
        regs.bx = bx;
        regs.ebp = (uintptr_t)ebp;

        return call(0x00473D1D, regs) & (1 << 8);
    }

    // 0x00473948
    static void onScrollMouseDown(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(self, y);
        auto index = objIndex.index;
        auto object = objIndex.object._header;

        if (index == -1)
            return;

        self->invalidate();
        Audio::playSound(Audio::sound_id::click_down, Input::getMouseLocation().x);

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

        Ui::Windows::Error::open(errorTitle, gGameCommandErrorText);
    }

    // 0x00474821
    static void unloadUnselectedObjects()
    {
        call(0x00474821); // unload_unselected_objects
    }

    // 0x00474874
    static void editorLoadSelectedObjects()
    {
        call(0x00474874); // editor_load_selected_objects
    }

    // 0x0047237D
    static void resetLoadedObjects()
    {
        call(0x0047237D); // reset_loaded_objects
    }

    // 0x00473B91
    static void editorObjectFlagsFree0()
    {
        call(0x00473B91); // editor_object_flags_free_0
    }

    // 0x004739DD
    static void onClose(window* self)
    {
        if (!isEditorMode())
            return;

        unloadUnselectedObjects();
        editorLoadSelectedObjects();
        resetLoadedObjects();
        ObjectManager::freeScenarioText();
        editorObjectFlagsFree0();
    }

    // 0x00473A04
    static void onUpdate(window* self)
    {
        WindowManager::invalidateWidget(WindowType::objectSelection, self->number, widx::objectImage);
    }

    static void initEvents()
    {
        _events.on_close = onClose;
        _events.on_mouse_up = onMouseUp;
        _events.on_update = onUpdate;
        _events.get_scroll_size = getScrollSize;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
    }
}
