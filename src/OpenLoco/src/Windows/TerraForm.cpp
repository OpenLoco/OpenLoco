#include "Audio/Audio.h"
#include "Drawing/SoftwareDrawingEngine.h"
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
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "LastGameOptionManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Map/Tree.h"
#include "Map/TreeElement.h"
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
#include "Ui/WindowManager.h"
#include "Widget.h"
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

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_adjust_land) | (1 << widx::tab_adjust_water) | (1 << widx::tab_build_walls) | (1 << widx::tab_clear_area) | (1 << widx::tab_plant_trees);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { 130, 74 }, WidgetType::panel, WindowColour::secondary),                                                                              \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_clear_land),                             \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_adjust_land),                            \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_adjust_water),                           \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_plant_trees),                            \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::wt_6, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_walls)

        static WindowEventList _events;

        static void initEvents();
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void drawTabs(Window* self, Gfx::RenderTarget* rt);
        static void prepareDraw(Window& self);
        static void onUpdate(Window& self);
        static void onResize(Window& self, uint8_t height);
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex);
        static void sub_4A69DD();

        enum class GhostPlacedFlags : uint8_t
        {
            none = 0U,
            tree = 1 << 0,
            wall = 1 << 1,
        };
        OPENLOCO_ENABLE_ENUM_OPERATORS(GhostPlacedFlags);
    }
    static loco_global<int16_t, 0x0052337A> _dragLastY;
    static loco_global<uint8_t, 0x009C870E> _adjustLandToolSize;
    static loco_global<uint8_t, 0x009C870F> _clearAreaToolSize;
    static loco_global<uint8_t, 0x009C8710> _adjustWaterToolSize;
    static loco_global<uint8_t, 0x00F003D2> _lastSelectedLand;
    static loco_global<uint8_t, 0x01136496> _treeRotation;
    static loco_global<Colour, 0x01136497> _treeColour;
    static loco_global<Common::GhostPlacedFlags, 0x0113649A> _terraformGhostPlacedFlags;
    static loco_global<uint8_t, 0x0113649E> _treeClusterType;
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;
    static loco_global<uint32_t, 0x00F2530C> _raiseLandCost;
    static loco_global<uint32_t, 0x00F25310> _lowerLandCost;
    static loco_global<uint32_t, 0x01136484> _lastTreeCost;
    static loco_global<World::TileElement*, 0x01136470> _lastPlacedWall;
    static loco_global<World::TreeElement*, 0x01136470> _lastPlacedTree;
    static loco_global<World::Pos2, 0x01136488> _terraformGhostPos;
    static loco_global<uint16_t, 0x01136490> _lastTreeColourFlag;
    static loco_global<uint16_t, 0x01136492> _terraformGhostTreeRotationFlag;
    static loco_global<uint8_t, 0x01136499> _terraformGhostBaseZ;
    static loco_global<uint8_t, 0x0113649B> _terraformGhostTreeElementType;
    static loco_global<uint8_t, 0x0113649C> _terraformGhostType;
    static loco_global<uint8_t, 0x0113649D> _terraformGhostRotation; // wall
    static loco_global<uint8_t, 0x0113649D> _terraformGhostQuadrant; // tree
    static loco_global<uint32_t, 0x0113652C> _raiseWaterCost;
    static loco_global<uint32_t, 0x01136528> _lowerWaterCost;
    namespace PlantTrees
    {
        static constexpr Ui::Size kWindowSize = { 634, 162 };

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

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour) | (1 << plant_cluster_selected) | (1 << plant_cluster_random);
        const uint64_t holdableWidgets = 0;

        Widget widgets[] = {
            commonWidgets(634, 162, StringIds::title_plant_trees),
            makeWidget({ 3, 45 }, { 605, 101 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            makeWidget({ 609, 46 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_object_90),
            makeWidget({ 609, 70 }, { 24, 24 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_object_colour),
            makeWidget({ 609, 94 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::plant_cluster_selected_tree, StringIds::plant_cluster_selected_tree),
            makeWidget({ 609, 118 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::plant_cluster_random_tree, StringIds::plant_cluster_random_tree),
            widgetEnd(),
        };

        static WindowEventList events;

        enum treeCluster
        {
            none = 0,
            selected,
            random,
        };

        // 0x004BB6B2
        static void updateTreeColours(Window* self)
        {
            if (self->rowHover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self->rowHover);
                if (treeObj->colours != 0)
                {
                    auto bit = Numerics::bitScanReverse(treeObj->colours);
                    auto colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                    _treeColour = colour;
                }
            }
        }

        // 0x004BC4B7
        static void updateActiveThumb(Window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->rowInfo[i] == self->rowHover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 9) * kRowHeight;

            self->scrollAreas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB63F
        static void refreshTreeList(Window* self)
        {
            auto treeCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::tree); i++)
            {
                auto treeObj = ObjectManager::get<TreeObject>(i);
                if (treeObj == nullptr)
                    continue;
                self->rowInfo[treeCount] = i;
                treeCount++;
            }

            self->var_83C = treeCount;
            auto rowHover = -1;

            if (LastGameOptionManager::getLastTree() != LastGameOptionManager::kNoLastOption)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (LastGameOptionManager::getLastTree() == self->rowInfo[i])
                    {
                        rowHover = LastGameOptionManager::getLastTree();
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->rowInfo[0];
            }

            self->rowHover = rowHover;

            updateActiveThumb(self);
            updateTreeColours(self);
        }

        static void removeTreeGhost();

        // 0x004BBB0A
        static void onClose([[maybe_unused]] Window& self)
        {
            removeTreeGhost();
            Ui::Windows::hideGridlines();
        }

        // 0x004BBC7D
        static void tabReset(Window* self)
        {
            Input::toolSet(self, Common::widx::panel, CursorId::plantTree);
            Input::setFlag(Input::Flags::flag6);
            _terraformGhostPlacedFlags = Common::GhostPlacedFlags::none;
            _lastTreeCost = 0x80000000;
            self->var_83C = 0;
            self->rowHover = -1;
            refreshTreeList(self);
            updateTreeColours(self);
        }

        // 0x004BBAB5
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
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
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::rotate_object:
                {
                    _treeRotation++;
                    _treeRotation = _treeRotation & 3;
                    self.invalidate();
                    break;
                }

                case widx::plant_cluster_selected:
                {
                    if (_treeClusterType == treeCluster::selected)
                        _treeClusterType = treeCluster::none;
                    else
                        _treeClusterType = treeCluster::selected;
                    self.invalidate();
                    break;
                }

                case widx::plant_cluster_random:
                {
                    if (_treeClusterType == treeCluster::random)
                        _treeClusterType = treeCluster::none;
                    else
                        _treeClusterType = treeCluster::random;
                    self.invalidate();
                }
            }
        }

        // 0x004BBFBD
        static void onResize(Window& self)
        {
            self.invalidate();
            Ui::Size kMinWindowSize = { self.minWidth, self.minHeight };
            Ui::Size kMaxWindowSize = { self.maxWidth, self.maxHeight };
            bool hasResized = self.setSize(kMinWindowSize, kMaxWindowSize);
            if (hasResized)
                updateActiveThumb(&self);
        }

        // 0x004BBAEA
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::object_colour && self.rowHover != -1)
            {
                auto obj = ObjectManager::get<TreeObject>(self.rowHover);
                Dropdown::showColour(&self, &self.widgets[widgetIndex], obj->colours, _treeColour, self.getColour(WindowColour::secondary));
            }
        }

        // 0x004BBAF5
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
                return;
            if (itemIndex == -1)
                return;

            _treeColour = static_cast<Colour>(Dropdown::getHighlightedItem());
            self.invalidate();
        }

        // 0x004BBDA5
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
                WindowManager::close(&self);

            if (ToolManager::getToolWindowType() != WindowType::terraform)
                WindowManager::close(&self);

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
                            self.savedView.mapX += 1;
                            if (self.savedView.mapX >= 8)
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
                    self.savedView.mapX = 0;
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
                args.pos = World::Pos3((*_terraformGhostPos).x, (*_terraformGhostPos).y, _terraformGhostBaseZ * World::kSmallZStep);
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
                _terraformGhostTreeElementType = (*_lastPlacedTree)->rawData()[0];
                _terraformGhostType = placementArgs.type;
                _terraformGhostBaseZ = (*_lastPlacedTree)->baseZ();
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
            args.quadrant = ViewportInteraction::getQuadrantFromPos(res->first) ^ (1 << 1);
            args.colour = *_treeColour;
            args.rotation = (_treeRotation - WindowManager::getCurrentRotation()) & 0x3;
            if (isEditorMode())
            {
                args.buildImmediately = true;
            }
            return { args };
        }

        // 0x004BBB15
        static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            World::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            auto placementArgs = getTreePlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeTreeGhost();
                return;
            }

            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            World::TileManager::setMapSelectionCorner((placementArgs->quadrant ^ (1 << 1)) + 6);
            World::TileManager::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            World::TileManager::mapInvalidateSelectionRect();

            if ((_terraformGhostPlacedFlags & Common::GhostPlacedFlags::tree) != Common::GhostPlacedFlags::none)
            {
                if (*_terraformGhostPos == placementArgs->pos
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
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
                        auto previousId = CompanyManager::getUpdatingCompanyId();
                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);

                        if (World::placeTreeCluster(World::toTileSpace(placementArgs->pos), 320, 3, placementArgs->type))
                        {
                            auto height = TileManager::getHeight(placementArgs->pos);
                            Audio::playSound(Audio::SoundId::construct, World::Pos3{ placementArgs->pos.x, placementArgs->pos.y, height.landHeight });
                        }
                        else
                        {
                            Error::open(StringIds::cant_plant_this_here, StringIds::empty);
                        }

                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(previousId);
                        break;
                    }
                    case treeCluster::random:
                        auto previousId = CompanyManager::getUpdatingCompanyId();
                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);

                        if (World::placeTreeCluster(World::toTileSpace(placementArgs->pos), 384, 4, std::nullopt))
                        {
                            auto height = TileManager::getHeight(placementArgs->pos);
                            Audio::playSound(Audio::SoundId::construct, World::Pos3{ placementArgs->pos.x, placementArgs->pos.y, height.landHeight });
                        }
                        else
                        {
                            Error::open(StringIds::cant_plant_this_here, StringIds::empty);
                        }

                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(previousId);
                        break;
                }
            }
        }

        // 0x004BBEC1
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (self.var_83C + 8) / 9;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= kRowHeight;
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
                    LastGameOptionManager::setLastTree(static_cast<uint8_t>(rowInfo));

                    updateTreeColours(&self);

                    int32_t pan = (self.width >> 1) + self.x;
                    Audio::playSound(Audio::SoundId::clickDown, pan);
                    self.savedView.mapX = -16;
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
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
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
                self.activatedWidgets |= (1ULL << widx::plant_cluster_selected);

            if (_treeClusterType == treeCluster::random)
                self.activatedWidgets |= (1ULL << widx::plant_cluster_random);

            self.widgets[widx::rotate_object].type = WidgetType::none;
            self.widgets[widx::object_colour].type = WidgetType::none;

            if (self.rowHover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self.rowHover);
                if (treeObj->name != 0xFFFF)
                {
                    if (treeObj->numRotations != 1)
                        self.widgets[widx::rotate_object].type = WidgetType::buttonWithImage;

                    if (treeObj->colours != 0)
                    {
                        self.widgets[widx::object_colour].image = Widget::kImageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, *_treeColour);
                        self.widgets[widx::object_colour].type = WidgetType::buttonWithColour;
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
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto treeId = self.var_846;
            if (treeId == 0xFFFF)
            {
                treeId = self.rowHover;
                if (treeId == 0xFFFF)
                    return;
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
            auto args = FormatArguments();
            args.push<uint32_t>(treeCost);

            if (!isEditorMode())
            {
                auto xPos = self.x + 3 + self.width - 17;
                auto yPos = self.y + self.height - 13;
                drawingCtx.drawStringRight(*rt, xPos, yPos, Colour::black, StringIds::build_cost, &args);
            }
            auto xPos = self.x + 3;
            auto yPos = self.y + self.height - 13;
            auto width = self.width - 19 - xPos;
            drawingCtx.drawStringLeftClipped(*rt, xPos, yPos, width, Colour::black, StringIds::black_stringid, &treeObj->name);
        }

        static void drawTreeThumb(TreeObject* treeObj, Gfx::RenderTarget* clipped)
        {
            uint32_t image = treeObj->getTreeGrowthDisplayOffset() * treeObj->numRotations;
            auto rotation = (treeObj->numRotations - 1) & _treeRotation;
            image += rotation;
            image += treeObj->sprites[treeObj->seasonState];

            auto colourOptions = treeObj->colours;
            if (colourOptions != 0)
            {
                auto colour = *_treeColour;
                if (!(_lastTreeColourFlag & (1 << 5)))
                {
                    auto bit = Numerics::bitScanReverse(colourOptions);
                    colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                }
                image = Gfx::recolour(image, colour);
            }
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            drawingCtx.drawImage(clipped, 32, 96, image);
        }

        // 0x004BB982
        static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(rt, shade);

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
                        drawingCtx.drawRectInset(rt, xPos, yPos, 65, kRowHeight - 1, self.getColour(WindowColour::secondary), Drawing::RectInsetFlags::colourLight);
                    }
                }
                else
                {
                    _lastTreeColourFlag = AdvancedColour::translucentFlag | AdvancedColour::outlineFlag;
                    drawingCtx.drawRectInset(rt, xPos, yPos, 65, kRowHeight - 1, self.getColour(WindowColour::secondary), (Drawing::RectInsetFlags::colourLight | Drawing::RectInsetFlags::borderInset));
                }

                auto treeObj = ObjectManager::get<TreeObject>(self.rowInfo[i]);
                auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(xPos + 1, yPos + 1, 64, kRowHeight - 2));
                if (clipped)
                {
                    drawTreeThumb(treeObj, &*clipped);
                }

                xPos += kColumnWidth;

                if (xPos >= kColumnWidth * 9) // full row
                {
                    xPos = 0;
                    yPos += kRowHeight;
                }
            }
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onDropdown = onDropdown;
            events.onUpdate = onUpdate;
            events.event_08 = event_08;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.getScrollSize = getScrollSize;
            events.scrollMouseDown = scrollMouseDown;
            events.scrollMouseOver = scrollMouseOver;
            events.tooltip = tooltip;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
            events.drawScroll = drawScroll;
        }
    }

    // 0x004BB4A3
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::terraform, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_plant_trees);
        }
        else
        {
            // 0x004BB586
            auto origin = Ui::Point(Ui::width() - PlantTrees::kWindowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::terraform,
                origin,
                PlantTrees::kWindowSize,
                WindowFlags::flag_11,
                &PlantTrees::events);

            window->number = 0;
            window->currentTab = Common::widx::tab_plant_trees - Common::widx::tab_clear_area;
            window->frameNo = 0;
            _terraformGhostPlacedFlags = Common::GhostPlacedFlags::none;
            _lastTreeCost = 0x80000000;
            window->owner = CompanyManager::getControllingId();
            window->var_846 = 0xFFFF;
            window->savedView.mapX = 0;
            _treeClusterType = PlantTrees::treeCluster::none;

            WindowManager::sub_4CEE0B(window);

            window->minWidth = PlantTrees::kWindowSize.width;
            window->minHeight = PlantTrees::kWindowSize.height;
            window->maxWidth = PlantTrees::kWindowSize.width;
            window->maxHeight = PlantTrees::kWindowSize.height;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_0E);

            // End of 0x004BB586

            Ui::Windows::showGridlines();
            _treeRotation = 2;

            Common::initEvents();

            window->invalidate();

            window->widgets = PlantTrees::widgets;
            window->enabledWidgets = PlantTrees::enabledWidgets;
            window->holdableWidgets = 0;
            window->activatedWidgets = 0;

            window->disabledWidgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();

            window->var_83C = 0;
            window->rowHover = -1;

            PlantTrees::refreshTreeList(window);

            Input::toolSet(window, Common::widx::panel, CursorId::landTool);

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

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area);
        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);

        Widget widgets[] = {
            commonWidgets(130, 105, StringIds::clear_area),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, WidgetType::wt_3, WindowColour::secondary, ImageIds::tool_area, StringIds::tooltip_clear_area),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_clear_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_clear_area),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004BC671
        static void onClose([[maybe_unused]] Window& self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBBC7
        static void tabReset(Window* self)
        {
            Input::toolSet(self, Common::widx::panel, CursorId::bulldozerTool);
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
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                        _adjustToolSize = 1;
                    _clearAreaToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 64)
                        _adjustToolSize = 64;
                    _clearAreaToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BC677
        static void onToolUpdate([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            World::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

            uint32_t cost = 0x80000000;
            auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (res)
            {
                if (World::TileManager::setMapSelectionTiles(res->first, 4) == 0)
                {
                    return;
                }
                const auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
                const Pos2 centre = (pointA + pointB) / 2;

                ClearLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                cost = GameCommands::doCommand(args, GameCommands::Flags::flag_2 | GameCommands::Flags::ghost);
            }

            if (cost != _raiseLandCost)
            {
                _raiseLandCost = cost;
                WindowManager::invalidate(WindowType::terraform);
            }
        }

        static void clearLand(uint8_t flags)
        {
            if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
            {
                auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
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
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            clearLand(Flags::apply);
        }

        // 0x004BC682
        static void toolDragContinue([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;

            auto window = WindowManager::find(WindowType::error);
            if (window == nullptr)
            {
                clearLand(Flags::apply);
            }
        }

        // 0x004BC701
        static void toolDragEnd([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            }
        }

        // 0x004BC555
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.activatedWidgets |= (1ULL << widx::tool_area);

            if (_adjustToolSize <= 10)
                self.widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;
            else
                self.widgets[widx::tool_area].image = Widget::kContentNull;

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BC5E7
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto& toolArea = self.widgets[widx::tool_area];

            // Draw as a number if we can't fit a sprite
            if (_adjustToolSize > 10)
            {

                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.midY() + self.y - 5;

                auto args = FormatArguments();
                args.push<uint16_t>(_adjustToolSize);
                drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::tile_inspector_coord, &args);
            }

            if (_raiseLandCost == 0x80000000)
                return;

            if (_raiseLandCost == 0)
                return;

            auto xPos = toolArea.midX() + self.x;
            auto yPos = toolArea.bottom + self.y + 5;

            auto args = FormatArguments();
            args.push<uint32_t>(_raiseLandCost);

            drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::clear_land_cost, &args);
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onUpdate = Common::onUpdate;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.toolDragContinue = toolDragContinue;
            events.toolDragEnd = toolDragEnd;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
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

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area) | (1 << land_material) | (1 << mountain_mode) | (1 << paint_mode);
        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);
        static bool isMountainMode = false;
        static bool isPaintMode = false;

        Widget widgets[] = {
            commonWidgets(130, 105, StringIds::title_adjust_land),
            makeWidget({ 49, 45 }, { 64, 44 }, WidgetType::wt_3, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_adjust_land_tool),
            makeWidget({ 50, 46 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_land_area),
            makeWidget({ 96, 72 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_land_area),
            makeWidget({ 57, 92 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_slope_up, StringIds::mountainModeTooltip),
            makeWidget({ 83, 92 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::paintbrush, StringIds::tooltip_paint_landscape_tool),
            makeWidget({ 112, 94 }, { 20, 20 }, WidgetType::wt_6, WindowColour::primary),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004BC9D1
        static void onClose([[maybe_unused]] Window& self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBBF7
        static void tabReset(Window* self)
        {
            if (isPaintMode)
                Input::toolSet(self, widx::paint_mode, CursorId::landTool);
            else if (isMountainMode)
                Input::toolSet(self, widx::mountain_mode, CursorId::landTool);
            else
                Input::toolSet(self, Common::widx::panel, CursorId::landTool);

            Input::setFlag(Input::Flags::flag6);
            for (auto i = 0; i < 32; i++)
            {
                auto landObj = ObjectManager::get<LandObject>(i);
                if (landObj == nullptr)
                    continue;

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
            if (isEditorMode())
            {
                Common::onResize(self, 115);
            }
            else
            {
                // CHANGE: Resizes window to allow Dropdown and cost string to be drawn seperately
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
                    landCount++;
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
                    continue;

                if (i == _lastSelectedLand)
                    Dropdown::setHighlightedItem(landIndex);

                auto args = FormatArguments();
                args.push(landObj->mapPixelImage + Land::ImageIds::landscape_generator_tile_icon);
                args.push<uint16_t>(i);

                Dropdown::add(landIndex, 0xFFFE, args);

                landIndex++;
            }
        }

        // 0x004BC9A7
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
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
                        _adjustToolSize = 1;
                    _adjustLandToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 64)
                        _adjustToolSize = 64;
                    _adjustLandToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }
            }
        }

        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
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
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::mountain_mode:
                {
                    isMountainMode = !isMountainMode;
                    isPaintMode = false;
                    tabReset(&self);
                    self.invalidate();
                    break;
                }

                case widx::paint_mode:
                {
                    isMountainMode = false;
                    isPaintMode = !isPaintMode;
                    tabReset(&self);
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x004BC9C6
        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::land_material)
                return;
            if (itemIndex == -1)
                return;
            _lastSelectedLand = Dropdown::getItemArgument(itemIndex, 2);
            self.invalidate();
        }

        // 0x00468DFD
        static uint32_t lowerLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                Common::sub_4A69DD();

            auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
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
                args.corner = World::TileManager::getMapSelectionCorner();
                cost = GameCommands::doCommand(args, flags);
            }
            return cost;
        }

        // 0x00468D1D
        static uint32_t raiseLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                Common::sub_4A69DD();

            auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
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
                args.corner = World::TileManager::getMapSelectionCorner();
                cost = GameCommands::doCommand(args, flags);
            }
            return cost;
        }

        static void setAdjustCost(uint32_t raiseCost, uint32_t lowerCost)
        {
            if (_raiseLandCost == raiseCost)
            {
                if (_lowerLandCost == lowerCost)
                    return;
            }

            _raiseLandCost = raiseCost;
            _lowerLandCost = lowerCost;

            WindowManager::invalidate(WindowType::terraform, 0);
        }

        static void onPaintToolUpdate([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            World::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

            uint32_t cost = 0x80000000;
            auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (res)
            {
                if (World::TileManager::setMapSelectionTiles(res->first, 4) == 0)
                {
                    return;
                }
                const auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
                const Pos2 centre = (pointA + pointB) / 2;

                ClearLandArgs args{};
                args.centre = centre;
                args.pointA = pointA;
                args.pointB = pointB;
                cost = GameCommands::doCommand(args, GameCommands::Flags::flag_2 | GameCommands::Flags::ghost);
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

            TileManager::mapInvalidateSelectionRect();

            if (ToolManager::getToolCursor() != CursorId::upDownArrow)
            {
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
                auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
                if (res)
                {
                    if (_adjustLandToolSize == 1 && !(isMountainMode || isPaintMode))
                    {
                        auto count = TileManager::setMapSelectionSingleTile(res->first, true);

                        if (!count)
                            return;
                    }
                    else
                    {
                        auto count = TileManager::setMapSelectionTiles(res->first, 4);

                        if (!count)
                            return;
                    }
                }
                else
                {
                    xPos = 0x8000;
                }
            }
            else
            {
                if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
                    return;
            }

            uint32_t raiseCost = 0;
            uint32_t lowerCost = 0;

            if (isEditorMode() || xPos == 0x8000)
            {
                raiseCost = 0x80000000;
                lowerCost = 0x80000000;
            }
            else
            {
                lowerCost = lowerLand(Flags::flag_2);
                raiseCost = raiseLand(Flags::flag_2);
            }
            setAdjustCost(raiseCost, lowerCost);
        }

        // 0x004BC9D7
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
                    auto [pointA, pointB] = World::TileManager::getMapSelectionArea();

                    ChangeLandMaterialArgs args{};
                    args.pointA = pointA;
                    args.pointB = pointB;
                    args.landType = _lastSelectedLand;
                    GameCommands::doCommand(args, Flags::apply);
                }
            }
        }

        // 0x004BC9ED
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            switch (widgetIndex)
            {
                case widx::paint_mode:
                    paintLand();
                    break;

                case widx::mountain_mode:
                case Common::widx::panel:
                {
                    if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
                        return;

                    ToolManager::setToolCursor(CursorId::upDownArrow);
                    break;
                }
            }
        }

        // 0x004BC9E2
        static void toolDragContinue([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
                    if (newWidgetIndex == -1)
                        break;

                    auto widget = window->widgets[newWidgetIndex];
                    if (widget.type != WidgetType::viewport)
                        break;

                    auto viewport = window->viewports[0];
                    if (viewport == nullptr)
                        break;

                    auto zoom = viewport->zoom;

                    auto dY = -(16 >> zoom);
                    if (dY == 0)
                        dY = -1;
                    auto deltaY = y - _dragLastY;
                    auto flags = Flags::apply;

                    if (deltaY <= dY)
                    {
                        _dragLastY = _dragLastY + dY;
                        raiseLand(flags);
                    }
                    else
                    {
                        dY = -dY;
                        if (deltaY < dY)
                            break;
                        _dragLastY = _dragLastY + dY;
                        lowerLand(flags);
                    }
                    _raiseLandCost = 0x80000000;
                    _lowerLandCost = 0x80000000;
                    break;
                }
            }
        }

        // 0x004BCA5D
        static void toolDragEnd([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::mountain_mode:
                case widx::paint_mode:
                case Common::widx::panel:
                {
                    TileManager::mapInvalidateSelectionRect();
                    Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
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
                self.activatedWidgets |= (1 << widx::mountain_mode);
            else
                self.activatedWidgets &= ~(1 << widx::mountain_mode);

            if (isPaintMode)
                self.activatedWidgets |= (1 << widx::paint_mode);
            else
                self.activatedWidgets &= ~(1 << widx::paint_mode);

            auto landObj = ObjectManager::get<LandObject>(_lastSelectedLand);
            auto pixelColour = static_cast<Colour>(Gfx::getG1Element(landObj->mapPixelImage)->offset[0]);
            self.widgets[widx::paint_mode].image = Gfx::recolour2(ImageIds::paintbrush, Colour::white, pixelColour);

            if (isPaintMode)
            {
                self.widgets[widx::land_material].type = WidgetType::wt_6;
                self.widgets[widx::land_material].image = landObj->mapPixelImage + OpenLoco::Land::ImageIds::landscape_generator_tile_icon;
            }
            else
            {
                self.widgets[widx::land_material].type = WidgetType::none;
            }

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BC909
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            self.draw(rt);

            Common::drawTabs(&self, rt);

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
                        drawingCtx.drawImage(*rt, placeForImage, areaImage);

                        placeForImage += { 8, 0 };
                        drawingCtx.drawImage(*rt, placeForImage, areaImage);
                    }
                    else
                    {
                        // For odd sizes, we just need the one
                        drawingCtx.drawImage(*rt, placeForImage, areaImage);
                    }
                }

                // Draw tool size
                if (!isMountainMode || _adjustToolSize > 1)
                {
                    auto areaImage = ImageId(ImageIds::tool_area).withIndexOffset(_adjustToolSize);
                    Ui::Point placeForImage(toolArea.left + self.x, toolArea.top + self.y);
                    drawingCtx.drawImage(*rt, placeForImage, areaImage);
                }
            }
            // Or draw as a number, if we can't fit a sprite
            else
            {
                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.midY() + self.y - 5;

                auto args = FormatArguments();
                args.push<uint16_t>(_adjustToolSize);
                drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::tile_inspector_coord, &args);
            }

            auto xPos = toolArea.midX() + self.x;
            auto yPos = toolArea.bottom + self.y + 28;

            if (_raiseLandCost != 0x80000000)
            {
                if (_raiseLandCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_raiseLandCost);
                    drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::increase_height_cost, &args);
                }
            }

            yPos += 10;

            if (_lowerLandCost != 0x80000000)
            {
                if (_lowerLandCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_lowerLandCost);
                    drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::decrease_height_cost, &args);
                }
            }
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.onMouseUp = onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onDropdown = onDropdown;
            events.onUpdate = Common::onUpdate;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.toolDragContinue = toolDragContinue;
            events.toolDragEnd = toolDragEnd;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
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

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area);
        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);

        Widget widgets[] = {
            commonWidgets(130, 105, StringIds::title_adjust_water),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, WidgetType::wt_3, WindowColour::secondary, ImageIds::tool_area, StringIds::tooltip_adjust_water_tool),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_water_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_water_area),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004BCDAE
        static void onClose([[maybe_unused]] Window& self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBC46
        static void tabReset(Window* self)
        {
            Input::toolSet(self, Common::widx::panel, CursorId::waterTool);
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
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                        _adjustToolSize = 1;
                    _adjustWaterToolSize = _adjustToolSize;
                    self.invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 64)
                        _adjustToolSize = 64;
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
                    return;
            }

            _raiseWaterCost = raiseCost;
            _lowerWaterCost = lowerCost;

            WindowManager::invalidate(WindowType::terraform);
        }

        static uint32_t raiseWater(uint8_t flags);
        static uint32_t lowerWater(uint8_t flags);

        // 0x004BCDB4
        static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            TileManager::mapInvalidateSelectionRect();

            if (ToolManager::getToolCursor() != CursorId::upDownArrow)
            {
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

                auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::surface | ViewportInteraction::InteractionItemFlags::water));
                auto& interaction = res.first;
                if (interaction.type == ViewportInteraction::InteractionItem::noInteraction)
                {
                    setAdjustCost(0x80000000, 0x80000000);
                    return;
                }
                if (!TileManager::setMapSelectionTiles(interaction.pos + World::Pos2(16, 16), 5))
                {
                    // no change in selection
                    return;
                }
            }
            else
            {
                if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
                    return;
            }

            if (isEditorMode())
            {
                setAdjustCost(0x80000000, 0x80000000);
            }
            else
            {
                setAdjustCost(raiseWater(0), lowerWater(0));
            }
        }

        // 0x004BCDCA
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const int16_t x, [[maybe_unused]] const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
                return;

            ToolManager::setToolCursor(CursorId::upDownArrow);
        }

        static uint32_t raiseWater(uint8_t flags)
        {
            if (flags & GameCommands::Flags::apply)
            {
                Common::sub_4A69DD();
                GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            }

            auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
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

            auto [pointA, pointB] = World::TileManager::getMapSelectionArea();
            GameCommands::LowerWaterArgs args{};
            args.pointA = pointA;
            args.pointB = pointB;
            return GameCommands::doCommand(args, flags);
        }

        // 0x004BCDBF
        static void toolDragContinue([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;

            auto window = WindowManager::findAt(x, y);
            if (window == nullptr)
                return;

            WidgetIndex_t newWidgetIndex = window->findWidgetAt(x, y);
            if (newWidgetIndex == -1)
                return;

            auto widget = window->widgets[newWidgetIndex];
            if (widget.type != WidgetType::viewport)
                return;

            auto viewport = window->viewports[0];
            if (viewport == nullptr)
                return;

            auto zoom = viewport->zoom;

            auto dY = -(16 >> zoom);
            if (dY == 0)
                dY = -1;

            auto deltaY = y - _dragLastY;
            auto flags = Flags::apply;

            if (deltaY <= dY)
            {
                _dragLastY = _dragLastY + dY;
                raiseWater(flags);
            }
            else
            {
                dY = -dY;
                if (deltaY < dY)
                    return;
                _dragLastY = _dragLastY + dY;
                lowerWater(flags);
            }
            _raiseWaterCost = 0x80000000;
            _lowerWaterCost = 0x80000000;
        }

        // 0x004BCDE8
        static void toolDragEnd([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();

                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
                ToolManager::setToolCursor(CursorId::waterTool);
            }
        }

        // 0x004BCC6D
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.activatedWidgets |= (1ULL << widx::tool_area);

            if (_adjustToolSize <= 10)
                self.widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;
            else
                self.widgets[widx::tool_area].image = Widget::kContentNull;

            Widget::leftAlignTabs(self, Common::widx::tab_clear_area, Common::widx::tab_build_walls);
        }

        // 0x004BCCFF
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto& toolArea = self.widgets[widx::tool_area];

            // Draw as a number if we can't fit a sprite
            if (_adjustToolSize > 10)
            {
                auto xPos = toolArea.midX() + self.x;
                auto yPos = toolArea.midY() + self.y - 5;

                auto args = FormatArguments();
                args.push<uint16_t>(_adjustToolSize);
                drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::tile_inspector_coord, &args);
            }

            auto xPos = toolArea.midX() + self.x;
            auto yPos = toolArea.bottom + self.y + 5;

            if (_raiseWaterCost != 0x80000000)
            {
                if (_raiseWaterCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_raiseWaterCost);

                    drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::increase_height_cost, &args);
                }
            }

            yPos += 10;

            if (_lowerWaterCost != 0x80000000)
            {
                if (_lowerWaterCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_lowerWaterCost);

                    drawingCtx.drawStringCentred(*rt, xPos, yPos, Colour::black, StringIds::decrease_height_cost, &args);
                }
            }
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onMouseDown = onMouseDown;
            events.onUpdate = Common::onUpdate;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.toolDragContinue = toolDragContinue;
            events.toolDragEnd = toolDragEnd;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace BuildWalls
    {
        static constexpr Ui::Size kWindowSize = { 418, 108 };

        static constexpr uint8_t kRowHeight = 48;

        enum widx
        {
            scrollview = 9,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview);
        const uint64_t holdableWidgets = 0;

        Widget widgets[] = {
            commonWidgets(418, 108, StringIds::title_build_walls),
            makeWidget({ 2, 45 }, { 391, 48 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004BC506
        static void updateActiveThumb(Window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->rowInfo[i] == self->rowHover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 10) * kRowHeight;

            self->scrollAreas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB6D5
        static void refreshWallList(Window* self)
        {
            auto wallCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::wall); i++)
            {
                auto wallObj = ObjectManager::get<WallObject>(i);
                if (wallObj == nullptr)
                    continue;
                self->rowInfo[wallCount] = i;
                wallCount++;
            }

            self->var_83C = wallCount;
            auto rowHover = -1;

            if (LastGameOptionManager::getLastWall() != LastGameOptionManager::kNoLastOption)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (LastGameOptionManager::getLastWall() == self->rowInfo[i])
                    {
                        rowHover = LastGameOptionManager::getLastWall();
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->rowInfo[0];
            }

            self->rowHover = rowHover;

            updateActiveThumb(self);
        }

        static void removeWallGhost();

        // 0x004BC21C
        static void onClose([[maybe_unused]] Window& self)
        {
            removeWallGhost();
            Ui::Windows::hideGridlines();
        }

        // 0x004BBCBF
        static void tabReset(Window* self)
        {
            Input::toolSet(self, Common::widx::panel, CursorId::placeFence);
            Input::setFlag(Input::Flags::flag6);
            _terraformGhostPlacedFlags = Common::GhostPlacedFlags::none;
            self->var_83C = 0;
            self->rowHover = -1;
            refreshWallList(self);
        }

        // 0x004BC44B
        static void onResize(Window& self)
        {
            self.invalidate();
            Ui::Size kMinWindowSize = { self.minWidth, self.minHeight };
            Ui::Size kMaxWindowSize = { self.maxWidth, self.maxHeight };
            bool hasResized = self.setSize(kMinWindowSize, kMaxWindowSize);
            if (hasResized)
                updateActiveThumb(&self);
        }

        // 0x004BC23D
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
                WindowManager::close(&self);

            if (ToolManager::getToolWindowType() != WindowType::terraform)
                WindowManager::close(&self);

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
                            self.savedView.mapX += 1;
                            if (self.savedView.mapX >= 8)
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
                    self.savedView.mapX = 0;
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
                args.pos = World::Pos3((*_terraformGhostPos).x, (*_terraformGhostPos).y, _terraformGhostBaseZ * World::kSmallZStep);
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
                _terraformGhostBaseZ = (*_lastPlacedWall)->baseZ();
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
            args.rotation = ViewportInteraction::getSideFromPos(res->first);
            args.primaryColour = Colour::black;
            args.secondaryColour = Colour::black;
            args.tertiaryColour = Colour::black;
            return { args };
        }

        // 0x004BC227
        static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            World::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            auto placementArgs = getWallPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeWallGhost();
                return;
            }

            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            World::TileManager::setMapSelectionCorner(placementArgs->rotation + 10);
            World::TileManager::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            World::TileManager::mapInvalidateSelectionRect();

            if ((_terraformGhostPlacedFlags & Common::GhostPlacedFlags::wall) != Common::GhostPlacedFlags::none)
            {
                if (*_terraformGhostPos == placementArgs->pos && _terraformGhostRotation == placementArgs->rotation && _terraformGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeWallGhost();
            _terraformGhostRotation = placementArgs->rotation;
            placeWallGhost(*placementArgs);
        }

        // 0x004BC232
        static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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

        // 0x004BC359
        static void getScrollSize(Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (self.var_83C + 9) / 10;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= kRowHeight;
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
                    LastGameOptionManager::setLastWall(static_cast<uint8_t>(rowInfo));

                    int32_t pan = (self.width >> 1) + self.x;
                    Audio::playSound(Audio::SoundId::clickDown, pan);
                    self.savedView.mapX = -16;
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
                    break;
            }
            if (i >= self.var_83C)
                rowInfo = 0xFFFF;
            self.var_846 = rowInfo;
            self.invalidate();
        }

        // 0x004BC212
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
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
        static void draw(Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto wallId = self.var_846;
            if (wallId == 0xFFFF)
            {
                wallId = self.rowHover;
                if (wallId == 0xFFFF)
                    return;
            }

            auto wallObj = ObjectManager::get<WallObject>(wallId);
            auto xPos = self.x + 3;
            auto yPos = self.y + self.height - 13;
            auto width = self.width - 19;

            drawingCtx.drawStringLeftClipped(*rt, xPos, yPos, width, Colour::black, StringIds::black_stringid, &wallObj->name);
        }

        // 0x004BC11C
        static void drawScroll(Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] uint32_t scrollIndex)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(rt, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        drawingCtx.drawRectInset(rt, xPos, yPos, 40, kRowHeight, self.getColour(WindowColour::secondary), Drawing::RectInsetFlags::colourLight);
                    }
                }
                else
                {
                    drawingCtx.drawRectInset(rt, xPos, yPos, 40, kRowHeight, self.getColour(WindowColour::secondary), (Drawing::RectInsetFlags::colourLight | Drawing::RectInsetFlags::borderInset));
                }

                auto wallObj = ObjectManager::get<WallObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(xPos + 1, yPos + 1, 39, 47));
                if (clipped)
                    drawingCtx.drawImage(&*clipped, 34, 28, wallObj->sprite);

                xPos += 40;

                if (xPos >= 40 * 10) // full row
                {
                    xPos = 0;
                    yPos += kRowHeight;
                }
            }
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.onMouseUp = Common::onMouseUp;
            events.onResize = onResize;
            events.onUpdate = onUpdate;
            events.event_08 = event_08;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.getScrollSize = getScrollSize;
            events.scrollMouseDown = scrollMouseDown;
            events.scrollMouseOver = scrollMouseOver;
            events.tooltip = tooltip;
            events.prepareDraw = prepareDraw;
            events.draw = draw;
            events.drawScroll = drawScroll;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t enabledWidgets;
            const uint64_t holdableWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { ClearArea::widgets, widx::tab_clear_area, &ClearArea::events, ClearArea::enabledWidgets, ClearArea::holdableWidgets },
            { AdjustLand::widgets, widx::tab_adjust_land, &AdjustLand::events, AdjustLand::enabledWidgets, AdjustLand::holdableWidgets },
            { AdjustWater::widgets, widx::tab_adjust_water, &AdjustWater::events, AdjustWater::enabledWidgets, AdjustWater::holdableWidgets },
            { PlantTrees::widgets, widx::tab_plant_trees, &PlantTrees::events, PlantTrees::enabledWidgets, PlantTrees::holdableWidgets },
            { BuildWalls::widgets, widx::tab_build_walls, &BuildWalls::events, BuildWalls::enabledWidgets, BuildWalls::holdableWidgets },
        };

        static void onResize(Window& self, uint8_t height)
        {
            self.flags |= WindowFlags::resizable;

            /*auto width = 130;
            if (isEditorMode())
                width += 31;*/

            // CHANGE: width set to 161 to include building walls tab
            uint16_t width = 161;
            Ui::Size kWindowSize = { width, height };
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x004BC78A, 0x004BCB0B
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
                WindowManager::close(&self);

            if (ToolManager::getToolWindowType() != WindowType::terraform)
                WindowManager::close(&self);

            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self.number, self.currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BCD82
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
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
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        static void prepareDraw(Window& self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self.currentTab].widgets;
            if (self.widgets != tabWidgets)
            {
                self.widgets = tabWidgets;
                self.initScrollWidgets();
            }

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
        static void drawTabs(Window* self, Gfx::RenderTarget* rt)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Clear Land Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_bulldozer;

                Widget::drawTab(self, rt, imageId, widx::tab_clear_area);
            }
            // Adjust Land Tab
            {
                auto landObj = ObjectManager::get<LandObject>(LastGameOptionManager::getLastLand());
                uint32_t imageId = landObj->mapPixelImage + Land::ImageIds::toolbar_terraform_land;

                Widget::drawTab(self, rt, imageId, widx::tab_adjust_land);
            }
            // Adjust Water Tab
            {
                auto waterObj = ObjectManager::get<WaterObject>();
                uint32_t imageId = waterObj->image + Water::ImageIds::kToolbarTerraformWater;
                if (self->currentTab == widx::tab_adjust_water - widx::tab_clear_area)
                    imageId += (self->frameNo / 2) % 16;

                Widget::drawTab(self, rt, imageId, widx::tab_adjust_water);
            }
            // Plant Trees Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_plant_trees;

                Widget::drawTab(self, rt, imageId, widx::tab_plant_trees);
            }
            // Build Walls Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_build_walls;

                Widget::drawTab(self, rt, imageId, widx::tab_build_walls);
            }
        }

        // 0x004BBB2B
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->currentTab = widgetIndex - widx::tab_clear_area;
            self->frameNo = 0;

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_clear_area];

            self->enabledWidgets = tabInfo.enabledWidgets;
            self->holdableWidgets = tabInfo.holdableWidgets;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;

            auto disabledWidgets = 0;

            // CHANGE: Disabled so the build walls tab shows outside of editor mode
            /*if (!isEditorMode() && !isSandboxMode())
                disabledWidgets |= common::widx::tab_build_walls;*/

            self->disabledWidgets = disabledWidgets;
            self->invalidate();

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

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
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

        static void initEvents()
        {
            PlantTrees::initEvents();
            ClearArea::initEvents();
            AdjustLand::initEvents();
            AdjustWater::initEvents();
            BuildWalls::initEvents();
        }
    }

    // 0x004BB566
    void openClearArea()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_clear_area);
    }

    // 0x004BB546
    void openAdjustLand()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_adjust_land);
    }

    // 0x004BB556
    void openAdjustWater()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_adjust_water);
    }

    // 0x004BB4A3
    void openPlantTrees()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_plant_trees);
    }

    // 0x004BB576
    void openBuildWalls()
    {
        auto terraformWindow = open();
        terraformWindow->callOnMouseUp(Common::widx::tab_build_walls);
    }

    bool rotate(Window* self)
    {
        if (self->currentTab == Common::widx::tab_plant_trees - Common::widx::tab_clear_area)
        {
            if (!self->isDisabled(PlantTrees::widx::rotate_object))
            {
                if (self->widgets[PlantTrees::widx::rotate_object].type != WidgetType::none)
                {
                    self->callOnMouseUp(PlantTrees::widx::rotate_object);
                    return true;
                }
            }
        }

        return false;
    }

    void setLastPlacedTree(World::TreeElement* elTree)
    {
        _lastPlacedTree = elTree;
    }
}
