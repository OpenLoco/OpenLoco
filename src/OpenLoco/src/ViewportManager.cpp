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

    static Viewport* initViewport(Ui::Point origin, Ui::Size size, ZoomLevel zoom)
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

        if (Config::get().gridlinesOnLandscape)
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
        Viewport* viewport = initViewport(origin, size, zoom);

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
        Viewport* viewport = initViewport(origin, size, zoom);

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

        auto level = static_cast<ZoomLevel>(std::min<uint8_t>(Config::get().vehiclesMinScale, zoom));
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
}
