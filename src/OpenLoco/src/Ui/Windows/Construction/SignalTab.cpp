#include "Audio/Audio.h"
#include "Construction.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Track/CreateSignal.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
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
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::World::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Signal
{
    static constexpr auto widgets = makeWidgets(
        Common::makeCommonWidgets(138, 167, StringIds::stringid_2),
        Widgets::dropdownWidgets({ 3, 45 }, { 132, 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_signal_type),
        Widgets::ImageButton({ 27, 110 }, { 40, 40 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_signal_both_directions),
        Widgets::ImageButton({ 71, 110 }, { 40, 40 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_signal_single_direction));

    std::span<const Widget> getWidgets()
    {
        return widgets;
    }

    WindowEventList events;

    // 0x0049E64E
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
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
                Common::switchTab(self, widgetIndex);
                break;
        }
    }

    // 0x0049E669
    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::signal_dropdown:
            {
                uint8_t signalCount = 0;
                while (_cState->signalList[signalCount] != 0xFF)
                {
                    signalCount++;
                }

                auto widget = self.widgets[widx::signal];
                auto xPos = self.x + widget.left;
                auto yPos = self.y + widget.top;
                auto width = widget.width() + 2;
                auto height = widget.height();

                Dropdown::show(xPos, yPos, width, height, self.getColour(WindowColour::secondary), signalCount, (1 << 7));

                for (auto signalIndex = 0; signalIndex < signalCount; signalIndex++)
                {
                    auto signal = _cState->signalList[signalIndex];
                    if (signal == _cState->lastSelectedSignal)
                    {
                        Dropdown::setHighlightedItem(signalIndex);
                    }

                    auto trainSignalObj = ObjectManager::get<TrainSignalObject>(signal);

                    Dropdown::add(signalIndex, trainSignalObj->name);
                }
                break;
            }

            case widx::both_directions:
            {
                _cState->isSignalBothDirections = 1;
                ToolManager::toolCancel();
                ToolManager::toolSet(self, widgetIndex, CursorId::placeSignal);
                break;
            }

            case widx::single_direction:
            {
                _cState->isSignalBothDirections = 0;
                ToolManager::toolCancel();
                ToolManager::toolSet(self, widgetIndex, CursorId::placeSignal);
                break;
            }
        }
    }

    // 0x0049E67C
    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
    {
        if (widgetIndex != widx::signal_dropdown)
        {
            return;
        }

        if (itemIndex != -1)
        {
            _cState->lastSelectedSignal = _cState->signalList[itemIndex];
            Scenario::getConstruction().signals[_cState->trackType] = _cState->signalList[itemIndex];
            self.invalidate();
        }
    }

    // 0x0049E76F
    static void onUpdate(Window& self)
    {
        Common::onUpdate(&self, GhostVisibilityFlags::signal);
    }

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
        args.type = _cState->lastSelectedSignal;
        args.pos = World::Pos3(interaction.pos.x, interaction.pos.y, elTrack->baseHeight());
        args.rotation = elTrack->rotation();
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
        auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
        if (res != GameCommands::FAILURE)
        {
            _ghostVisibilityFlags = _ghostVisibilityFlags | GhostVisibilityFlags::signal;
            _cState->signalGhostPos = args.pos;
            _cState->signalGhostRotation = args.rotation;
            _cState->signalGhostTrackId = args.trackId;
            _cState->signalGhostTileIndex = args.index;
            _cState->signalGhostSides = args.sides;
            _cState->signalGhostTrackObjId = args.trackObjType;
        }
        return res;
    }

    // 0x0049FEF6
    void removeSignalGhost()
    {
        if ((_ghostVisibilityFlags & GhostVisibilityFlags::signal) != GhostVisibilityFlags::none)
        {
            GameCommands::SignalRemovalArgs args;
            args.pos = _cState->signalGhostPos;
            args.rotation = _cState->signalGhostRotation;
            args.trackId = _cState->signalGhostTrackId;
            args.index = _cState->signalGhostTileIndex;
            args.flags = _cState->signalGhostSides;
            args.trackObjType = _cState->signalGhostTrackObjId;
            GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);

            _ghostVisibilityFlags = _ghostVisibilityFlags & ~GhostVisibilityFlags::signal;
        }
    }

    // 0x0049E745
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::single_direction && widgetIndex != widx::both_directions)
        {
            return;
        }

        const bool isBothDirections = widgetIndex == widx::both_directions;

        auto placementArgs = getSignalPlacementArgsFromCursor(x, y, isBothDirections);
        if (!placementArgs || (placementArgs->trackObjType != _cState->trackType))
        {
            removeConstructionGhosts();
            if (_cState->signalCost != 0x80000000)
            {
                _cState->signalCost = 0x80000000;
                self.invalidate();
            }
            return;
        }

        if ((_ghostVisibilityFlags & GhostVisibilityFlags::signal) != GhostVisibilityFlags::none)
        {
            if (_cState->signalGhostPos == placementArgs->pos
                && _cState->signalGhostRotation == placementArgs->rotation
                && _cState->signalGhostTrackId == placementArgs->trackId
                && _cState->signalGhostTileIndex == placementArgs->index
                && _cState->signalGhostSides == placementArgs->sides
                && _cState->signalGhostTrackObjId == placementArgs->trackObjType)
            {
                return;
            }
        }

        removeConstructionGhosts();

        auto cost = placeSignalGhost(*placementArgs);
        if (cost != _cState->signalCost)
        {
            _cState->signalCost = cost;
            self.invalidate();
        }
    }

    // 0x0049E75A
    static void onToolUp([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
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

        if (args->trackObjType != _cState->trackType)
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

        auto trackObj = ObjectManager::get<TrackObject>(_cState->trackType);

        auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
        args.push(trackObj->name);

        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(_cState->lastSelectedSignal);

        self.widgets[widx::signal].text = trainSignalObject->name;

        Common::repositionTabs(&self);
    }

    // 0x0049E501
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);
        Common::drawTabs(self, drawingCtx);

        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(_cState->lastSelectedSignal);

        auto xPos = 3;
        auto yPos = 63;
        auto width = 130;

        {
            FormatArguments args{};
            args.push(trainSignalObject->description);

            auto point = Point(xPos, yPos);
            tr.drawStringLeftWrapped(point, width, Colour::black, StringIds::signal_black, args);
        }

        auto imageId = trainSignalObject->image;

        xPos = self.widgets[widx::both_directions].midX();
        yPos = self.widgets[widx::both_directions].bottom - 4;

        drawingCtx.drawImage(xPos - 8, yPos, imageId);

        drawingCtx.drawImage(xPos + 8, yPos, imageId + 4);

        xPos = self.widgets[widx::single_direction].midX();
        yPos = self.widgets[widx::single_direction].bottom - 4;

        drawingCtx.drawImage(xPos, yPos, imageId);

        if (_cState->signalCost != 0x80000000 && _cState->signalCost != 0)
        {
            FormatArguments args{};
            args.push<uint32_t>(_cState->signalCost);

            auto point = Point(69, self.widgets[widx::single_direction].bottom + 5);
            tr.drawStringCentred(point, Colour::black, StringIds::build_cost, args);
        }
    }

    void tabReset(Window& self)
    {
        self.callOnMouseDown(Signal::widx::both_directions, self.widgets[Signal::widx::both_directions].id);
    }

    static constexpr WindowEventList kEvents = {
        .onClose = Common::onClose,
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .onToolUpdate = onToolUpdate,
        .toolUp = onToolUp,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
