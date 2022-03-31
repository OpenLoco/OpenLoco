#include "ViewportManager.h"
#include "Config.h"
#include "Console.h"
#include "Entities/EntityManager.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Station.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include "Window.h"
#include <algorithm>
#include <cassert>
#include <memory>

using namespace OpenLoco::Ui;
using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ViewportManager
{
    static std::vector<std::unique_ptr<Viewport>> _viewports;

    static Viewport* create(registers regs, int index);

    void init()
    {
        _viewports.clear();
    }

    // 0x004CEC25
    void collectGarbage()
    {
        _viewports.erase(
            std::remove_if(
                _viewports.begin(),
                _viewports.end(),
                [](std::unique_ptr<Viewport>& viewport) {
                    return viewport->width == 0;
                }),
            _viewports.end());
    }

    static Viewport* initViewport(Ui::Point origin, Ui::Size size, ZoomLevel zoom)
    {
        auto vp = _viewports.emplace_back(std::make_unique<Viewport>()).get();

        vp->x = origin.x;
        vp->y = origin.y;
        vp->width = size.width;
        vp->height = size.height;

        vp->view_width = size.width << static_cast<uint8_t>(zoom);
        vp->view_height = size.height << static_cast<uint8_t>(zoom);
        vp->zoom = static_cast<uint8_t>(zoom);
        vp->flags = 0;

        auto& cfg = OpenLoco::Config::get();
        if ((cfg.flags & Config::Flags::gridlinesOnLandscape) != 0)
        {
            vp->flags |= ViewportFlags::gridlines_on_landscape;
        }

        return vp;
    }

    static void focusViewportOn(Window* w, int index, EntityId dx)
    {
        assert(index >= 0 && index < viewportsPerWindow);
        Viewport* viewport = w->viewports[index];

        w->viewportConfigurations[index].viewport_target_sprite = dx;

        auto t = EntityManager::get<EntityBase>(dx);

        const auto dest = viewport->centre2dCoordinates(t->position);
        w->viewportConfigurations[index].saved_view_x = dest.x;
        w->viewportConfigurations[index].saved_view_y = dest.y;
        viewport->view_x = dest.x;
        viewport->view_y = dest.y;
    }

    static void focusViewportOn(Window* w, int index, Map::Pos3 tile)
    {
        assert(index >= 0 && index < viewportsPerWindow);
        Viewport* viewport = w->viewports[index];

        w->viewportConfigurations[index].viewport_target_sprite = EntityId::null;

        const auto dest = viewport->centre2dCoordinates(tile);
        w->viewportConfigurations[index].saved_view_x = dest.x;
        w->viewportConfigurations[index].saved_view_y = dest.y;
        viewport->view_x = dest.x;
        viewport->view_y = dest.y;
    }

    static Viewport* create(registers regs, int index)
    {
        Ui::Window* window = (Ui::Window*)regs.esi;
        ZoomLevel zoom = ZoomLevel::full;
        if (regs.edx & (1 << 30))
        {
            regs.edx &= ~(1 << 30);
            zoom = (ZoomLevel)regs.cl;
        }

        int16_t x = regs.ax;
        int16_t y = regs.eax >> 16;
        uint16_t width = regs.bx;
        uint16_t height = regs.ebx >> 16;

        if (regs.edx & (1 << 31))
        {
            EntityId id = EntityId(regs.dx);
            return create(window, index, { x, y }, { width, height }, zoom, id);
        }
        else
        {
            Map::Pos3 tile;
            tile.x = regs.dx;
            tile.y = regs.edx >> 16;
            tile.z = regs.ecx >> 16;
            return create(window, index, { x, y }, { width, height }, zoom, tile);
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
    Viewport* create(Window* window, int viewportIndex, Ui::Point origin, Ui::Size size, ZoomLevel zoom, EntityId thing_id)
    {
        Viewport* viewport = initViewport(origin, size, zoom);

        if (viewport == nullptr)
            return nullptr;

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, thing_id);

        collectGarbage();
        return viewport;
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
    Viewport* create(Window* window, int viewportIndex, Ui::Point origin, Ui::Size size, ZoomLevel zoom, Map::Pos3 tile)
    {
        // Viewports of 0 width are automatically removed
        if (size.width == 0)
        {
            return nullptr;
        }
        Viewport* viewport = initViewport(origin, size, zoom);

        if (viewport == nullptr)
            return nullptr;

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, tile);

        collectGarbage();
        return viewport;
    }

    static void invalidate(const ViewportRect& rect, ZoomLevel zoom)
    {
        bool doGarbageCollect = false;

        for (auto& viewport : _viewports)
        {
            // TODO: check for invalid viewports
            if (viewport->width == 0)
            {
                doGarbageCollect = true;
                continue;
            }

            // Skip if zoomed out further than zoom argument
            if (viewport->zoom > (uint8_t)zoom)
                continue;

            if (!viewport->intersects(rect))
                continue;

            auto intersection = viewport->getIntersection(rect);

            // offset rect by (negative) viewport origin
            int16_t left = intersection.left - viewport->view_x;
            int16_t right = intersection.right - viewport->view_x;
            int16_t top = intersection.top - viewport->view_y;
            int16_t bottom = intersection.bottom - viewport->view_y;

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

            Gfx::setDirtyBlocks(left, top, right, bottom);
        }

        if (doGarbageCollect)
        {
            collectGarbage();
        }
    }

    // 0x004CBA2D
    void invalidate(Station* station)
    {
        bool doGarbageCollect = false;

        for (auto& viewport : _viewports)
        {
            // TODO: check for invalid viewports
            if (viewport->width == 0)
            {
                doGarbageCollect = true;
                continue;
            }

            ViewportRect rect;
            rect.left = station->labelFrame.left[viewport->zoom];
            rect.top = station->labelFrame.top[viewport->zoom];
            rect.right = station->labelFrame.right[viewport->zoom] + 1;
            rect.bottom = station->labelFrame.bottom[viewport->zoom] + 1;

            rect.left <<= viewport->zoom;
            rect.top <<= viewport->zoom;
            rect.right <<= viewport->zoom;
            rect.bottom <<= viewport->zoom;

            if (!viewport->intersects(rect))
                continue;

            auto intersection = viewport->getIntersection(rect);

            // offset rect by (negative) viewport origin
            int16_t left = intersection.left - viewport->view_x;
            int16_t right = intersection.right - viewport->view_x;
            int16_t top = intersection.top - viewport->view_y;
            int16_t bottom = intersection.bottom - viewport->view_y;

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

            Gfx::setDirtyBlocks(left, top, right, bottom);
        }

        if (doGarbageCollect)
        {
            collectGarbage();
        }
    }

    /**
     * 0x004CBB01 (eight)
     * 0x004CBBD2 (quarter)
     * 0x004CBCAC (half)
     * 0x004CBD86 (full)
     *
     * @param t @<esi>
     * @param zoom
     */
    void invalidate(EntityBase* t, ZoomLevel zoom)
    {
        if (t->sprite_left == Location::null)
            return;

        ViewportRect rect;
        rect.left = t->sprite_left;
        rect.top = t->sprite_top;
        rect.right = t->sprite_right;
        rect.bottom = t->sprite_bottom;

        auto level = (ZoomLevel)std::min(Config::get().vehiclesMinScale, (uint8_t)zoom);
        invalidate(rect, level);
    }

    void invalidate(const Map::Pos2 pos, coord_t zMin, coord_t zMax, ZoomLevel zoom, int radius)
    {
        auto axbx = Map::gameToScreen(Map::Pos3(pos.x + 16, pos.y + 16, zMax), WindowManager::getCurrentRotation());
        axbx.x -= radius;
        axbx.y -= radius;
        auto dxbp = Map::gameToScreen(Map::Pos3(pos.x + 16, pos.y + 16, zMin), WindowManager::getCurrentRotation());
        dxbp.x += radius;
        dxbp.y += radius;

        ViewportRect rect = {};
        rect.left = axbx.x;
        rect.top = axbx.y;
        rect.right = dxbp.x;
        rect.bottom = dxbp.y;

        invalidate(rect, zoom);
    }

    void registerHooks()
    {
        registerHook(
            0x0046112C,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Map::TileManager::mapInvalidateMapSelectionTiles();
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CA2D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto viewport = create(regs, 0);
                regs = backup;
                regs.edi = X86Pointer(viewport);
                return 0;
            });
        registerHook(
            0x004CA38A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto viewport = create(regs, 1);
                regs = backup;
                regs.edi = X86Pointer(viewport);
                return 0;
            });
        registerHook(
            0x004CBA2D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((Station*)regs.esi);
                return 0;
            });
        registerHook(
            0x004CBB01,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((EntityBase*)regs.esi, ZoomLevel::eighth);
                return 0;
            });
        registerHook(
            0x004CBBD2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((EntityBase*)regs.esi, ZoomLevel::quarter);
                return 0;
            });
        registerHook(
            0x004CBCAC,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((EntityBase*)regs.esi, ZoomLevel::half);
                return 0;
            });
        registerHook(
            0x004CBD86,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                invalidate((EntityBase*)regs.esi, ZoomLevel::full);
                return 0;
            });
        registerHook(
            0x004CBE5F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = Map::Pos2(regs.ax, regs.cx);
                Map::TileManager::mapInvalidateTileFull(pos);
                return 0;
            });
        registerHook(
            0x004CBFBF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = Map::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::eighth, 56);
                return 0;
            });
        registerHook(
            0x004CC098,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = Map::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::eighth);
                return 0;
            });
        registerHook(
            0x004CC20F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = Map::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::full);
                return 0;
            });
        registerHook(
            0x004CC390,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = Map::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::half);
                return 0;
            });
        registerHook(
            0x004CC511,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto pos = Map::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::quarter);
                return 0;
            });
        registerHook(
            0x004CEC25,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                collectGarbage();
                regs = backup;
                return 0;
            });
        registerHook(
            0x00459E54,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto [interaction, vp] = Ui::ViewportInteraction::getMapCoordinatesFromPos(regs.ax, regs.bx, regs.edx);
                regs = backup;
                regs.ax = interaction.pos.x;
                regs.cx = interaction.pos.y;
                regs.bl = static_cast<uint8_t>(interaction.type);
                regs.bh = static_cast<uint8_t>(interaction.unkBh);
                regs.edx = static_cast<uint32_t>(interaction.value);
                regs.edi = X86Pointer(vp);
                return 0;
            });
    }
}
