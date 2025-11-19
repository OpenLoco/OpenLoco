#include "Audio/Audio.h"
#include "Economy/Economy.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Terraform/ChangeLandMaterial.h"
#include "GameCommands/Terraform/ClearLand.h"
#include "GameCommands/Terraform/CreateTree.h"
#include "GameCommands/Terraform/CreateWall.h"
#include "GameCommands/Terraform/LowerLand.h"
#include "GameCommands/Terraform/LowerRaiseLandMountain.h"
#include "GameCommands/Terraform/LowerWater.h"
#include "GameCommands/Terraform/RaiseLand.h"
#include "GameCommands/Terraform/RaiseWater.h"
#include "GameCommands/Terraform/RemoveTree.h"
#include "GameCommands/Terraform/RemoveWall.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/MapSelection.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/Tree.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Objects/WallObject.h"
#include "Objects/WaterObject.h"
#include "Scenario.h"
#include "SceneManager.h"
#include "Ui/Dropdown.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/ColourButtonWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Trigonometry.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Ui::Windows::Terraform
{

    namespace Common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_clear_area,
            tab_adjust_land,
            tab_adjust_water,
            tab_plant_trees,
            tab_build_walls,
        };

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                Widgets::Caption({ 1, 1 }, { frameWidth - 2, 13 }, Widgets::Caption::Style::colourText, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { 130, 74 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_clear_land),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_adjust_land),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_adjust_water),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_plant_trees),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_walls));
        }

        static void switchTab(Window& self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx);
        static void prepareDraw(Window& self);
        static void onUpdate(Window& self);
        static void onResize(Window& self, uint8_t height);
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id);
        static void sub_4A69DD();

        enum class GhostPlacedFlags : uint8_t
        {
            none = 0U,
            tree = 1 << 0,
            wall = 1 << 1,
        };
        OPENLOCO_ENABLE_ENUM_OPERATORS(GhostPlacedFlags);
    }

    static int16_t _adjustToolSize;                             // 0x0050A000
    static uint8_t _adjustLandToolSize;                         // 0x009C870E
    static uint8_t _clearAreaToolSize;                          // 0x009C870F
    static uint8_t _adjustWaterToolSize;                        // 0x009C8710
    static uint8_t _lastSelectedLand;                           // 0x00F003D2
    static uint32_t _raiseLandCost;                             // 0x00F2530C
    static uint32_t _lowerLandCost;                             // 0x00F25310
    static World::TreeElement* _lastPlacedTree;                 // 0x01136470
    static uint32_t _lastTreeCost;                              // 0x01136484
    static World::Pos2 _terraformGhostPos;                      // 0x01136488
    static uint16_t _lastTreeColourFlag;                        // 0x01136490
    static uint16_t _terraformGhostTreeRotationFlag;            // 0x01136492
    static uint8_t _treeRotation;                               // 0x01136496
    static Colour _treeColour;                                  // 0x01136497
    static uint8_t _terraformGhostBaseZ;                        // 0x01136499
    static Common::GhostPlacedFlags _terraformGhostPlacedFlags; // 0x0113649A
    static uint8_t _terraformGhostTreeElementType;              // 0x0113649B
    static uint8_t _terraformGhostType;                         // 0x0113649C
    static uint8_t _terraformGhostQuadrant;                     // 0x0113649D (trees)
    static uint8_t _terraformGhostRotation;                     // 0x0113649D (walls)
    static uint8_t _treeClusterType;                            // 0x0113649E
    static uint32_t _lowerWaterCost;                            // 0x01136528
    static uint32_t _raiseWaterCost;                            // 0x0113652C

    namespace PlantTrees
    {
        static constexpr Ui::Size32 kWindowSize = { 634, 162 };

        static constexpr uint8_t kRowHeight = 102;
        static constexpr uint8_t kColumnWidth = 66;

        enum widx
        {
            scrollview = 9,
            rotate_object,
            object_colour,
            plant_cluster_selected,
            plant_cluster_random,
        };

        const uint64_t holdableWidgets = 0;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(634, 162, StringIds::title_plant_trees),
            Widgets::ScrollView({ 3, 45 }, { 605, 101 }, WindowColour::secondary, Scrollbars::vertical),
            Widgets::ImageButton({ 609, 46 }, { 24, 24 }, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_object_90),
            Widgets::ColourButton({ 609, 70 }, { 24, 24 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_object_colour),
            Widgets::ImageButton({ 609, 94 }, { 24, 24 }, WindowColour::secondary, ImageIds::plant_cluster_selected_tree, StringIds::plant_cluster_selected_tree),
            Widgets::ImageButton({ 609, 118 }, { 24, 24 }, WindowColour::secondary, ImageIds::plant_cluster_random_tree, StringIds::plant_cluster_random_tree)

        );

        enum treeCluster
        {
            none = 0,
            selected,
            random,
        };

        // 0x004BB6B2
        static void updateTreeColours(Window& self)
        {
            if (self.rowHover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self.rowHover);
                if (treeObj->colours != 0)
                {
                    auto bit = Numerics::bitScanReverse(treeObj->colours);
                    auto colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                    _treeColour = colour;
                }
            }
        }

        // 0x004BC4B7
        static void updateActiveThumb(Window& self)
        {
            int32_t scrollWidth = 0, scrollHeight = 0;
            self.callGetScrollSize(0, scrollWidth, scrollHeight);
            self.scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self.var_83C; i++)
            {
                if (self.rowInfo[i] == self.rowHover)
                {
                    break;
                }
            }

            if (i >= self.var_83C)
            {
                i = 0;
            }

            i = (i / 9) * kRowHeight;

            self.scrollAreas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB63F
        static void refreshTreeList(Window& self)
        {
            auto treeCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::tree); i++)
            {
                auto treeObj = ObjectManager::get<TreeObject>(i);
                if (treeObj == nullptr)
                {
                    continue;
                }
                self.rowInfo[treeCount] = i;
                treeCount++;
            }

            self.var_83C = treeCount;
            auto rowHover = -1;

            if (getGameState().lastTreeOption != 0xFF)
            {
                for (auto i = 0; i < self.var_83C; i++)
                {
                    if (getGameState().lastTreeOption == self.rowInfo[i])
                    {
                        rowHover = getGameState().lastTreeOption;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self.var_83C != 0)
            {
                rowHover = self.rowInfo[0];
            }

            self.rowHover = rowHover;

            updateActiveThumb(self);
            updateTreeColours(self);
        }

        static void removeTreeGhost();

        // 0x004BBB0A
        static void onClose([[maybe_unused]] Window& self)
        {
            removeTreeGhost();
            Ui::Windows::Main::hideGridlines();
        }

        // 0x004BBC7D
        static void tabReset(Window& self)
        {
            ToolManager::toolSet(self, Common::widx::panel, CursorId::plantTree);
            Input::setFlag(Input::Flags::flag6);
            _terraformGhostPlacedFlags = Common::GhostPlacedFlags::none;
            _lastTreeCost = 0x80000000;
            self.var_83C = 0;
            self.rowHover = -1;
            refreshTreeList(self);
            updateTreeColours(self);
        }

        // 0x004BBAB5
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_adjust_land:
                case Common::widx::tab_adjust_water:
                case Common::widx::tab_build_walls:
                case Common::widx::tab_clear_area:
                case Common::widx::tab_plant_trees:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::rotate_object:
                {
                    _treeRotation++;
                    _treeRotation &= 3;
                    self.invalidate();
                    break;
                }

                case widx::plant_cluster_selected:
                {
                    if (_treeClusterType == treeCluster::selected)
                    {
                        _treeClusterType = treeCluster::none;
                    }
                    else
                    {
                        _treeClusterType = treeCluster::selected;
                    }
                    self.invalidate();
                    break;
                }

                case widx::plant_cluster_random:
                {
                    if (_treeClusterType == treeCluster::random)
                    {
                        _treeClusterType = treeCluster::none;
                    }
                    else
                    {
                        _treeClusterType = treeCluster::random;
                    }
                    self.invalidate();
                }
            }
        }

        // 0x004BBFBD
        static void onResize(Window& self)
        {
            self.invalidate();
            Ui::Size32 kMinWindowSize = { self.minWidth, self.minHeight };
            Ui::Size32 kMaxWindowSize = { self.maxWidth, self.maxHeight };
            bool hasResized = self.setSize(kMinWindowSize, kMaxWindowSize);
            if (hasResized)
            {
                updateActiveThumb(self);
            }
        }

        // 0x004BBAEA
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            if (widgetIndex == widx::object_colour && self.rowHover != -1)
            {
                auto obj = ObjectManager::get<TreeObject>(self.rowHover);
                Dropdown::showColour(&self, &self.widgets[widgetIndex], obj->colours, _treeColour, self.getColour(WindowColour::secondary));
            }
        }

        // 0x004BBAF5
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
            {
                return;
            }
            if (itemIndex == -1)
            {
                return;
            }

            _treeColour = static_cast<Colour>(Dropdown::getHighlightedItem());
            self.invalidate();
        }

        // 0x004BBDA5
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
            {
                WindowManager::close(&self);
            }

            if (ToolManager::getToolWindowType() != WindowType::terraform)
            {
                WindowManager::close(&self);
            }

            if (!Input::hasFlag(Input::Flags::rightMousePressed))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == &self)
                {
                    xPos -= self.x;
                    xPos += 26;
                    yPos -= self.y;

                    if ((yPos < 42) || (xPos <= self.width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self.findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self.expandContentCounter += 1;
                            if (self.expandContentCounter >= 8)
                            {
                                auto y = std::min(self.scrollAreas[0].contentHeight - 1 + 60, 562);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 358);
                                }
                                self.minWidth = kWindowSize.width;
                                self.minHeight = y;
                                self.maxWidth = kWindowSize.width;
                                self.maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self.minWidth = kWindowSize.width;
                                    self.minHeight = kWindowSize.height;
                                    self.maxWidth = kWindowSize.width;
                                    self.maxHeight = kWindowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self.expandContentCounter = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self.minWidth = kWindowSize.width;
                        self.minHeight = kWindowSize.height;
                        self.maxWidth = kWindowSize.width;
                        self.maxHeight = kWindowSize.height;
                    }
                }
            }
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self.number, self.currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BBEDF
        static void event_08(Window& self)
        {
            if (self.var_846 != 0xFFFFU)
            {
                self.var_846 = 0xFFFFU;
                self.invalidate();
            }
        }
        // 0x004BD297 (bits of)
        static void removeTreeGhost()
        {
            if ((_terraformGhostPlacedFlags & Common::GhostPlacedFlags::tree) != Common::GhostPlacedFlags::none)
            {
                _terraformGhostPlacedFlags = _terraformGhostPlacedFlags & ~Common::GhostPlacedFlags::tree;
                GameCommands::TreeRemovalArgs args;
                args.pos = World::Pos3((_terraformGhostPos).x, (_terraformGhostPos).y, _terraformGhostBaseZ * World::kSmallZStep);
                args.type = _terraformGhostType;
                args.elementType = _terraformGhostTreeElementType;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
        }

        // 0x004BD237
        static currency32_t placeTreeGhost(const GameCommands::TreePlacementArgs& placementArgs)
        {
            removeTreeGhost();

            auto res = GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            if (res != GameCommands::FAILURE)
            {
                _terraformGhostPos = placementArgs.pos;
                _terraformGhostTreeElementType = (_lastPlacedTree)->rawData()[0];
                _terraformGhostType = placementArgs.type;
                _terraformGhostBaseZ = (_lastPlacedTree)->baseZ();
                _terraformGhostPlacedFlags |= Common::GhostPlacedFlags::tree;

                _terraformGhostQuadrant = placementArgs.quadrant;
                _terraformGhostTreeRotationFlag = placementArgs.rotation | (placementArgs.buildImmediately ? 0x8000 : 0);
            }
            return res;
        }

        // 0x004BD1D9
        static std::optional<GameCommands::TreePlacementArgs> getTreePlacementArgsFromCursor(const int16_t x, const int16_t y)
        {
            auto* self = WindowManager::find(WindowType::terraform);
            if (self == nullptr)
            {
                return {};
            }

            auto res = ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (!res)
            {
                return {};
            }

            if (self->rowHover == -1)
            {
                return {};
            }

            GameCommands::TreePlacementArgs args;
            // 0 for Z value means game command finds first available height
            args.pos = World::Pos3(res->first.x & 0xFFE0, res->first.y & 0xFFE0, 0);
            args.type = self->rowHover;
            args.quadrant = World::getQuadrantFromPos(res->first) ^ (1 << 1);
            args.colour = _treeColour;
            args.rotation = (_treeRotation - WindowManager::getCurrentRotation()) & 0x3;
            if (SceneManager::isEditorMode())
            {
                args.buildImmediately = true;
            }
            return { args };
        }

        // 0x004BBB15
        static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
            auto placementArgs = getTreePlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeTreeGhost();
                return;
            }

            auto cornerValue = enumValue(MapSelectionType::quarter0) + (placementArgs->quadrant ^ (1 << 1));
            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            World::setMapSelectionCorner(static_cast<MapSelectionType>(cornerValue));
            World::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            World::mapInvalidateSelectionRect();

            if ((_terraformGhostPlacedFlags & Common::GhostPlacedFlags::tree) != Common::GhostPlacedFlags::none)
            {
                if (_terraformGhostPos == placementArgs->pos
                    && _terraformGhostQuadrant == placementArgs->quadrant
                    && _terraformGhostType == placementArgs->type
                    && _terraformGhostTreeRotationFlag == (placementArgs->rotation | (placementArgs->buildImmediately ? 0x8000 : 0)))
                {
                    return;
                }
            }

            removeTreeGhost();
            _terraformGhostQuadrant = placementArgs->quadrant;
            _terraformGhostTreeRotationFlag = placementArgs->rotation | (placementArgs->buildImmediately ? 0x8000 : 0);
            _lastTreeCost = placeTreeGhost(*placementArgs);
        }

        // 0x004BBB20
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            removeTreeGhost();
            auto placementArgs = getTreePlacementArgsFromCursor(x, y);
            if (placementArgs)
            {
                GameCommands::setErrorTitle(StringIds::cant_plant_this_here);
                switch (_treeClusterType)
                {
                    case treeCluster::none:
                        if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply) != GameCommands::FAILURE)
                        {
                            Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
                        }
                        break;
                    case treeCluster::selected:
                    {
                        auto previousId = GameCommands::getUpdatingCompanyId();
                        if (SceneManager::isEditorMode())
                        {
                            GameCommands::setUpdatingCompanyId(CompanyId::neutral);
                        }

                        if (World::placeTreeCluster(World::toTileSpace(placementArgs->pos), 320, 3, placementArgs->type))
                        {
                            auto height = TileManager::getHeight(placementArgs->pos);
                            Audio::playSound(Audio::SoundId::construct, World::Pos3{ placementArgs->pos.x, placementArgs->pos.y, height.landHeight });
                        }
                        else
                        {
                            Error::open(StringIds::cant_plant_this_here, StringIds::empty);
                        }

                        if (SceneManager::isEditorMode())
                        {
                            GameCommands::setUpdatingCompanyId(previousId);
                        }
                        break;
                    }
                    case treeCluster::random:
                        auto previousId = GameCommands::getUpdatingCompanyId();
                        if (SceneManager::isEditorMode())
                        {
                            GameCommands::setUpdatingCompanyId(CompanyId::neutral);
                        }

                        if (World::placeTreeCluster(World::toTileSpace(placementArgs->pos), 384, 4, std::nullopt))
                        {
                            auto height = TileManager::getHeight(placementArgs->pos);
                            Audio::playSound(Audio::SoundId::construct, World::Pos3{ placementArgs->pos.x, placementArgs->pos.y, height.landHeight });
                        }
                        else
                        {
                            Error::open(StringIds::cant_plant_this_here, StringIds::empty);
                        }

                        if (SceneManager::isEditorMode())
                        {
                            GameCommands::setUpdatingCompanyId(previousId);
                        }
                        break;
                }
            }
        }

        static void onToolAbort([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            removeTreeGhost();
        }

        // 0x004BBEC1
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            scrollHeight = (self.var_83C + 8) / 9;
            if (scrollHeight == 0)
            {
                scrollHeight += 1;
            }
            scrollHeight *= kRowHeight;
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / kColumnWidth) + (y / kRowHeight) * 9;
        }

        // 0x004BBF3B
        static void scrollMouseDown(Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self.var_83C; i++)
            {
                auto rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    self.rowHover = rowInfo;
                    getGameState().lastTreeOption = static_cast<uint8_t>(rowInfo);

                    updateTreeColours(self);

                    int32_t pan = (self.width >> 1) + self.x;
                    Audio::playSound(Audio::SoundId::clickDown, pan);
                    self.expandContentCounter = -16;
                    _lastTreeCost = 0x80000000;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BBEF8
        static void scrollMouseOver(Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = y;
            auto i = 0;
            for (; i < self.var_83C; i++)
            {
                rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    self.var_846 = rowInfo;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BBB00
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_trees_list);
            return args;
        }

        // 0x004BB756
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.activatedWidgets &= ~((1ULL << widx::plant_cluster_selected) | (1ULL << widx::plant_cluster_random));

            if (_treeClusterType == treeCluster::selected)
            {
                self.activatedWidgets |= (1ULL << widx::plant_cluster_selected);
            }

            if (_treeClusterType == treeCluster::random)
            {
                self.activatedWidgets |= (1ULL << widx::plant_cluster_random);
            }

            self.widgets[widx::rotate_object].hidden = true;
            self.widgets[widx::object_colour].hidden = true;

            if (self.rowHover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self.rowHover);
                if (treeObj->name != 0xFFFF)
                {
                    if (treeObj->numRotations != 1)
                    {
                        self.widgets[widx::rotate_object].hidden = false;
                    }

                    if (treeObj->colours != 0)
                    {
                        self.widgets[widx::object_colour].image = Widget::kImageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, _treeColour);
                        self.widgets[widx::object_colour].hidden = false;
                    }
                }
            }

            self.widgets[widx::scrollview].right = self.width - 26;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            self.widgets[widx::rotate_object].left = self.width - 25;
            self.widgets[widx::object_colour].left = self.width - 25;
            self.widgets[widx::plant_cluster_selected].left = self.width - 25;
            self.widgets[widx::plant_cluster_random].left = self.width - 25;

            self.widgets[widx::rotate_object].right = self.width - 2;
            self.widgets[widx::object_colour].right = self.width - 2;
            self.widgets[widx::plant_cluster_selected].right = self.width - 2;
            self.widgets[widx::plant_cluster_random].right = self.width - 2;

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BB8C9
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);

            auto treeId = self.var_846;
            if (treeId == 0xFFFF)
            {
                treeId = self.rowHover;
                if (treeId == 0xFFFF)
                {
                    return;
                }
            }

            auto treeObj = ObjectManager::get<TreeObject>(treeId);

            uint32_t treeCost = 0x80000000;
            if (self.var_846 == 0xFFFF)
            {
                treeCost = _lastTreeCost;
                if (treeCost == 0x80000000)
                {
                    treeCost = Economy::getInflationAdjustedCost(treeObj->buildCostFactor, treeObj->costIndex, 12);
                }
            }
            else
            {
                treeCost = Economy::getInflationAdjustedCost(treeObj->buildCostFactor, treeObj->costIndex, 12);
            }

            if (!SceneManager::isEditorMode())
            {
                FormatArguments args{};
                args.push<uint32_t>(treeCost);

                auto point = Point(self.x + 3 + self.width - 17, self.y + self.height - 13);
                tr.drawStringRight(point, Colour::black, StringIds::build_cost, args);
            }

            {
                FormatArguments args{};
                args.push(treeObj->name);

                auto point = Point(self.x + 3, self.y + self.height - 13);
                auto width = self.width - 19 - point.x;
                tr.drawStringLeftClipped(point, width, Colour::black, StringIds::black_stringid, args);
            }
        }

        static void drawTreeThumb(const TreeObject* treeObj, Gfx::DrawingContext& drawingCtx)
        {
            uint32_t image = treeObj->getTreeGrowthDisplayOffset() * treeObj->numRotations;
            auto rotation = (treeObj->numRotations - 1) & _treeRotation;
            image += rotation;
            image += treeObj->sprites[treeObj->seasonState];

            auto colourOptions = treeObj->colours;
            if (colourOptions != 0)
            {
                auto colour = _treeColour;
                if (!(_lastTreeColourFlag & (1 << 5)))
                {
                    auto bit = Numerics::bitScanReverse(colourOptions);
                    colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                }
                image = Gfx::recolour(image, colour);
            }
            drawingCtx.drawImage(32, 96, image);
        }

        // 0x004BB982
        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            const auto& rt = drawingCtx.currentRenderTarget();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                _lastTreeColourFlag = 0xFFFF;
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        _lastTreeColourFlag = AdvancedColour::translucentFlag;
                        drawingCtx.drawRectInset(xPos, yPos, 65, kRowHeight - 1, self.getColour(WindowColour::secondary), Gfx::RectInsetFlags::colourLight);
                    }
                }
                else
                {
                    _lastTreeColourFlag = AdvancedColour::translucentFlag | AdvancedColour::outlineFlag;
                    drawingCtx.drawRectInset(xPos, yPos, 65, kRowHeight - 1, self.getColour(WindowColour::secondary), (Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::borderInset));
                }

                const auto* treeObj = ObjectManager::get<TreeObject>(self.rowInfo[i]);
                auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(xPos + 1, yPos + 1, 64, kRowHeight - 2));
                if (clipped)
                {
                    drawingCtx.pushRenderTarget(*clipped);

                    drawTreeThumb(treeObj, drawingCtx);

                    drawingCtx.popRenderTarget();
                }

                xPos += kColumnWidth;

                if (xPos >= kColumnWidth * 9) // full row
                {
                    xPos = 0;
                    yPos += kRowHeight;
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .event_08 = event_08,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .onToolAbort = onToolAbort,
            .getScrollSize = getScrollSize,
            .scrollMouseDown = scrollMouseDown,
            .scrollMouseOver = scrollMouseOver,
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

    // 0x004BB4A3
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::terraform, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_plant_trees, window->widgets[Common::widx::tab_plant_trees].id);
        }
        else
        {
            // 0x004BB586
            window = WindowManager::createWindow(
                WindowType::terraform,
                { Ui::width() - PlantTrees::kWindowSize.width, 30 },
                PlantTrees::kWindowSize,
                WindowFlags::flag_11,
                PlantTrees::getEvents());

            window->number = 0;
            window->currentTab = Common::widx::tab_plant_trees - Common::widx::tab_clear_area;
            window->frameNo = 0;
            _terraformGhostPlacedFlags = Common::GhostPlacedFlags::none;
            _lastTreeCost = 0x80000000;
            window->owner = CompanyManager::getControllingId();
            window->var_846 = 0xFFFF;
            window->expandContentCounter = 0;
            _treeClusterType = PlantTrees::treeCluster::none;

            WindowManager::moveOtherWindowsDown(*window);

            window->minWidth = PlantTrees::kWindowSize.width;
            window->minHeight = PlantTrees::kWindowSize.height;
            window->maxWidth = PlantTrees::kWindowSize.width;
            window->maxHeight = PlantTrees::kWindowSize.height;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->windowTerraFormColour);

            // End of 0x004BB586

            Ui::Windows::Main::showGridlines();
            _treeRotation = 2;

            window->invalidate();

            window->setWidgets(PlantTrees::widgets);
            window->holdableWidgets = 0;
            window->activatedWidgets = 0;

            window->disabledWidgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();

            window->var_83C = 0;
            window->rowHover = -1;

            PlantTrees::refreshTreeList(*window);

            ToolManager::toolSet(*window, Common::widx::panel, CursorId::landTool);

            Input::setFlag(Input::Flags::flag6);
        }
        return window;
    }

    namespace ClearArea
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
        };

        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(130, 105, StringIds::clear_area),
            Widgets::Wt3Widget({ 33 + 16, 45 }, { 64, 44 }, WindowColour::secondary, ImageIds::tool_area, StringIds::tooltip_clear_area),
            Widgets::ImageButton({ 34 + 16, 46 }, { 16, 16 }, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_clear_area),
            Widgets::ImageButton({ 80 + 16, 72 }, { 16, 16 }, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_clear_area)

        );

        // 0x004BC671
        static void onClose([[maybe_unused]] Window& self)
        {
            Ui::Windows::Main::hideGridlines();
        }

        // 0x004BBBC7
        static void tabReset(Window& self)
        {
            ToolManager::toolSet(self, Common::widx::panel, CursorId::bulldozerTool);
            Input::setFlag(Input::Flags::flag6);
            _raiseLandCost = 0x80000000;
            _adjustToolSize = _clearAreaToolSize;
        }

        // 0x004BC7C6
        static void onResize(Window& self)
        {
            Common::onResize(self, 105);
        }

        // 0x004BC65C
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                    {
                        _adjustToolSize = 1;
                    }
                    _clearAreaToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 64)
                    {
                        _adjustToolSize = 64;
                    }
                    _clearAreaToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BC677
        static void onToolUpdate([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);

            uint32_t cost = 0x80000000;
            auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (res)
            {
                if (setMapSelectionTiles(res->first, MapSelectionType::full, _adjustToolSize) == 0)
                {
                    return;
                }
                const auto [pointA, pointB] = World::getMapSelectionArea();
                const Pos2 centre = (pointA + pointB) / 2;

                ClearLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                cost = GameCommands::doCommand(args, GameCommands::Flags::allowNegativeCashFlow | GameCommands::Flags::ghost);
            }

            if (cost != _raiseLandCost)
            {
                _raiseLandCost = cost;
                WindowManager::invalidate(WindowType::terraform);
            }
        }

        static void clearLand(uint8_t flags)
        {
            if (World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
            {
                auto [pointA, pointB] = World::getMapSelectionArea();
                Pos2 centre = (pointA + pointB) / 2;
                GameCommands::setErrorTitle(StringIds::error_cant_clear_entire_area);

                ClearLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                GameCommands::doCommand(args, flags);
            }
        }

        // 0x004BC689
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            clearLand(Flags::apply);
        }

        // 0x004BC682
        static void toolDrag([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }

            auto window = WindowManager::find(WindowType::error);
            if (window == nullptr)
            {
                clearLand(Flags::apply);
            }
        }

        // 0x004BC701
        static void toolUp([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex == Common::widx::panel)
            {
                World::mapInvalidateSelectionRect();
                World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
            }
        }

        // 0x004BC555
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.activatedWidgets |= (1ULL << widx::tool_area);

            if (_adjustToolSize <= 10)
            {
                self.widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;
            }
            else
            {
                self.widgets[widx::tool_area].image = Widget::kContentNull;
            }

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BC5E7
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);

            auto& toolArea = self.widgets[widx::tool_area];

            // Draw as a number if we can't fit a sprite
            if (_adjustToolSize > 10)
            {
                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.midY() + self.y - 5;
                auto point = Point(xPos, yPos);

                FormatArguments args{};
                args.push<uint16_t>(_adjustToolSize);
                tr.drawStringCentred(point, Colour::black, StringIds::tile_inspector_coord, args);
            }

            if (_raiseLandCost == 0x80000000)
            {
                return;
            }

            if (_raiseLandCost == 0)
            {
                return;
            }

            {
                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.bottom + self.y + 5;
                auto point = Point(xPos, yPos);

                FormatArguments args{};
                args.push<uint32_t>(_raiseLandCost);

                tr.drawStringCentred(point, Colour::black, StringIds::clear_land_cost, args);
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onUpdate = Common::onUpdate,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .toolDrag = toolDrag,
            .toolUp = toolUp,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace AdjustLand
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
            mountain_mode,
            paint_mode,
            land_material
        };

        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);
        static bool isMountainMode = false;
        static bool isPaintMode = false;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(130, 105, StringIds::title_adjust_land),
            Widgets::Wt3Widget({ 49, 45 }, { 64, 44 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_adjust_land_tool),
            Widgets::ImageButton({ 50, 46 }, { 16, 16 }, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_land_area),
            Widgets::ImageButton({ 96, 72 }, { 16, 16 }, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_land_area),
            Widgets::ImageButton({ 57, 92 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_slope_up, StringIds::mountainModeTooltip),
            Widgets::ImageButton({ 83, 92 }, { 24, 24 }, WindowColour::secondary, ImageIds::paintbrush, StringIds::tooltip_paint_landscape_tool),
            Widgets::ImageButton({ 112, 94 }, { 20, 20 }, WindowColour::primary)

        );

        // 0x004BC9D1
        static void onClose([[maybe_unused]] Window& self)
        {
            Ui::Windows::Main::hideGridlines();
        }

        // 0x004BBBF7
        static void tabReset(Window& self)
        {
            if (isPaintMode)
            {
                ToolManager::toolSet(self, widx::paint_mode, CursorId::landTool);
            }
            else if (isMountainMode)
            {
                ToolManager::toolSet(self, widx::mountain_mode, CursorId::landTool);
            }
            else
            {
                ToolManager::toolSet(self, Common::widx::panel, CursorId::landTool);
            }

            Input::setFlag(Input::Flags::flag6);
            for (auto i = 0; i < 32; i++)
            {
                auto landObj = ObjectManager::get<LandObject>(i);
                if (landObj == nullptr)
                {
                    continue;
                }

                _lastSelectedLand = i;
                _raiseLandCost = 0x80000000;
                _lowerLandCost = 0x80000000;
                _adjustToolSize = _adjustLandToolSize;
                break;
            }
        }

        // 0x004BCBF8
        static void onResize(Window& self)
        {
            if (SceneManager::isEditorMode())
            {
                Common::onResize(self, 115);
            }
            else
            {
                // CHANGE: Resizes window to allow Dropdown and cost string to be drawn separately
                Common::onResize(self, 140);
            }
        }

        // 0x004BCB47
        static void showDropdown(Window* self, WidgetIndex_t widgetIndex)
        {
            auto landCount = 0;
            for (auto i = 0; i < 32; i++)
            {
                auto landObj = ObjectManager::get<LandObject>(i);
                if (landObj != nullptr)
                {
                    landCount++;
                }
            }

            auto xPos = self->widgets[widgetIndex].left + self->x;
            auto yPos = self->widgets[widgetIndex].bottom + self->y;
            auto heightOffset = self->widgets[widgetIndex].height() - 18;
            auto colour = self->getColour(WindowColour::secondary).translucent();
            auto count = Dropdown::getItemsPerRow(landCount);

            Dropdown::showImage(xPos, yPos, 20, 20, heightOffset, colour, count, landCount);

            auto landIndex = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::land); i++)
            {
                auto landObj = ObjectManager::get<LandObject>(i);
                if (landObj == nullptr)
                {
                    continue;
                }

                if (i == _lastSelectedLand)
                {
                    Dropdown::setHighlightedItem(landIndex);
                }

                auto args = FormatArguments::common();
                args.push(landObj->mapPixelImage + Land::ImageIds::landscape_generator_tile_icon);
                args.push<uint16_t>(i);

                Dropdown::add(landIndex, 0xFFFE, args);

                landIndex++;
            }
        }

        // 0x004BC9A7
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case widx::land_material:
                {
                    showDropdown(&self, widgetIndex);
                    break;
                }

                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                    {
                        _adjustToolSize = 1;
                    }
                    _adjustLandToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 64)
                    {
                        _adjustToolSize = 64;
                    }
                    _adjustLandToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }
            }
        }

        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_adjust_land:
                case Common::widx::tab_adjust_water:
                case Common::widx::tab_build_walls:
                case Common::widx::tab_clear_area:
                case Common::widx::tab_plant_trees:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::mountain_mode:
                {
                    isMountainMode = !isMountainMode;
                    isPaintMode = false;
                    tabReset(self);
                    self.invalidate();
                    break;
                }

                case widx::paint_mode:
                {
                    isMountainMode = false;
                    isPaintMode = !isPaintMode;
                    tabReset(self);
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BC9C6
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
        {
            if (widgetIndex != widx::land_material)
            {
                return;
            }
            if (itemIndex == -1)
            {
                return;
            }
            _lastSelectedLand = Dropdown::getItemArgument(itemIndex, 2);
            self.invalidate();
        }

        // 0x00468DFD
        static uint32_t lowerLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
            {
                Common::sub_4A69DD();
            }

            auto [pointA, pointB] = World::getMapSelectionArea();
            auto centre = (pointA + pointB) / 2;
            GameCommands::setErrorTitle(StringIds::error_cant_lower_land_here);

            if (isMountainMode)
            {
                GameCommands::LowerRaiseLandMountainArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                args.adjustment = -1;
                cost = GameCommands::doCommand(args, flags);
            }
            else
            {
                GameCommands::LowerLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                args.corner = World::getMapSelectionCorner();
                cost = GameCommands::doCommand(args, flags);
            }
            return cost;
        }

        // 0x00468D1D
        static uint32_t raiseLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
            {
                Common::sub_4A69DD();
            }

            auto [pointA, pointB] = World::getMapSelectionArea();
            auto centre = (pointA + pointB) / 2;
            GameCommands::setErrorTitle(StringIds::error_cant_raise_land_here);

            if (isMountainMode)
            {
                GameCommands::LowerRaiseLandMountainArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                args.adjustment = 1;
                cost = GameCommands::doCommand(args, flags);
            }
            else
            {
                GameCommands::RaiseLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                args.corner = World::getMapSelectionCorner();
                cost = GameCommands::doCommand(args, flags);
            }
            return cost;
        }

        static void setAdjustCost(uint32_t raiseCost, uint32_t lowerCost)
        {
            if (_raiseLandCost == raiseCost)
            {
                if (_lowerLandCost == lowerCost)
                {
                    return;
                }
            }

            _raiseLandCost = raiseCost;
            _lowerLandCost = lowerCost;

            WindowManager::invalidate(WindowType::terraform, 0);
        }

        static void onPaintToolUpdate([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);

            uint32_t cost = 0x80000000;
            auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (res)
            {
                if (setMapSelectionTiles(res->first, MapSelectionType::full, _adjustToolSize) == 0)
                {
                    return;
                }
                const auto [pointA, pointB] = World::getMapSelectionArea();
                const Pos2 centre = (pointA + pointB) / 2;

                ClearLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                cost = GameCommands::doCommand(args, GameCommands::Flags::allowNegativeCashFlow | GameCommands::Flags::ghost);
            }

            if (cost != _raiseLandCost)
            {
                _raiseLandCost = cost;
                WindowManager::invalidate(WindowType::terraform);
            }
        }

        static void onAdjustLandToolUpdate([[maybe_unused]] const OpenLoco::Ui::WidgetIndex_t& widgetIndex, const int16_t& x, const int16_t& y)
        {
            uint16_t xPos = 0;

            World::mapInvalidateSelectionRect();

            if (ToolManager::getToolCursor() != CursorId::upDownArrow)
            {
                World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
                auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
                if (res)
                {
                    if (_adjustLandToolSize == 1 && !(isMountainMode || isPaintMode))
                    {
                        auto count = setMapSelectionSingleTile(res->first, true);

                        if (!count)
                        {
                            return;
                        }
                    }
                    else
                    {
                        auto count = setMapSelectionTiles(res->first, MapSelectionType::full, _adjustToolSize);

                        if (!count)
                        {
                            return;
                        }
                    }
                }
                else
                {
                    xPos = 0x8000;
                }
            }
            else
            {
                if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
                {
                    return;
                }
            }

            uint32_t raiseCost = 0;
            uint32_t lowerCost = 0;

            if (SceneManager::isEditorMode() || xPos == 0x8000)
            {
                raiseCost = 0x80000000;
                lowerCost = 0x80000000;
            }
            else
            {
                lowerCost = lowerLand(Flags::allowNegativeCashFlow);
                raiseCost = raiseLand(Flags::allowNegativeCashFlow);
            }
            setAdjustCost(raiseCost, lowerCost);
        }

        // 0x004BC9D7
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            switch (widgetIndex)
            {
                case widx::paint_mode:
                    onPaintToolUpdate(self, widgetIndex, x, y);
                    break;

                case widx::mountain_mode:
                case Common::widx::panel:
                    onAdjustLandToolUpdate(widgetIndex, x, y);
                    break;
            }
        }

        static void paintLand()
        {
            // CHANGE: Allows the player to change land type outside of the scenario editor.
            if (_adjustToolSize != 0)
            {
                if (_lastSelectedLand != 0xFF)
                {
                    GameCommands::setErrorTitle(StringIds::error_cant_change_land_type);
                    auto [pointA, pointB] = World::getMapSelectionArea();

                    ChangeLandMaterialArgs args{};
                    args.pointA = pointA;
                    args.pointB = pointB;
                    args.landType = _lastSelectedLand;
                    GameCommands::doCommand(args, Flags::apply);
                }
            }
        }

        // 0x004BC9ED
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            switch (widgetIndex)
            {
                case widx::paint_mode:
                    paintLand();
                    break;

                case widx::mountain_mode:
                case Common::widx::panel:
                {
                    if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
                    {
                        return;
                    }

                    ToolManager::setToolCursor(CursorId::upDownArrow);
                    break;
                }
            }
        }

        // 0x004BC9E2
        static void toolDrag([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            switch (widgetIndex)
            {
                case widx::paint_mode:
                {
                    auto window = WindowManager::find(WindowType::error);
                    if (window == nullptr)
                    {
                        paintLand();
                    }
                    break;
                }

                case widx::mountain_mode:
                case Common::widx::panel:
                {
                    auto window = WindowManager::findAt(x, y);
                    if (window == nullptr)
                    {
                        break;
                    }

                    WidgetIndex_t newWidgetIndex = window->findWidgetAt(x, y);
                    if (newWidgetIndex == kWidgetIndexNull)
                    {
                        break;
                    }

                    auto widget = window->widgets[newWidgetIndex];
                    if (widget.type != WidgetType::viewport)
                    {
                        break;
                    }

                    auto viewport = window->viewports[0];
                    if (viewport == nullptr)
                    {
                        break;
                    }

                    auto zoom = viewport->zoom;

                    auto dY = -(16 >> zoom);
                    if (dY == 0)
                    {
                        dY = -1;
                    }
                    auto deltaY = y - Input::getDragLastLocation().y;
                    auto flags = Flags::apply;

                    if (deltaY <= dY)
                    {
                        Input::setDragLastLocation(Input::getDragLastLocation() + Ui::Point{ 0, dY });
                        raiseLand(flags);
                    }
                    else
                    {
                        dY = -dY;
                        if (deltaY < dY)
                        {
                            break;
                        }
                        Input::setDragLastLocation(Input::getDragLastLocation() + Ui::Point{ 0, dY });
                        lowerLand(flags);
                    }
                    _raiseLandCost = 0x80000000;
                    _lowerLandCost = 0x80000000;
                    break;
                }
            }
        }

        // 0x004BCA5D
        static void toolUp([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            switch (widgetIndex)
            {
                case widx::mountain_mode:
                case widx::paint_mode:
                case Common::widx::panel:
                {
                    World::mapInvalidateSelectionRect();
                    World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
                    ToolManager::setToolCursor(CursorId::landTool);
                    break;
                }
            }
        }

        // 0x004BC83B
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.activatedWidgets |= (1ULL << widx::tool_area);

            if (isMountainMode)
            {
                self.activatedWidgets |= (1 << widx::mountain_mode);
            }
            else
            {
                self.activatedWidgets &= ~(1 << widx::mountain_mode);
            }

            if (isPaintMode)
            {
                self.activatedWidgets |= (1 << widx::paint_mode);
            }
            else
            {
                self.activatedWidgets &= ~(1 << widx::paint_mode);
            }

            auto landObj = ObjectManager::get<LandObject>(_lastSelectedLand);
            auto pixelColour = static_cast<Colour>(Gfx::getG1Element(landObj->mapPixelImage)->offset[0]);
            self.widgets[widx::paint_mode].image = Gfx::recolour2(ImageIds::paintbrush, Colour::white, pixelColour);

            if (isPaintMode)
            {
                self.widgets[widx::land_material].hidden = false;
                self.widgets[widx::land_material].image = landObj->mapPixelImage + OpenLoco::Land::ImageIds::landscape_generator_tile_icon;
            }
            else
            {
                self.widgets[widx::land_material].hidden = true;
            }

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BC909
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);

            Common::drawTabs(self, drawingCtx);

            auto& toolArea = self.widgets[widx::tool_area];

            // Draw land tool size as a grid
            if (_adjustToolSize < 10)
            {
                // For mountain mode, we first draw the background grid
                if (isMountainMode)
                {
                    auto areaImage = ImageId(ImageIds::tool_area);
                    Ui::Point placeForImage(toolArea.left + self.x, toolArea.top + self.y);

                    if ((_adjustToolSize & 1) == 0)
                    {
                        // For even sizes, we need to draw the image twice
                        // TODO: replace with proper grid images
                        placeForImage -= { 4, 0 };
                        drawingCtx.drawImage(placeForImage, areaImage);

                        placeForImage += { 8, 0 };
                        drawingCtx.drawImage(placeForImage, areaImage);
                    }
                    else
                    {
                        // For odd sizes, we just need the one
                        drawingCtx.drawImage(placeForImage, areaImage);
                    }
                }

                // Draw tool size
                if (!isMountainMode || _adjustToolSize > 1)
                {
                    auto areaImage = ImageId(ImageIds::tool_area).withIndexOffset(_adjustToolSize);
                    Ui::Point placeForImage(toolArea.left + self.x, toolArea.top + self.y);
                    drawingCtx.drawImage(placeForImage, areaImage);
                }
            }
            // Or draw as a number, if we can't fit a sprite
            else
            {
                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.midY() + self.y - 5;
                auto point = Point(xPos, yPos);

                FormatArguments args{};
                args.push<uint16_t>(_adjustToolSize);
                tr.drawStringCentred(point, Colour::black, StringIds::tile_inspector_coord, args);
            }

            auto xPos = toolArea.midX() + self.x;
            auto yPos = toolArea.bottom + self.y + 28;

            if (_raiseLandCost != 0x80000000)
            {
                if (_raiseLandCost != 0)
                {
                    FormatArguments args{};
                    args.push<uint32_t>(_raiseLandCost);

                    auto point = Point(xPos, yPos);
                    tr.drawStringCentred(point, Colour::black, StringIds::increase_height_cost, args);
                }
            }

            yPos += 10;

            if (_lowerLandCost != 0x80000000)
            {
                if (_lowerLandCost != 0)
                {
                    FormatArguments args{};
                    args.push<uint32_t>(_lowerLandCost);

                    auto point = Point(xPos, yPos);
                    tr.drawStringCentred(point, Colour::black, StringIds::decrease_height_cost, args);
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::onUpdate,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .toolDrag = toolDrag,
            .toolUp = toolUp,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace AdjustWater
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
        };

        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(130, 105, StringIds::title_adjust_water),
            Widgets::Wt3Widget({ 33 + 16, 45 }, { 64, 44 }, WindowColour::secondary, ImageIds::tool_area, StringIds::tooltip_adjust_water_tool),
            Widgets::ImageButton({ 34 + 16, 46 }, { 16, 16 }, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_water_area),
            Widgets::ImageButton({ 80 + 16, 72 }, { 16, 16 }, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_water_area)

        );

        // 0x004BCDAE
        static void onClose([[maybe_unused]] Window& self)
        {
            Ui::Windows::Main::hideGridlines();
        }

        // 0x004BBC46
        static void tabReset(Window& self)
        {
            ToolManager::toolSet(self, Common::widx::panel, CursorId::waterTool);
            Input::setFlag(Input::Flags::flag6);
            _raiseWaterCost = 0x80000000;
            _lowerWaterCost = 0x80000000;
            _adjustToolSize = _adjustWaterToolSize;
        }

        // 0x004BCEB4
        static void onResize(Window& self)
        {
            Common::onResize(self, 115);
        }

        // 0x004BCD9D
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                    {
                        _adjustToolSize = 1;
                    }
                    _adjustWaterToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 64)
                    {
                        _adjustToolSize = 64;
                    }
                    _adjustWaterToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }
            }
        }

        static void setAdjustCost(uint32_t raiseCost, uint32_t lowerCost)
        {
            if (_raiseWaterCost == raiseCost)
            {
                if (_lowerWaterCost == lowerCost)
                {
                    return;
                }
            }

            _raiseWaterCost = raiseCost;
            _lowerWaterCost = lowerCost;

            WindowManager::invalidate(WindowType::terraform);
        }

        static uint32_t raiseWater(uint8_t flags);
        static uint32_t lowerWater(uint8_t flags);

        // 0x004BCDB4
        static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            World::mapInvalidateSelectionRect();

            if (ToolManager::getToolCursor() != CursorId::upDownArrow)
            {
                World::resetMapSelectionFlag(World::MapSelectionFlags::enable);

                auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::surface | ViewportInteraction::InteractionItemFlags::water));
                auto& interaction = res.first;
                if (interaction.type == ViewportInteraction::InteractionItem::noInteraction)
                {
                    setAdjustCost(0x80000000, 0x80000000);
                    return;
                }
                if (!setMapSelectionTiles(interaction.pos + World::Pos2(16, 16), MapSelectionType::fullWater, _adjustToolSize))
                {
                    // no change in selection
                    return;
                }
            }
            else
            {
                if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
                {
                    return;
                }
            }

            if (SceneManager::isEditorMode())
            {
                setAdjustCost(0x80000000, 0x80000000);
            }
            else
            {
                setAdjustCost(raiseWater(0), lowerWater(0));
            }
        }

        // 0x004BCDCA
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            if (!World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
            {
                return;
            }

            ToolManager::setToolCursor(CursorId::upDownArrow);
        }

        static uint32_t raiseWater(uint8_t flags)
        {
            if (flags & GameCommands::Flags::apply)
            {
                Common::sub_4A69DD();
                GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            }

            auto [pointA, pointB] = World::getMapSelectionArea();
            GameCommands::RaiseWaterArgs args{};
            args.pointA = pointA;
            args.pointB = pointB;
            return GameCommands::doCommand(args, flags);
        }

        static uint32_t lowerWater(uint8_t flags)
        {
            if (flags & GameCommands::Flags::apply)
            {
                Common::sub_4A69DD();
                GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            }

            auto [pointA, pointB] = World::getMapSelectionArea();
            GameCommands::LowerWaterArgs args{};
            args.pointA = pointA;
            args.pointB = pointB;
            return GameCommands::doCommand(args, flags);
        }

        // 0x004BCDBF
        static void toolDrag([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }

            auto window = WindowManager::findAt(x, y);
            if (window == nullptr)
            {
                return;
            }

            WidgetIndex_t newWidgetIndex = window->findWidgetAt(x, y);
            if (newWidgetIndex == kWidgetIndexNull)
            {
                return;
            }

            auto widget = window->widgets[newWidgetIndex];
            if (widget.type != WidgetType::viewport)
            {
                return;
            }

            auto viewport = window->viewports[0];
            if (viewport == nullptr)
            {
                return;
            }

            auto zoom = viewport->zoom;

            auto dY = -(16 >> zoom);
            if (dY == 0)
            {
                dY = -1;
            }

            auto deltaY = y - Input::getDragLastLocation().y;
            auto flags = Flags::apply;

            if (deltaY <= dY)
            {
                Input::setDragLastLocation(Input::getDragLastLocation() + Ui::Point{ 0, dY });
                raiseWater(flags);
            }
            else
            {
                dY = -dY;
                if (deltaY < dY)
                {
                    return;
                }
                Input::setDragLastLocation(Input::getDragLastLocation() + Ui::Point{ 0, dY });
                lowerWater(flags);
            }
            _raiseWaterCost = 0x80000000;
            _lowerWaterCost = 0x80000000;
        }

        // 0x004BCDE8
        static void toolUp([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex == Common::widx::panel)
            {
                World::mapInvalidateSelectionRect();

                World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
                ToolManager::setToolCursor(CursorId::waterTool);
            }
        }

        // 0x004BCC6D
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.activatedWidgets |= (1ULL << widx::tool_area);

            if (_adjustToolSize <= 10)
            {
                self.widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;
            }
            else
            {
                self.widgets[widx::tool_area].image = Widget::kContentNull;
            }

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BCCFF
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);

            auto& toolArea = self.widgets[widx::tool_area];

            // Draw as a number if we can't fit a sprite
            if (_adjustToolSize > 10)
            {
                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.midY() + self.y - 5;
                auto point = Point(xPos, yPos);

                FormatArguments args{};
                args.push<uint16_t>(_adjustToolSize);
                tr.drawStringCentred(point, Colour::black, StringIds::tile_inspector_coord, args);
            }

            auto xPos = toolArea.midX() + self.x;
            auto yPos = toolArea.bottom + self.y + 5;

            if (_raiseWaterCost != 0x80000000)
            {
                if (_raiseWaterCost != 0)
                {
                    FormatArguments args{};
                    args.push<uint32_t>(_raiseWaterCost);

                    auto point = Point(xPos, yPos);
                    tr.drawStringCentred(point, Colour::black, StringIds::increase_height_cost, args);
                }
            }

            yPos += 10;

            if (_lowerWaterCost != 0x80000000)
            {
                if (_lowerWaterCost != 0)
                {
                    FormatArguments args{};
                    args.push<uint32_t>(_lowerWaterCost);

                    auto point = Point(xPos, yPos);
                    tr.drawStringCentred(point, Colour::black, StringIds::decrease_height_cost, args);
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onMouseDown = onMouseDown,
            .onUpdate = Common::onUpdate,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .toolDrag = toolDrag,
            .toolUp = toolUp,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace BuildWalls
    {
        static constexpr Ui::Size32 kWindowSize = { 418, 108 };

        static constexpr uint8_t kRowHeight = 48;

        enum widx
        {
            scrollview = 9,
        };

        const uint64_t holdableWidgets = 0;

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(418, 108, StringIds::title_build_walls),
            Widgets::ScrollView({ 2, 45 }, { 391, 48 }, WindowColour::secondary, Scrollbars::vertical)

        );

        // 0x004BC506
        static void updateActiveThumb(Window& self)
        {
            int32_t scrollWidth = 0, scrollHeight = 0;
            self.callGetScrollSize(0, scrollWidth, scrollHeight);
            self.scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self.var_83C; i++)
            {
                if (self.rowInfo[i] == self.rowHover)
                {
                    break;
                }
            }

            if (i >= self.var_83C)
            {
                i = 0;
            }

            i = (i / 10) * kRowHeight;

            self.scrollAreas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB6D5
        static void refreshWallList(Window& self)
        {
            auto wallCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::wall); i++)
            {
                auto wallObj = ObjectManager::get<WallObject>(i);
                if (wallObj == nullptr)
                {
                    continue;
                }
                self.rowInfo[wallCount] = i;
                wallCount++;
            }

            self.var_83C = wallCount;
            auto rowHover = -1;

            if (getGameState().lastWallOption != 0xFF)
            {
                for (auto i = 0; i < self.var_83C; i++)
                {
                    if (getGameState().lastWallOption == self.rowInfo[i])
                    {
                        rowHover = getGameState().lastWallOption;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self.var_83C != 0)
            {
                rowHover = self.rowInfo[0];
            }

            self.rowHover = rowHover;

            updateActiveThumb(self);
        }

        static void removeWallGhost();

        // 0x004BC21C
        static void onClose([[maybe_unused]] Window& self)
        {
            removeWallGhost();
            Ui::Windows::Main::hideGridlines();
        }

        // 0x004BBCBF
        static void tabReset(Window& self)
        {
            ToolManager::toolSet(self, Common::widx::panel, CursorId::placeFence);
            Input::setFlag(Input::Flags::flag6);
            _terraformGhostPlacedFlags = Common::GhostPlacedFlags::none;
            self.var_83C = 0;
            self.rowHover = -1;
            refreshWallList(self);
        }

        // 0x004BC44B
        static void onResize(Window& self)
        {
            self.invalidate();
            Ui::Size32 kMinWindowSize = { self.minWidth, self.minHeight };
            Ui::Size32 kMaxWindowSize = { self.maxWidth, self.maxHeight };
            bool hasResized = self.setSize(kMinWindowSize, kMaxWindowSize);
            if (hasResized)
            {
                updateActiveThumb(self);
            }
        }

        // 0x004BC23D
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
            {
                WindowManager::close(&self);
            }

            if (ToolManager::getToolWindowType() != WindowType::terraform)
            {
                WindowManager::close(&self);
            }

            if (!Input::hasFlag(Input::Flags::rightMousePressed))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == &self)
                {
                    xPos -= self.x;
                    xPos += 26;
                    yPos -= self.y;

                    if ((yPos < 42) || (xPos <= self.width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self.findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self.expandContentCounter += 1;
                            if (self.expandContentCounter >= 8)
                            {
                                auto y = std::min(self.scrollAreas[0].contentHeight - 1 + 60, 562);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 358);
                                }
                                self.minWidth = kWindowSize.width;
                                self.minHeight = y;
                                self.maxWidth = kWindowSize.width;
                                self.maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self.minWidth = kWindowSize.width;
                                    self.minHeight = kWindowSize.height;
                                    self.maxWidth = kWindowSize.width;
                                    self.maxHeight = kWindowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self.expandContentCounter = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self.minWidth = kWindowSize.width;
                        self.minHeight = kWindowSize.height;
                        self.maxWidth = kWindowSize.width;
                        self.maxHeight = kWindowSize.height;
                    }
                }
            }
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self.number, self.currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BC377
        static void event_08(Window& self)
        {
            if (self.var_846 != 0xFFFFU)
            {
                self.var_846 = 0xFFFFU;
                self.invalidate();
            }
        }

        // 0x004BD297 (bits of)
        static void removeWallGhost()
        {
            if ((_terraformGhostPlacedFlags & Common::GhostPlacedFlags::wall) != Common::GhostPlacedFlags::none)
            {
                _terraformGhostPlacedFlags = _terraformGhostPlacedFlags & ~Common::GhostPlacedFlags::wall;
                GameCommands::WallRemovalArgs args;
                args.pos = World::Pos3((_terraformGhostPos).x, (_terraformGhostPos).y, _terraformGhostBaseZ * World::kSmallZStep);
                args.rotation = _terraformGhostRotation;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
        }

        // 0x004BD4C8
        static void placeWallGhost(const GameCommands::WallPlacementArgs& placementArgs)
        {
            removeWallGhost();

            if (GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost) != GameCommands::FAILURE)
            {
                _terraformGhostPos = placementArgs.pos;
                _terraformGhostRotation = placementArgs.rotation;
                _terraformGhostTreeElementType = placementArgs.rotation; // Unsure why duplicated not used
                _terraformGhostType = placementArgs.type;
                _terraformGhostBaseZ = getLegacyReturnState().lastPlacedWall->baseZ();
                _terraformGhostPlacedFlags |= Common::GhostPlacedFlags::wall;
            }
        }

        // 0x004BD48E
        static std::optional<GameCommands::WallPlacementArgs> getWallPlacementArgsFromCursor(const int16_t x, const int16_t y)
        {
            auto* self = WindowManager::find(WindowType::terraform);
            if (self == nullptr)
            {
                return {};
            }

            auto res = ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (!res)
            {
                return {};
            }

            if (self->rowHover == -1)
            {
                return {};
            }

            GameCommands::WallPlacementArgs args;
            // 0 for Z value means game command finds first available height
            args.pos = World::Pos3(res->first.x & 0xFFE0, res->first.y & 0xFFE0, 0);
            args.type = self->rowHover;
            args.rotation = World::getSideFromPos(res->first);
            args.primaryColour = Colour::black;
            args.secondaryColour = Colour::black;
            args.tertiaryColour = Colour::black;
            return { args };
        }

        // 0x004BC227
        static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
            auto placementArgs = getWallPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeWallGhost();
                return;
            }

            auto cornerValue = enumValue(MapSelectionType::edge0) + placementArgs->rotation;
            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            World::setMapSelectionCorner(static_cast<MapSelectionType>(cornerValue));
            World::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            World::mapInvalidateSelectionRect();

            if ((_terraformGhostPlacedFlags & Common::GhostPlacedFlags::wall) != Common::GhostPlacedFlags::none)
            {
                if (_terraformGhostPos == placementArgs->pos && _terraformGhostRotation == placementArgs->rotation && _terraformGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeWallGhost();
            _terraformGhostRotation = placementArgs->rotation;
            placeWallGhost(*placementArgs);
        }

        // 0x004BC232
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            removeWallGhost();
            auto placementArgs = getWallPlacementArgsFromCursor(x, y);
            if (placementArgs)
            {
                GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);
                if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply) != GameCommands::FAILURE)
                {
                    Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
                }
            }
        }

        static void onToolAbort([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            removeWallGhost();
        }

        // 0x004BC359
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
        {
            scrollHeight = (self.var_83C + 9) / 10;
            if (scrollHeight == 0)
            {
                scrollHeight += 1;
            }
            scrollHeight *= kRowHeight;
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 40) + (y / kRowHeight) * 10;
        }

        // 0x004BC3D3
        static void scrollMouseDown(Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self.var_83C; i++)
            {
                auto rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    self.rowHover = rowInfo;
                    getGameState().lastWallOption = static_cast<uint8_t>(rowInfo);

                    int32_t pan = (self.width >> 1) + self.x;
                    Audio::playSound(Audio::SoundId::clickDown, pan);
                    self.expandContentCounter = -16;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BC390
        static void scrollMouseOver(Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = 0xFFFF;
            auto i = 0;

            for (; i < self.var_83C; i++)
            {
                rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    break;
                }
            }
            if (i >= self.var_83C)
            {
                rowInfo = 0xFFFF;
            }
            self.var_846 = rowInfo;
            self.invalidate();
        }

        // 0x004BC212
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_walls_list);
            return args;
        }

        // 0x004BC029
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BC0C2
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(self, drawingCtx);

            auto wallId = self.var_846;
            if (wallId == 0xFFFF)
            {
                wallId = self.rowHover;
                if (wallId == 0xFFFF)
                {
                    return;
                }
            }

            auto wallObj = ObjectManager::get<WallObject>(wallId);
            auto xPos = self.x + 3;
            auto yPos = self.y + self.height - 13;
            auto width = self.width - 19;
            auto point = Point(xPos, yPos);

            FormatArguments args{};
            args.push(wallObj->name);

            tr.drawStringLeftClipped(point, width, Colour::black, StringIds::black_stringid, args);
        }

        // 0x004BC11C
        static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] uint32_t scrollIndex)
        {
            const auto& rt = drawingCtx.currentRenderTarget();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        drawingCtx.drawRectInset(xPos, yPos, 40, kRowHeight, self.getColour(WindowColour::secondary), Gfx::RectInsetFlags::colourLight);
                    }
                }
                else
                {
                    drawingCtx.drawRectInset(xPos, yPos, 40, kRowHeight, self.getColour(WindowColour::secondary), (Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::borderInset));
                }

                auto wallObj = ObjectManager::get<WallObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(xPos + 1, yPos + 1, 39, 47));
                if (clipped)
                {
                    drawingCtx.pushRenderTarget(*clipped);

                    drawingCtx.drawImage(34, 28, wallObj->sprite);

                    drawingCtx.popRenderTarget();
                }

                xPos += 40;

                if (xPos >= 40 * 10) // full row
                {
                    xPos = 0;
                    yPos += kRowHeight;
                }
            }
        }

        static constexpr WindowEventList kEvents = {
            .onClose = onClose,
            .onMouseUp = Common::onMouseUp,
            .onResize = onResize,
            .onUpdate = onUpdate,
            .event_08 = event_08,
            .onToolUpdate = onToolUpdate,
            .onToolDown = onToolDown,
            .onToolAbort = onToolAbort,
            .getScrollSize = getScrollSize,
            .scrollMouseDown = scrollMouseDown,
            .scrollMouseOver = scrollMouseOver,
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

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t holdableWidgets;
        };

        // clang-format off
        static TabInformation tabInformationByTabOffset[] = {
            { ClearArea::widgets,   widx::tab_clear_area,   ClearArea::getEvents(),   ClearArea::holdableWidgets },
            { AdjustLand::widgets,  widx::tab_adjust_land,  AdjustLand::getEvents(),  AdjustLand::holdableWidgets },
            { AdjustWater::widgets, widx::tab_adjust_water, AdjustWater::getEvents(), AdjustWater::holdableWidgets },
            { PlantTrees::widgets,  widx::tab_plant_trees,  PlantTrees::getEvents(),  PlantTrees::holdableWidgets },
            { BuildWalls::widgets,  widx::tab_build_walls,  BuildWalls::getEvents(),  BuildWalls::holdableWidgets },
        };
        // clang-format on

        static void onResize(Window& self, uint8_t height)
        {
            self.flags |= WindowFlags::resizable;

            /*auto width = 130;
            if (isEditorMode())
                width += 31;*/

            // CHANGE: width set to 161 to include building walls tab
            uint16_t width = 161;
            Ui::Size32 kWindowSize = { width, height };
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x004BC78A, 0x004BCB0B
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
            {
                WindowManager::close(&self);
            }

            if (ToolManager::getToolWindowType() != WindowType::terraform)
            {
                WindowManager::close(&self);
            }

            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self.number, self.currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BCD82
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_adjust_land:
                case Common::widx::tab_adjust_water:
                case Common::widx::tab_build_walls:
                case Common::widx::tab_clear_area:
                case Common::widx::tab_plant_trees:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        static void prepareDraw(Window& self)
        {
            // Activate the current tab..
            self.activatedWidgets &= ~((1ULL << tab_adjust_land) | (1ULL << tab_adjust_water) | (1ULL << tab_build_walls) | (1ULL << tab_clear_area) | (1ULL << tab_plant_trees));
            self.activatedWidgets |= (1ULL << tabInformationByTabOffset[self.currentTab].widgetIndex);

            self.widgets[widx::frame].right = self.width - 1;
            self.widgets[widx::frame].bottom = self.height - 1;

            self.widgets[widx::panel].right = self.width - 1;
            self.widgets[widx::panel].bottom = self.height - 1;

            self.widgets[widx::caption].right = self.width - 2;

            self.widgets[widx::close_button].left = self.width - 15;
            self.widgets[widx::close_button].right = self.width - 3;
        }

        // 0x004BCF7F
        void drawTabs(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Clear Land Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_bulldozer;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_clear_area);
            }
            // Adjust Land Tab
            {
                auto landObj = ObjectManager::get<LandObject>(getGameState().lastLandOption);
                uint32_t imageId = landObj->mapPixelImage + Land::ImageIds::toolbar_terraform_land;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_adjust_land);
            }
            // Adjust Water Tab
            {
                auto waterObj = ObjectManager::get<WaterObject>();
                uint32_t imageId = waterObj->image + Water::ImageIds::kToolbarTerraformWater;
                if (self.currentTab == widx::tab_adjust_water - widx::tab_clear_area)
                {
                    imageId += (self.frameNo / 2) % 16;
                }

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_adjust_water);
            }
            // Plant Trees Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_plant_trees;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_plant_trees);
            }
            // Build Walls Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_build_walls;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_build_walls);
            }
        }

        // 0x004BBB2B
        static void switchTab(Window& self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self.type, self.number))
            {
                ToolManager::toolCancel();
            }

            self.currentTab = widgetIndex - widx::tab_clear_area;
            self.frameNo = 0;

            self.viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_clear_area];

            self.holdableWidgets = tabInfo.holdableWidgets;
            self.eventHandlers = &tabInfo.events;
            self.activatedWidgets = 0;
            self.setWidgets(tabInfo.widgets);

            auto disabledWidgets = 0;

            // CHANGE: Disabled so the build walls tab shows outside of editor mode
            /*if (!isEditorMode() && !isSandboxMode())
                disabledWidgets |= common::widx::tab_build_walls;*/

            self.disabledWidgets = disabledWidgets;
            self.invalidate();

            switch (widgetIndex)
            {
                case Common::widx::tab_adjust_land:
                    AdjustLand::tabReset(self);
                    break;

                case Common::widx::tab_adjust_water:
                    AdjustWater::tabReset(self);
                    break;

                case Common::widx::tab_build_walls:
                    BuildWalls::tabReset(self);
                    break;

                case Common::widx::tab_clear_area:
                    ClearArea::tabReset(self);
                    break;

                case Common::widx::tab_plant_trees:
                    PlantTrees::tabReset(self);
                    break;
            }

            self.callOnResize();
            self.callPrepareDraw();
            self.initScrollWidgets();
            self.invalidate();
            self.moveInsideScreenEdges();
        }

        // 0x004A69DD
        static void sub_4A69DD()
        {
            auto* window = WindowManager::find(WindowType::construction);
            if (window != nullptr)
            {
                Ui::Windows::Construction::removeConstructionGhosts();
            }
        }
    }

    // 0x004BB566
    void openClearArea()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_clear_area, terraformWindow->widgets[Common::widx::tab_clear_area].id);
    }

    // 0x004BB546
    void openAdjustLand()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_adjust_land, terraformWindow->widgets[Common::widx::tab_adjust_land].id);
    }

    // 0x004BB556
    void openAdjustWater()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_adjust_water, terraformWindow->widgets[Common::widx::tab_adjust_water].id);
    }

    // 0x004BB4A3
    void openPlantTrees()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_plant_trees, terraformWindow->widgets[Common::widx::tab_plant_trees].id);
    }

    // 0x004BB576
    void openBuildWalls()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_build_walls, terraformWindow->widgets[Common::widx::tab_build_walls].id);
    }

    bool rotate(Window& self)
    {
        if (self.currentTab == Common::widx::tab_plant_trees - Common::widx::tab_clear_area)
        {
            if (!self.isDisabled(PlantTrees::widx::rotate_object))
            {
                if (!self.widgets[PlantTrees::widx::rotate_object].hidden)
                {
                    self.callOnMouseUp(PlantTrees::widx::rotate_object, self.widgets[PlantTrees::widx::rotate_object].id);
                    return true;
                }
            }
        }

        return false;
    }

    void setAdjustLandToolSize(uint8_t size)
    {
        _adjustLandToolSize = size;
    }

    void setAdjustWaterToolSize(uint8_t size)
    {
        _adjustWaterToolSize = size;
    }

    void setClearAreaToolSize(uint8_t size)
    {
        _clearAreaToolSize = size;
    }

    void setLastPlacedTree(World::TreeElement* elTree)
    {
        _lastPlacedTree = elTree;
    }

    // 0x004BAEC4
    void resetLastSelections()
    {
        _treeRotation = 2;

        auto& gameState = getGameState();
        gameState.lastTreeOption = 0xFF;
        gameState.lastWallOption = 0xFF;
    }
}
