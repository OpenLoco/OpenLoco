#include "viewportmgr.h"
#include "config.h"
#include "console.h"
#include "interop/interop.hpp"
#include "map/tile.h"
#include "station.h"
#include "things/thing.h"
#include "things/thingmgr.h"
#include "ui.h"
#include "window.h"
#include <algorithm>
#include <cassert>
#include <memory>

using namespace openloco::ui;
using namespace openloco::interop;

namespace openloco::ui::viewportmgr
{
    static std::vector<std::unique_ptr<viewport>> _viewports;

    static loco_global<int32_t, 0x00E3F0B8> currentRotation;

    static void create(registers regs, int index);

    void init()
    {
        _viewports.clear();
    }

    // 0x004CEC25
    void updatePointers()
    {
        _viewports.erase(
            std::remove_if(
                _viewports.begin(),
                _viewports.end(),
                [](std::unique_ptr<viewport>& viewport) {
                    return viewport->width == 0;
                }),
            _viewports.end());
    }

    static viewport* init_viewport(gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom)
    {
        auto vp = _viewports.emplace_back(std::make_unique<viewport>()).get();

        vp->x = origin.x;
        vp->y = origin.y;
        vp->width = size.width;
        vp->height = size.height;

        vp->view_width = size.width << static_cast<uint8_t>(zoom);
        vp->view_height = size.height << static_cast<uint8_t>(zoom);
        vp->zoom = static_cast<uint8_t>(zoom);
        vp->var_12 = 0;

        auto& cfg = openloco::config::get();
        if ((cfg.flags & config::flags::gridlines_on_landscape) != 0)
        {
            vp->var_12 |= (1 << 5);
        }

        return vp;
    }

    static void focusViewportOn(window* w, int index, thing_id_t dx)
    {
        assert(index >= 0 && index < viewportsPerWindow);
        viewport* viewport = w->viewports[index];

        w->viewport_configurations[index].viewport_target_sprite = dx;

        auto t = thingmgr::get<thing>(dx);

        int16_t dest_x, dest_y;
        viewport->centre_2d_coordinates(t->x, t->y, t->z, &dest_x, &dest_y);
        w->viewport_configurations[index].saved_view_x = dest_x;
        w->viewport_configurations[index].saved_view_y = dest_y;
        viewport->view_x = dest_x;
        viewport->view_y = dest_y;
    }

    static void focusViewportOn(window* w, int index, map::map_pos3 tile)
    {
        assert(index >= 0 && index < viewportsPerWindow);
        viewport* viewport = w->viewports[index];

        w->viewport_configurations[index].viewport_target_sprite = 0xFFFF;

        int16_t dest_x, dest_y;
        viewport->centre_2d_coordinates(tile.x, tile.y, tile.z, &dest_x, &dest_y);
        w->viewport_configurations[index].saved_view_x = dest_x;
        w->viewport_configurations[index].saved_view_y = dest_y;
        viewport->view_x = dest_x;
        viewport->view_y = dest_y;
    }

    static void create(registers regs, int index)
    {
        ui::window* window = (ui::window*)regs.esi;
        uint8_t zoom = 0;
        if (regs.edx & (1 << 30))
        {
            regs.edx &= ~(1 << 30);
            zoom = regs.cl;
        }

        int16_t x = regs.ax;
        int16_t y = regs.eax >> 16;
        uint16_t width = regs.bx;
        uint16_t height = regs.ebx >> 16;

        if (regs.edx & (1 << 31))
        {
            thing_id_t id = regs.dx;
            create(window, index, { x, y }, { width, height }, (ZoomLevel)zoom, id);
        }
        else
        {
            int16_t tile_x = regs.dx;
            int16_t tile_y = regs.edx >> 16;
            int16_t tile_z = regs.ecx >> 16;
            create(window, index, { x, y }, { width, height }, (ZoomLevel)zoom, { tile_x, tile_y, tile_z });
        }
    }

    /* 0x004CA2D0
     * ax : x
     * eax >> 16 : y
     * bx : width
     * ebx >> 16 : height
     * cl : zoom
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if thing_id used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : thing_id
     */
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom, thing_id_t thing_id)
    {
        viewport* viewport = init_viewport(origin, size, zoom);

        if (viewport == nullptr)
            return;

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, thing_id);

        updatePointers();
    }

    /* 0x004CA2D0
     * ax : x
     * eax >> 16 : y
     * bx : width
     * ebx >> 16 : height
     * cl : zoom
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if thing_id used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : thing_id
     */
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, ZoomLevel zoom, map::map_pos3 tile)
    {
        viewport* viewport = init_viewport(origin, size, zoom);

        if (viewport == nullptr)
            return;

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, tile);

        updatePointers();
    }

    static void invalidate(const ViewportRect& rect, ZoomLevel zoom)
    {
        for (auto& i : _viewports)
        {
            auto viewport = i.get();

            if (viewport == nullptr)
                break;

            if (viewport->zoom > (uint8_t)zoom)
                continue;

            if (!viewport->intersects(rect))
                continue;

            ViewportRect intersection = viewport->intersection(rect);

            // offset rect by (negative) viewport origin
            auto left = intersection.left - viewport->view_x;
            auto right = intersection.right - viewport->view_x;
            auto top = intersection.top - viewport->view_y;
            auto bottom = intersection.bottom - viewport->view_y;

            // apply zoom
            left = left >> viewport->zoom;
            right = right >> viewport->zoom;
            top = top >> viewport->zoom;
            bottom = bottom >> viewport->zoom;

            // offset calculated area by viewport offset
            left += viewport->x;
            right += viewport->x;
            top += viewport->y;
            bottom += viewport->y;

            gfx::set_dirty_blocks(left, top, right, bottom);
        }
    }

    // 0x004CBA2D
    void invalidate(station* station)
    {
        for (auto& i : _viewports)
        {
            auto viewport = i.get();

            if (viewport == nullptr)
                break;

            ViewportRect rect;
            rect.left = station->var_08[viewport->zoom];
            rect.top = station->var_18[viewport->zoom];
            rect.right = station->var_10[viewport->zoom] + 1;
            rect.bottom = station->var_20[viewport->zoom] + 1;

            rect.left <<= viewport->zoom;
            rect.top <<= viewport->zoom;
            rect.right <<= viewport->zoom;
            rect.bottom <<= viewport->zoom;

            if (!viewport->intersects(rect))
                continue;

            ViewportRect intersection = viewport->intersection(rect);

            // offset rect by (negative) viewport origin
            auto left = intersection.left - viewport->view_x;
            auto right = intersection.right - viewport->view_x;
            auto top = intersection.top - viewport->view_y;
            auto bottom = intersection.bottom - viewport->view_y;

            // apply zoom
            left = left >> viewport->zoom;
            right = right >> viewport->zoom;
            top = top >> viewport->zoom;
            bottom = bottom >> viewport->zoom;

            // offset calculated area by viewport offset
            left += viewport->x;
            right += viewport->x;
            top += viewport->y;
            bottom += viewport->y;

            gfx::set_dirty_blocks(left, top, right, bottom);
        }
    }

    void invalidate(thing_base* t, ZoomLevel zoom)
    {
        if (t->sprite_left == (int16_t)0x8000u)
            return;

        ViewportRect rect;
        rect.left = t->sprite_left;
        rect.top = t->sprite_top;
        rect.right = t->sprite_right;
        rect.bottom = t->sprite_bottom;

        invalidate(rect, (ZoomLevel)std::min(config::get().vehicles_min_scale, (uint8_t)zoom));
    }

    void invalidate(const map::map_pos pos, map::coord_t di, map::coord_t si, ZoomLevel zoom, int range)
    {
        auto axbx = map::coordinate_3d_to_2d(pos.x + 16, pos.y + 16, si, currentRotation);
        axbx.x -= range;
        axbx.y -= range;
        auto dxbp = map::coordinate_3d_to_2d(pos.x + 16, pos.y + 16, di, currentRotation);
        dxbp.x += range;
        dxbp.y += range;

        ViewportRect rect = {};
        rect.left = axbx.x;
        rect.top = axbx.y;
        rect.right = dxbp.x;
        rect.bottom = dxbp.y;

        invalidate(rect, zoom);
    }

    void registerHooks()
    {
        register_hook(
            0x004CA2D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                create(regs, 0);
                regs = backup;
                return 0;
            });
        register_hook(
            0x004CA38A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                create(regs, 1);
                regs = backup;
                return 0;
            });
        register_hook(
            0x004CBA2D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((station*)regs.esi);
                return 0;
            });
        register_hook(
            0x004CBB01,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((thing*)regs.esi, ZoomLevel::eight);
                return 0;
            });
        register_hook(
            0x004CBBD2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((thing*)regs.esi, ZoomLevel::quarter);
                return 0;
            });
        register_hook(
            0x004CBCAC,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((thing*)regs.esi, ZoomLevel::half);
                return 0;
            });
        register_hook(
            0x004CBD86,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((thing*)regs.esi, ZoomLevel::full);
                return 0;
            });
        register_hook(
            0x004CBE5F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = map::map_pos(regs.ax, regs.cx);
                invalidate(pos, 0, 1088, ZoomLevel::eight);
                return 0;
            });
        register_hook(
            0x004CBFBF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = map::map_pos(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::eight, 56);
                return 0;
            });
        register_hook(
            0x004CC098,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = map::map_pos(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::eight);
                return 0;
            });
        register_hook(
            0x004CC20F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = map::map_pos(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::full);
                return 0;
            });
        register_hook(
            0x004CC390,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = map::map_pos(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::half);
                return 0;
            });
        register_hook(
            0x004CC511,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = map::map_pos(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::quarter);
                return 0;
            });
        register_hook(
            0x004CEC25,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                updatePointers();
                regs = backup;
                return 0;
            });
    }
}
