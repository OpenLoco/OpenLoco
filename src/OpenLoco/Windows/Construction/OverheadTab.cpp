#include "../../Audio/Audio.h"
#include "../../CompanyManager.h"
#include "../../GameCommands/GameCommands.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadExtraObject.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/TrackExtraObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Ui/Dropdown.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Overhead
{
    Widget widgets[] = {
        commonWidgets(138, 192, StringIds::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 3, 57 }, { 132, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 3, 69 }, { 132, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 3, 81 }, { 132, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 35, 110 }, { 66, 66 }, WidgetType::wt_3, WindowColour::secondary),
        makeDropdownWidgets({ 3, 95 }, { 132, 12 }, WidgetType::combobox, WindowColour::secondary, 0xFFFFFFFF, StringIds::tooltip_select_track_to_upgrade),
        widgetEnd(),
    };

    WindowEventList events;

    // 0x0049EBD1
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(self, widgetIndex);
                break;

            case widx::checkbox_1:
            case widx::checkbox_2:
            case widx::checkbox_3:
            case widx::checkbox_4:
            {
                auto checkboxIndex = widgetIndex - widx::checkbox_1;

                _lastSelectedMods = _lastSelectedMods ^ (1 << checkboxIndex);

                // TODO: & ~(1 << 7) added to prevent crashing when selecting/deselecting overhead wires for trams
                _scenarioTrackMods[_trackType & ~(1 << 7)] = _lastSelectedMods;

                self->invalidate();
                break;
            }
        }
    }

    // 0x0049EBFC
    static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::track_dropdown:
            {
                uint8_t modCount = 3;

                auto widget = self->widgets[widx::track];
                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                auto width = widget.width() + 2;
                auto height = widget.height();

                Dropdown::show(xPos, yPos, width, height, self->getColour(WindowColour::secondary), modCount, (1 << 7));

                Dropdown::add(0, StringIds::single_section);
                Dropdown::add(1, StringIds::block_section);
                Dropdown::add(2, StringIds::all_connected_track);

                Dropdown::setHighlightedItem(_lastSelectedTrackModSection);
                break;
            }

            case widx::image:
            {
                Input::toolCancel();
                Input::toolSet(self, widgetIndex, CursorId::crosshair);
                break;
            }
        }
    }

    // 0x0049EC09
    static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::track_dropdown)
            return;

        if (itemIndex != -1)
        {
            _lastSelectedTrackModSection = itemIndex;
            self->invalidate();
        }
    }

    // 0x0049ECD1
    static void onUpdate(Window* self)
    {
        Common::onUpdate(self, (1 << 5));
    }

    static std::optional<GameCommands::RoadModsPlacementArgs> getRoadModsPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        static loco_global<Viewport*, 0x01135F52> _1135F52;

        _113600C = { x, y };

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::roadAndTram));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::road)
        {
            return std::nullopt;
        }

        auto* elRoad = reinterpret_cast<Map::TileElement*>(interaction.object)->as<RoadElement>();
        if (elRoad == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::RoadModsPlacementArgs args;
        args.type = _lastSelectedMods;
        args.pos = Map::Pos3(interaction.pos.x, interaction.pos.y, elRoad->baseZ() * 4);
        args.rotation = elRoad->unkDirection();
        args.roadId = elRoad->roadId();
        args.index = elRoad->sequenceIndex();
        args.roadObjType = elRoad->roadObjectId();
        args.modSection = _lastSelectedTrackModSection;
        return { args };
    }

    static std::optional<GameCommands::TrackModsPlacementArgs> getTrackModsPlacementArgsFromCursor(const int16_t x, const int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        static loco_global<Viewport*, 0x01135F52> _1135F52;

        _113600C = { x, y };

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        auto* elTrack = reinterpret_cast<Map::TileElement*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::TrackModsPlacementArgs args;
        args.type = _lastSelectedMods;
        args.pos = Map::Pos3(interaction.pos.x, interaction.pos.y, elTrack->baseZ() * 4);
        args.rotation = elTrack->unkDirection();
        args.trackId = elTrack->trackId();
        args.index = elTrack->sequenceIndex();
        args.trackObjType = elTrack->trackObjectId();
        args.modSection = _lastSelectedTrackModSection;
        return { args };
    }

    // 0x0049FF0
    void removeTrackModsGhost()
    {
        if (_byte_522096 & (1 << 4))
        {
            if (_modGhostTrackObjId & (1 << 7))
            {
                GameCommands::RoadModsRemovalArgs args;
                args.pos = _modGhostPos;
                args.rotation = _modGhostRotation;
                args.roadId = _modGhostTrackId;
                args.index = _modGhostTileIndex;
                args.roadObjType = _modGhostTrackObjId & ~(1 << 7);
                args.type = _lastSelectedMods;
                args.modSection = _lastSelectedTrackModSection;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            else
            {
                GameCommands::TrackModsRemovalArgs args;
                args.pos = _modGhostPos;
                args.rotation = _modGhostRotation;
                args.trackId = _modGhostTrackId;
                args.index = _modGhostTileIndex;
                args.trackObjType = _modGhostTrackObjId & ~(1 << 7);
                args.type = _lastSelectedMods;
                args.modSection = _lastSelectedTrackModSection;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            _byte_522096 = _byte_522096 & ~(1 << 4);
        }
    }

    static uint32_t placeRoadModGhost(const GameCommands::RoadModsPlacementArgs& args)
    {
        auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
        if (res != GameCommands::FAILURE)
        {
            _byte_522096 = _byte_522096 | (1 << 4);
            _modGhostPos = args.pos;
            _modGhostRotation = args.rotation;
            _modGhostTrackId = args.roadId;
            _modGhostTileIndex = args.index;
            _modGhostTrackObjId = args.roadObjType | (1 << 7); // This looks wrong!
        }
        return res;
    }

    static uint32_t placeTrackModGhost(const GameCommands::TrackModsPlacementArgs& args)
    {
        auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
        if (res != GameCommands::FAILURE)
        {
            _byte_522096 = _byte_522096 | (1 << 4);
            _modGhostPos = args.pos;
            _modGhostRotation = args.rotation;
            _modGhostTrackId = args.trackId;
            _modGhostTileIndex = args.index;
            _modGhostTrackObjId = args.trackObjType;
        }
        return res;
    }

    // 0x0049EC15
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::image)
        {
            return;
        }

        if (_trackType & (1 << 7))
        {
            auto placementArgs = getRoadModsPlacementArgsFromCursor(x, y);
            if (!placementArgs || ((placementArgs->roadObjType | (1 << 7)) != _trackType))
            {
                removeConstructionGhosts();
                if (_modCost != 0x80000000)
                {
                    _modCost = 0x80000000;
                    self.invalidate();
                }
                return;
            }

            if (_byte_522096 & (1 << 4))
            {
                if (*_modGhostPos == placementArgs->pos
                    && _modGhostRotation == placementArgs->rotation
                    && _modGhostTrackId == placementArgs->roadId
                    && _modGhostTileIndex == placementArgs->index
                    && _modGhostTrackObjId == placementArgs->roadObjType)
                {
                    return;
                }
            }

            removeConstructionGhosts();

            auto cost = placeRoadModGhost(*placementArgs);
            if (cost != _modCost)
            {
                _modCost = cost;
                self.invalidate();
            }
        }
        else
        {
            auto placementArgs = getTrackModsPlacementArgsFromCursor(x, y);
            if (!placementArgs || (placementArgs->trackObjType != _trackType))
            {
                removeConstructionGhosts();
                if (_modCost != 0x80000000)
                {
                    _modCost = 0x80000000;
                    self.invalidate();
                }
                return;
            }

            if (_byte_522096 & (1 << 4))
            {
                if (*_modGhostPos == placementArgs->pos
                    && _modGhostRotation == placementArgs->rotation
                    && _modGhostTrackId == placementArgs->trackId
                    && _modGhostTileIndex == placementArgs->index
                    && _modGhostTrackObjId == placementArgs->trackObjType)
                {
                    return;
                }
            }

            removeConstructionGhosts();

            auto cost = placeTrackModGhost(*placementArgs);
            if (cost != _modCost)
            {
                _modCost = cost;
                self.invalidate();
            }
        }
    }

    // 0x0049EC20
    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::image)
        {
            return;
        }

        removeConstructionGhosts();

        if (_trackType & (1 << 7))
        {
            auto args = getRoadModsPlacementArgsFromCursor(x, y);
            if (!args)
            {
                return;
            }

            if ((args->roadObjType | (1 << 7)) != _trackType)
            {
                Error::open(StringIds::error_cant_build_this_here, StringIds::wrong_type_of_track_road);
                return;
            }
            GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);
            auto res = GameCommands::doCommand(*args, GameCommands::Flags::apply);
            if (res == GameCommands::FAILURE || res == 0)
            {
                return;
            }
            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
        }
        else
        {
            auto args = getTrackModsPlacementArgsFromCursor(x, y);
            if (!args)
            {
                return;
            }

            if (args->trackObjType != _trackType)
            {
                Error::open(StringIds::error_cant_build_this_here, StringIds::wrong_type_of_track_road);
                return;
            }
            GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);
            auto res = GameCommands::doCommand(*args, GameCommands::Flags::apply);
            if (res == GameCommands::FAILURE || res == 0)
            {
                return;
            }
            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
        }
    }

    static void setCheckbox(Window* self, WidgetIndex_t checkboxIndex, string_id name)
    {
        auto widgetIndex = checkboxIndex + widx::checkbox_1;
        self->widgets[widgetIndex].type = WidgetType::checkbox;
        self->widgets[widgetIndex].text = name;

        if (_lastSelectedMods & (1 << checkboxIndex))
            self->activatedWidgets |= (1ULL << widgetIndex);
    }

    // 0x0049E7D3
    static void prepareDraw(Window* self)
    {
        Common::prepareDraw(self);

        self->activatedWidgets &= ~(1 << widx::checkbox_1 | 1 << widx::checkbox_2 | 1 << widx::checkbox_3 | 1 << widx::checkbox_4);

        self->widgets[widx::checkbox_1].type = WidgetType::none;
        self->widgets[widx::checkbox_2].type = WidgetType::none;
        self->widgets[widx::checkbox_3].type = WidgetType::none;
        self->widgets[widx::checkbox_4].type = WidgetType::none;

        if (_trackType & (1 << 7))
        {
            auto trackType = _trackType & ~(1 << 7);
            auto roadObj = ObjectManager::get<RoadObject>(trackType);

            auto args = FormatArguments();
            args.push(roadObj->name);

            for (auto i = 0; i < 2; i++)
            {
                if (_modList[i] != 0xFF)
                {
                    auto extraName = ObjectManager::get<RoadExtraObject>(_modList[i])->name;
                    setCheckbox(self, i, extraName);
                }
            }
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(_trackType);

            auto args = FormatArguments();
            args.push(trackObj->name);

            for (auto i = 0; i < 4; i++)
            {
                if (_modList[i] != 0xFF)
                {
                    auto extraName = ObjectManager::get<TrackExtraObject>(_modList[i])->name;
                    setCheckbox(self, i, extraName);
                }
            }
        }

        // self->activatedWidgets = activatedWidgets;

        self->widgets[widx::image].type = WidgetType::none;
        self->widgets[widx::track].type = WidgetType::none;
        self->widgets[widx::track_dropdown].type = WidgetType::none;

        self->widgets[widx::image].tooltip = StringIds::null;

        if (_lastSelectedMods & 0xF)
        {
            self->widgets[widx::image].type = WidgetType::wt_3;
            self->widgets[widx::track].type = WidgetType::combobox;
            self->widgets[widx::track_dropdown].type = WidgetType::button;

            self->widgets[widx::image].tooltip = StringIds::upgrade_track_with_mods;

            if (isNetworkHost())
            {
                if (_toolWindowType == WindowType::construction)
                    self->widgets[widx::image].tooltip = StringIds::click_track_to_upgrade;
            }
        }

        static string_id modString[] = {
            StringIds::single_section,
            StringIds::block_section,
            StringIds::all_connected_track,
        };

        self->widgets[widx::track].text = modString[_lastSelectedTrackModSection];

        Common::repositionTabs(self);
    }

    // 0x0049EA3E
    static void draw(Window* self, Gfx::Context* context)
    {
        self->draw(context);
        Common::drawTabs(self, context);
        if (_lastSelectedMods & 0xF)
        {
            auto xPos = self->x + self->widgets[widx::image].left + 1;
            auto yPos = self->y + self->widgets[widx::image].top + 1;
            auto width = self->widgets[widx::image].width();
            auto height = self->widgets[widx::image].height();
            auto clipped = Gfx::clipContext(*context, Ui::Rect(xPos, yPos, width, height));
            if (clipped)
            {
                coord_t x = 0x2010;
                coord_t y = 0x2010;

                auto rotCoord = Math::Vector::rotate(Pos2{ x, y }, WindowManager::getCurrentRotation());
                Ui::Point screenPos = { static_cast<int16_t>(rotCoord.y - rotCoord.x), static_cast<int16_t>(((rotCoord.x + rotCoord.y) >> 1) - 460) };

                screenPos.x -= (self->widgets[widx::image].width() / 2);
                screenPos.y -= ((self->widgets[widx::image].width() / 2) + 16);
                clipped->x += screenPos.x;
                clipped->y += screenPos.y;

                _dword_E0C3E0 = &*clipped;

                x = 0x2000;
                y = 0x2000;

                auto company = CompanyManager::get(_playerCompany);
                auto companyColour = company->mainColours.primary;
                _byte_522095 = _byte_522095 | (1 << 0);

                if (_trackType & (1 << 7))
                {
                    uint8_t trackType = _trackType & ~(1 << 7);
                    Construction::drawRoad(x, y, _lastSelectedMods, 0x1D0, trackType, 0, enumValue(companyColour), WindowManager::getCurrentRotation());
                }
                else
                {
                    Construction::drawTrack(x, y, _lastSelectedMods, 0x1D0, _trackType, 0, enumValue(companyColour), WindowManager::getCurrentRotation());
                }
                _byte_522095 = _byte_522095 & ~(1 << 0);
                _dword_E0C3E0 = nullptr;
            }
        }

        auto xPos = self->x + 69;
        auto yPos = self->widgets[widx::image].bottom + self->y + 4;

        if (_modCost != 0x80000000 && _modCost != 0)
        {
            auto args = FormatArguments();
            args.push<uint32_t>(_modCost);

            Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }
    }

    void tabReset(Window* self)
    {
        self->callOnMouseDown(Overhead::widx::image);
    }

    void initEvents()
    {
        events.onClose = Common::onClose;
        events.onMouseUp = onMouseUp;
        events.onMouseDown = onMouseDown;
        events.onDropdown = onDropdown;
        events.onUpdate = onUpdate;
        events.onToolUpdate = onToolUpdate;
        events.onToolDown = onToolDown;
        events.prepareDraw = prepareDraw;
        events.draw = draw;
    }
}
