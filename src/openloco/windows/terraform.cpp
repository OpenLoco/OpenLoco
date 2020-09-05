#include "../audio/audio.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../map/tile.h"
#include "../map/tilemgr.h"
#include "../objects/interface_skin_object.h"
#include "../objects/land_object.h"
#include "../objects/objectmgr.h"
#include "../objects/tree_object.h"
#include "../objects/wall_object.h"
#include "../objects/water_object.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../ui/scrollview.h"
#include "../utility/numeric.hpp"
#include "../widget.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::game_commands;

namespace openloco::ui::windows::terraform
{
    static loco_global<std::uint8_t[10], 0x00500775> _byte_500775;
    static loco_global<int16_t, 0x0052337A> _dragLastY;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<int8_t, 0x00523393> _currentTool;
    static loco_global<company_id_t, 0x00525E3C> _player_company;
    static loco_global<uint32_t[32], 0x00525E5E> _currencyMultiplicationFactor;
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
    static loco_global<uint16_t, 0x009C68E8> _gGameCommandErrorTitle;
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

    namespace common
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                          \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                       \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        makeWidget({ 0, 41 }, { 130, 74 }, widget_type::panel, 1),                                                                       \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_clear_land),                    \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_adjust_land),                   \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_adjust_water),                  \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_plant_trees),                   \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tooltip_build_walls)

        static window_event_list _events;

        static void initEvents();
        static void sub_4BD297();
        static void switchTab(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void prepareDraw(window* self);
        static void onUpdate(window* self);
        static void onResize(window* self, uint8_t height);
        static void onMouseUp(window* self, widget_index widgetIndex);
        static void sub_4A69DD();
    }

    namespace plant_trees
    {
        static const gfx::ui_size_t windowSize = { 634, 162 };

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

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour) | (1 << plant_cluster_selected) | (1 << plant_cluster_random);

        widget_t widgets[] = {
            commonWidgets(634, 162, string_ids::title_plant_trees),
            makeWidget({ 3, 45 }, { 605, 101 }, widget_type::scrollview, 1, scrollbars::vertical),
            makeWidget({ 609, 46 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_object_90),
            makeWidget({ 609, 70 }, { 24, 24 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_object_colour),
            makeWidget({ 609, 94 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::plant_cluster_selected_tree, string_ids::plant_cluster_selected_tree),
            makeWidget({ 609, 118 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::plant_cluster_random_tree, string_ids::plant_cluster_random_tree),
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
                auto treeObj = objectmgr::get<tree_object>(self->row_hover);
                if (treeObj->colours != 0)
                {
                    colour_t colour = utility::bitScanReverse(treeObj->colours);
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
            ui::scrollview::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB63F
        static void refreshTreeList(window* self)
        {
            auto treeCount = 0;
            for (uint16_t i = 0; i < objectmgr::get_max_objects(object_type::tree); i++)
            {
                auto treeObj = objectmgr::get<tree_object>(i);
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
            common::sub_4BD297();
            ui::windows::hideGridlines();
        }

        // 0x004BBC7D
        static void tabReset(window* self)
        {
            input::toolSet(self, common::widx::panel, 8);
            input::setFlag(input::input_flags::flag6);
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
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_adjust_land:
                case common::widx::tab_adjust_water:
                case common::widx::tab_build_walls:
                case common::widx::tab_clear_area:
                case common::widx::tab_plant_trees:
                    common::switchTab(self, widgetIndex);
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
            gfx::ui_size_t minWindowSize = { self->min_width, self->min_height };
            gfx::ui_size_t maxWindowSize = { self->max_width, self->max_height };
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

            _treeColour = dropdown::getHighlightedItem();
            self->invalidate();
        }

        // 0x004BBDA5
        static void onUpdate(window* self)
        {
            if (!input::hasFlag(input::input_flags::tool_active))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            if (!input::hasFlag(input::input_flags::flag5))
            {
                auto cursor = input::getMouseLocation();
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

                        if (activeWidget > common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].contentHeight - 1 + 60, 562);
                                if (ui::height() < 600)
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
                                if (input::state() != input::input_state::scroll_left)
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
                    if (input::state() != input::input_state::scroll_left)
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
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->current_tab + common::widx::tab_clear_area);
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
                    loc16 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    audio::playSound(audio::sound_id::click_down, loc, pan);
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
        static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_trees_list);
        }

        // 0x004BB756
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->activated_widgets &= ~((1ULL << widx::plant_cluster_selected) | (1ULL << widx::plant_cluster_random));

            if (_treeClusterType == treeCluster::selected)
                self->activated_widgets |= (1ULL << widx::plant_cluster_selected);

            if (_treeClusterType == treeCluster::random)
                self->activated_widgets |= (1ULL << widx::plant_cluster_random);

            self->widgets[widx::rotate_object].type = widget_type::none;
            self->widgets[widx::object_colour].type = widget_type::none;

            if (self->row_hover != -1)
            {
                auto treeObj = objectmgr::get<tree_object>(self->row_hover);
                if (treeObj->name != 0xFFFF)
                {
                    if (treeObj->num_rotations != 1)
                        self->widgets[widx::rotate_object].type = widget_type::wt_9;

                    if (treeObj->colours != 0)
                    {

                        self->widgets[widx::object_colour].image = (1 << 30) | gfx::recolour(image_ids::colour_swatch_recolourable, _treeColour);
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

            common::repositionTabs(self);
        }

        // 0x004BB8C9
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto treeId = self->var_846;
            if (treeId == 0xFFFF)
            {
                treeId = self->row_hover;
                if (treeId == 0xFFFF)
                    return;
            }

            auto treeObj = objectmgr::get<tree_object>(treeId);

            uint32_t treeCost = 0x80000000;
            if (self->var_846 == 0xFFFF)
            {
                treeCost = _lastTreeCost;
                if (treeCost == 0x80000000)
                {
                    treeCost = treeObj->build_cost_factor * _currencyMultiplicationFactor[treeObj->cost_index] / (1 << 12);
                }
            }
            else
            {
                treeCost = treeObj->build_cost_factor * _currencyMultiplicationFactor[treeObj->cost_index] / (1 << 12);
            }
            auto args = FormatArguments();
            args.push<uint32_t>(treeCost);

            if (!isEditorMode())
            {
                auto xPos = self->x + 3 + self->width - 17;
                auto yPos = self->y + self->height - 13;
                gfx::draw_string_494C78(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
            }
            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            auto width = self->width - 19 - xPos;
            gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::black_stringid, &treeObj->name);
        }

        static void drawTreeThumb(tree_object* treeObj, gfx::drawpixelinfo_t* clipped)
        {
            uint32_t image = _byte_500775[treeObj->growth] * treeObj->num_rotations;
            auto rotation = (treeObj->num_rotations - 1) & _treeRotation;
            image += rotation;
            image += treeObj->sprites[treeObj->season_state];

            auto colourOptions = treeObj->colours;
            if (colourOptions != 0)
            {
                colour_t colour = _treeColour;
                if (!(_lastTreeColourFlag & (1 << 5)))
                {
                    colour = utility::bitScanReverse(colourOptions);
                    if (colour == 0xFF)
                        colour = 0;
                }
                image = gfx::recolour(image, colour);
            }
            gfx::draw_image(clipped, 32, 96, image);
        }

        // 0x004BB982
        static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::get_shade(self->colours[1], 3);
            gfx::clear_single(*dpi, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                _lastTreeColourFlag = 0xFFFF;
                if (self->row_info[i] != self->row_hover)
                {
                    if (self->row_info[i] == self->var_846)
                    {
                        _lastTreeColourFlag = colour::translucent_flag;
                        gfx::draw_rect_inset(dpi, xPos, yPos, 65, rowHeight - 1, self->colours[1], colour::translucent_flag);
                    }
                }
                else
                {
                    _lastTreeColourFlag = colour::translucent_flag | colour::outline_flag;
                    gfx::draw_rect_inset(dpi, xPos, yPos, 65, rowHeight - 1, self->colours[1], (colour::translucent_flag | colour::outline_flag));
                }

                auto treeObj = objectmgr::get<tree_object>(self->row_info[i]);
                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, xPos + 1, yPos + 1, 64, rowHeight - 2))
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
            window->callOnMouseUp(common::widx::tab_plant_trees);
        }
        else
        {
            // 0x004BB586
            auto origin = gfx::point_t(ui::width() - plant_trees::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::terraform,
                origin,
                plant_trees::windowSize,
                window_flags::flag_11,
                &plant_trees::events);

            window->number = 0;
            window->current_tab = common::widx::tab_plant_trees - common::widx::tab_clear_area;
            window->frame_no = 0;
            _byte_113649A = 0;
            _lastTreeCost = 0x80000000;
            window->owner = _player_company;
            window->var_846 = 0xFFFF;
            window->saved_view.mapX = 0;
            _treeClusterType = plant_trees::treeCluster::none;

            WindowManager::sub_4CEE0B(window);

            window->min_width = plant_trees::windowSize.width;
            window->min_height = plant_trees::windowSize.height;
            window->max_width = plant_trees::windowSize.width;
            window->max_height = plant_trees::windowSize.height;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0E;

            // End of 0x004BB586

            ui::windows::showGridlines();
            _treeRotation = 2;

            common::initEvents();

            window->invalidate();

            window->widgets = plant_trees::widgets;
            window->enabled_widgets = plant_trees::enabledWidgets;
            window->holdable_widgets = 0;
            window->activated_widgets = 0;

            window->disabled_widgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();

            window->var_83C = 0;
            window->row_hover = -1;

            plant_trees::refreshTreeList(window);

            input::toolSet(window, common::widx::panel, 18);

            input::setFlag(input::input_flags::flag6);
        }
        return window;
    }

    namespace clear_area
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area);

        widget_t widgets[] = {
            commonWidgets(130, 105, string_ids::clear_area),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, widget_type::wt_3, 1, image_ids::tool_area, string_ids::tooltip_clear_area),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, widget_type::wt_7, 1, gfx::recolour(image_ids::decrease_tool_area, colour::white), string_ids::tooltip_decrease_clear_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, widget_type::wt_7, 1, gfx::recolour(image_ids::increase_tool_area, colour::white), string_ids::tooltip_increase_clear_area),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BC671
        static void onClose(window* self)
        {
            ui::windows::hideGridlines();
        }

        // 0x004BBBC7
        static void tabReset(window* self)
        {
            input::toolSet(self, common::widx::panel, 41);
            input::setFlag(input::input_flags::flag6);
            _raiseLandCost = 0x80000000;
            _adjustToolSize = _clearAreaToolSize;
        }

        // 0x004BC7C6
        static void onResize(window* self)
        {
            common::onResize(self, 105);
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
                map_pos centre = { x, y };
                map_pos pointA = { _mapSelectionAX, _mapSelectionAY };
                map_pos pointB = { _mapSelectionBX, _mapSelectionBY };
                _gGameCommandErrorTitle = string_ids::error_cant_clear_entire_area;

                game_commands::do_66(centre, pointA, pointB, flags);
            }
        }

        // 0x004BC689
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != common::widx::panel)
                return;
            clearLand(GameCommandFlag::apply);
        }

        // 0x004BC682
        static void event_12(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != common::widx::panel)
                return;

            auto window = WindowManager::find(WindowType::error);
            if (window == nullptr)
            {
                clearLand(GameCommandFlag::apply);
            }
        }

        // 0x004BC701
        static void event_13(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::panel)
            {
                tilemgr::mapInvalidateSelectionRect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
            }
        }

        // 0x004BC555
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->activated_widgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + image_ids::tool_area;

            common::repositionTabs(self);
        }

        // 0x004BC5E7
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

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

            gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::clear_land_cost, &args);
        }

        static void initEvents()
        {
            events.on_close = onClose;
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_update = common::onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.event_12 = (void (*)(window&, const widget_index))0x004BC682;
            events.event_13 = (void (*)(window&, const widget_index))0x004BC701;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace adjust_land
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
            land_material,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area) | (1 << land_material);

        widget_t widgets[] = {
            commonWidgets(130, 105, string_ids::title_adjust_land),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, widget_type::wt_3, 1, image_ids::tool_area, string_ids::tooltip_adjust_land_tool),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, widget_type::wt_7, 1, gfx::recolour(image_ids::decrease_tool_area, colour::white), string_ids::tooltip_decrease_adjust_land_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, widget_type::wt_7, 1, gfx::recolour(image_ids::increase_tool_area, colour::white), string_ids::tooltip_increase_adjust_land_area),
            makeWidget({ 55 + 16, 92 }, { 20, 20 }, widget_type::wt_6, 0),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BC9D1
        static void onClose(window* self)
        {
            ui::windows::hideGridlines();
        }

        // 0x004BBBF7
        static void tabReset(window* self)
        {
            input::toolSet(self, common::widx::panel, 18);
            input::setFlag(input::input_flags::flag6);
            for (auto i = 0; i < 32; i++)
            {
                auto landObj = objectmgr::get<land_object>(i);
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
                common::onResize(self, 115);
            }
            else
            {
                // CHANGE: Resizes window to allow dropdown and cost string to be drawn seperately
                common::onResize(self, 140);
            }
        }

        // 0x004BCB47
        static void showDropdown(window* self, widget_index widgetIndex)
        {
            auto landCount = 0;
            for (auto i = 0; i < 32; i++)
            {
                auto landObj = objectmgr::get<land_object>(i);
                if (landObj != nullptr)
                    landCount++;
            }

            auto xPos = self->widgets[widgetIndex].left + self->x;
            auto yPos = self->widgets[widgetIndex].bottom + self->y;
            auto heightOffset = self->widgets[widgetIndex].height() - 18;
            auto colour = self->colours[1] | 0x80;
            auto count = dropdown::getItemsPerRow(landCount);

            dropdown::showImage(xPos, yPos, 20, 20, heightOffset, colour, count, landCount);

            auto landIndex = 0;
            for (uint16_t i = 0; i < objectmgr::get_max_objects(object_type::land); i++)
            {
                auto landObj = objectmgr::get<land_object>(i);
                if (landObj == nullptr)
                    continue;

                if (landObj->name == _lastSelectedLand)
                    dropdown::setHighlightedItem(landIndex);

                auto args = FormatArguments();
                args.push(landObj->var_16 + land::image_ids::landscape_generator_tile_icon);
                args.push<uint16_t>(i);

                dropdown::add(landIndex, 0xFFFE, args);

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
            _lastSelectedLand = dropdown::getHighlightedItem();
            self->invalidate();
        }

        // 0x00468DFD
        static uint32_t lowerLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                common::sub_4A69DD();

            int16_t x = _mapSelectionAX + _mapSelectionBX;
            int16_t y = _mapSelectionAY + _mapSelectionBY;
            x /= 2;
            y /= 2;
            map_pos centre = { x, y };
            map_pos pointA = { _mapSelectionAX, _mapSelectionAY };
            map_pos pointB = { _mapSelectionBX, _mapSelectionBY };
            _gGameCommandErrorTitle = string_ids::error_cant_lower_land_here;

            if (_adjustToolSize == 0)
            {
                uint16_t di = 0xFFFF;
                cost = game_commands::do_27(centre, pointA, pointB, di, flags);
            }
            else
            {
                uint16_t di = _word_F2448E;
                cost = game_commands::do_26(centre, pointA, pointB, di, flags);
            }
            return cost;
        }

        // 0x00468D1D
        static uint32_t raiseLand(uint8_t flags)
        {
            uint32_t cost;
            if ((flags & 1))
                common::sub_4A69DD();

            int16_t x = _mapSelectionAX + _mapSelectionBX;
            int16_t y = _mapSelectionAY + _mapSelectionBY;
            x /= 2;
            y /= 2;
            map_pos centre = { x, y };
            map_pos pointA = { _mapSelectionAX, _mapSelectionAY };
            map_pos pointB = { _mapSelectionBX, _mapSelectionBY };
            _gGameCommandErrorTitle = string_ids::error_cant_raise_land_here;

            if (_adjustToolSize == 0)
            {
                uint16_t di = 1;
                cost = game_commands::do_27(centre, pointA, pointB, di, flags);
            }
            else
            {
                uint16_t di = _word_F2448E;
                cost = game_commands::do_25(centre, pointA, pointB, di, flags);
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

        static map_pos sub_45F1A7(int16_t x, int16_t y)
        {
            registers regs;
            regs.ax = x;
            regs.bx = y;
            call(0x0045F1A7, regs);
            map_pos pos = { regs.ax, regs.bx };
            return pos;
        }

        static uint16_t setMapSelectionTiles(int16_t x, int16_t y)
        {
            auto pos = sub_45F1A7(x, y);

            uint16_t xPos = pos.x;
            uint16_t yPos = pos.y;
            if (xPos != 0x8000)
            {
                uint8_t count = 0;
                if ((_mapSelectionFlags & 1) == 0)
                {
                    _mapSelectionFlags = _mapSelectionFlags | 1;
                    count++;
                }

                if (_word_F2448E != 4)
                {
                    _word_F2448E = 4;
                    count++;
                }

                uint16_t toolSizeA = _adjustToolSize;

                if (!toolSizeA)
                    toolSizeA = 1;

                toolSizeA = toolSizeA << 5;
                uint16_t toolSizeB = toolSizeA;
                toolSizeB -= 32;
                toolSizeA = toolSizeA >> 1;
                toolSizeA -= 16;
                xPos -= toolSizeA;
                yPos -= toolSizeA;
                xPos &= 0xFFE0;
                yPos &= 0xFFE0;

                if (xPos != _mapSelectionAX)
                {
                    _mapSelectionAX = xPos;
                    count++;
                }

                if (yPos != _mapSelectionAY)
                {
                    _mapSelectionAY = yPos;
                    count++;
                }

                xPos += toolSizeB;
                yPos += toolSizeB;

                if (xPos != _mapSelectionBX)
                {
                    _mapSelectionBX = xPos;
                    count++;
                }

                if (yPos != _mapSelectionBY)
                {
                    _mapSelectionBY = yPos;
                    count++;
                }

                tilemgr::mapInvalidateSelectionRect();

                return count;
            }
            return 0x8000;
        }

        static map_pos3 sub_45FD8E(int16_t x, int16_t y)
        {
            registers regs;
            regs.ax = x;
            regs.bx = y;
            call(0x0045FD8E, regs);
            map_pos3 pos = { regs.ax, regs.bx, regs.cx };
            return pos;
        }

        static uint16_t setMapSelectionSingleTile(int16_t x, int16_t y)
        {
            auto pos = sub_45FD8E(x, y);

            uint16_t xPos = pos.x;
            uint16_t yPos = pos.y;
            uint16_t cursorQuadrant = pos.z;

            if (xPos != 0x8000)
            {
                auto count = 0;
                if ((_mapSelectionFlags & 1) == 0)
                {
                    _mapSelectionFlags = _mapSelectionFlags | 1;
                    count++;
                }

                if (_word_F2448E != cursorQuadrant)
                {
                    _word_F2448E = cursorQuadrant;
                    count++;
                }
                if (xPos != _mapSelectionAX)
                {
                    _mapSelectionAX = xPos;
                    count++;
                }

                if (yPos != _mapSelectionAY)
                {
                    _mapSelectionAY = yPos;
                    count++;
                }

                if (xPos != _mapSelectionBX)
                {
                    _mapSelectionBX = xPos;
                    count++;
                }

                if (yPos != _mapSelectionBY)
                {
                    _mapSelectionBY = yPos;
                    count++;
                }

                tilemgr::mapInvalidateSelectionRect();

                return count;
            }
            return 0x8000;
        }

        // 0x004BC9D7
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            uint16_t xPos = 0;
            if (widgetIndex != common::widx::panel)
                return;

            tilemgr::mapInvalidateSelectionRect();

            if (_currentTool != 3)
            {
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
                if (_adjustLandToolSize != 1)
                {
                    auto count = setMapSelectionTiles(x, y);

                    if (count == 0x8000)
                        xPos = 0x8000;

                    if (!count)
                        return;
                }
                else
                {
                    auto count = setMapSelectionSingleTile(x, y);

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
                lowerCost = lowerLand(GameCommandFlag::flag_2);

                raiseCost = raiseLand(GameCommandFlag::flag_2);
            }
            setAdjustCost(raiseCost, lowerCost);
        }

        // 0x004BC9ED
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != common::widx::panel)
                return;
            if (!(_mapSelectionFlags & 1))
                return;

            // CHANGE: Allows the player to change land type outside of the scenario editor.
            if (_adjustToolSize != 0)
            {
                if (_lastSelectedLand != 0xFF)
                {
                    _gGameCommandErrorTitle = string_ids::error_cant_change_land_type;
                    map_pos pointA = { _mapSelectionAX, _mapSelectionAY };
                    map_pos pointB = { _mapSelectionBX, _mapSelectionBY };

                    game_commands::do_24(pointA, pointB, _lastSelectedLand, GameCommandFlag::apply);
                }
            }

            _currentTool = 3;
        }

        // 0x004BC9E2
        static void event_12(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != common::widx::panel)
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
            auto flags = GameCommandFlag::apply;

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
        static void event_13(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::panel)
            {
                tilemgr::mapInvalidateSelectionRect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
                _currentTool = 18;
            }
        }

        // 0x004BC83B
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->activated_widgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + image_ids::tool_area;

            self->widgets[widx::land_material].type = widget_type::none;

            // CHANGE: Allows the player to select which material is used in the adjust land tool outside of editor mode.
            if (_adjustToolSize != 0)
            {
                self->widgets[widx::land_material].type = widget_type::wt_6;

                auto landObj = objectmgr::get<land_object>(_lastSelectedLand);

                self->widgets[widx::land_material].image = landObj->var_16 + openloco::land::image_ids::landscape_generator_tile_icon;
            }

            common::repositionTabs(self);
        }

        // 0x004BC909
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

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
                    gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::increase_height_cost, &args);
                }
            }

            yPos += 10;

            if (_lowerLandCost != 0x80000000)
            {
                if (_lowerLandCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_lowerLandCost);
                    gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::decrease_height_cost, &args);
                }
            }
        }

        static void initEvents()
        {
            events.on_close = onClose;
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.on_update = common::onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.event_12 = (void (*)(window&, const widget_index))0x004BC9E2;
            events.event_13 = (void (*)(window&, const widget_index))0x004BCA5D;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace adjust_water
    {
        enum widx
        {
            tool_area = 9,
            decrease_area,
            increase_area,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << tool_area) | (1 << decrease_area) | (1 << increase_area);

        widget_t widgets[] = {
            commonWidgets(130, 105, string_ids::title_adjust_water),
            makeWidget({ 33 + 16, 45 }, { 64, 44 }, widget_type::wt_3, 1, image_ids::tool_area, string_ids::tooltip_adjust_water_tool),
            makeWidget({ 34 + 16, 46 }, { 16, 16 }, widget_type::wt_7, 1, gfx::recolour(image_ids::decrease_tool_area, colour::white), string_ids::tooltip_decrease_adjust_water_area),
            makeWidget({ 80 + 16, 72 }, { 16, 16 }, widget_type::wt_7, 1, gfx::recolour(image_ids::increase_tool_area, colour::white), string_ids::tooltip_increase_adjust_water_area),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x004BCDAE
        static void onClose(window* self)
        {
            ui::windows::hideGridlines();
        }

        // 0x004BBC46
        static void tabReset(window* self)
        {
            input::toolSet(self, common::widx::panel, 19);
            input::setFlag(input::input_flags::flag6);
            _raiseWaterCost = 0x80000000;
            _lowerWaterCost = 0x80000000;
            _adjustToolSize = _adjustWaterToolSize;
        }

        // 0x004BCEB4
        static void onResize(window* self)
        {
            common::onResize(self, 115);
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
            common::sub_4A69DD();
            _gGameCommandErrorTitle = string_ids::error_cant_raise_water_here;
            map_pos pointA = { _mapSelectionAX, _mapSelectionAY };
            map_pos pointB = { _mapSelectionBX, _mapSelectionBY };

            game_commands::do_28(pointA, pointB, flags);
        }

        static void lowerWater(uint8_t flags)
        {
            common::sub_4A69DD();
            _gGameCommandErrorTitle = string_ids::error_cant_raise_water_here;
            map_pos pointA = { _mapSelectionAX, _mapSelectionAY };
            map_pos pointB = { _mapSelectionBX, _mapSelectionBY };

            game_commands::do_29(pointA, pointB, flags);
        }

        // 0x004BCDBF
        static void event_12(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != common::widx::panel)
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
            auto flags = GameCommandFlag::apply;

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
        static void event_13(window& self, const widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::panel)
            {
                tilemgr::mapInvalidateSelectionRect();

                // Reset map selection
                _mapSelectionFlags = _mapSelectionFlags & ~(1 << 0);
                _currentTool = 19;
            }
        }

        // 0x004BCC6D
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->activated_widgets |= (1ULL << widx::tool_area);

            self->widgets[widx::tool_area].image = _adjustToolSize + image_ids::tool_area;

            common::repositionTabs(self);
        }

        // 0x004BCCFF
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

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

                    gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::increase_height_cost, &args);
                }
            }

            yPos += 10;

            if (_lowerWaterCost != 0x80000000)
            {
                if (_lowerWaterCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_lowerWaterCost);

                    gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::decrease_height_cost, &args);
                }
            }
        }

        static void initEvents()
        {
            events.on_close = onClose;
            events.on_mouse_up = common::onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_update = common::onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.event_12 = (void (*)(window&, const widget_index))0x004BCDBF;
            events.event_13 = (void (*)(window&, const widget_index))0x004BCDE8;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace build_walls
    {
        static const gfx::ui_size_t windowSize = { 418, 108 };

        static const uint8_t rowHeight = 48;

        enum widx
        {
            scrollview = 9,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(418, 108, string_ids::title_build_walls),
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
            ui::scrollview::updateThumbs(self, widx::scrollview);
        }

        // 0x004BB6D5
        static void refreshWallList(window* self)
        {
            auto wallCount = 0;
            for (uint16_t i = 0; i < objectmgr::get_max_objects(object_type::wall); i++)
            {
                auto wallObj = objectmgr::get<wall_object>(i);
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
            common::sub_4BD297();
            ui::windows::hideGridlines();
        }

        // 0x004BBCBF
        static void tabReset(window* self)
        {
            input::toolSet(self, common::widx::panel, 15);
            input::setFlag(input::input_flags::flag6);
            _byte_113649A = 0;
            self->var_83C = 0;
            self->row_hover = -1;
            refreshWallList(self);
        }

        // 0x004BC44B
        static void onResize(window* self)
        {
            self->invalidate();
            gfx::ui_size_t minWindowSize = { self->min_width, self->min_height };
            gfx::ui_size_t maxWindowSize = { self->max_width, self->max_height };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x004BC23D
        static void onUpdate(window* self)
        {
            if (!input::hasFlag(input::input_flags::tool_active))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            if (!input::hasFlag(input::input_flags::flag5))
            {
                auto cursor = input::getMouseLocation();
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

                        if (activeWidget > common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].contentHeight - 1 + 60, 562);
                                if (ui::height() < 600)
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
                                if (input::state() != input::input_state::scroll_left)
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
                    if (input::state() != input::input_state::scroll_left)
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
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->current_tab + common::widx::tab_clear_area);
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
                    loc16 loc = { xPos, yPos, static_cast<int16_t>(pan) };
                    audio::playSound(audio::sound_id::click_down, loc, pan);
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
        static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_walls_list);
        }

        // 0x004BC029
        static void prepareDraw(window* self)
        {
            common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            common::repositionTabs(self);
        }

        // 0x004BC0C2
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto wallId = self->var_846;
            if (wallId == 0xFFFF)
            {
                wallId = self->row_hover;
                if (wallId == 0xFFFF)
                    return;
            }

            auto wallObj = objectmgr::get<wall_object>(wallId);
            auto xPos = self->x + 3;
            auto yPos = self->y + self->height - 13;
            auto width = self->width - 19;

            gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::black_stringid, &wallObj->name);
        }

        // 0x004BC11C
        static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::get_shade(self->colours[1], 3);
            gfx::clear_single(*dpi, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                if (self->row_info[i] != self->row_hover)
                {
                    if (self->row_info[i] == self->var_846)
                    {
                        gfx::draw_rect_inset(dpi, xPos, yPos, 40, rowHeight, self->colours[1], colour::translucent_flag);
                    }
                }
                else
                {
                    gfx::draw_rect_inset(dpi, xPos, yPos, 40, rowHeight, self->colours[1], (colour::translucent_flag | colour::outline_flag));
                }

                auto wallObj = objectmgr::get<wall_object>(self->row_info[i]);

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, xPos + 1, yPos + 1, 39, 47))
                    gfx::draw_image(clipped, 34, 28, wallObj->sprite);

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
            events.on_mouse_up = common::onMouseUp;
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

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { clear_area::widgets, widx::tab_clear_area, &clear_area::events, clear_area::enabledWidgets },
            { adjust_land::widgets, widx::tab_adjust_land, &adjust_land::events, adjust_land::enabledWidgets },
            { adjust_water::widgets, widx::tab_adjust_water, &adjust_water::events, adjust_water::enabledWidgets },
            { plant_trees::widgets, widx::tab_plant_trees, &plant_trees::events, plant_trees::enabledWidgets },
            { build_walls::widgets, widx::tab_build_walls, &build_walls::events, build_walls::enabledWidgets },
        };

        static void onResize(window* self, uint8_t height)
        {
            self->flags |= window_flags::resizable;

            /*auto width = 130;
            if (isEditorMode)
                width += 31;*/

            // CHANGE: width set to 161 to include building walls tab
            uint16_t width = 161;
            gfx::ui_size_t windowSize = { width, height };
            self->setSize(windowSize, windowSize);
        }

        // 0x004BC78A, 0x004BCB0B
        static void onUpdate(window* self)
        {
            if (!input::hasFlag(input::input_flags::tool_active))
                WindowManager::close(self);

            if (_toolWindowType != WindowType::terraform)
                WindowManager::close(self);

            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::terraform, self->number, self->current_tab + common::widx::tab_clear_area);
        }

        // 0x004BCD82
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_adjust_land:
                case common::widx::tab_adjust_water:
                case common::widx::tab_build_walls:
                case common::widx::tab_clear_area:
                case common::widx::tab_plant_trees:
                    common::switchTab(self, widgetIndex);
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
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Clear Land Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_bulldozer;

                widget::draw_tab(self, dpi, imageId, widx::tab_clear_area);
            }
            // Adjust Land Tab
            {
                auto landObj = objectmgr::get<land_object>(_grassLand);
                uint32_t imageId = landObj->var_16 + land::image_ids::toolbar_terraform_land;

                widget::draw_tab(self, dpi, imageId, widx::tab_adjust_land);
            }
            // Adjust Water Tab
            {
                auto waterObj = objectmgr::get<water_object>();
                uint32_t imageId = waterObj->var_06 + water::image_ids::toolbar_terraform_water;
                if (self->current_tab == widx::tab_adjust_water - widx::tab_clear_area)
                    imageId += (self->frame_no / 2) % 16;

                widget::draw_tab(self, dpi, imageId, widx::tab_adjust_water);
            }
            // Plant Trees Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_plant_trees;

                widget::draw_tab(self, dpi, imageId, widx::tab_plant_trees);
            }
            // Build Walls Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_build_walls;

                widget::draw_tab(self, dpi, imageId, widx::tab_build_walls);
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
            if (input::isToolActive(self->type, self->number))
                input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_clear_area;
            self->frame_no = 0;

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_clear_area];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            auto disabledWidgets = 0;

            // CHANGE: Disabled so the build walls tab shows outside of editor mode
            /*if (!isEditorMode())
                disabledWidgets |= common::widx::tab_build_walls;*/

            self->disabled_widgets = disabledWidgets;
            self->invalidate();

            switch (widgetIndex)
            {
                case common::widx::tab_adjust_land:
                    adjust_land::tabReset(self);
                    break;

                case common::widx::tab_adjust_water:
                    adjust_water::tabReset(self);
                    break;

                case common::widx::tab_build_walls:
                    build_walls::tabReset(self);
                    break;

                case common::widx::tab_clear_area:
                    clear_area::tabReset(self);
                    break;

                case common::widx::tab_plant_trees:
                    plant_trees::tabReset(self);
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
            plant_trees::initEvents();
            clear_area::initEvents();
            adjust_land::initEvents();
            adjust_water::initEvents();
            build_walls::initEvents();
        }
    }

    // 0x004BB566
    void openClearArea()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(common::widx::tab_clear_area);
    }

    // 0x004BB546
    void openAdjustLand()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(common::widx::tab_adjust_land);
    }

    // 0x004BB556
    void openAdjustWater()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(common::widx::tab_adjust_water);
    }

    // 0x004BB4A3
    void openPlantTrees()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(common::widx::tab_plant_trees);
    }

    // 0x004BB576
    void openBuildWalls()
    {
        auto terraform_window = open();
        terraform_window->callOnMouseUp(common::widx::tab_build_walls);
    }

    void registerHooks()
    {
        register_hook(
            0x004BC9E2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                adjust_land::event_12((ui::window&)regs.esi, (widget_index)regs.dx, (int16_t)regs.ax, (int16_t)regs.bx);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004BCA5D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                adjust_land::event_13((ui::window&)regs.esi, (widget_index)regs.dx);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004BCDBF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                adjust_water::event_12((ui::window&)regs.esi, (widget_index)regs.dx, (int16_t)regs.ax, (int16_t)regs.bx);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004BCDE8,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                adjust_water::event_13((ui::window&)regs.esi, (widget_index)regs.dx);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004BC682,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                clear_area::event_12((ui::window&)regs.esi, (widget_index)regs.dx, (int16_t)regs.ax, (int16_t)regs.bx);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004BC701,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                clear_area::event_13((ui::window&)regs.esi, (widget_index)regs.dx);
                regs = backup;
                return 0;
            });
    }
}
