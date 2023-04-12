#include "Audio/Audio.h"
#include "Construction.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/SignalElement.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Ui/Dropdown.h"
#include "Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::World::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Signal
{
    Widget widgets[] = {
        commonWidgets(138, 167, StringIds::stringid_2),
        makeDropdownWidgets({ 3, 45 }, { 132, 12 }, WidgetType::combobox, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_signal_type),
        makeWidget({ 27, 110 }, { 40, 40 }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_signal_both_directions),
        makeWidget({ 71, 110 }, { 40, 40 }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_signal_single_direction),
        widgetEnd(),
    };

    WindowEventList events;

    // 0x0049E64E
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(&self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(&self, widgetIndex);
                break;
        }
    }

    // 0x0049E669
    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::signal_dropdown:
            {
                uint8_t signalCount = 0;
                while (_signalList[signalCount] != 0xFF)
                    signalCount++;

                auto widget = self.widgets[widx::signal];
                auto xPos = widget.left + self.x;
                auto yPos = widget.top + self.y;
                auto width = widget.width() + 2;
                auto height = widget.height();

                Dropdown::show(xPos, yPos, width, height, self.getColour(WindowColour::secondary), signalCount, (1 << 7));

                for (auto signalIndex = 0; signalIndex < signalCount; signalIndex++)
                {
                    auto signal = _signalList[signalIndex];
                    if (signal == _lastSelectedSignal)
                        Dropdown::setHighlightedItem(signalIndex);

                    auto trainSignalObj = ObjectManager::get<TrainSignalObject>(signal);

                    Dropdown::add(signalIndex, trainSignalObj->name);
                }
                break;
            }

            case widx::both_directions:
            {
                _isSignalBothDirections = 1;
                Input::toolCancel();
                Input::toolSet(&self, widgetIndex, CursorId::placeSignal);
                break;
            }

            case widx::single_direction:
            {
                _isSignalBothDirections = 0;
                Input::toolCancel();
                Input::toolSet(&self, widgetIndex, CursorId::placeSignal);
                break;
            }
        }
    }

    // 0x0049E67C
    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::signal_dropdown)
            return;

        if (itemIndex != -1)
        {
            _lastSelectedSignal = _signalList[itemIndex];
            Scenario::getConstruction().signals[_trackType] = _signalList[itemIndex];
            self.invalidate();
        }
    }

    // 0x0049E76F
    static void onUpdate(Window& self)
    {
        Common::onUpdate(&self, (1 << 2));
    }

    // Reverse direction map?
    static loco_global<uint8_t[16], 0x00503CAC> _503CAC;
    static loco_global<World::Pos2[16], 0x00503C6C> _503C6C;

    // 0x004A417A
    // false for left, true for right
    static bool getSide(const World::Pos3& loc, const Point& mousePos, const TrackElement& elTrack, const Viewport& viewport)
    {
        Common::setNextAndPreviousTrackTile(elTrack, loc);

        const bool isCloserToNext = Common::isPointCloserToNextOrPreviousTile(mousePos, viewport);

        return isCloserToNext;
    }

    static std::optional<GameCommands::SignalPlacementArgs> getSignalPlacementArgsFromCursor(const int16_t x, const int16_t y, const bool isBothDirectons)
    {
        static loco_global<Viewport*, 0x01135F52> _1135F52;

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        auto* elTrack = reinterpret_cast<World::TileElement*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        GameCommands::SignalPlacementArgs args;
        args.type = _lastSelectedSignal;
        args.pos = World::Pos3(interaction.pos.x, interaction.pos.y, elTrack->baseHeight());
        args.rotation = elTrack->unkDirection();
        args.trackId = elTrack->trackId();
        args.index = elTrack->sequenceIndex();
        args.trackObjType = elTrack->trackObjectId();
        if (isBothDirectons)
        {
            args.sides = 0xC000;
        }
        else
        {
            args.sides = getSide(args.pos, { x, y }, *elTrack, *viewport) ? 0x4000 : 0x8000;
        }
        return { args };
    }

    static uint32_t placeSignalGhost(const GameCommands::SignalPlacementArgs& args)
    {
        auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
        if (res != GameCommands::FAILURE)
        {
            _byte_522096 = _byte_522096 | (1 << 2);
            _signalGhostPos = args.pos;
            _signalGhostRotation = args.rotation;
            _signalGhostTrackId = args.trackId;
            _signalGhostTileIndex = args.index;
            _signalGhostSides = args.sides;
            _signalGhostTrackObjId = args.trackObjType;
        }
        return res;
    }

    // 0x0049FEF6
    void removeSignalGhost()
    {
        if (_byte_522096 & (1 << 2))
        {
            GameCommands::SignalRemovalArgs args;
            args.pos = _signalGhostPos;
            args.rotation = _signalGhostRotation;
            args.trackId = _signalGhostTrackId;
            args.index = _signalGhostTileIndex;
            args.flags = _signalGhostSides;
            args.type = _signalGhostTrackObjId;
            GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);

            _byte_522096 = _byte_522096 & ~(1 << 2);
        }
    }

    // 0x0049E745
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::single_direction && widgetIndex != widx::both_directions)
        {
            return;
        }

        const bool isBothDirections = widgetIndex == widx::both_directions;

        auto placementArgs = getSignalPlacementArgsFromCursor(x, y, isBothDirections);
        if (!placementArgs || (placementArgs->trackObjType != _trackType))
        {
            removeConstructionGhosts();
            if (_signalCost != 0x80000000)
            {
                _signalCost = 0x80000000;
                self.invalidate();
            }
            return;
        }

        if (_byte_522096 & (1 << 2))
        {
            if (*_signalGhostPos == placementArgs->pos
                && _signalGhostRotation == placementArgs->rotation
                && _signalGhostTrackId == placementArgs->trackId
                && _signalGhostTileIndex == placementArgs->index
                && _signalGhostSides == placementArgs->sides
                && _signalGhostTrackObjId == placementArgs->trackObjType)
            {
                return;
            }
        }

        removeConstructionGhosts();

        auto cost = placeSignalGhost(*placementArgs);
        if (cost != _signalCost)
        {
            _signalCost = cost;
            self.invalidate();
        }
    }

    // 0x0049E75A
    static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::single_direction && widgetIndex != widx::both_directions)
        {
            return;
        }

        removeConstructionGhosts();

        const bool isBothDirections = widgetIndex == widx::both_directions;
        auto args = getSignalPlacementArgsFromCursor(x, y, isBothDirections);
        if (!args)
        {
            return;
        }

        if (args->trackObjType != _trackType)
        {
            Error::open(StringIds::cant_build_signal_here, StringIds::wrong_type_of_track_road);
            return;
        }

        GameCommands::setErrorTitle(isBothDirections ? StringIds::cant_build_signals_here : StringIds::cant_build_signal_here);
        auto res = GameCommands::doCommand(*args, GameCommands::Flags::apply);
        if (res == GameCommands::FAILURE)
        {
            return;
        }
        Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
    }

    // 0x0049E499
    static void prepareDraw(Window& self)
    {
        Common::prepareDraw(&self);

        auto trackObj = ObjectManager::get<TrackObject>(_trackType);

        auto args = FormatArguments();
        args.push(trackObj->name);

        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(_lastSelectedSignal);

        self.widgets[widx::signal].text = trainSignalObject->name;

        Common::repositionTabs(&self);
    }

    // 0x0049E501
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        self.draw(rt);
        Common::drawTabs(&self, rt);

        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(_lastSelectedSignal);

        auto xPos = self.x + 3;
        auto yPos = self.y + 63;
        auto width = 130;

        {
            auto args = FormatArguments();
            args.push(trainSignalObject->var_0C);

            drawingCtx.drawStringLeftWrapped(*rt, xPos, yPos, width, Colour::black, StringIds::signal_black, &args);
        }

        auto imageId = trainSignalObject->image;

        xPos = self.widgets[widx::both_directions].midX() + self.x;
        yPos = self.widgets[widx::both_directions].bottom + self.y - 4;

        drawingCtx.drawImage(rt, xPos - 8, yPos, imageId);

        drawingCtx.drawImage(rt, xPos + 8, yPos, imageId + 4);

        xPos = self.widgets[widx::single_direction].midX() + self.x;
        yPos = self.widgets[widx::single_direction].bottom + self.y - 4;

        drawingCtx.drawImage(rt, xPos, yPos, imageId);

        if (_signalCost != 0x80000000 && _signalCost != 0)
        {
            auto args = FormatArguments();
            args.push<uint32_t>(_signalCost);

            xPos = self.x + 69;
            yPos = self.widgets[widx::single_direction].bottom + self.y + 5;

            drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }
    }

    void tabReset(Window* self)
    {
        self->callOnMouseDown(Signal::widx::both_directions);
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
