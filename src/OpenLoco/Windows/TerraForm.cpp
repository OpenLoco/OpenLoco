#include "../Audio/Audio.h"
#include "../Economy/Economy.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Map/Map.hpp"
#include "../Map/Tile.h"
#include "../Map/TileManager.h"
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
    static loco_global<std::uint8_t[10], 0x00500775> _byte_500775;
    static loco_global<int16_t, 0x0052337A> _dragLastY;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<int8_t, 0x00523393> _currentTool;
    static loco_global<CompanyId_t, 0x00525E3C> _player_company;
    static loco_global<uint8_t, 0x00525FB1> _lastSelectedTree;
    static loco_global<uint8_t, 0x00525FB6> _grassLand;
    static loco_global<uint8_t, 0x00525FCA> _lastSelectedWall;
    static loco_global<uint8_t, 0x009C870E> _adjustLandToolSize;
    static loco_global<uint8_t, 0x009C870F> _clearAreaToolSize;
    static loco_global<uint8_t, 0x009C8710> _adjustWaterToolSize;
    static loco_global<uint8_t, 0x00F003D2> _lastSelectedLand;
    static loco_global<coord_t, 0x00F24486> _mapSelectionAX;
    static loco_global<coord_t, 0x00F24488> _mapSelectionBX;
    static loco_global<coord_t, 0x00F2448A> _mapSelectionAY;
    static loco_global<coord_t, 0x00F2448C> _mapSelectionBY;
    static loco_global<uint16_t, 0x00F2448E> _word_F2448E;
    static loco_global<uint8_t, 0x01136496> _treeRotation;
    static loco_global<uint8_t, 0x01136497> _treeColour;
    static loco_global<uint8_t, 0x0113649A> _byte_113649A;
    static loco_global<uint8_t, 0x0113649E> _treeClusterType;
    static loco_global<int16_t, 0x0050A000> _adjustToolSize;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    static loco_global<uint32_t, 0x00F2530C> _raiseLandCost;
    static loco_global<uint32_t, 0x00F25310> _lowerLandCost;
    static loco_global<uint32_t, 0x01136484> _lastTreeCost;
    static loco_global<uint16_t, 0x01136488> _word_1136488;
    static loco_global<uint16_t, 0x0113648A> _word_113648A;
    static loco_global<uint16_t, 0x01136490> _lastTreeColourFlag;
    static loco_global<uint8_t, 0x01136499> _byte_1136499;
    static loco_global<uint8_t, 0x0113649B> _byte_113649B;
    static loco_global<uint8_t, 0x0113649C> _byte_113649C;
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                        \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                          \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                     \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { 130, 74 }, widget_type::panel, 1),                                                                     \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, ImageIds::tab, StringIds::tooltip_clear_land),                    \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, ImageIds::tab, StringIds::tooltip_adjust_land),                   \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, ImageIds::tab, StringIds::tooltip_adjust_water),                  \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, ImageIds::tab, StringIds::tooltip_plant_trees),                   \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, ImageIds::tab, StringIds::tooltip_build_walls)

        static window_event_list _events;

        static void initEvents();
        static void sub_4BD297();
        static void switchTab(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void drawTabs(window* self, Gfx::Context* context);
        static void prepareDraw(window* self);
        static void onUpdate(window* self);
        static void onResize(window* self, uint8_t height);
        static void onMouseUp(window* self, widget_index widgetIndex);
        static void sub_4A69DD();
    }

    namespace PlantTrees
    {
        static const Gfx::ui_size_t windowSize = { 634, 162 };

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

        widget_t widgets[] = {
            commonWidgets(634, 162, StringIds::title_plant_trees),
            makeWidget({ 3, 45 }, { 605, 101 }, widget_type::scrollview, 1, scrollbars::vertical),
            makeWidget({ 609, 46 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::rotate_object, StringIds::rotate_object_90),
            makeWidget({ 609, 70 }, { 24, 24 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_object_colour),
            makeWidget({ 609, 94 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::plant_cluster_selected_tree, StringIds::plant_cluster_selected_tree),
            makeWidget({ 609, 118 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::plant_cluster_random_tree, StringIds::plant_cluster_random_tree),
            widgetEnd(),
        };

        static window_event_list events;

        enum treeCluster
        {
            none = 0,
            selected,
            random,
        };

        // 0x004BB6B2
        static void updateTreeColours(window* self)
        {
            if (self->row_hover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self->row_hover);
                if (treeObj->colours != 0)
                {
                    Colour_t colour = Utility::bitScanReverse(treeObj->colours);
                    if (colour == 0xFF)
                        colour = 0;
                    _treeColour = colour;
                }
            }
        }

        // 0x004BC4B7
        static void updateActiveThumb(window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scroll_areas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->row_info[i] == self->row_hover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 9) * rowHeight;

            self->scroll_areas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB63F
        static void refreshTreeList(window* self)
        {
            auto treeCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(object_type::tree); i++)
            {
                auto treeObj = ObjectManager::get<TreeObject>(i);
                if (treeObj == nullptr)
                    continue;
                self->row_info[treeCount] = i;
                treeCount++;
            }

            self->var_83C = treeCount;
            auto rowHover = -1;

            if (_lastSelectedTree != 0xFF)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (_lastSelectedTree == self->row_info[i])
                    {
                        rowHover = _lastSelectedTree;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->row_info[0];
            }

            self->row_hover = rowHover;

            updateActiveThumb(self);
            updateTreeColours(self);
        }

        // 0x004BBB0A
        static void onClose(window* self)
        {
            Common::sub_4BD297();
            Ui::Windows::hideGridlines();
        }

        // 0x004BBC7D
        static void tabReset(window* self)
        {
            Input::toolSet(self, Common::widx::panel, 8);
            Input::setFlag(Input::input_flags::flag6);
            _byte_113649A = 0;
            _lastTreeCost = 0x80000000;
            self->var_83C = 0;
            self->row_hover = -1;
            refreshTreeList(self);
            updateTreeColours(self);
        }

        // 0x004BBAB5
        static void onMouseUp(window* self, widget_index widgetIndex)
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
        static void onResize(window* self)
        {
            self->invalidate();
            Gfx::ui_size_t minWindowSize = { self->min_width, self->min_height };
            Gfx::ui_size_t maxWindowSize = { self->max_width, self->max_height };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x004BBAEA
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == widx::object_colour)
            {
                registers regs;
                regs.edx = widgetIndex;
                regs.esi = (uint32_t)self;
                regs.edi = (int32_t)&self->widgets[widgetIndex];
                call(0x004BBD59, regs);
            }
        }

        // 0x004BBAF5
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
                return;
            if (itemIndex == -1)
                return;

            _treeColour = Dropdown::getHighlightedItem();
            self->invalidate();
        }

        // 0x004BBDA5
        static void onUpdate(window* self)
        {
            if (!Input::hasFlag(Input::input_flags::tool_active))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            if (!Input::hasFlag(Input::input_flags::flag5))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        widget_index activeWidget = self->findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].contentHeight - 1 + 60, 562);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 358);
                                }
                                self->min_width = windowSize.width;
                                self->min_height = y;
                                self->max_width = windowSize.width;
                                self->max_height = y;
                            }
                            else
                            {
                                if (Input::state() != Input::input_state::scroll_left)
                                {
                                    self->min_width = windowSize.width;
                                    self->min_height = windowSize.height;
                                    self->max_width = windowSize.width;
                                    self->max_height = windowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->saved_view.mapX = 0;
                    if (Input::state() != Input::input_state::scroll_left)
                    {
                        self->min_width = windowSize.width;
                        self->min_height = windowSize.height;
                        self->max_width = windowSize.width;
                        self->max_height = windowSize.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->current_tab + Common::widx::tab_clear_area);
        }

        // 0x004BBEDF
        static void event_08(window* self)
        {
            if (self->var_846 != 0xFFFF)
            {
                self->var_846 = -1;
                self->invalidate();
            }
        }

        // 0x004BBB15
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = uint32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BBB15, regs);
        }

        // 0x004BBB20
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = uint32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BBB20, regs);
        }

        // 0x004BBEC1
        static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
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
        static void scrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            int16_t xPos = (x / columnWidth);
            int16_t yPos = (y / rowHeight) * 5;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->row_info[i];
                index--;
                if (index < 0)
                {
                    self->row_hover = rowInfo;
                    _lastSelectedTree = static_cast<uint8_t>(rowInfo);

                    updateTreeColours(self);

                    int32_t pan = (self->width >> 1) + self->x;
                    Map::Pos3 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    Audio::playSound(Audio::SoundId::clickDown, loc, pan);
                    self->saved_view.mapX = -16;
                    _lastTreeCost = 0x80000000;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x004BBEF8
        static void scrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = y;
            auto i = 0;
            for (; i < self->var_83C; i++)
            {
                rowInfo = self->row_info[i];
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
        static std::optional<FormatArguments> tooltip(window* self, widget_index widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_trees_list);
            return args;
        }

        // 0x004BB756
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            self->activated_widgets &= ~((1ULL << widx::plant_cluster_selected) | (1ULL << widx::plant_cluster_random));

            if (_treeClusterType == treeCluster::selected)
                self->activated_widgets |= (1ULL << widx::plant_cluster_selected);

            if (_treeClusterType == treeCluster::random)
                self->activated_widgets |= (1ULL << widx::plant_cluster_random);

            self->widgets[widx::rotate_object].type = widget_type::none;
            self->widgets[widx::object_colour].type = widget_type::none;

            if (self->row_hover != -1)
            {
                auto treeObj = ObjectManager::get<TreeObject>(self->row_hover);
                if (treeObj->name != 0xFFFF)
                {
                    if (treeObj->num_rotations != 1)
                        self->widgets[widx::rotate_object].type = widget_type::wt_9;

                    if (treeObj->colours != 0)
                    {

                        self->widgets[widx::object_colour].image = (1 << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, _treeColour);
                        self->widgets[widx::object_colour].type = widget_type::wt_10;
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
        static void draw(window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto treeId = self->var_846;
            if (treeId == 0xFFFF)
            {
                treeId = self->row_hover;
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
            uint32_t image = _byte_500775[treeObj->growth] * treeObj->num_rotations;
            auto rotation = (treeObj->num_rotations - 1) & _treeRotation;
            image += rotation;
            image += treeObj->sprites[treeObj->season_state];

            auto colourOptions = treeObj->colours;
            if (colourOptions != 0)
            {
                Colour_t colour = _treeColour;
                if (!(_lastTreeColourFlag & (1 << 5)))
                {
                    colour = Utility::bitScanReverse(colourOptions);
                    if (colour == 0xFF)
                        colour = 0;
                }
                image = Gfx::recolour(image, colour);
            }
            Gfx::drawImage(clipped, 32, 96, image);
        }

        // 0x004BB982
        static void drawScroll(window* self, Gfx::Context* context, uint32_t scrollIndex)
        {
            auto shade = Colour::getShade(self->colours[1], 3);
            Gfx::clearSingle(*context, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                _lastTreeColourFlag = 0xFFFF;
                if (self->row_info[i] != self->row_hover)
                {
                    if (self->row_info[i] == self->var_846)
                    {
                        _lastTreeColourFlag = Colour::translucent_flag;
                        Gfx::drawRectInset(context, xPos, yPos, 65, rowHeight - 1, self->colours[1], Colour::translucent_flag);
                    }
                }
                else
                {
                    _lastTreeColourFlag = Colour::translucent_flag | Colour::outline_flag;
                    Gfx::drawRectInset(context, xPos, yPos, 65, rowHeight - 1, self->colours[1], (Colour::translucent_flag | Colour::outline_flag));
                }

                auto treeObj = ObjectManager::get<TreeObject>(self->row_info[i]);
                Gfx::Context* clipped = nullptr;

                if (Gfx::clipContext(&clipped, context, xPos + 1, yPos + 1, 64, rowHeight - 2))
                {
                    drawTreeThumb(treeObj, clipped);
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
            events.on_close = onClose;
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.on_update = onUpdate;
            events.event_08 = event_08;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.get_scroll_size = getScrollSize;
            events.scroll_mouse_down = scrollMouseDown;
            events.scroll_mouse_over = scrollMouseOver;
            events.tooltip = tooltip;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.draw_scroll = drawScroll;
        }
    }

    // 0x004BB4A3
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::terraform, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_plant_trees);
        }
        else
        {
            // 0x004BB586
            auto origin = Gfx::point_t(Ui::width() - PlantTrees::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::terraform,
                origin,
                PlantTrees::windowSize,
                WindowFlags::flag_11,
                &PlantTrees::events);

            window->number = 0;
            window->current_tab = Common::widx::tab_plant_trees - Common::widx::tab_clear_area;
            window->frame_no = 0;
            _byte_113649A = 0;
            _lastTreeCost = 0x80000000;
            window->owner = _player_company;
            window->var_846 = 0xFFFF;
            window->saved_view.mapX = 0;
            _treeClusterType = PlantTrees::treeCluster::none;

            WindowManager::sub_4CEE0B(window);

            window->min_width = PlantTrees::windowSize.width;
            window->min_height = PlantTrees::windowSize.height;
            window->max_width = PlantTrees::windowSize.width;
            window->max_height = PlantTrees::windowSize.height;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->colours[1] = skin->colour_0E;

            // End of 0x004BB586

            Ui::Windows::showGridlines();
            _treeRotation = 2;

            Common::initEvents();

            window->invalidate();

            window->widgets = PlantTrees::widgets;
            window->enabled_widgets = PlantTrees::enabledWidgets;
            window->holdable_widgets = 0;
            window->activated_widgets = 0;

            window->disabled_widgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();

            window->var_83C = 0;
            window->row_hover = -1;

            PlantTrees::refreshTreeList(window);

            Input::toolSet(window, Common::widx::panel, 18);

            Input::setFlag(Input::input_flags::flag6);
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

        widget_t widgets[] = {
            commonWidgets(130, 105, StringIds::clear_area),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, widget_type::wt_3, 1, ImageIds::tool_area, StringIds::tooltip_clear_area),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, widget_type::wt_7, 1, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_clear_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, widget_type::wt_7, 1, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_clear_area),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BC671
        static void onClose(window* self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBBC7
        static void tabReset(window* self)
        {
            Input::toolSet(self, Common::widx::panel, 41);
            Input::setFlag(Input::input_flags::flag6);
            _raiseLandCost = 0x80000000;
            _adjustToolSize = _clearAreaToolSize;
        }

        // 0x004BC7C6
        static void onResize(window* self)
        {
            Common::onResize(self, 105);
        }

        // 0x004BC65C
        static void onMouseDown(window* self, widget_index widgetIndex)
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
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC677, regs);
        }

        static void clearLand(uint8_t flags)
        {
            if ((_mapSelectionFlags & 1))
            {
                int16_t x = _mapSelectionAX + _mapSelectionBX;
                int16_t y = _mapSelectionAY + _mapSelectionBY;
                x /= 2;
                y /= 2;
                Pos2 centre = { x, y };
                Pos2 pointA = { _mapSelectionAX, _mapSelectionAY };
                Pos2 pointB = { _mapSelectionBX, _mapSelectionBY };
                GameCommands::setErrorTitle(StringIds::error_cant_clear_entire_area);

                GameCommands::do_66(centre, pointA, pointB, flags);
            }
        }

        // 0x004BC689
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            clearLand(Flags::apply);
        }

        // 0x004BC682
        static void toolDragContinue(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
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
        static void toolDragEnd(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
            }
        }

        // 0x004BC555
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            self->activated_widgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;

            Common::repositionTabs(self);
        }

        // 0x004BC5E7
        static void draw(window* self, Gfx::Context* context)
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
            events.on_close = onClose;
            events.on_mouse_up = Common::onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_update = Common::onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.toolDragContinue = (void (*)(window&, const widget_index, const int16_t, const int16_t))0x004BC682;
            events.toolDragEnd = (void (*)(window&, const widget_index))0x004BC701;
            events.prepare_draw = prepareDraw;
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
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area) | (1 << land_material);

        widget_t widgets[] = {
            commonWidgets(130, 105, StringIds::title_adjust_land),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, widget_type::wt_3, 1, ImageIds::tool_area, StringIds::tooltip_adjust_land_tool),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, widget_type::wt_7, 1, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_land_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, widget_type::wt_7, 1, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_land_area),
            makeWidget({ 55 + 16, 92 }, { 20, 20 }, widget_type::wt_6, 0),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BC9D1
        static void onClose(window* self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBBF7
        static void tabReset(window* self)
        {
            Input::toolSet(self, Common::widx::panel, 18);
            Input::setFlag(Input::input_flags::flag6);
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
        static void onResize(window* self)
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
        static void showDropdown(window* self, widget_index widgetIndex)
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
            auto colour = self->colours[1] | 0x80;
            auto count = Dropdown::getItemsPerRow(landCount);

            Dropdown::showImage(xPos, yPos, 20, 20, heightOffset, colour, count, landCount);

            auto landIndex = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(object_type::land); i++)
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
        static void onMouseDown(window* self, widget_index widgetIndex)
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
            }
        }

        // 0x004BC9C6
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
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

            int16_t x = _mapSelectionAX + _mapSelectionBX;
            int16_t y = _mapSelectionAY + _mapSelectionBY;
            x /= 2;
            y /= 2;
            Pos2 centre = { x, y };
            Pos2 pointA = { _mapSelectionAX, _mapSelectionAY };
            Pos2 pointB = { _mapSelectionBX, _mapSelectionBY };
            GameCommands::setErrorTitle(StringIds::error_cant_lower_land_here);

            if (_adjustToolSize == 0)
            {
                uint16_t di = 0xFFFF;
                cost = GameCommands::do_27(centre, pointA, pointB, di, flags);
            }
            else
            {
                uint16_t di = _word_F2448E;
                cost = GameCommands::do_26(centre, pointA, pointB, di, flags);
            }
            return cost;
        }

        // 0x00468D1D
        static uint32_t raiseLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                Common::sub_4A69DD();

            int16_t x = _mapSelectionAX + _mapSelectionBX;
            int16_t y = _mapSelectionAY + _mapSelectionBY;
            x /= 2;
            y /= 2;
            Pos2 centre = { x, y };
            Pos2 pointA = { _mapSelectionAX, _mapSelectionAY };
            Pos2 pointB = { _mapSelectionBX, _mapSelectionBY };
            GameCommands::setErrorTitle(StringIds::error_cant_raise_land_here);

            if (_adjustToolSize == 0)
            {
                uint16_t di = 1;
                cost = GameCommands::do_27(centre, pointA, pointB, di, flags);
            }
            else
            {
                uint16_t di = _word_F2448E;
                cost = GameCommands::do_25(centre, pointA, pointB, di, flags);
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

        // 0x004BC9D7
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            uint16_t xPos = 0;
            if (widgetIndex != Common::widx::panel)
                return;

            TileManager::mapInvalidateSelectionRect();

            if (_currentTool != 3)
            {
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
                if (_adjustLandToolSize != 1)
                {
                    auto count = TileManager::setMapSelectionTiles(x, y);

                    if (count == 0x8000)
                        xPos = 0x8000;

                    if (!count)
                        return;
                }
                else
                {
                    auto count = TileManager::setMapSelectionSingleTile(x, y, true);

                    if (count == 0x8000)
                        xPos = 0x8000;

                    if (!count)
                        return;
                }
            }
            else
            {
                if (!(_mapSelectionFlags & 1))
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

        // 0x004BC9ED
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;
            if (!(_mapSelectionFlags & 1))
                return;

            // CHANGE: Allows the player to change land type outside of the scenario editor.
            if (_adjustToolSize != 0)
            {
                if (_lastSelectedLand != 0xFF)
                {
                    GameCommands::setErrorTitle(StringIds::error_cant_change_land_type);
                    Pos2 pointA = { _mapSelectionAX, _mapSelectionAY };
                    Pos2 pointB = { _mapSelectionBX, _mapSelectionBY };

                    GameCommands::do_24(pointA, pointB, _lastSelectedLand, Flags::apply);
                }
            }

            _currentTool = 3;
        }

        // 0x004BC9E2
        static void toolDragContinue(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;

            auto window = WindowManager::findAt(x, y);
            if (window == nullptr)
                return;

            widget_index newWidgetIndex = window->findWidgetAt(x, y);
            if (newWidgetIndex == -1)
                return;

            auto widget = window->widgets[newWidgetIndex];
            if (widget.type != widget_type::viewport)
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
                raiseLand(flags);
            }
            else
            {
                dY = -dY;
                if (deltaY < dY)
                    return;
                _dragLastY = _dragLastY + dY;
                lowerLand(flags);
            }
            _raiseLandCost = 0x80000000;
            _lowerLandCost = 0x80000000;
        }

        // 0x004BCA5D
        static void toolDragEnd(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
                _currentTool = 18;
            }
        }

        // 0x004BC83B
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            self->activated_widgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;

            self->widgets[widx::land_material].type = widget_type::none;

            // CHANGE: Allows the player to select which material is used in the adjust land tool outside of editor mode.
            if (_adjustToolSize != 0)
            {
                self->widgets[widx::land_material].type = widget_type::wt_6;

                auto landObj = ObjectManager::get<LandObject>(_lastSelectedLand);

                self->widgets[widx::land_material].image = landObj->var_16 + OpenLoco::Land::ImageIds::landscape_generator_tile_icon;
            }

            Common::repositionTabs(self);
        }

        // 0x004BC909
        static void draw(window* self, Gfx::Context* context)
        {
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
            events.on_close = onClose;
            events.on_mouse_up = Common::onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.on_update = Common::onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.toolDragContinue = (void (*)(window&, const widget_index, const int16_t, const int16_t))0x004BC9E2;
            events.toolDragEnd = (void (*)(window&, const widget_index))0x004BCA5D;
            events.prepare_draw = prepareDraw;
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

        widget_t widgets[] = {
            commonWidgets(130, 105, StringIds::title_adjust_water),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, widget_type::wt_3, 1, ImageIds::tool_area, StringIds::tooltip_adjust_water_tool),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, widget_type::wt_7, 1, Gfx::recolour(ImageIds::decrease_tool_area, Colour::white), StringIds::tooltip_decrease_adjust_water_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, widget_type::wt_7, 1, Gfx::recolour(ImageIds::increase_tool_area, Colour::white), StringIds::tooltip_increase_adjust_water_area),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BCDAE
        static void onClose(window* self)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x004BBC46
        static void tabReset(window* self)
        {
            Input::toolSet(self, Common::widx::panel, 19);
            Input::setFlag(Input::input_flags::flag6);
            _raiseWaterCost = 0x80000000;
            _lowerWaterCost = 0x80000000;
            _adjustToolSize = _adjustWaterToolSize;
        }

        // 0x004BCEB4
        static void onResize(window* self)
        {
            Common::onResize(self, 115);
        }

        // 0x004BCD9D
        static void onMouseDown(window* self, widget_index widgetIndex)
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

        // 0x004BCDB4
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BCDB4, regs);
        }

        // 0x004BCDCA
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BCDCA, regs);
        }

        static void raiseWater(uint8_t flags)
        {
            Common::sub_4A69DD();
            GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            Pos2 pointA = { _mapSelectionAX, _mapSelectionAY };
            Pos2 pointB = { _mapSelectionBX, _mapSelectionBY };

            GameCommands::do_28(pointA, pointB, flags);
        }

        static void lowerWater(uint8_t flags)
        {
            Common::sub_4A69DD();
            GameCommands::setErrorTitle(StringIds::error_cant_raise_water_here);
            Pos2 pointA = { _mapSelectionAX, _mapSelectionAY };
            Pos2 pointB = { _mapSelectionBX, _mapSelectionBY };

            GameCommands::do_29(pointA, pointB, flags);
        }

        // 0x004BCDBF
        static void toolDragContinue(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != Common::widx::panel)
                return;

            auto window = WindowManager::findAt(x, y);
            if (window == nullptr)
                return;

            widget_index newWidgetIndex = window->findWidgetAt(x, y);
            if (newWidgetIndex == -1)
                return;

            auto widget = window->widgets[newWidgetIndex];
            if (widget.type != widget_type::viewport)
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
        static void toolDragEnd(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == Common::widx::panel)
            {
                TileManager::mapInvalidateSelectionRect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
                _currentTool = 19;
            }
        }

        // 0x004BCC6D
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            self->activated_widgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + ImageIds::tool_area;

            Common::repositionTabs(self);
        }

        // 0x004BCCFF
        static void draw(window* self, Gfx::Context* context)
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
            events.on_close = onClose;
            events.on_mouse_up = Common::onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_update = Common::onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.toolDragContinue = (void (*)(window&, const widget_index, const int16_t, const int16_t))0x004BCDBF;
            events.toolDragEnd = (void (*)(window&, const widget_index))0x004BCDE8;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace BuildWalls
    {
        static const Gfx::ui_size_t windowSize = { 418, 108 };

        static const uint8_t rowHeight = 48;

        enum widx
        {
            scrollview = 9,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(418, 108, StringIds::title_build_walls),
            makeWidget({ 2, 45 }, { 391, 48 }, widget_type::scrollview, 1, scrollbars::vertical),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BC506
        static void updateActiveThumb(window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scroll_areas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->row_info[i] == self->row_hover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 10) * rowHeight;

            self->scroll_areas[0].contentOffsetY = i;
            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB6D5
        static void refreshWallList(window* self)
        {
            auto wallCount = 0;
            for (uint16_t i = 0; i < ObjectManager::getMaxObjects(object_type::wall); i++)
            {
                auto wallObj = ObjectManager::get<WallObject>(i);
                if (wallObj == nullptr)
                    continue;
                self->row_info[wallCount] = i;
                wallCount++;
            }

            self->var_83C = wallCount;
            auto rowHover = -1;

            if (_lastSelectedTree != 0xFF)
            {
                for (auto i = 0; i < self->var_83C; i++)
                {
                    if (_lastSelectedWall == self->row_info[i])
                    {
                        rowHover = _lastSelectedWall;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->row_info[0];
            }

            self->row_hover = rowHover;

            updateActiveThumb(self);
        }

        // 0x004BC21C
        static void onClose(window* self)
        {
            Common::sub_4BD297();
            Ui::Windows::hideGridlines();
        }

        // 0x004BBCBF
        static void tabReset(window* self)
        {
            Input::toolSet(self, Common::widx::panel, 15);
            Input::setFlag(Input::input_flags::flag6);
            _byte_113649A = 0;
            self->var_83C = 0;
            self->row_hover = -1;
            refreshWallList(self);
        }

        // 0x004BC44B
        static void onResize(window* self)
        {
            self->invalidate();
            Gfx::ui_size_t minWindowSize = { self->min_width, self->min_height };
            Gfx::ui_size_t maxWindowSize = { self->max_width, self->max_height };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x004BC23D
        static void onUpdate(window* self)
        {
            if (!Input::hasFlag(Input::input_flags::tool_active))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            if (!Input::hasFlag(Input::input_flags::flag5))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        widget_index activeWidget = self->findWidgetAt(xPos, yPos);

                        if (activeWidget > Common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].contentHeight - 1 + 60, 562);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 358);
                                }
                                self->min_width = windowSize.width;
                                self->min_height = y;
                                self->max_width = windowSize.width;
                                self->max_height = y;
                            }
                            else
                            {
                                if (Input::state() != Input::input_state::scroll_left)
                                {
                                    self->min_width = windowSize.width;
                                    self->min_height = windowSize.height;
                                    self->max_width = windowSize.width;
                                    self->max_height = windowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->saved_view.mapX = 0;
                    if (Input::state() != Input::input_state::scroll_left)
                    {
                        self->min_width = windowSize.width;
                        self->min_height = windowSize.height;
                        self->max_width = windowSize.width;
                        self->max_height = windowSize.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->current_tab + Common::widx::tab_clear_area);
        }

        // 0x004BC377
        static void event_08(window* self)
        {
            if (self->var_846 != 0xFFFF)
            {
                self->var_846 = -1;
                self->invalidate();
            }
        }

        // 0x004BC227
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC227, regs);
        }

        // 0x004BC232
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = int32_t(&self);
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004BC232, regs);
        }

        // 0x004BC359
        static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
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
        static void scrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            int16_t xPos = (x / 40);
            int16_t yPos = (y / rowHeight) * 10;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->row_info[i];
                index--;
                if (index < 0)
                {
                    self->row_hover = rowInfo;
                    _lastSelectedWall = static_cast<uint8_t>(rowInfo);

                    int32_t pan = (self->width >> 1) + self->x;
                    Map::Pos3 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    Audio::playSound(Audio::SoundId::clickDown, loc, pan);
                    self->saved_view.mapX = -16;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x004BC390
        static void scrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = 0xFFFF;
            auto i = 0;

            for (; i < self->var_83C; i++)
            {
                rowInfo = self->row_info[i];
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
        static std::optional<FormatArguments> tooltip(window* self, widget_index widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_walls_list);
            return args;
        }

        // 0x004BC029
        static void prepareDraw(window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            Common::repositionTabs(self);
        }

        // 0x004BC0C2
        static void draw(window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto wallId = self->var_846;
            if (wallId == 0xFFFF)
            {
                wallId = self->row_hover;
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
        static void drawScroll(window* self, Gfx::Context* context, uint32_t scrollIndex)
        {
            auto shade = Colour::getShade(self->colours[1], 3);
            Gfx::clearSingle(*context, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                if (self->row_info[i] != self->row_hover)
                {
                    if (self->row_info[i] == self->var_846)
                    {
                        Gfx::drawRectInset(context, xPos, yPos, 40, rowHeight, self->colours[1], Colour::translucent_flag);
                    }
                }
                else
                {
                    Gfx::drawRectInset(context, xPos, yPos, 40, rowHeight, self->colours[1], (Colour::translucent_flag | Colour::outline_flag));
                }

                auto wallObj = ObjectManager::get<WallObject>(self->row_info[i]);

                Gfx::Context* clipped = nullptr;

                if (Gfx::clipContext(&clipped, context, xPos + 1, yPos + 1, 39, 47))
                    Gfx::drawImage(clipped, 34, 28, wallObj->sprite);

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
            events.on_close = onClose;
            events.on_mouse_up = Common::onMouseUp;
            events.on_resize = onResize;
            events.on_update = onUpdate;
            events.event_08 = event_08;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.get_scroll_size = getScrollSize;
            events.scroll_mouse_down = scrollMouseDown;
            events.scroll_mouse_over = scrollMouseOver;
            events.tooltip = tooltip;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.draw_scroll = drawScroll;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { ClearArea::widgets, widx::tab_clear_area, &ClearArea::events, ClearArea::enabledWidgets },
            { AdjustLand::widgets, widx::tab_adjust_land, &AdjustLand::events, AdjustLand::enabledWidgets },
            { AdjustWater::widgets, widx::tab_adjust_water, &AdjustWater::events, AdjustWater::enabledWidgets },
            { PlantTrees::widgets, widx::tab_plant_trees, &PlantTrees::events, PlantTrees::enabledWidgets },
            { BuildWalls::widgets, widx::tab_build_walls, &BuildWalls::events, BuildWalls::enabledWidgets },
        };

        static void onResize(window* self, uint8_t height)
        {
            self->flags |= WindowFlags::resizable;

            /*auto width = 130;
            if (isEditorMode())
                width += 31;*/

            // CHANGE: width set to 161 to include building walls tab
            uint16_t width = 161;
            Gfx::ui_size_t windowSize = { width, height };
            self->setSize(windowSize, windowSize);
        }

        // 0x004BC78A, 0x004BCB0B
        static void onUpdate(window* self)
        {
            if (!Input::hasFlag(Input::input_flags::tool_active))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->current_tab + Common::widx::tab_clear_area);
        }

        // 0x004BCD82
        static void onMouseUp(window* self, widget_index widgetIndex)
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
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_clear_area].left;
            const int16_t tabWidth = self->widgets[widx::tab_clear_area].right - xPos;

            for (uint8_t i = widx::tab_clear_area; i <= widx::tab_build_walls; i++)
            {
                if (self->isDisabled(i))
                {
                    self->widgets[i].type = widget_type::none;
                    continue;
                }

                self->widgets[i].type = widget_type::wt_8;
                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        static void prepareDraw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab..
            self->activated_widgets &= ~((1ULL << tab_adjust_land) | (1ULL << tab_adjust_water) | (1ULL << tab_build_walls) | (1ULL << tab_clear_area) | (1ULL << tab_plant_trees));
            self->activated_widgets |= (1ULL << tabInformationByTabOffset[self->current_tab].widgetIndex);

            self->widgets[widx::frame].right = self->width - 1;
            self->widgets[widx::frame].bottom = self->height - 1;

            self->widgets[widx::panel].right = self->width - 1;
            self->widgets[widx::panel].bottom = self->height - 1;

            self->widgets[widx::caption].right = self->width - 2;

            self->widgets[widx::close_button].left = self->width - 15;
            self->widgets[widx::close_button].right = self->width - 3;
        }

        // 0x004BCF7F
        static void drawTabs(window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Clear Land Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_bulldozer;

                Widget::draw_tab(self, context, imageId, widx::tab_clear_area);
            }
            // Adjust Land Tab
            {
                auto landObj = ObjectManager::get<LandObject>(_grassLand);
                uint32_t imageId = landObj->var_16 + Land::ImageIds::toolbar_terraform_land;

                Widget::draw_tab(self, context, imageId, widx::tab_adjust_land);
            }
            // Adjust Water Tab
            {
                auto waterObj = ObjectManager::get<WaterObject>();
                uint32_t imageId = waterObj->image + Water::ImageIds::toolbar_terraform_water;
                if (self->current_tab == widx::tab_adjust_water - widx::tab_clear_area)
                    imageId += (self->frame_no / 2) % 16;

                Widget::draw_tab(self, context, imageId, widx::tab_adjust_water);
            }
            // Plant Trees Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_plant_trees;

                Widget::draw_tab(self, context, imageId, widx::tab_plant_trees);
            }
            // Build Walls Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_build_walls;

                Widget::draw_tab(self, context, imageId, widx::tab_build_walls);
            }
        }

        // 0x004BD297
        static void sub_4BD297()
        {
            registers regs;
            call(0x004BD297, regs);
        }

        // 0x004BBB2B
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_clear_area;
            self->frame_no = 0;

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_clear_area];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            auto disabledWidgets = 0;

            // CHANGE: Disabled so the build walls tab shows outside of editor mode
            /*if (!isEditorMode() && !isSandboxMode())
                disabledWidgets |= common::widx::tab_build_walls;*/

            self->disabled_widgets = disabledWidgets;
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
            registers regs;
            call(0x004A69DD, regs);
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

    bool rotate(window* self)
    {
        if (self->current_tab == Common::widx::tab_plant_trees - Common::widx::tab_clear_area)
        {
            if (!self->isDisabled(PlantTrees::widx::rotate_object))
            {
                if (self->widgets[PlantTrees::widx::rotate_object].type != widget_type::none)
                {
                    self->callOnMouseUp(PlantTrees::widx::rotate_object);
                    return true;
                }
            }
        }

        return false;
    }

    void registerHooks()
    {
        registerHook(
            0x004BC9E2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                AdjustLand::toolDragContinue((Ui::window&)regs.esi, (widget_index)regs.dx, (int16_t)regs.ax, (int16_t)regs.bx);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004BCA5D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                AdjustLand::toolDragEnd((Ui::window&)regs.esi, (widget_index)regs.dx);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004BCDBF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                AdjustWater::toolDragContinue((Ui::window&)regs.esi, (widget_index)regs.dx, (int16_t)regs.ax, (int16_t)regs.bx);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004BCDE8,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                AdjustWater::toolDragEnd((Ui::window&)regs.esi, (widget_index)regs.dx);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004BC682,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                ClearArea::toolDragContinue((Ui::window&)regs.esi, (widget_index)regs.dx, (int16_t)regs.ax, (int16_t)regs.bx);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004BC701,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                ClearArea::toolDragEnd((Ui::window&)regs.esi, (widget_index)regs.dx);
                regs = backup;
                return 0;
            });
    }
}
