#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Track/CreateSignal.h"
#include "GameCommands/Track/CreateSignalsAuto.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "GameCommands/Track/RemoveSignalsAuto.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/SignalElement.h"
#include "Map/TileElementEntry.h"
#include "Map/TileManager.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Scenario/ScenarioConstruction.h"
#include "Ui/Dropdown.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CheckboxWidget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/StepperWidget.h"
#include "Ui/Windows/Construction/Construction.h"

using namespace OpenLoco::World;
using namespace OpenLoco::World::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Signal
{
    constexpr int32_t kWidth = 132;
    constexpr int32_t kHeight = 196;
    constexpr int32_t kSpacing = 4;
    constexpr int32_t kImageButtonSize = 40;
    constexpr int32_t kStepperWidth = 48;

    static constexpr auto widgets = makeWidgets(
        Common::makeCommonWidgets(kWidth, kHeight, StringIds::stringid_2),
        Widgets::dropdownWidgets(Widx::kSignal, Widx::kSignalDropdown, { kSpacing, 45 }, { kWidth - (kSpacing * 2), 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_signal_type),
        Widgets::ImageButton(Widx::kBothDirections, { ((kWidth - (kSpacing * 2)) / 2) - kSpacing - kImageButtonSize, 96 }, { kImageButtonSize, kImageButtonSize }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_signal_both_directions),
        Widgets::ImageButton(Widx::kSingleDirection, { ((kWidth - (kSpacing * 2)) / 2) + kSpacing, 96 }, { kImageButtonSize, kImageButtonSize }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_signal_single_direction),
        Widgets::Checkbox(Widx::kAutoMode, { kSpacing, 144 }, { kWidth - (kSpacing * 2), 12 }, WindowColour::secondary, StringIds::signal_placement_repeat, StringIds::signal_placement_repeat_tooltip),
        Widgets::Label(Widx::kStepLabel, { kSpacing, 160 }, { kWidth - (kSpacing * 2), 13 }, WindowColour::secondary, ContentAlign::left, StringIds::signal_placement_step_size),
        Widgets::stepperWidgets(Widx::kStepValue, Widx::kStepDecrease, Widx::kStepIncrease, { kWidth - kSpacing - kStepperWidth, 160 }, { kStepperWidth, 12 }, WindowColour::secondary, StringIds::uint16_raw, StringIds::tooltip_select_signal_type)
        // cost of signal placement is drawn at the bottom of the window as the last widget

    );

    std::span<const Widget> getWidgets()
    {
        return widgets;
    }

    WindowEventList events;

    // 0x0049E64E
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (id)
        {
            case Common::Widx::kCloseButton:
                WindowManager::close(&self);
                break;

            case Common::Widx::kTabConstruction:
            case Common::Widx::kTabOverhead:
            case Common::Widx::kTabSignal:
            case Common::Widx::kTabStation:
                Common::switchTab(self, widgetIndex);
                break;

            case Widx::kAutoMode:
                auto& cState = getConstructionState();
                cState.repeatedSignalMode = !cState.repeatedSignalMode;
                self.invalidate();
                break;
        }
    }

    // 0x0049E669
    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        auto& cState = getConstructionState();
        switch (id)
        {
            case Widx::kSignalDropdown:
            {
                uint8_t signalCount = 0;
                while (cState.signalList[signalCount] != 0xFF)
                {
                    signalCount++;
                }

                auto widget = self.widgets[widx::signal];
                auto xPos = widget.left + self.x;
                auto yPos = widget.top + self.y;
                auto width = widget.width() + 2;
                auto height = widget.height();

                Dropdown::show(xPos, yPos, width, height, self.getColour(WindowColour::secondary), signalCount, (1 << 7));

                for (auto signalIndex = 0; signalIndex < signalCount; signalIndex++)
                {
                    auto signal = cState.signalList[signalIndex];
                    if (signal == cState.lastSelectedSignal)
                    {
                        Dropdown::setHighlightedItem(signalIndex);
                    }

                    auto trainSignalObj = ObjectManager::get<TrainSignalObject>(signal);

                    Dropdown::add(signalIndex, trainSignalObj->name);
                }
                break;
            }

            case Widx::kBothDirections:
            {
                cState.isSignalBothDirections = 1;
                ToolManager::toolCancel();
                ToolManager::toolSet(self, widgetIndex, CursorId::placeSignal);
                break;
            }

            case Widx::kSingleDirection:
            {
                cState.isSignalBothDirections = 0;
                ToolManager::toolCancel();
                ToolManager::toolSet(self, widgetIndex, CursorId::placeSignal);
                break;
            }

            case Widx::kStepDecrease:
            {
                cState.signalPlacementStepSize = std::max<uint8_t>(1, cState.signalPlacementStepSize - 1);
                self.invalidate();

                break;
            }

            case Widx::kStepIncrease:
            {
                cState.signalPlacementStepSize = std::min<uint8_t>(kMaxSignalPlacementStepSize, cState.signalPlacementStepSize + 1);
                self.invalidate();
                break;
            }
        }
    }

    // 0x0049E67C
    static void onDropdown(Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, const WidgetId id, int16_t itemIndex)
    {
        if (id != Widx::kSignalDropdown)
        {
            return;
        }

        auto& cState = getConstructionState();
        if (itemIndex != -1)
        {
            cState.lastSelectedSignal = cState.signalList[itemIndex];
            Scenario::getConstruction().signals[cState.trackType] = cState.signalList[itemIndex];
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
        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track));
        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        auto* elTrack = reinterpret_cast<World::TileElementEntry*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        auto& cState = getConstructionState();

        GameCommands::SignalPlacementArgs args;
        args.type = cState.lastSelectedSignal;
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

    static std::optional<GameCommands::SignalsPlacementAutoArgs> getSignalsPlacementAutoArgsFromCursor(const int16_t x, const int16_t y, const bool isBothDirectons)
    {
        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track));
        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        auto* elTrack = reinterpret_cast<World::TileElementEntry*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        auto& cState = getConstructionState();

        GameCommands::SignalsPlacementAutoArgs args;
        args.type = cState.lastSelectedSignal;
        args.pos = World::Pos3(interaction.pos.x, interaction.pos.y, elTrack->baseHeight());
        args.rotation = elTrack->rotation();
        args.trackId = elTrack->trackId();
        args.index = elTrack->sequenceIndex();
        args.trackObjType = elTrack->trackObjectId();
        args.step = cState.signalPlacementStepSize;

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
        auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::preventBuildingClearing | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
        if (res != GameCommands::kFailure)
        {
            Common::setGhostVisibilityFlag(GhostVisibilityFlags::signal);

            auto& cState = getConstructionState();
            cState.signalGhostPos = args.pos;
            cState.signalGhostRotation = args.rotation;
            cState.signalGhostTrackId = args.trackId;
            cState.signalGhostTileIndex = args.index;
            cState.signalGhostSides = args.sides;
            cState.signalGhostTrackObjId = args.trackObjType;
        }
        return res;
    }

    static uint32_t placeRepeatedSignalGhost(const GameCommands::SignalsPlacementAutoArgs& args)
    {
        auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::preventBuildingClearing | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
        if (res != GameCommands::kFailure)
        {
            Common::setGhostVisibilityFlag(GhostVisibilityFlags::signal);

            auto& cState = getConstructionState();
            cState.signalGhostPos = args.pos;
            cState.signalGhostRotation = args.rotation;
            cState.signalGhostTrackId = args.trackId;
            cState.signalGhostTileIndex = args.index;
            cState.signalGhostSides = args.sides;
            cState.signalGhostTrackObjId = args.trackObjType;
            cState.signalPlacementStepSize = args.step;
        }
        return res;
    }

    // 0x0049FEF6
    void removeSignalGhost()
    {
        if (Common::hasGhostVisibilityFlag(GhostVisibilityFlags::signal))
        {
            auto& cState = getConstructionState();

            {
                GameCommands::SignalRemovalArgs args;
                args.pos = cState.signalGhostPos;
                args.rotation = cState.signalGhostRotation;
                args.trackId = cState.signalGhostTrackId;
                args.index = cState.signalGhostTileIndex;
                args.flags = cState.signalGhostSides;
                args.trackObjType = cState.signalGhostTrackObjId;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }

            {
                GameCommands::SignalsRemovalAutoArgs args;
                args.pos = cState.signalGhostPos;
                args.rotation = cState.signalGhostRotation;
                args.trackId = cState.signalGhostTrackId;
                args.index = cState.signalGhostTileIndex;
                args.flags = cState.signalGhostSides;
                args.trackObjType = cState.signalGhostTrackObjId;
                args.step = cState.signalPlacementStepSize;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }

            Common::unsetGhostVisibilityFlag(GhostVisibilityFlags::signal);
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

        auto& cState = getConstructionState();

        if (cState.repeatedSignalMode)
        {
            auto placementArgs = getSignalsPlacementAutoArgsFromCursor(x, y, isBothDirections);
            if (!placementArgs || (placementArgs->trackObjType != cState.trackType))
            {
                removeConstructionGhosts();
                if (cState.signalCost != GameCommands::kFailure)
                {
                    cState.signalCost = GameCommands::kFailure;
                    self.invalidate();
                }
                return;
            }

            if (Common::hasGhostVisibilityFlag(GhostVisibilityFlags::signal))
            {
                if (cState.signalGhostPos == placementArgs->pos
                    && cState.signalGhostRotation == placementArgs->rotation
                    && cState.signalGhostTrackId == placementArgs->trackId
                    && cState.signalGhostTileIndex == placementArgs->index
                    && cState.signalGhostSides == placementArgs->sides
                    && cState.signalGhostTrackObjId == placementArgs->trackObjType)
                {
                    return;
                }
            }

            removeConstructionGhosts();

            auto cost = placeRepeatedSignalGhost(*placementArgs);
            if (cost != cState.signalCost)
            {
                cState.signalCost = cost;
                self.invalidate();
            }
        }
        else
        {

            auto placementArgs = getSignalPlacementArgsFromCursor(x, y, isBothDirections);
            if (!placementArgs || (placementArgs->trackObjType != cState.trackType))
            {
                removeConstructionGhosts();
                if (cState.signalCost != GameCommands::kFailure)
                {
                    cState.signalCost = GameCommands::kFailure;
                    self.invalidate();
                }
                return;
            }

            if (Common::hasGhostVisibilityFlag(GhostVisibilityFlags::signal))
            {
                if (cState.signalGhostPos == placementArgs->pos
                    && cState.signalGhostRotation == placementArgs->rotation
                    && cState.signalGhostTrackId == placementArgs->trackId
                    && cState.signalGhostTileIndex == placementArgs->index
                    && cState.signalGhostSides == placementArgs->sides
                    && cState.signalGhostTrackObjId == placementArgs->trackObjType)
                {
                    return;
                }
            }

            removeConstructionGhosts();

            auto cost = placeSignalGhost(*placementArgs);
            if (cost != cState.signalCost)
            {
                cState.signalCost = cost;
                self.invalidate();
            }
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
        auto& cState = getConstructionState();

        if (cState.repeatedSignalMode)
        {
            auto args = getSignalsPlacementAutoArgsFromCursor(x, y, isBothDirections);
            if (!args)
            {
                return;
            }

            if (args->trackObjType != cState.trackType)
            {
                Error::open(StringIds::cant_build_signal_here, StringIds::wrong_type_of_track_road);
                return;
            }

            GameCommands::setErrorTitle(isBothDirections ? StringIds::cant_build_signals_here : StringIds::cant_build_signal_here);
            GameCommands::SignalsPlacementAutoArgs autoArgs{};
            autoArgs.index = args->index;
            autoArgs.pos = args->pos;
            autoArgs.rotation = args->rotation;
            autoArgs.trackId = args->trackId;
            autoArgs.sides = args->sides;
            autoArgs.trackObjType = args->trackObjType;
            autoArgs.step = cState.signalPlacementStepSize;

            auto res = GameCommands::doCommand(autoArgs, GameCommands::Flags::apply);
            if (res == GameCommands::kFailure)
            {
                return;
            }
        }
        else
        {
            auto args = getSignalPlacementArgsFromCursor(x, y, isBothDirections);
            if (!args)
            {
                return;
            }

            if (args->trackObjType != cState.trackType)
            {
                Error::open(StringIds::cant_build_signal_here, StringIds::wrong_type_of_track_road);
                return;
            }

            GameCommands::setErrorTitle(isBothDirections ? StringIds::cant_build_signals_here : StringIds::cant_build_signal_here);
            auto res = GameCommands::doCommand(*args, GameCommands::Flags::apply);
            if (res == GameCommands::kFailure)
            {
                return;
            }
        }

        Audio::playSound(Audio::SoundId::construct, Audio::ChannelId::effects, GameCommands::getPosition());
    }

    // 0x0049E499
    static void prepareDraw(Window& self)
    {
        Common::prepareDraw(&self);

        auto& cState = getConstructionState();
        auto trackObj = ObjectManager::get<TrackObject>(cState.trackType);

        {
            auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
            args.push(trackObj->name);
        }

        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(cState.lastSelectedSignal);
        self.widgets[widx::signal].text = trainSignalObject->name;

        if (cState.repeatedSignalMode)
        {
            self.activatedWidgets |= (1 << widx::auto_mode);
        }
        else
        {
            self.activatedWidgets &= ~(1 << widx::auto_mode);
        }

        // Update step size value display
        {
            auto& widget = self.widgets[widx::step_value];
            FormatArguments args{ widget.textArgs };
            args.push<uint16_t>(cState.signalPlacementStepSize); // can't push a single byte???
        }

        Common::repositionTabs(&self);
    }

    // 0x0049E501
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);
        Common::drawTabs(self, drawingCtx);

        auto& cState = getConstructionState();
        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(cState.lastSelectedSignal);

        {
            FormatArguments args{};
            args.push(trainSignalObject->description);

            tr.drawStringLeftWrapped({ 3, 63 }, 130, Colour::black, StringIds::signal_black, args);
        }

        auto baseImageId = trainSignalObject->image;

        // Both directions
        {
            auto xPos = self.widgets[widx::both_directions].midX();
            auto yPos = self.widgets[widx::both_directions].bottom - 4;

            drawingCtx.drawImage(ZoomLevel::full, xPos - 8, yPos, baseImageId);

            drawingCtx.drawImage(ZoomLevel::full, xPos + 8, yPos, baseImageId + 4);
        }

        // Single direction
        {
            auto xPos = self.widgets[widx::single_direction].midX();
            auto yPos = self.widgets[widx::single_direction].bottom - 4;

            drawingCtx.drawImage(ZoomLevel::full, xPos, yPos, baseImageId);
        }

        if (cState.signalCost != GameCommands::kFailure && cState.signalCost != 0)
        {
            FormatArguments args{};
            args.push<uint32_t>(cState.signalCost);

            auto point = Point(kWidth / 2, kHeight - kSpacing - 12); // drawn at the bottom of the window, centered
            tr.drawStringCentred(point, Colour::black, StringIds::build_cost, args);
        }
    }

    void tabReset(Window& self)
    {
        self.holdableWidgets = kHoldableWidgets;
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
