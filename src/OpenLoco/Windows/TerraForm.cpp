#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Economy/Economy.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Map.hpp"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
#include "../Math/Trigonometry.hpp"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/TreeObject.h"
#include "../Objects/WallObject.h"
#include "../Objects/WaterObject.h"
#include "../Ui/Dropdown.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Numeric.hpp"
#include "../Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::GameCommands;

namespace OpenLoco::Ui::Windows::Terraform
{
    static loco_global<int16_t, 0x0052337A> _dragLastY;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<CompanyId, 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00525FB1> _lastSelectedTree;
    static loco_global<uint8_t, 0x00525FB6> _grassLand;
    static loco_global<uint8_t, 0x00525FCA> _lastSelectedWall;
    static loco_global<uint8_t, 0x009C870E> _adjustLandToolSize;
    static loco_global<uint8_t, 0x009C870F> _clearAreaToolSize;
    static loco_global<uint8_t, 0x009C8710> _adjustWaterToolSize;
    static loco_global<uint8_t, 0x00F003D2> _lastSelectedLand;
    static loco_global<uint8_t, 0x01136496> _treeRotation;
    static loco_global<Colour, 0x01136497> _treeColour;
    static loco_global<uint8_t, 0x0113649A> _terraformGhostPlaced;
    static loco_global<uint8_t, 0x0113649E> _treeClusterType;
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;
    static loco_global<uint32_t, 0x00F2530C> _raiseLandCost;
    static loco_global<uint32_t, 0x00F25310> _lowerLandCost;
    static loco_global<uint32_t, 0x01136484> _lastTreeCost;
    static loco_global<Map::TileElement*, 0x01136470> _lastPlacedWall;
    static loco_global<Map::TileElement*, 0x01136470> _lastPlacedTree;
    static loco_global<Map::Pos2, 0x01136488> _terraformGhostPos;
    static loco_global<uint16_t, 0x01136490> _lastTreeColourFlag;
    static loco_global<uint16_t, 0x01136492> _terraformGhostTreeRotationFlag;
    static loco_global<uint8_t, 0x01136499> _terraformGhostBaseZ;
    static loco_global<uint8_t, 0x0113649B> _terraformGhostTreeElementType;
    static loco_global<uint8_t, 0x0113649C> _terraformGhostType;
    static loco_global<uint8_t, 0x0113649D> _terraformGhostRotation; // wall
    static loco_global<uint8_t, 0x0113649D> _terraformGhostQuadrant; // tree
    static loco_global<uint32_t, 0x0113652C> _raiseWaterCost;
    static loco_global<uint32_t, 0x01136528> _lowerWaterCost;

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
        static void repositionTabs(Window* self);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void prepareDraw(Window* self);
        static void onUpdate(Window* self);
        static void onResize(Window* self, uint8_t height);
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex);
        static void sub_4A69DD();

        namespace GhostPlaced
        {
            constexpr uint8_t tree = (1 << 0);
            constexpr uint8_t wall = (1 << 1);
        }
    }

    namespace PlantTrees
    {
        static const Ui::Size windowSize = { 634, 162 };

        static const uint8_t rowHeight = 102;
        static const uint8_t columnWidth = 66;

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
            makeWidget({ 609, 70 }, { 24, 24 }, WidgetType::buttonWithColour, WindowColour::secondary, ImageIds::null, StringIds::tooltip_object_colour),
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
                    auto bit = Utility::bitScanReverse(treeObj->colours);
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

            i = (i / 9) * rowHeight;

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

            if (_lastSelectedTree != 0xFF)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (_lastSelectedTree == self->rowInfo[i])
                    {
                        rowHover = _lastSelectedTree;
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
        static void onClose(Window* self)
        {
            removeTreeGhost();
            Ui::Windows::hideGridlines();
        }

        // 0x004BBC7D
        static void tabReset(Window* self)
        {
            Input::toolSet(self, Common::widx::panel, CursorId::plantTree);
            Input::setFlag(Input::Flags::flag6);
            _terraformGhostPlaced = 0;
            _lastTreeCost = 0x80000000;
            self->var_83C = 0;
            self->rowHover = -1;
            refreshTreeList(self);
            updateTreeColours(self);
        }

        // 0x004BBAB5
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
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
                    _treeRotation = _treeRotation & 3;
                    self->invalidate();
                    break;
                }

                case widx::plant_cluster_selected:
                {
                    if (_treeClusterType == treeCluster::selected)
                        _treeClusterType = treeCluster::none;
                    else
                        _treeClusterType = treeCluster::selected;
                    self->invalidate();
                    break;
                }

                case widx::plant_cluster_random:
                {
                    if (_treeClusterType == treeCluster::random)
                        _treeClusterType = treeCluster::none;
                    else
                        _treeClusterType = treeCluster::random;
                    self->invalidate();
                }
            }
        }

        // 0x004BBFBD
        static void onResize(Window* self)
        {
            self->invalidate();
            Ui::Size minWindowSize = { self->minWidth, self->minHeight };
            Ui::Size maxWindowSize = { self->maxWidth, self->maxHeight };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x004BBAEA
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::object_colour && self->rowHover != -1)
            {
                auto obj = ObjectManager::get<TreeObject>(self->rowHover);
                Dropdown::showColour(self, &self->widgets[widgetIndex], obj->colours, _treeColour, self->getColour(WindowColour::secondary));
            }
        }

        // 0x004BBAF5
        static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
                return;
            if (itemIndex == -1)
                return;

            _treeColour = static_cast<Colour>(Dropdown::getHighlightedItem());
            self->invalidate();
        }

        // 0x004BBDA5
        static void onUpdate(Window* self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            if (!Input::hasFlag(Input::Flags::flag5))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self->findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self->savedView.mapX += 1;
                            if (self->savedView.mapX >= 8)
                            {
                                auto y = std::min(self->scrollAreas[0].contentHeight - 1 + 60, 562);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 358);
                                }
                                self->minWidth = windowSize.width;
                                self->minHeight = y;
                                self->maxWidth = windowSize.width;
                                self->maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self->minWidth = windowSize.width;
                                    self->minHeight = windowSize.height;
                                    self->maxWidth = windowSize.width;
                                    self->maxHeight = windowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->savedView.mapX = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self->minWidth = windowSize.width;
                        self->minHeight = windowSize.height;
                        self->maxWidth = windowSize.width;
                        self->maxHeight = windowSize.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BBEDF
        static void event_08(Window* self)
        {
            if (self->var_846 != 0xFFFF)
            {
                self->var_846 = -1;
                self->invalidate();
            }
        }
        // 0x004BD297 (bits of)
        static void removeTreeGhost()
        {
            if (_terraformGhostPlaced & Common::GhostPlaced::tree)
            {
                _terraformGhostPlaced = _terraformGhostPlaced & ~Common::GhostPlaced::tree;
                GameCommands::TreeRemovalArgs args;
                args.pos = Map::Pos3((*_terraformGhostPos).x, (*_terraformGhostPos).y, _terraformGhostBaseZ * 4);
                args.type = _terraformGhostType;
                args.elementType = _terraformGhostTreeElementType;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
        }

        // 0x004BD237
        static currency32_t placeTreeGhost(const GameCommands::TreePlacementArgs& placementArgs)
        {
            removeTreeGhost();

            auto res = GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            if (res != GameCommands::FAILURE)
            {
                _terraformGhostPos = placementArgs.pos;
                _terraformGhostTreeElementType = (*_lastPlacedTree)->rawData()[0];
                _terraformGhostType = placementArgs.type;
                _terraformGhostBaseZ = (*_lastPlacedTree)->baseZ();
                _terraformGhostPlaced |= Common::GhostPlaced::tree;

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
            args.pos = Map::Pos3(res->first.x & 0xFFE0, res->first.y & 0xFFE0, 0);
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
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            auto placementArgs = getTreePlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeTreeGhost();
                return;
            }

            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            Map::TileManager::setMapSelectionCorner((placementArgs->quadrant ^ (1 << 1)) + 6);
            Map::TileManager::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            Map::TileManager::mapInvalidateSelectionRect();

            if (_terraformGhostPlaced & Common::GhostPlaced::tree)
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

        static loco_global<uint8_t, 0x00525FB4> _currentSnowLine;

        // 0x004BDF19
        static std::optional<uint8_t> getRandomTreeTypeFromSurface(const Map::TilePos2& loc, bool unk)
        {
            if (!Map::validCoords(loc))
            {
                return {};
            }

            auto* surface = Map::TileManager::get(loc).surface();
            if (surface == nullptr)
            {
                return {};
            }

            uint16_t mustNotTreeFlags = 0;
            if (unk)
            {
                mustNotTreeFlags |= TreeObjectFlags::unk1;
            }

            uint16_t mustTreeFlags = 0;
            if (surface->baseZ() - 4 > _currentSnowLine)
            {
                mustTreeFlags |= TreeObjectFlags::hasSnowVariation;
            }
            if (surface->baseZ() > 68)
            {
                mustTreeFlags |= TreeObjectFlags::veryHighAltitude;
            }
            if (surface->baseZ() > 48)
            {
                mustTreeFlags |= TreeObjectFlags::highAltitude;
            }

            auto* landObj = ObjectManager::get<LandObject>(surface->terrain());
            mustNotTreeFlags |= TreeObjectFlags::droughtResistant;
            if (landObj->flags & LandObjectFlags::isDesert)
            {
                mustTreeFlags |= TreeObjectFlags::droughtResistant;
                mustNotTreeFlags &= ~TreeObjectFlags::droughtResistant;
            }

            if (landObj->flags & LandObjectFlags::noTrees)
            {
                return {};
            }
            mustNotTreeFlags |= TreeObjectFlags::requiresWater;
            const uint16_t numSameTypeSurfaces = TileManager::countSurroundingWaterTiles(loc);
            if (numSameTypeSurfaces >= 8)
            {
                mustNotTreeFlags &= ~TreeObjectFlags::requiresWater;
            }

            std::vector<uint8_t> selectableTrees;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::tree); ++i)
            {
                auto* treeObj = ObjectManager::get<TreeObject>(i);
                if (treeObj == nullptr)
                {
                    continue;
                }
                if (treeObj->flags & mustNotTreeFlags)
                {
                    continue;
                }

                if ((treeObj->flags & mustTreeFlags) != mustTreeFlags)
                {
                    continue;
                }
                selectableTrees.push_back(i);
            }

            if (selectableTrees.empty())
            {
                return {};
            }

            auto& rng = gPrng();
            return { selectableTrees[rng.randNext(selectableTrees.size() - 1)] };
        }

        // 0x004BDC67 & 0x004BDDC6
        template<typename TTreeTypeFunc>
        static bool clusterToolDown(const GameCommands::TreePlacementArgs& baseArgs, const uint16_t range, const uint16_t density, TTreeTypeFunc&& getTreeType)
        {
            const auto numPlacements = (range * range * density) / 8192;
            uint16_t numErrors = 0;
            for (auto i = 0; i < numPlacements; ++i)
            {
                // Choose a random offset in a circle
                auto& rng = gPrng();
                auto randomMagnitude = rng.randNext(std::numeric_limits<uint16_t>::max()) * range / 65536;
                auto randomDirection = rng.randNext(Math::Trigonometry::directionPrecisionHigh - 1);
                Map::Pos2 randomOffset(
                    Math::Trigonometry::integerSinePrecisionHigh(randomDirection, randomMagnitude),
                    Math::Trigonometry::integerCosinePrecisionHigh(randomDirection, randomMagnitude));

                GameCommands::TreePlacementArgs args;
                Map::Pos2 newLoc = randomOffset + baseArgs.pos;
                args.quadrant = ViewportInteraction::getQuadrantFromPos(newLoc);
                args.pos = Map::Pos2(newLoc.x & 0xFFE0, newLoc.y & 0xFFE0);
                // Note: this is not the same as the randomDirection above as it is the trees rotation
                args.rotation = rng.randNext(3);
                args.colour = Colour::black;
                auto type = getTreeType(newLoc, false);
                if (!type)
                {
                    continue;
                }
                args.type = *type;
                args.buildImmediately = true;
                args.requiresFullClearance = true;

                // First query if we can place a tree at this location; skip if we can't.
                auto queryRes = doCommand(args, 0);
                if (queryRes == GameCommands::FAILURE)
                {
                    numErrors++;
                    continue;
                }

                // Actually place the tree
                doCommand(args, GameCommands::Flags::apply);
            }

            // Have we placed any trees?
            if (numErrors < numPlacements)
                return true;

            Error::open(StringIds::cant_plant_this_here, StringIds::empty);
            return false;
        }

        // 0x004BBB20
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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

                        if (clusterToolDown(*placementArgs, 320, 3, [type = placementArgs->type](const Map::TilePos2&, bool) { return std::optional<uint8_t>(type); }))
                        {
                            auto height = TileManager::getHeight(placementArgs->pos);
                            Audio::playSound(Audio::SoundId::construct, Map::Pos3{ placementArgs->pos.x, placementArgs->pos.y, height.landHeight });
                        }

                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(previousId);
                        break;
                    }
                    case treeCluster::random:
                        auto previousId = CompanyManager::getUpdatingCompanyId();
                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(CompanyId::neutral);

                        if (clusterToolDown(*placementArgs, 384, 4, getRandomTreeTypeFromSurface))
                        {
                            auto height = TileManager::getHeight(placementArgs->pos);
                            Audio::playSound(Audio::SoundId::construct, Map::Pos3{ placementArgs->pos.x, placementArgs->pos.y, height.landHeight });
                        }

                        if (isEditorMode())
                            CompanyManager::setUpdatingCompanyId(previousId);
                        break;
                }
            }
        }

        // 0x004BBEC1
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (self->var_83C + 8) / 9;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= rowHeight;
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / columnWidth) + (y / rowHeight) * 9;
        }

        // 0x004BBF3B
        static void scrollMouseDown(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            int16_t xPos = (x / columnWidth);
            int16_t yPos = (y / rowHeight) * 5;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                {
                    self->rowHover = rowInfo;
                    _lastSelectedTree = static_cast<uint8_t>(rowInfo);

                    updateTreeColours(self);

                    int32_t pan = (self->width >> 1) + self->x;
                    Map::Pos3 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    Audio::playSound(Audio::SoundId::clickDown, loc, pan);
                    self->savedView.mapX = -16;
                    _lastTreeCost = 0x80000000;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x004BBEF8
        static void scrollMouseOver(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = y;
            auto i = 0;
            for (; i < self->var_83C; i++)
            {
                rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                {
                    self->var_846 = rowInfo;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x004BBB00
        static std::optional<FormatArguments> tooltip(Window* self, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_trees_list);
            return args;
        }

        // 0x004BB756
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->activatedWidgets &= ~((1ULL << widx::plant_cluster_selected) | (1ULL << widx::plant_cluster_random));

            if (_treeClusterType == treeCluster::selected)
                self->activatedWidgets |= (1ULL << widx::plant_cluster_selected);

            if (_treeClusterType == treeCluster::random)
                self->activatedWidgets |= (1ULL << widx::plant_cluster_random);

            self->widgets[widx::rotate_object].type = WidgetType::none;
            self->widgets[widx::object_colour].type = WidgetType::none;

            if (self->rowHover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self->rowHover);
                if (treeObj->name != 0xFFFF)
                {
                    if (treeObj->num_rotations != 1)
                        self->widgets[widx::rotate_object].type = WidgetType::buttonWithImage;

                    if (treeObj->colours != 0)
                    {
                        self->widgets[widx::object_colour].image = Widget::imageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, *_treeColour);
                        self->widgets[widx::object_colour].type = WidgetType::buttonWithColour;
                    }
                }
            }

            self->widgets[widx::scrollview].right = self->width - 26;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::rotate_object].left = self->width - 25;
            self->widgets[widx::object_colour].left = self->width - 25;
            self->widgets[widx::plant_cluster_selected].left = self->width - 25;
            self->widgets[widx::plant_cluster_random].left = self->width - 25;

            self->widgets[widx::rotate_object].right = self->width - 2;
            self->widgets[widx::object_colour].right = self->width - 2;
            self->widgets[widx::plant_cluster_selected].right = self->width - 2;
            self->widgets[widx::plant_cluster_random].right = self->width - 2;

            Common::repositionTabs(self);
        }

        // 0x004BB8C9
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto treeId = self->var_846;
            if (treeId == 0xFFFF)
            {
                treeId = self->rowHover;
                if (treeId == 0xFFFF)
                    return;
            }

            auto treeObj = ObjectManager::get<TreeObject>(treeId);

            uint32_t treeCost = 0x80000000;
            if (self->var_846 == 0xFFFF)
            {
                treeCost = _lastTreeCost;
                if (treeCost == 0x80000000)
                {
                    treeCost = Economy::getInflationAdjustedCost(treeObj->build_cost_factor, treeObj->cost_index, 12);
                }
            }
            else
            {
                treeCost = Economy::getInflationAdjustedCost(treeObj->build_cost_factor, treeObj->cost_index, 12);
            }
            auto args = FormatArguments();
            args.push<uint32_t>(treeCost);

            if (!isEditorMode())
            {
                auto xPos = self->x + 3 + self->width - 17;
                auto yPos = self->y + self->height - 13;
                Gfx::drawString_494C78(*context, xPos, yPos, Colour::black, StringIds::build_cost, &args);
            }
            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            auto width = self->width - 19 - xPos;
            Gfx::drawString_494BBF(*context, xPos, yPos, width, Colour::black, StringIds::black_stringid, &treeObj->name);
        }

        static void drawTreeThumb(TreeObject* treeObj, Gfx::Context* clipped)
        {
            uint32_t image = treeObj->getTreeGrowthDisplayOffset() * treeObj->num_rotations;
            auto rotation = (treeObj->num_rotations - 1) & _treeRotation;
            image += rotation;
            image += treeObj->sprites[0][treeObj->season_state];

            auto colourOptions = treeObj->colours;
            if (colourOptions != 0)
            {
                auto colour = *_treeColour;
                if (!(_lastTreeColourFlag & (1 << 5)))
                {
                    auto bit = Utility::bitScanReverse(colourOptions);
                    colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                }
                image = Gfx::recolour(image, colour);
            }
            Gfx::drawImage(clipped, 32, 96, image);
        }

        // 0x004BB982
        static void drawScroll(Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            Gfx::clearSingle(context, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                _lastTreeColourFlag = 0xFFFF;
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        _lastTreeColourFlag = AdvancedColour::translucent_flag;
                        Gfx::drawRectInset(context, xPos, yPos, 65, rowHeight - 1, self.getColour(WindowColour::secondary).u8(), AdvancedColour::translucent_flag);
                    }
                }
                else
                {
                    _lastTreeColourFlag = AdvancedColour::translucent_flag | AdvancedColour::outline_flag;
                    Gfx::drawRectInset(context, xPos, yPos, 65, rowHeight - 1, self.getColour(WindowColour::secondary).u8(), (AdvancedColour::translucent_flag | AdvancedColour::outline_flag));
                }

                auto treeObj = ObjectManager::get<TreeObject>(self.rowInfo[i]);
                auto clipped = Gfx::clipContext(context, Ui::Rect(xPos + 1, yPos + 1, 64, rowHeight - 2));
                if (clipped)
                {
                    drawTreeThumb(treeObj, &*clipped);
                }

                xPos += columnWidth;

                if (xPos >= columnWidth * 9) // full row
                {
                    xPos = 0;
                    yPos += rowHeight;
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
            auto origin = Ui::Point(Ui::width() - PlantTrees::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::terraform,
                origin,
                PlantTrees::windowSize,
                WindowFlags::flag_11,
                &PlantTrees::events);

            window->number = 0;
            window->currentTab = Common::widx::tab_plant_trees - Common::widx::tab_clear_area;
            window->frame_no = 0;
            _terraformGhostPlaced = 0;
            _lastTreeCost = 0x80000000;
            window->owner = _player_company;
            window->var_846 = 0xFFFF;
            window->savedView.mapX = 0;
            _treeClusterType = PlantTrees::treeCluster::none;

            WindowManager::sub_4CEE0B(window);

            window->minWidth = PlantTrees::windowSize.width;
            window->minHeight = PlantTrees::windowSize.height;
            window->maxWidth = PlantTrees::windowSize.width;
            window->maxHeight = PlantTrees::windowSize.height;

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
        static void onClose(Window* self)
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
        static void onResize(Window* self)
        {
            Common::onResize(self, 105);
        }

        // 0x004BC65C
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                        _adjustToolSize = 1;
                    _clearAreaToolSize = _adjustToolSize;
                    self->invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 10)
                        _adjustToolSize = 10;
                    _clearAreaToolSize = _adjustToolSize;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x004BC677
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

            uint32_t cost = 0x80000000;
            auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (res)
            {
                if (Map::TileManager::setMapSelectionTiles(res->first, 4) == 0)
                {
                    return;
                }
                const auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();
                const Pos2 centre = (pointA + pointB) / 2;
                cost = GameCommands::do_66(centre, pointA, pointB, GameCommands::Flags::flag_2 | GameCommands::Flags::flag_6);
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
                auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();
                Pos2 centre = (pointA + pointB) / 2;
                GameCommands::setErrorTitle(StringIds::error_cant_clear_entire_area);

                GameCommands::do_66(centre, pointA, pointB, flags);
            }
        }

        // 0x004BC689
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            clearLand(Flags::apply);
        }

        // 0x004BC682
        static void toolDragContinue(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
        static void toolDragEnd(Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            }
        }

        // 0x004BC555
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->activatedWidgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;

            Common::repositionTabs(self);
        }

        // 0x004BC5E7
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            if (_raiseLandCost == 0x80000000)
                return;

            if (_raiseLandCost == 0)
                return;

            auto xPos = self->widgets[widx::tool_area].left + self->widgets[widx::tool_area].right;
            xPos /= 2;
            xPos += self->x;
            auto yPos = self->widgets[widx::tool_area].bottom + self->y + 5;

            auto args = FormatArguments();
            args.push<uint32_t>(_raiseLandCost);

            Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::clear_land_cost, &args);
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
            land_material,
            paint_mode
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area) | (1 << land_material) | (1 << paint_mode);
        const uint64_t holdableWidgets = (1 << decrease_area) | (1 << increase_area);
        static bool isPaintMode = false;

        Widget widgets[] = {
            commonWidgets(130, 105, StringIds::title_adjust_land),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, WidgetType::wt_3, WindowColour::secondary, ImageIds::tool_area, StringIds::tooltip_adjust_land_tool),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_land_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, WidgetType::toolbarTab, WindowColour::secondary, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_land_area),
            makeWidget({ 69 + 16, 92 }, { 20, 20 }, WidgetType::wt_6, WindowColour::primary),
            makeWidget({ 39 + 16, 92 }, { 28, 28 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::null, StringIds::tooltip_paint_landscape_tool),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x004BC9D1
        static void onClose(Window* self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBBF7
        static void tabReset(Window* self)
        {
            Input::toolSet(
                self,
                isPaintMode
                    ? static_cast<uint16_t>(widx::paint_mode)
                    : static_cast<uint16_t>(Common::widx::panel),
                CursorId::landTool);

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
        static void onResize(Window* self)
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
                args.push(landObj->var_16 + Land::ImageIds::landscape_generator_tile_icon);
                args.push<uint16_t>(i);

                Dropdown::add(landIndex, 0xFFFE, args);

                landIndex++;
            }
        }

        // 0x004BC9A7
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::land_material:
                {
                    showDropdown(self, widgetIndex);
                    break;
                }

                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 0)
                        _adjustToolSize = 0;
                    _adjustLandToolSize = _adjustToolSize;
                    self->invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 10)
                        _adjustToolSize = 10;
                    _adjustLandToolSize = _adjustToolSize;
                    self->invalidate();
                    break;
                }

                case widx::paint_mode:
                {
                    isPaintMode = !isPaintMode;
                    tabReset(self);
                }
            }
        }

        // 0x004BC9C6
        static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::land_material)
                return;
            if (itemIndex == -1)
                return;
            _lastSelectedLand = Dropdown::getItemArgument(itemIndex, 2);
            self->invalidate();
        }

        // 0x00468DFD
        static uint32_t lowerLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                Common::sub_4A69DD();

            auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();
            auto centre = (pointA + pointB) / 2;
            GameCommands::setErrorTitle(StringIds::error_cant_lower_land_here);

            if (_adjustToolSize == 0)
            {
                uint16_t di = 0xFFFF;
                cost = GameCommands::do_27(centre, pointA, pointB, di, flags);
            }
            else
            {
                auto corner = Map::TileManager::getMapSelectionCorner();
                cost = GameCommands::do_26(centre, pointA, pointB, corner, flags);
            }
            return cost;
        }

        // 0x00468D1D
        static uint32_t raiseLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                Common::sub_4A69DD();

            auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();
            auto centre = (pointA + pointB) / 2;
            GameCommands::setErrorTitle(StringIds::error_cant_raise_land_here);

            if (_adjustToolSize == 0)
            {
                uint16_t di = 1;
                cost = GameCommands::do_27(centre, pointA, pointB, di, flags);
            }
            else
            {
                uint16_t corner = Map::TileManager::getMapSelectionCorner();
                cost = GameCommands::do_25(centre, pointA, pointB, corner, flags);
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

        static void onPaintToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

            uint32_t cost = 0x80000000;
            auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
            if (res)
            {
                if (Map::TileManager::setMapSelectionTiles(res->first, 4) == 0)
                {
                    return;
                }
                const auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();
                const Pos2 centre = (pointA + pointB) / 2;
                cost = GameCommands::do_66(centre, pointA, pointB, GameCommands::Flags::flag_2 | GameCommands::Flags::flag_6);
            }

            if (cost != _raiseLandCost)
            {
                _raiseLandCost = cost;
                WindowManager::invalidate(WindowType::terraform);
            }
        }

        static void onAdjustLandToolUpdate(const OpenLoco::Ui::WidgetIndex_t& widgetIndex, const int16_t& x, const int16_t& y)
        {
            uint16_t xPos = 0;

            TileManager::mapInvalidateSelectionRect();

            if (Ui::getToolCursor() != CursorId::upDownArrow)
            {
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
                auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
                if (res)
                {
                    if (_adjustLandToolSize != 1)
                    {
                        auto count = TileManager::setMapSelectionTiles(res->first, 4);

                        if (!count)
                            return;
                    }
                    else
                    {
                        auto count = TileManager::setMapSelectionSingleTile(res->first, true);

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
                    auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();

                    ChangeLandMaterialArgs args{};
                    args.pointA = pointA;
                    args.pointB = pointB;
                    args.landType = _lastSelectedLand;
                    GameCommands::doCommand(args, Flags::apply);
                }
            }
        }

        // 0x004BC9ED
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            switch (widgetIndex)
            {
                case widx::paint_mode:
                    paintLand();
                    break;
                case Common::widx::panel:
                    if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
                        return;

                    Ui::setToolCursor(CursorId::upDownArrow);
                    break;
            }
        }

        // 0x004BC9E2
        static void toolDragContinue(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
        static void toolDragEnd(Window& self, const WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::paint_mode:
                case Common::widx::panel:
                    TileManager::mapInvalidateSelectionRect();

                    Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
                    Ui::setToolCursor(CursorId::landTool);
                    break;
            }
        }

        // 0x004BC83B
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->activatedWidgets |= (1ULL << widx::tool_area);

            if (isPaintMode)
            {
                self->activatedWidgets |= (1 << widx::paint_mode);
            }
            else
            {
                self->activatedWidgets &= ~(1 << widx::paint_mode);
            }

            self->widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;

            self->widgets[widx::land_material].type = WidgetType::none;

            // CHANGE: Allows the player to select which material is used in the adjust land tool outside of editor mode.
            if (_adjustToolSize != 0)
            {
                self->widgets[widx::land_material].type = WidgetType::wt_6;

                auto landObj = ObjectManager::get<LandObject>(_lastSelectedLand);

                self->widgets[widx::land_material].image = landObj->var_16 + OpenLoco::Land::ImageIds::landscape_generator_tile_icon;
            }

            Common::repositionTabs(self);
        }

        // 0x004BC909
        static void draw(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            auto imgId = skin->img;
            self->widgets[widx::paint_mode].image = imgId + InterfaceSkin::ImageIds::tab_colour_scheme_frame0;

            self->draw(context);

            Common::drawTabs(self, context);

            auto xPos = self->widgets[widx::tool_area].left + self->widgets[widx::tool_area].right;
            xPos /= 2;
            xPos += self->x;
            auto yPos = self->widgets[widx::tool_area].bottom + self->y + 28;

            if (_raiseLandCost != 0x80000000)
            {
                if (_raiseLandCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_raiseLandCost);
                    Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::increase_height_cost, &args);
                }
            }

            yPos += 10;

            if (_lowerLandCost != 0x80000000)
            {
                if (_lowerLandCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_lowerLandCost);
                    Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::decrease_height_cost, &args);
                }
            }
        }

        static void initEvents()
        {
            events.onClose = onClose;
            events.onMouseUp = Common::onMouseUp;
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
        static void onClose(Window* self)
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
        static void onResize(Window* self)
        {
            Common::onResize(self, 115);
        }

        // 0x004BCD9D
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::decrease_area:
                {
                    _adjustToolSize--;
                    if (_adjustToolSize < 1)
                        _adjustToolSize = 1;
                    _adjustWaterToolSize = _adjustToolSize;
                    self->invalidate();
                    break;
                }

                case widx::increase_area:
                {
                    _adjustToolSize++;
                    if (_adjustToolSize > 10)
                        _adjustToolSize = 10;
                    _adjustWaterToolSize = _adjustToolSize;
                    self->invalidate();
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

        // 0x004BCDB4
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            TileManager::mapInvalidateSelectionRect();

            if (Ui::getToolCursor() != CursorId::upDownArrow)
            {
                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

                auto res = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::surface | ViewportInteraction::InteractionItemFlags::water));
                auto& interaction = res.first;
                if (interaction.type == ViewportInteraction::InteractionItem::noInteraction)
                {
                    setAdjustCost(0x80000000, 0x80000000);
                    return;
                }
                if (!TileManager::setMapSelectionTiles(interaction.pos + Map::Pos2(16, 16), 5))
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
                auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();
                auto lowerCost = GameCommands::do_29(pointA, pointB, 0);
                auto raiseCost = GameCommands::do_28(pointA, pointB, 0);
                setAdjustCost(raiseCost, lowerCost);
            }
        }

        // 0x004BCDCA
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            if (!Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
                return;

            Ui::setToolCursor(CursorId::upDownArrow);
        }

        static void raiseWater(uint8_t flags)
        {
            Common::sub_4A69DD();
            GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();

            GameCommands::do_28(pointA, pointB, flags);
        }

        static void lowerWater(uint8_t flags)
        {
            Common::sub_4A69DD();
            GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            auto [pointA, pointB] = Map::TileManager::getMapSelectionArea();

            GameCommands::do_29(pointA, pointB, flags);
        }

        // 0x004BCDBF
        static void toolDragContinue(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
        static void toolDragEnd(Window& self, const WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();

                Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
                Ui::setToolCursor(CursorId::waterTool);
            }
        }

        // 0x004BCC6D
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->activatedWidgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;

            Common::repositionTabs(self);
        }

        // 0x004BCCFF
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto xPos = self->widgets[widx::tool_area].left + self->widgets[widx::tool_area].right;
            xPos /= 2;
            xPos += self->x;
            auto yPos = self->widgets[widx::tool_area].bottom + self->y + 5;

            if (_raiseWaterCost != 0x80000000)
            {
                if (_raiseWaterCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_raiseWaterCost);

                    Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::increase_height_cost, &args);
                }
            }

            yPos += 10;

            if (_lowerWaterCost != 0x80000000)
            {
                if (_lowerWaterCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_lowerWaterCost);

                    Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::decrease_height_cost, &args);
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
        static const Ui::Size windowSize = { 418, 108 };

        static const uint8_t rowHeight = 48;

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

            i = (i / 10) * rowHeight;

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

            if (_lastSelectedTree != 0xFF)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (_lastSelectedWall == self->rowInfo[i])
                    {
                        rowHover = _lastSelectedWall;
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
        static void onClose(Window* self)
        {
            removeWallGhost();
            Ui::Windows::hideGridlines();
        }

        // 0x004BBCBF
        static void tabReset(Window* self)
        {
            Input::toolSet(self, Common::widx::panel, CursorId::placeFence);
            Input::setFlag(Input::Flags::flag6);
            _terraformGhostPlaced = 0;
            self->var_83C = 0;
            self->rowHover = -1;
            refreshWallList(self);
        }

        // 0x004BC44B
        static void onResize(Window* self)
        {
            self->invalidate();
            Ui::Size minWindowSize = { self->minWidth, self->minHeight };
            Ui::Size maxWindowSize = { self->maxWidth, self->maxHeight };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x004BC23D
        static void onUpdate(Window* self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            if (!Input::hasFlag(Input::Flags::flag5))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self->findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self->savedView.mapX += 1;
                            if (self->savedView.mapX >= 8)
                            {
                                auto y = std::min(self->scrollAreas[0].contentHeight - 1 + 60, 562);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 358);
                                }
                                self->minWidth = windowSize.width;
                                self->minHeight = y;
                                self->maxWidth = windowSize.width;
                                self->maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self->minWidth = windowSize.width;
                                    self->minHeight = windowSize.height;
                                    self->maxWidth = windowSize.width;
                                    self->maxHeight = windowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->savedView.mapX = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self->minWidth = windowSize.width;
                        self->minHeight = windowSize.height;
                        self->maxWidth = windowSize.width;
                        self->maxHeight = windowSize.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BC377
        static void event_08(Window* self)
        {
            if (self->var_846 != 0xFFFF)
            {
                self->var_846 = -1;
                self->invalidate();
            }
        }

        // 0x004BD297 (bits of)
        static void removeWallGhost()
        {
            if (_terraformGhostPlaced & Common::GhostPlaced::wall)
            {
                _terraformGhostPlaced = _terraformGhostPlaced & ~Common::GhostPlaced::wall;
                GameCommands::WallRemovalArgs args;
                args.pos = Map::Pos3((*_terraformGhostPos).x, (*_terraformGhostPos).y, _terraformGhostBaseZ * 4);
                args.rotation = _terraformGhostRotation;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
        }

        // 0x004BD4C8
        static void placeWallGhost(const GameCommands::WallPlacementArgs& placementArgs)
        {
            removeWallGhost();

            if (GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6) != GameCommands::FAILURE)
            {
                _terraformGhostPos = placementArgs.pos;
                _terraformGhostRotation = placementArgs.rotation;
                _terraformGhostTreeElementType = placementArgs.rotation; // Unsure why duplicated not used
                _terraformGhostType = placementArgs.type;
                _terraformGhostBaseZ = (*_lastPlacedWall)->baseZ();
                _terraformGhostPlaced |= Common::GhostPlaced::wall;
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
            args.pos = Map::Pos3(res->first.x & 0xFFE0, res->first.y & 0xFFE0, 0);
            args.type = self->rowHover;
            args.rotation = ViewportInteraction::getSideFromPos(res->first);
            args.primaryColour = Colour::black;
            args.secondaryColour = Colour::black;
            args.unk = 0;
            return { args };
        }

        // 0x004BC227
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
            {
                return;
            }
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            auto placementArgs = getWallPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeWallGhost();
                return;
            }

            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            Map::TileManager::setMapSelectionCorner(placementArgs->rotation + 10);
            Map::TileManager::setMapSelectionArea(placementArgs->pos, placementArgs->pos);
            Map::TileManager::mapInvalidateSelectionRect();

            if (_terraformGhostPlaced & Common::GhostPlaced::wall)
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
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
        static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (self->var_83C + 9) / 10;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= rowHeight;
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 40) + (y / rowHeight) * 10;
        }

        // 0x004BC3D3
        static void scrollMouseDown(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            int16_t xPos = (x / 40);
            int16_t yPos = (y / rowHeight) * 10;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                {
                    self->rowHover = rowInfo;
                    _lastSelectedWall = static_cast<uint8_t>(rowInfo);

                    int32_t pan = (self->width >> 1) + self->x;
                    Map::Pos3 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    Audio::playSound(Audio::SoundId::clickDown, loc, pan);
                    self->savedView.mapX = -16;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x004BC390
        static void scrollMouseOver(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = 0xFFFF;
            auto i = 0;

            for (; i < self->var_83C; i++)
            {
                rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                    break;
            }
            if (i >= self->var_83C)
                rowInfo = 0xFFFF;
            self->var_846 = rowInfo;
            self->invalidate();
        }

        // 0x004BC212
        static std::optional<FormatArguments> tooltip(Window* self, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_walls_list);
            return args;
        }

        // 0x004BC029
        static void prepareDraw(Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            Common::repositionTabs(self);
        }

        // 0x004BC0C2
        static void draw(Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto wallId = self->var_846;
            if (wallId == 0xFFFF)
            {
                wallId = self->rowHover;
                if (wallId == 0xFFFF)
                    return;
            }

            auto wallObj = ObjectManager::get<WallObject>(wallId);
            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            auto width = self->width - 19;

            Gfx::drawString_494BBF(*context, xPos, yPos, width, Colour::black, StringIds::black_stringid, &wallObj->name);
        }

        // 0x004BC11C
        static void drawScroll(Window& self, Gfx::Context& context, uint32_t scrollIndex)
        {
            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            Gfx::clearSingle(context, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        Gfx::drawRectInset(context, xPos, yPos, 40, rowHeight, self.getColour(WindowColour::secondary).u8(), AdvancedColour::translucent_flag);
                    }
                }
                else
                {
                    Gfx::drawRectInset(context, xPos, yPos, 40, rowHeight, self.getColour(WindowColour::secondary).u8(), (AdvancedColour::translucent_flag | AdvancedColour::outline_flag));
                }

                auto wallObj = ObjectManager::get<WallObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipContext(context, Ui::Rect(xPos + 1, yPos + 1, 39, 47));
                if (clipped)
                    Gfx::drawImage(&*clipped, 34, 28, wallObj->sprite);

                xPos += 40;

                if (xPos >= 40 * 10) // full row
                {
                    xPos = 0;
                    yPos += rowHeight;
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

        static void onResize(Window* self, uint8_t height)
        {
            self->flags |= WindowFlags::resizable;

            /*auto width = 130;
            if (isEditorMode())
                width += 31;*/

            // CHANGE: width set to 161 to include building walls tab
            uint16_t width = 161;
            Ui::Size windowSize = { width, height };
            self->setSize(windowSize, windowSize);
        }

        // 0x004BC78A, 0x004BCB0B
        static void onUpdate(Window* self)
        {
            if (!Input::hasFlag(Input::Flags::toolActive))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->currentTab + Common::widx::tab_clear_area);
        }

        // 0x004BCD82
        static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
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

        // 0x004BCF29, 0x004BCF2F
        static void repositionTabs(Window* self)
        {
            int16_t xPos = self->widgets[widx::tab_clear_area].left;
            const int16_t tabWidth = self->widgets[widx::tab_clear_area].right - xPos;

            for (uint8_t i = widx::tab_clear_area; i <= widx::tab_build_walls; i++)
            {
                if (self->isDisabled(i))
                {
                    self->widgets[i].type = WidgetType::none;
                    continue;
                }

                self->widgets[i].type = WidgetType::tab;
                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->currentTab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab..
            self->activatedWidgets &= ~((1ULL << tab_adjust_land) | (1ULL << tab_adjust_water) | (1ULL << tab_build_walls) | (1ULL << tab_clear_area) | (1ULL << tab_plant_trees));
            self->activatedWidgets |= (1ULL << tabInformationByTabOffset[self->currentTab].widgetIndex);

            self->widgets[widx::frame].right = self->width - 1;
            self->widgets[widx::frame].bottom = self->height - 1;

            self->widgets[widx::panel].right = self->width - 1;
            self->widgets[widx::panel].bottom = self->height - 1;

            self->widgets[widx::caption].right = self->width - 2;

            self->widgets[widx::close_button].left = self->width - 15;
            self->widgets[widx::close_button].right = self->width - 3;
        }

        // 0x004BCF7F
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Clear Land Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_bulldozer;

                Widget::drawTab(self, context, imageId, widx::tab_clear_area);
            }
            // Adjust Land Tab
            {
                auto landObj = ObjectManager::get<LandObject>(_grassLand);
                uint32_t imageId = landObj->var_16 + Land::ImageIds::toolbar_terraform_land;

                Widget::drawTab(self, context, imageId, widx::tab_adjust_land);
            }
            // Adjust Water Tab
            {
                auto waterObj = ObjectManager::get<WaterObject>();
                uint32_t imageId = waterObj->image + Water::ImageIds::toolbar_terraform_water;
                if (self->currentTab == widx::tab_adjust_water - widx::tab_clear_area)
                    imageId += (self->frame_no / 2) % 16;

                Widget::drawTab(self, context, imageId, widx::tab_adjust_water);
            }
            // Plant Trees Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_plant_trees;

                Widget::drawTab(self, context, imageId, widx::tab_plant_trees);
            }
            // Build Walls Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_build_walls;

                Widget::drawTab(self, context, imageId, widx::tab_build_walls);
            }
        }

        // 0x004BBB2B
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->currentTab = widgetIndex - widx::tab_clear_area;
            self->frame_no = 0;

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
        auto terraform_window = open();
        terraform_window->callOnMouseUp(Common::widx::tab_clear_area);
    }

    // 0x004BB546
    void openAdjustLand()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(Common::widx::tab_adjust_land);
    }

    // 0x004BB556
    void openAdjustWater()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(Common::widx::tab_adjust_water);
    }

    // 0x004BB4A3
    void openPlantTrees()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(Common::widx::tab_plant_trees);
    }

    // 0x004BB576
    void openBuildWalls()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(Common::widx::tab_build_walls);
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
}
