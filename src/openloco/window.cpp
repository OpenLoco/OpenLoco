#include "window.h"
#include "console.h"
#include "graphics/colours.h"
#include "input.h"
#include "interop/interop.hpp"
#include "map/tile.h"
#include "map/tilemgr.h"
#include "things/thingmgr.h"
#include "ui/scrollview.h"
#include "widget.h"
#include <cassert>
#include <cinttypes>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::map;

namespace openloco::ui
{
    template<typename T>
    static bool is_interop_event(T e)
    {
        return (uint32_t)e < 0x004D7000;
    }

    bool window::can_resize()
    {
        return (this->flags & window_flags::resizable) && (this->min_width != this->max_width || this->min_height != this->max_height);
    }

    void window::cap_size(int32_t minWidth, int32_t minHeight, int32_t maxWidth, int32_t maxHeight)
    {
        auto w = this->width;
        auto h = this->height;
        auto shouldInvalidateBefore = false;
        auto shouldInvalidateAfter = false;
        if (w < minWidth)
        {
            w = minWidth;
            shouldInvalidateAfter = true;
        }
        if (h < minHeight)
        {
            h = minHeight;
            shouldInvalidateAfter = true;
        }
        if (w > maxWidth)
        {
            shouldInvalidateBefore = true;
            w = maxWidth;
        }
        if (h > maxHeight)
        {
            shouldInvalidateBefore = true;
            h = maxHeight;
        }

        if (shouldInvalidateBefore)
        {
            invalidate();
        }
        this->width = w;
        this->height = h;
        this->min_width = minWidth;
        this->min_height = minHeight;
        this->max_width = maxWidth;
        this->max_height = maxHeight;
        if (shouldInvalidateAfter)
        {
            invalidate();
        }
    }

    bool window::is_enabled(int8_t widget_index)
    {
        return (this->enabled_widgets & (1ULL << widget_index)) != 0;
    }

    bool window::is_disabled(int8_t widget_index)
    {
        return (this->disabled_widgets & (1ULL << widget_index)) != 0;
    }

    bool window::is_activated(widget_index index)
    {
        return (this->activated_widgets & (1ULL << index)) != 0;
    }

    bool window::is_holdable(ui::widget_index index)
    {
        return (this->holdable_widgets & (1ULL << index)) != 0;
    }

    static void sub_45FD41(int16_t x, int16_t y, int16_t bp, int32_t rotation, int16_t* outX, int16_t* outY, int16_t* outZ)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        regs.bp = bp;
        regs.edx = rotation;
        call(0x45FD41, regs);
        *outX = regs.ax;
        *outY = regs.bx;
        *outZ = regs.dx;
    }

    static void viewport_set_underground_flag(bool underground, ui::window* w, ui::viewport* vp)
    {
        registers regs;
        regs.esi = (int32_t)w;
        regs.edi = (int32_t)vp;
        regs.dl = underground;
        call(0x4C641F, regs);
    }

    loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;

    // 0x004C68E4
    static void viewport_move(int16_t x, int16_t y, ui::window* w, ui::viewport* vp)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        regs.esi = (uint32_t)w;
        regs.edi = (uint32_t)vp;
        call(0x004C68E4, regs);
    }

    // 0x004CA444
    static void centre_2d_coordinates(int16_t x, int16_t y, int16_t z, int16_t* outX, int16_t* outY, ui::viewport* vp)
    {
        auto centre = coordinate_3d_to_2d(x, y, z, gCurrentRotation);

        *outX = centre.x - vp->view_width / 2;
        *outY = centre.y - vp->view_height / 2;
    }

    // 0x004C6456
    void window::viewports_update_position()
    {
        this->call_on_resize();

        for (int i = 0; i < 2; i++)
        {
            viewport* viewport = this->viewports[i];
            viewport_config* config = &this->viewport_configurations[i];

            if (viewport == nullptr)
            {
                continue;
            }

            int16_t centreX, centreY;

            if (config->viewport_target_sprite != 0xFFFF)
            {
                auto thing = thingmgr::get<thing_base>(config->viewport_target_sprite);

                int z = (tile_element_height(thing->x, thing->y) & 0xFFFF) - 16;
                bool underground = (thing->z < z);

                viewport_set_underground_flag(underground, this, viewport);

                centre_2d_coordinates(thing->x, thing->y, thing->z + 12, &centreX, &centreY, viewport);
            }
            else
            {
                int16_t outX, outY, outZ;

                int16_t midX = config->saved_view_x + (viewport->view_width / 2);
                int16_t midY = config->saved_view_y + (viewport->view_height / 2);

                sub_45FD41(midX, midY, 128, gCurrentRotation, &outX, &outY, &outZ);
                viewport_set_underground_flag(false, this, viewport);

                bool atMapEdge = false;
                if (outX < -256)
                {
                    outX = -256;
                    atMapEdge = true;
                }
                if (outY < -256)
                {
                    outY = -256;
                    atMapEdge = true;
                }
                if (outX > 0x30FE)
                {
                    outX = 0x30FE;
                    atMapEdge = true;
                }
                if (outY > 0x30FE)
                {
                    outY = 0x30FE;
                    atMapEdge = true;
                }

                if (atMapEdge)
                {
                    auto coord_2d = coordinate_3d_to_2d(outX, outY, 128, gCurrentRotation);

                    config->saved_view_x = coord_2d.x - viewport->view_width / 2;
                    config->saved_view_y = coord_2d.y - viewport->view_height / 2;
                }

                centreX = config->saved_view_x;
                centreY = config->saved_view_y;

                if (this->flags & window_flags::scrolling_to_location)
                {
                    bool flippedX = false;
                    centreX -= viewport->view_x;
                    if (centreX < 0)
                    {
                        centreX = -centreX;
                        flippedX = true;
                    }

                    bool flippedY = false;
                    centreY -= viewport->view_y;
                    if (centreY < 0)
                    {
                        centreY = -centreY;
                        flippedY = true;
                    }

                    centreX = (centreX + 7) / 8; // ceil(centreX / 8.0);
                    centreY = (centreY + 7) / 8; // ceil(centreX / 8.0);

                    if (centreX == 0 && centreY == 0)
                    {
                        this->flags &= ~window_flags::scrolling_to_location;
                    }

                    if (flippedX)
                    {
                        centreX = -centreX;
                    }

                    if (flippedY)
                    {
                        centreY = -centreY;
                    }

                    centreX += viewport->view_x;
                    centreY += viewport->view_y;
                }
            }
            viewport_move(centreX, centreY, this, viewport);
        }
    }

    // 0x004C99B9
    void window::invalidate_pressed_image_buttons()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x004C99B9, regs);
    }

    // 0x004CA4BD
    void window::invalidate()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x004CA4BD, regs);
    }

    // 0x004CA115
    void window::update_scroll_widgets()
    {
        uint32_t s = 0;
        for (int w = 0;; ++w)
        {
            ui::widget_t* widget = &this->widgets[w];

            if (widget->type == widget_type::end)
                break;

            if (widget->type != widget_type::scrollview)
                continue;

            uint16_t scrollWidth = 0, scrollHeight = 0;
            this->call_get_scroll_size(s, &scrollWidth, &scrollHeight);

            bool invalidate = false;

            if (widget->content & scrollbars::horizontal)
            {
                if (this->scroll_areas[s].h_right != scrollWidth + 1)
                {
                    this->scroll_areas[s].h_right = scrollWidth + 1;
                    invalidate = true;
                }
            }

            if (widget->content & scrollbars::vertical)
            {
                if (this->scroll_areas[s].v_bottom != scrollHeight + 1)
                {
                    this->scroll_areas[s].v_bottom = scrollHeight + 1;
                    invalidate = true;
                }
            }

            if (invalidate)
            {
                ui::scrollview::update_thumbs(this, w);
                this->invalidate();
            }

            s++;
        }
    }

    // 0x004CA17F
    void window::init_scroll_widgets()
    {
        uint32_t s = 0;
        for (int w = 0;; ++w)
        {
            ui::widget_t* widget = &this->widgets[w];

            if (widget->type == widget_type::end)
                break;

            if (widget->type != widget_type::scrollview)
                continue;

            this->scroll_areas[s].flags = 0;

            uint16_t scrollWidth = 0, scrollHeight = 0;
            this->call_get_scroll_size(s, &scrollWidth, &scrollHeight);
            this->scroll_areas[s].h_left = 0;
            this->scroll_areas[s].h_right = scrollWidth + 1;
            this->scroll_areas[s].v_top = 0;
            this->scroll_areas[s].v_bottom = scrollHeight + 1;

            if (widget->content & scrollbars::horizontal)
            {
                this->scroll_areas[s].flags |= 1 << 0;
            }
            if (widget->content & scrollbars::vertical)
            {
                this->scroll_areas[s].flags |= 1 << 4;
            }

            ui::scrollview::update_thumbs(this, w);
            s++;
        }
    }

    int8_t window::get_scroll_data_index(widget_index index)
    {
        int8_t scrollIndex = 0;
        for (int i = 0; i < index; i++)
        {
            if (this->widgets[i].type == ui::widget_type::scrollview)
            {
                scrollIndex++;
            }
        }

        return scrollIndex;
    }

    // 0x00459E54
    // TODO: needs expansion in terms of (output) parameters.
    static void get_map_coordinates_from_pos(int32_t screenX, int32_t screenY, int32_t flags, int16_t* x, int16_t* y)
    {
        registers regs;
        regs.ax = screenX;
        regs.cx = screenY;
        regs.edx = flags;
        call(0x00459E54, regs);

        *x = regs.ax;
        *y = regs.cx;
    }

    void window::viewport_get_map_coords_by_cursor(int16_t* map_x, int16_t* map_y, int16_t* offset_x, int16_t* offset_y)
    {
        // Get mouse position to offset against.
        int32_t mouse_x, mouse_y;
        ui::get_cursor_pos(mouse_x, mouse_y);

        // Compute map coordinate by mouse position.
        get_map_coordinates_from_pos(mouse_x, mouse_y, 0, map_x, map_y);

        // Get viewport coordinates centring around the tile.
        int32_t base_height = map::tile_element_height(*map_x, *map_y);
        int16_t dest_x, dest_y;
        viewport* v = this->viewports[0];
        centre_2d_coordinates(*map_x, *map_y, base_height, &dest_x, &dest_y, v);

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int16_t rebased_x = ((this->width >> 1) - mouse_x) * (1 << v->zoom),
                rebased_y = ((this->height >> 1) - mouse_y) * (1 << v->zoom);

        // Compute cursor offset relative to tile.
        viewport_config* vc = &this->viewport_configurations[0];
        *offset_x = (vc->saved_view_x - (dest_x + rebased_x)) * (1 << v->zoom);
        *offset_y = (vc->saved_view_y - (dest_y + rebased_y)) * (1 << v->zoom);
    }

    void window::viewport_centre_tile_around_cursor(int16_t map_x, int16_t map_y, int16_t offset_x, int16_t offset_y)
    {
        // Get viewport coordinates centring around the tile.
        int16_t dest_x, dest_y;
        int32_t base_height = map::tile_element_height(map_x, map_y);
        viewport* v = this->viewports[0];
        centre_2d_coordinates(map_x, map_y, base_height, &dest_x, &dest_y, v);

        // Get mouse position to offset against.
        int32_t mouse_x, mouse_y;
        ui::get_cursor_pos(mouse_x, mouse_y);

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int16_t rebased_x = ((this->width >> 1) - mouse_x) * (1 << v->zoom),
                rebased_y = ((this->height >> 1) - mouse_y) * (1 << v->zoom);

        // Apply offset to the viewport.
        viewport_config* vc = &this->viewport_configurations[0];
        vc->saved_view_x = dest_x + rebased_x + (offset_x / (1 << v->zoom));
        vc->saved_view_y = dest_y + rebased_y + (offset_y / (1 << v->zoom));
    }

    void window::viewport_zoom_set(int8_t zoomLevel, bool toCursor)
    {
        viewport* v = this->viewports[0];
        viewport_config* vc = &this->viewport_configurations[0];

        zoomLevel = std::clamp<int8_t>(zoomLevel, 0, 3);
        if (v->zoom == zoomLevel)
            return;

        // Zooming to cursor? Remember where we're pointing at the moment.
        int16_t saved_map_x = 0;
        int16_t saved_map_y = 0;
        int16_t offset_x = 0;
        int16_t offset_y = 0;
        if (toCursor)
        {
            this->viewport_get_map_coords_by_cursor(&saved_map_x, &saved_map_y, &offset_x, &offset_y);
        }

        // Zoom in
        while (v->zoom > zoomLevel)
        {
            v->zoom--;
            vc->saved_view_x += v->view_width / 4;
            vc->saved_view_y += v->view_height / 4;
            v->view_width /= 2;
            v->view_height /= 2;
        }

        // Zoom out
        while (v->zoom < zoomLevel)
        {
            v->zoom++;
            vc->saved_view_x -= v->view_width / 2;
            vc->saved_view_y -= v->view_height / 2;
            v->view_width *= 2;
            v->view_height *= 2;
        }

        // Zooming to cursor? Centre around the tile we were hovering over just now.
        if (toCursor)
        {
            this->viewport_centre_tile_around_cursor(saved_map_x, saved_map_y, offset_x, offset_y);
        }

        this->invalidate();
    }

    // 0x0045F015
    void window::viewport_zoom_in(bool toCursor)
    {
        if (this->viewports[0] == nullptr)
            return;

        this->viewport_zoom_set(this->viewports[0]->zoom + 1, toCursor);
    }

    // 0x0045EFDB
    void window::viewport_zoom_out(bool toCursor)
    {
        if (this->viewports[0] == nullptr)
            return;

        this->viewport_zoom_set(this->viewports[0]->zoom - 1, toCursor);
    }

    // 0x0045F04F
    void window::viewport_rotate_right()
    {
        registers regs;
        regs.esi = (uintptr_t)this;
        call(0x0045F04F, regs);
    }

    // 0x0045F0ED
    void window::viewport_rotate_left()
    {
        registers regs;
        regs.esi = (uintptr_t)this;
        call(0x0045F0ED, regs);
    }

    bool window::move(int16_t dx, int16_t dy)
    {
        if (dx == 0 && dy == 0)
        {
            return false;
        }

        this->invalidate();

        this->x += dx;
        this->y += dy;

        if (this->viewports[0] != nullptr)
        {
            this->viewports[0]->x += dx;
            this->viewports[0]->y += dy;
        }

        if (this->viewports[1] != nullptr)
        {
            this->viewports[1]->x += dx;
            this->viewports[1]->y += dy;
        }

        this->invalidate();

        return true;
    }

    // 0x004C9513
    widget_index window::find_widget_at(int16_t xPos, int16_t yPos)
    {
        this->call_prepare_draw();

        widget_index activeWidget = -1;

        widget_index widgetIndex = -1;
        for (ui::widget_t* widget = &this->widgets[0]; widget->type != widget_type::end; widget++)
        {
            widgetIndex++;

            if (widget->type == widget_type::none)
                continue;

            if (xPos < this->x + widget->left)
                continue;

            if (xPos > this->x + widget->right)
                continue;

            if (yPos < this->y + widget->top)
                continue;

            if (yPos > this->y + widget->bottom)
                continue;

            activeWidget = widgetIndex;
        }

        if (activeWidget == -1)
        {
            return -1;
        }

        if (this->widgets[activeWidget].type == widget_type ::wt_18)
        {
            activeWidget++;
        }

        return activeWidget;
    }

    void window::call_close()
    {
        if (event_handlers->on_close == nullptr)
            return;

        if (is_interop_event(event_handlers->on_close))
        {
            registers regs;
            regs.esi = (int32_t)this;
            call((uint32_t)this->event_handlers->on_close, regs);
            return;
        }

        event_handlers->on_close(this);
    }

    void window::call_update()
    {
        if (event_handlers->on_update == nullptr)
            return;

        if (is_interop_event(event_handlers->on_update))
        {
            registers regs;
            regs.esi = (int32_t)this;
            call((uintptr_t)this->event_handlers->on_update, regs);
            return;
        }

        event_handlers->on_update(this);
    }

    void window::call_tool_down(int16_t widget_index, int16_t xPos, int16_t yPos)
    {
        registers regs;
        regs.ax = xPos;
        regs.bx = yPos;
        regs.dx = widget_index;
        regs.esi = (int32_t)this;
        call((uint32_t)this->event_handlers->on_tool_down, regs);
    }

    ui::cursor_id window::call_15(int16_t xPos, int16_t yPos, ui::cursor_id fallback, bool* out)
    {
        registers regs;
        regs.ax = xPos;
        regs.bl = *out;
        regs.cx = yPos;
        regs.edi = (int32_t)fallback;
        regs.esi = (int32_t)this;
        call(this->event_handlers->event_15, regs);

        *out = regs.bl;

        return (cursor_id)regs.edi;
    }

    ui::cursor_id window::call_cursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        if (event_handlers->cursor == nullptr)
            return fallback;

        if (is_interop_event(event_handlers->cursor))
        {
            registers regs;
            regs.cx = xPos;
            regs.dx = yPos;
            regs.ax = widgetIdx;
            regs.ebx = -1;
            regs.edi = (int32_t) & this->widgets[widgetIdx];
            regs.esi = (int32_t)this;
            call((uintptr_t)this->event_handlers->cursor, regs);

            if (regs.ebx == -1)
            {
                return fallback;
            }

            return (cursor_id)regs.ebx;
        }

        return event_handlers->cursor(widgetIdx, xPos, yPos, fallback);
    }

    void window::call_on_mouse_up(widget_index widgetIndex)
    {
        if (event_handlers->on_mouse_up == nullptr)
            return;

        if (is_interop_event(event_handlers->on_mouse_up))
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (uint32_t)this;

            // Not sure if this is used
            regs.edi = (uint32_t) & this->widgets[widgetIndex];

            call((uintptr_t)this->event_handlers->on_mouse_up, regs);
            return;
        }

        event_handlers->on_mouse_up(this, widgetIndex);
    }

    ui::window* window::call_on_resize()
    {
        auto handler = event_handlers->on_resize;
        if (handler != nullptr)
        {
            if (is_interop_event(handler))
            {
                registers regs;
                regs.esi = (int32_t)this;
                call((uint32_t)handler, regs);
            }
            else
            {
                handler(this);
            }
        }
        return this;
    }

    void window::call_3(int8_t widget_index)
    {
        registers regs;
        regs.edx = widget_index;
        regs.esi = (uint32_t)this;
        regs.edi = (uint32_t) & this->widgets[widget_index];
        call((uint32_t)this->event_handlers->event_03, regs);
    }

    void window::call_on_mouse_down(ui::widget_index widget_index)
    {
        if (event_handlers->on_mouse_down == nullptr)
            return;

        if (is_interop_event(event_handlers->on_mouse_down))
        {
            registers regs;
            regs.edx = widget_index;
            regs.esi = (uint32_t)this;
            regs.edi = (uint32_t) & this->widgets[widget_index];
            call((uint32_t)this->event_handlers->on_mouse_down, regs);
            return;
        }

        event_handlers->on_mouse_down(this, widget_index);
    }

    void window::call_on_dropdown(ui::widget_index widget_index, int16_t item_index)
    {
        if (event_handlers->on_dropdown == nullptr)
            return;

        if (is_interop_event(event_handlers->on_dropdown))
        {
            registers regs;
            regs.ax = item_index;
            regs.edx = widget_index;
            regs.esi = (uint32_t)this;
            call((uint32_t)this->event_handlers->on_dropdown, regs);
            return;
        }

        event_handlers->on_dropdown(this, widget_index, item_index);
    }

    void window::call_get_scroll_size(uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        if (event_handlers->get_scroll_size == nullptr)
            return;

        if (is_interop_event(event_handlers->get_scroll_size))
        {
            registers regs;
            regs.eax = scrollIndex;
            regs.esi = (uintptr_t)this;
            call((uint32_t)this->event_handlers->get_scroll_size, regs);
            *scrollWidth = regs.cx;
            *scrollHeight = regs.dx;
            return;
        }

        event_handlers->get_scroll_size(this, scrollIndex, scrollWidth, scrollHeight);
    }

    void window::call_scroll_mouse_down(int16_t xPos, int16_t yPos, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)this;
        regs.cx = xPos;
        regs.dx = yPos;
        call((uint32_t)this->event_handlers->scroll_mouse_down, regs);
    }

    void window::call_scroll_mouse_over(int16_t xPos, int16_t yPos, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)this;
        regs.cx = xPos;
        regs.dx = yPos;
        call((uint32_t)this->event_handlers->scroll_mouse_over, regs);
    }

    void window::call_viewport_rotate()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call((int32_t)this->event_handlers->viewport_rotate, regs);
    }

    void window::call_text_input(widget_index caller, char* buffer)
    {
        if (event_handlers->text_input == nullptr)
            return;

        if (is_interop_event(event_handlers->text_input))
        {
            registers regs;
            regs.dx = caller;
            regs.esi = (int32_t)this;
            regs.cl = 1;
            regs.edi = (uintptr_t)buffer;
            call((uintptr_t)this->event_handlers->text_input, regs);
            return;
        }

        this->event_handlers->text_input(this, caller, buffer);
    }

    bool window::call_tooltip(int16_t widget_index)
    {
        if (event_handlers->tooltip == nullptr)
            return false;

        if (is_interop_event(event_handlers->tooltip))
        {
            registers regs;
            regs.ax = widget_index;
            regs.esi = (int32_t)this;
            call((int32_t)this->event_handlers->tooltip, regs);
            return regs.ax != (int16_t)string_ids::null;
        }

        event_handlers->tooltip(this, widget_index);
        return true;
    }

    void window::call_on_move(int16_t xPos, int16_t yPos)
    {
        registers regs;
        regs.cx = xPos;
        regs.dx = yPos;
        regs.esi = (int32_t)this;
        call(this->event_handlers->on_move, regs);
    }

    void window::call_prepare_draw()
    {
        if (event_handlers->prepare_draw == nullptr)
            return;

        if (is_interop_event(event_handlers->prepare_draw))
        {
            registers regs;
            regs.esi = (int32_t)this;
            call((int32_t)this->event_handlers->prepare_draw, regs);
            return;
        }

        event_handlers->prepare_draw(this);
    }

    void window::call_draw(gfx::drawpixelinfo_t* dpi)
    {
        if (event_handlers->draw == nullptr)
            return;

        if (is_interop_event(this->event_handlers->draw))
        {
            registers regs;
            regs.esi = (int32_t)this;
            regs.edi = (int32_t)dpi;
            call((int32_t)this->event_handlers->draw, regs);
            return;
        }

        event_handlers->draw(this, dpi);
    }

    void window::call_draw_scroll(gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        if (event_handlers->draw_scroll == nullptr)
            return;

        if (is_interop_event(this->event_handlers->draw_scroll))
        {
            registers regs;
            regs.ax = scrollIndex;
            regs.esi = (int32_t)this;
            regs.edi = (int32_t)dpi;
            call((int32_t)event_handlers->draw_scroll, regs);
            return;
        }

        event_handlers->draw_scroll(this, dpi, scrollIndex);
    }

    // 0x004CA4DF
    void window::draw(gfx::drawpixelinfo_t* dpi)
    {
        if ((this->flags & window_flags::transparent) && !(this->flags & window_flags::no_background))
        {
            gfx::fill_rect(dpi, this->x, this->y, this->x + this->width - 1, this->y + this->height - 1, 0x2000000 | 52);
        }

        uint64_t pressed_widget = 0;
        if (input::state() == input::input_state::dropdown_active || input::state() == input::input_state::widget_pressed)
        {
            if (this->type == addr<0x0052336F, WindowType>() && this->number == addr<0x00523370, uint16_t>())
            {
                if (input::has_flag((input::input_flags)(1 << 0)))
                {
                    pressed_widget = 1ULL << addr<0x00523372, uint32_t>();
                }
            }
        }

        uint64_t tool_widget = 0;
        if (this->type == addr<0x00523392, WindowType>() && this->number == addr<0x00523390, uint16_t>())
        {
            tool_widget = 1ULL << addr<0x00523394, uint32_t>();
        }

        uint64_t hovered_widget = 0;
        if (input::is_hovering(this->type, this->number))
        {
            hovered_widget = 1ULL << input::get_hovered_widget_index();
        }

        int scrollviewIndex = 0;
        for (int widgetIndex = 0; widgetIndex < 64; widgetIndex++)
        {
            auto widget = &this->widgets[widgetIndex];

            if (widget->type == widget_type::end)
            {
                break;
            }

            if ((this->flags & window_flags::no_background) == 0)
            {
                // Check if widget is outside the draw region
                if (this->x + widget->left >= dpi->x + dpi->width && this->x + widget->right < dpi->x)
                {
                    if (this->y + widget->top >= dpi->y + dpi->height && this->y + widget->bottom < dpi->y)
                    {
                        continue;
                    }
                }
            }

            uint16_t widgetFlags = 0;
            if (widget->colour == 0 && this->flags & window_flags::flag_11)
            {
                widgetFlags = 0x80;
            }

            uint8_t colour = this->colours[widget->colour];

            bool enabled = (this->enabled_widgets & (1ULL << widgetIndex)) != 0;
            bool disabled = (this->disabled_widgets & (1ULL << widgetIndex)) != 0;
            bool activated = (this->activated_widgets & (1ULL << widgetIndex)) != 0;
            activated |= (pressed_widget & (1ULL << widgetIndex)) != 0;
            activated |= (tool_widget & (1ULL << widgetIndex)) != 0;
            bool hovered = (hovered_widget & (1ULL << widgetIndex)) != 0;

            switch (widget->type)
            {
                case widget_type::none:
                case widget_type::end:
                    break;

                case widget_type::panel:
                    widget::draw_1(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::frame:
                    widget::draw_2(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::wt_3:
                    widget::draw_3(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_4:
                    assert(false); // Unused
                    break;

                case widget_type::wt_5:
                case widget_type::wt_6:
                case widget_type::wt_7:
                case widget_type::wt_8:
                    widget::draw_5(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_9:
                    widget::draw_9(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated, hovered);
                    break;

                case widget_type::wt_10:
                    widget::draw_10(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated, hovered);
                    break;

                case widget_type::wt_11:
                case widget_type::wt_12:
                case widget_type::wt_14:
                    if (widget->type == widget_type::wt_12)
                    {
                        assert(false); // Unused
                    }
                    widget::draw_11_a(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    widget::draw_13(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_13:
                    widget::draw_13(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_15:
                    widget::draw_15(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_16:
                    assert(false); // Unused
                    break;

                case widget_type::wt_17:
                case widget_type::wt_18:
                case widget_type::viewport:
                    widget::draw_17(dpi, this, widget, widgetFlags, colour);
                    widget::draw_15(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_20:
                case widget_type::wt_21:
                    assert(false); // Unused
                    break;

                case widget_type::caption_22:
                    widget::draw_22_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::caption_23:
                    widget::draw_23_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::caption_24:
                    widget::draw_24_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::caption_25:
                    widget::draw_25_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::scrollview:
                    widget::draw_26(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated, hovered, scrollviewIndex);
                    scrollviewIndex++;
                    break;

                case widget_type::checkbox:
                    widget::draw_27_checkbox(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    widget::draw_27_label(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_28:
                    assert(false); // Unused
                    widget::draw_27_label(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_29:
                    assert(false); // Unused
                    widget::draw_29(dpi, this, widget);
                    break;
            }
        }

        if (this->flags & window_flags::white_border_mask)
        {
            gfx::fill_rect_inset(
                dpi,
                this->x,
                this->y,
                this->x + this->width - 1,
                this->y + this->height - 1,
                colour::white,
                0x10);
        }
    }

    viewport_pos viewport::map_from_3d(loc16 loc, int32_t rotation)
    {
        ui::viewport_pos result;
        switch (rotation & 3)
        {
            case 0:
                result.x = loc.y - loc.x;
                result.y = ((loc.y + loc.x) / 2) - loc.z;
                break;
            case 1:
                result.x = -loc.x - loc.y;
                result.y = ((loc.y - loc.x) / 2) - loc.z;
                break;
            case 2:
                result.x = loc.x - loc.y;
                result.y = ((-loc.y - loc.x) / 2) - loc.z;
                break;
            case 3:
                result.x = loc.y + loc.x;
                result.y = ((loc.x - loc.y) / 2) - loc.z;
                break;
        }
        return result;
    }
}
