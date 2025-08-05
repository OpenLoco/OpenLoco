#include "ViewportManager.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Logging.h"
#include "Map/MapSelection.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include "World/Station.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <cassert>
#include <memory>
#include <ranges>
#include <sfl/static_vector.hpp>

using namespace OpenLoco::Ui;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Ui::ViewportManager
{
    static sfl::static_vector<Viewport, kMaxViewports> _viewports{};

    static Viewport* create(registers regs, int index);

    void init()
    {
        _viewports.clear();
    }

    static Viewport* allocate()
    {
        // Find a free viewport, it is considered unused when width is set to 0.
        auto itFree = std::ranges::find(_viewports, 0, &Viewport::width);
        if (itFree != std::ranges::end(_viewports))
        {
            return &(*itFree);
        }

        // Allocate a new viewport.
        if (_viewports.size() < _viewports.capacity())
        {
            _viewports.emplace_back();
            return &_viewports.back();
        }

        Logging::warn("No free viewports are available, reached max of {}", kMaxViewports);
        return nullptr;
    }

    void destroy(Viewport* vp)
    {
        assert(vp->isValid());

        vp->width = 0;
        while (!_viewports.empty() && _viewports.back().width == 0)
        {
            _viewports.pop_back();
        }
    }

    static Viewport* initViewport(Ui::Point origin, Ui::Size size, ZoomLevel zoom, Window* owner)
    {
        // Viewports with 0 width are invalid.
        if (size.width == 0)
        {
            assert(size.width != 0);
            return nullptr;
        }

        auto* vp = allocate();
        if (vp == nullptr)
        {
            return nullptr;
        }

        vp->x = origin.x;
        vp->y = origin.y;
        vp->width = size.width;
        vp->height = size.height;

        vp->viewWidth = size.width << static_cast<uint8_t>(zoom);
        vp->viewHeight = size.height << static_cast<uint8_t>(zoom);
        vp->zoom = static_cast<uint8_t>(zoom);
        vp->flags = ViewportFlags::none;
        vp->owner = owner;

        if (OpenLoco::Config::get().hasFlags(Config::Flags::gridlinesOnLandscape))
        {
            vp->flags |= ViewportFlags::gridlines_on_landscape;
        }

        return vp;
    }

    static void focusViewportOn(Window* w, int index, EntityId dx)
    {
        assert(index >= 0 && index < viewportsPerWindow);
        Viewport* viewport = w->viewports[index];

        w->viewportConfigurations[index].viewportTargetSprite = dx;

        auto t = EntityManager::get<EntityBase>(dx);

        const auto dest = viewport->centre2dCoordinates(t->position);
        w->viewportConfigurations[index].savedViewX = dest.x;
        w->viewportConfigurations[index].savedViewY = dest.y;
        viewport->viewX = dest.x;
        viewport->viewY = dest.y;
    }

    static void focusViewportOn(Window* w, int index, World::Pos3 tile)
    {
        assert(index >= 0 && index < viewportsPerWindow);
        Viewport* viewport = w->viewports[index];

        w->viewportConfigurations[index].viewportTargetSprite = EntityId::null;

        const auto dest = viewport->centre2dCoordinates(tile);
        w->viewportConfigurations[index].savedViewX = dest.x;
        w->viewportConfigurations[index].savedViewY = dest.y;
        viewport->viewX = dest.x;
        viewport->viewY = dest.y;
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
            World::Pos3 tile;
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
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if entityId used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : entityId
     */
    Viewport* create(Window* window, int viewportIndex, Ui::Point origin, Ui::Size size, ZoomLevel zoom, EntityId entityId)
    {
        Viewport* viewport = initViewport(origin, size, zoom, window);

        if (viewport == nullptr)
        {
            return nullptr;
        }

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, entityId);

        return viewport;
    }

    /* 0x004CA2D0
     * ax : x
     * eax >> 16 : y
     * bx : width
     * ebx >> 16 : height
     * cl : zoom
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if entityId used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : entityId
     */
    Viewport* create(Window* window, int viewportIndex, Ui::Point origin, Ui::Size size, ZoomLevel zoom, World::Pos3 tile)
    {
        Viewport* viewport = initViewport(origin, size, zoom, window);

        if (viewport == nullptr)
        {
            return nullptr;
        }

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, tile);

        return viewport;
    }

    static void invalidate(const ViewportRect& rect, ZoomLevel zoom)
    {
        for (auto& viewport : _viewports)
        {
            // Skip destroyed viewports.
            if (viewport.isValid() == 0)
            {
                continue;
            }

            // Skip if zoomed out further than zoom argument
            if (viewport.zoom > (uint8_t)zoom)
            {
                continue;
            }

            if (!viewport.intersects(rect))
            {
                continue;
            }

            auto intersection = viewport.getIntersection(rect);

            // offset rect by (negative) viewport origin
            int16_t left = intersection.left - viewport.viewX;
            int16_t right = intersection.right - viewport.viewX;
            int16_t top = intersection.top - viewport.viewY;
            int16_t bottom = intersection.bottom - viewport.viewY;

            // apply zoom
            left = left >> viewport.zoom;
            right = right >> viewport.zoom;
            top = top >> viewport.zoom;
            bottom = bottom >> viewport.zoom;

            // offset calculated area by viewport offset
            left += viewport.x;
            right += viewport.x;
            top += viewport.y;
            bottom += viewport.y;

            if (viewport.owner != nullptr)
            {
                left += viewport.owner->x;
                right += viewport.owner->x;
                top += viewport.owner->y;
                bottom += viewport.owner->y;
            }

            Gfx::invalidateRegion(left, top, right, bottom);
        }
    }

    // 0x004CBA2D
    void invalidate(Station* station)
    {
        for (auto& viewport : _viewports)
        {
            // Skip destroyed viewports.
            if (viewport.isValid() == 0)
            {
                continue;
            }

            ViewportRect rect;
            rect.left = station->labelFrame.left[viewport.zoom];
            rect.top = station->labelFrame.top[viewport.zoom];
            rect.right = station->labelFrame.right[viewport.zoom] + 1;
            rect.bottom = station->labelFrame.bottom[viewport.zoom] + 1;

            rect.left <<= viewport.zoom;
            rect.top <<= viewport.zoom;
            rect.right <<= viewport.zoom;
            rect.bottom <<= viewport.zoom;

            if (!viewport.intersects(rect))
            {
                continue;
            }

            auto intersection = viewport.getIntersection(rect);

            // offset rect by (negative) viewport origin
            int16_t left = intersection.left - viewport.viewX;
            int16_t right = intersection.right - viewport.viewX;
            int16_t top = intersection.top - viewport.viewY;
            int16_t bottom = intersection.bottom - viewport.viewY;

            // apply zoom
            left = left >> viewport.zoom;
            right = right >> viewport.zoom;
            top = top >> viewport.zoom;
            bottom = bottom >> viewport.zoom;

            // offset calculated area by viewport offset
            left += viewport.x;
            right += viewport.x;
            top += viewport.y;
            bottom += viewport.y;

            if (viewport.owner != nullptr)
            {
                left += viewport.owner->x;
                right += viewport.owner->x;
                top += viewport.owner->y;
                bottom += viewport.owner->y;
            }

            Gfx::invalidateRegion(left, top, right, bottom);
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
        if (t->spriteLeft == Location::null)
        {
            return;
        }

        ViewportRect rect;
        rect.left = t->spriteLeft;
        rect.top = t->spriteTop;
        rect.right = t->spriteRight;
        rect.bottom = t->spriteBottom;

        auto level = (ZoomLevel)std::min(Config::get().old.vehiclesMinScale, (uint8_t)zoom);
        invalidate(rect, level);
    }

    void invalidate(const World::Pos2 pos, coord_t zMin, coord_t zMax, ZoomLevel zoom, int radius)
    {
        auto axbx = World::gameToScreen(World::Pos3(pos.x + 16, pos.y + 16, zMax), WindowManager::getCurrentRotation());
        axbx.x -= radius;
        axbx.y -= radius;
        auto dxbp = World::gameToScreen(World::Pos3(pos.x + 16, pos.y + 16, zMin), WindowManager::getCurrentRotation());
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
                World::mapInvalidateMapSelectionTiles();
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
                registers backup = regs;
                invalidate((Station*)regs.esi);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CBB01,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidate((EntityBase*)regs.esi, ZoomLevel::eighth);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CBBD2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidate((EntityBase*)regs.esi, ZoomLevel::quarter);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CBCAC,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidate((EntityBase*)regs.esi, ZoomLevel::half);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CBD86,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidate((EntityBase*)regs.esi, ZoomLevel::full);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CBE5F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto pos = World::Pos2(regs.ax, regs.cx);
                World::TileManager::mapInvalidateTileFull(pos);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CBFBF,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto pos = World::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::eighth, 56);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CC098,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto pos = World::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::eighth);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CC20F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto pos = World::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::full);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CC390,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto pos = World::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::half);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CC511,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto pos = World::Pos2(regs.ax, regs.cx);
                invalidate(pos, regs.di, regs.si, ZoomLevel::quarter);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004CEC25,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                regs = backup;
                return 0;
            });
        registerHook(
            0x00459E54,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto flags = static_cast<Ui::ViewportInteraction::InteractionItemFlags>(regs.edx);
                auto [interaction, vp] = Ui::ViewportInteraction::getMapCoordinatesFromPos(regs.ax, regs.bx, flags);
                regs = backup;
                regs.ax = interaction.pos.x;
                regs.cx = interaction.pos.y;
                regs.bl = static_cast<uint8_t>(interaction.type);
                regs.bh = static_cast<uint8_t>(interaction.modId);
                regs.edx = static_cast<uint32_t>(interaction.value);
                regs.edi = X86Pointer(vp);
                return 0;
            });
    }

}
