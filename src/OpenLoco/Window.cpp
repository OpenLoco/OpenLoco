#include "Window.h"
#include "Config.h"
#include "Console.h"
#include "Entities/EntityManager.h"
#include "Graphics/Colour.h"
#include "Input.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Ui.h"
#include "Ui/Rect.h"
#include "Ui/ScrollView.h"
#include "Widget.h"
#include <cassert>
#include <cinttypes>

using namespace OpenLoco;
using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui
{
    template<typename T>
    static bool isInteropEvent(T e)
    {
        return (uint32_t)e < 0x004D7000;
    }

    Window::Window(Ui::Point position, Ui::Size size)
        : x(position.x)
        , y(position.y)
        , width(size.width)
        , height(size.height)
        , minWidth(size.width)
        , maxWidth(size.width)
        , minHeight(size.height)
        , maxHeight(size.height)
    {
    }

    bool Window::canResize()
    {
        return (this->flags & WindowFlags::resizable) && (this->minWidth != this->maxWidth || this->minHeight != this->maxHeight);
    }

    void Window::capSize(int32_t _minWidth, int32_t _minHeight, int32_t _maxWidth, int32_t _maxHeight)
    {
        auto w = this->width;
        auto h = this->height;
        auto shouldInvalidateBefore = false;
        auto shouldInvalidateAfter = false;
        if (w < _minWidth)
        {
            w = _minWidth;
            shouldInvalidateAfter = true;
        }
        if (h < _minHeight)
        {
            h = _minHeight;
            shouldInvalidateAfter = true;
        }
        if (w > _maxWidth)
        {
            shouldInvalidateBefore = true;
            w = _maxWidth;
        }
        if (h > _maxHeight)
        {
            shouldInvalidateBefore = true;
            h = _maxHeight;
        }

        if (shouldInvalidateBefore)
        {
            invalidate();
        }
        this->width = w;
        this->height = h;
        this->minWidth = _minWidth;
        this->minHeight = _minHeight;
        this->maxWidth = _maxWidth;
        this->maxHeight = _maxHeight;
        if (shouldInvalidateAfter)
        {
            invalidate();
        }
    }

    bool Window::isEnabled(int8_t widget_index)
    {
        return (this->enabledWidgets & (1ULL << widget_index)) != 0;
    }

    bool Window::isDisabled(int8_t widget_index)
    {
        return (this->disabledWidgets & (1ULL << widget_index)) != 0;
    }

    bool Window::isActivated(WidgetIndex_t index)
    {
        return (this->activatedWidgets & (1ULL << index)) != 0;
    }

    bool Window::isHoldable(Ui::WidgetIndex_t index)
    {
        return (this->holdableWidgets & (1ULL << index)) != 0;
    }

    // 0x0045A0B3
    void Window::drawViewports(Gfx::Context* context)
    {
        if (viewports[0] != nullptr)
            viewports[0]->render(context);

        if (viewports[1] != nullptr)
            viewports[1]->render(context);
    }

    // 0x0045FCE6
    // Input:
    // regs.ax:  x
    // regs.bx:  y
    // regs.bp:  z
    // Output:
    // {x: regs.ax, y: regs.bx}
    std::optional<Map::Pos2> screenGetMapXyWithZ(const Point& mouse, const int16_t z)
    {
        Window* w = WindowManager::findAt(mouse.x, mouse.y);
        if (w == nullptr)
        {
            return std::nullopt;
        }

        Viewport* vp = w->viewports[0];
        if (vp == nullptr)
        {
            return std::nullopt;
        }

        if (vp->containsUi(mouse))
        {
            viewport_pos vpos = vp->screenToViewport(mouse);
            Map::Pos2 position = viewportCoordToMapCoord(vpos.x, vpos.y, z, WindowManager::getCurrentRotation());
            if (Map::validCoords(position))
            {
                return position;
            }
        }

        return std::nullopt;
    }

    // 0x0045FD41
    // Input:
    // regs.ax:  x
    // regs.bx:  y
    // regs.bp:  z
    // regs.edx: rotation
    // Output:
    // {x: regs.ax, y: regs.bx}
    // Note: in the original code: regs.dx: x/2 (probably not used anywhere)
    Map::Pos2 viewportCoordToMapCoord(int16_t x, int16_t y, int16_t z, int32_t rotation)
    {
        constexpr uint8_t inverseRotationMapping[4] = { 0, 3, 2, 1 };
        const auto result = Map::Pos2(y - (x >> 1) + z, y + (x >> 1) + z);
        return Math::Vector::rotate(result, inverseRotationMapping[rotation]);
    }

    // 0x004C641F
    // regs.dl:  underground
    // regs.esi: w
    // regs.edi: vp
    static void viewportSetUndergroundFlag(bool underground, Ui::Window* w, Ui::Viewport* vp)
    {
        if (w->type == WindowType::main)
            return;

        bool shouldInvalidate;
        if (!underground)
        {
            shouldInvalidate = !(vp->flags & ViewportFlags::underground_view);
            vp->flags &= ~ViewportFlags::underground_view;
        }
        else
        {
            shouldInvalidate = vp->flags & ViewportFlags::underground_view;
            vp->flags |= ViewportFlags::underground_view;
        }
        if (shouldInvalidate)
            w->invalidate();
    }

    void Window::viewportSetUndergroundFlag(bool underground, Ui::Viewport* vp)
    {
        Ui::viewportSetUndergroundFlag(underground, this, vp);
    }

    // 0x004C68E4
    static void viewportMove(int16_t x, int16_t y, Ui::Window* w, Ui::Viewport* vp)
    {
        int origX = vp->view_x >> vp->zoom;
        int origY = vp->view_y >> vp->zoom;
        int newX = x >> vp->zoom;
        int newY = y >> vp->zoom;
        int diffX = origX - newX;
        int diffY = origY - newY;

        vp->view_x = x;
        vp->view_y = y;

        // If no change in viewing area
        if (diffX == 0 && diffY == 0)
            return;

        if (vp->flags & ViewportFlags::hide_foreground_tracks_roads || vp->flags & ViewportFlags::hide_foreground_scenery_buildings || w->flags & WindowFlags::flag_8)
        {
            auto rect = Ui::Rect(vp->x, vp->y, vp->width, vp->height);
            Gfx::redrawScreenRect(rect);
            return;
        }

        uint8_t zoom = (1 << vp->zoom);
        Viewport backup = *vp;

        if (vp->x < 0)
        {
            vp->width += vp->x;
            vp->view_width += vp->x * zoom;
            vp->view_x -= vp->x * zoom;
            vp->x = 0;
        }

        int32_t eax = vp->x + vp->width - Ui::width();
        if (eax > 0)
        {
            vp->width -= eax;
            vp->view_width -= eax * zoom;
        }

        if (vp->width <= 0)
        {
            *vp = backup;
            return;
        }

        if (vp->y < 0)
        {
            vp->height += vp->y;
            vp->view_height += vp->y * zoom;
            vp->view_y -= vp->y * zoom;
            vp->y = 0;
        }

        eax = vp->y + vp->height - Ui::height();
        if (eax > 0)
        {
            vp->height -= eax;
            vp->view_height -= eax * zoom;
        }

        if (vp->height <= 0)
        {
            *vp = backup;
            return;
        }

        WindowManager::viewportShiftPixels(w, vp, diffX, diffY);

        *vp = backup;
    }

    // 0x004C6456
    void Window::viewportsUpdatePosition()
    {
        for (int i = 0; i < 2; i++)
        {
            Viewport* viewport = this->viewports[i];
            ViewportConfig* config = &this->viewportConfigurations[i];

            if (viewport == nullptr)
            {
                continue;
            }
            this->callOnResize();

            viewport_pos centre;

            if (config->viewport_target_sprite != EntityId::null)
            {
                auto entity = EntityManager::get<EntityBase>(config->viewport_target_sprite);

                int z = (TileManager::getHeight(entity->position).landHeight) - 16;
                bool underground = (entity->position.z < z);

                viewportSetUndergroundFlag(underground, viewport);

                centre = viewport->centre2dCoordinates(entity->position + Pos3{ 0, 0, 12 });
            }
            else
            {
                int16_t midX = config->saved_view_x + (viewport->view_width / 2);
                int16_t midY = config->saved_view_y + (viewport->view_height / 2);

                Map::Pos2 mapCoord = viewportCoordToMapCoord(midX, midY, 128, viewport->getRotation());
                viewportSetUndergroundFlag(false, viewport);

                bool atMapEdge = false;
                if (mapCoord.x < -256)
                {
                    mapCoord.x = -256;
                    atMapEdge = true;
                }
                if (mapCoord.y < -256)
                {
                    mapCoord.y = -256;
                    atMapEdge = true;
                }
                if (mapCoord.x > 0x30FE)
                {
                    mapCoord.x = 0x30FE;
                    atMapEdge = true;
                }
                if (mapCoord.y > 0x30FE)
                {
                    mapCoord.y = 0x30FE;
                    atMapEdge = true;
                }

                if (atMapEdge)
                {
                    auto coord_2d = gameToScreen({ mapCoord.x, mapCoord.y, 128 }, viewport->getRotation());

                    config->saved_view_x = coord_2d.x - viewport->view_width / 2;
                    config->saved_view_y = coord_2d.y - viewport->view_height / 2;
                }

                centre.x = config->saved_view_x;
                centre.y = config->saved_view_y;

                if (this->flags & WindowFlags::scrolling_to_location)
                {
                    bool flippedX = false;
                    centre.x -= viewport->view_x;
                    if (centre.x < 0)
                    {
                        centre.x = -centre.x;
                        flippedX = true;
                    }

                    bool flippedY = false;
                    centre.y -= viewport->view_y;
                    if (centre.y < 0)
                    {
                        centre.y = -centre.y;
                        flippedY = true;
                    }

                    centre.x = (centre.x + 7) / 8; // ceil(centreX / 8.0);
                    centre.y = (centre.y + 7) / 8; // ceil(centreX / 8.0);

                    if (centre.x == 0 && centre.y == 0)
                    {
                        this->flags &= ~WindowFlags::scrolling_to_location;
                    }

                    if (flippedX)
                    {
                        centre.x = -centre.x;
                    }

                    if (flippedY)
                    {
                        centre.y = -centre.y;
                    }

                    centre.x += viewport->view_x;
                    centre.y += viewport->view_y;
                }
            }
            viewportMove(centre.x, centre.y, this, viewport);
        }
    }

    // 0x004C99B9
    void Window::invalidatePressedImageButtons()
    {
        registers regs;
        regs.esi = X86Pointer(this);
        call(0x004C99B9, regs);
    }

    // 0x004CA4BD
    // input: regs.esi - window (this)
    void Window::invalidate()
    {
        Gfx::setDirtyBlocks(x, y, x + width, y + height);
    }

    // 0x004CA115
    void Window::updateScrollWidgets()
    {
        uint32_t s = 0;
        for (int w = 0;; ++w)
        {
            Ui::Widget* widget = &this->widgets[w];

            if (widget->type == WidgetType::end)
                break;

            if (widget->type != WidgetType::scrollview)
                continue;

            uint16_t scrollWidth = 0, scrollHeight = 0;
            this->callGetScrollSize(s, &scrollWidth, &scrollHeight);

            bool invalidate = false;

            if (widget->content & Scrollbars::horizontal)
            {
                if (this->scrollAreas[s].contentWidth != scrollWidth + 1)
                {
                    this->scrollAreas[s].contentWidth = scrollWidth + 1;
                    invalidate = true;
                }
            }

            if (widget->content & Scrollbars::vertical)
            {
                if (this->scrollAreas[s].contentHeight != scrollHeight + 1)
                {
                    this->scrollAreas[s].contentHeight = scrollHeight + 1;
                    invalidate = true;
                }
            }

            if (invalidate)
            {
                Ui::ScrollView::updateThumbs(this, w);
                this->invalidate();
            }

            s++;
        }
    }

    // 0x004CA17F
    void Window::initScrollWidgets()
    {
        uint32_t s = 0;
        for (int w = 0;; ++w)
        {
            Ui::Widget* widget = &this->widgets[w];

            if (widget->type == WidgetType::end)
                break;

            if (widget->type != WidgetType::scrollview)
                continue;

            this->scrollAreas[s].flags = 0;

            uint16_t scrollWidth = 0, scrollHeight = 0;
            this->callGetScrollSize(s, &scrollWidth, &scrollHeight);
            this->scrollAreas[s].contentOffsetX = 0;
            this->scrollAreas[s].contentWidth = scrollWidth + 1;
            this->scrollAreas[s].contentOffsetY = 0;
            this->scrollAreas[s].contentHeight = scrollHeight + 1;

            if (widget->content & Scrollbars::horizontal)
            {
                this->scrollAreas[s].flags |= Ui::ScrollView::ScrollFlags::hscrollbarVisible;
            }
            if (widget->content & Scrollbars::vertical)
            {
                this->scrollAreas[s].flags |= Ui::ScrollView::ScrollFlags::vscrollbarVisible;
            }

            Ui::ScrollView::updateThumbs(this, w);
            s++;
        }
    }

    int8_t Window::getScrollDataIndex(WidgetIndex_t index)
    {
        int8_t scrollIndex = 0;
        for (int i = 0; i < index; i++)
        {
            if (this->widgets[i].type == Ui::WidgetType::scrollview)
            {
                scrollIndex++;
            }
        }

        return scrollIndex;
    }

    void Window::setDisabledWidgetsAndInvalidate(uint32_t _disabled_widgets)
    {
        registers regs;
        regs.eax = (int32_t)_disabled_widgets;
        regs.esi = X86Pointer(this);
        call(0x004CC7CB, regs);
    }

    void Window::viewportGetMapCoordsByCursor(int16_t* map_x, int16_t* map_y, int16_t* offset_x, int16_t* offset_y)
    {
        // Get mouse position to offset against.
        const auto mouse = Ui::getCursorPos();

        // Compute map coordinate by mouse position.
        auto res = ViewportInteraction::getMapCoordinatesFromPos(mouse.x, mouse.y, 0);
        auto& interaction = res.first;
        *map_x = interaction.pos.x;
        *map_y = interaction.pos.y;

        // Get viewport coordinates centring around the tile.
        auto base_height = TileManager::getHeight({ *map_x, *map_y }).landHeight;
        Viewport* v = this->viewports[0];
        const auto dest = v->centre2dCoordinates({ *map_x, *map_y, base_height });

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int16_t rebased_x = ((this->width >> 1) - mouse.x) * (1 << v->zoom),
                rebased_y = ((this->height >> 1) - mouse.y) * (1 << v->zoom);

        // Compute cursor offset relative to tile.
        ViewportConfig* vc = &this->viewportConfigurations[0];
        *offset_x = (vc->saved_view_x - (dest.x + rebased_x)) * (1 << v->zoom);
        *offset_y = (vc->saved_view_y - (dest.y + rebased_y)) * (1 << v->zoom);
    }

    // 0x004C6801
    void Window::moveWindowToLocation(viewport_pos pos)
    {
        if (this->viewportConfigurations->viewport_target_sprite != EntityId::null)
            return;

        if (this->flags & WindowFlags::viewport_no_scrolling)
            return;

        this->viewportConfigurations->saved_view_x = pos.x;
        this->viewportConfigurations->saved_view_y = pos.y;
        this->flags |= WindowFlags::scrolling_to_location;
    }

    // 0x004C6827
    void Window::viewportCentreOnTile(const Map::Pos3& loc)
    {
        auto viewport = this->viewports[0];
        if (viewport == nullptr)
            return;

        auto tileHeight = TileManager::getHeight(loc).landHeight;
        tileHeight -= 16;

        if (loc.z < tileHeight)
        {
            if (!(viewport->flags & ViewportFlags::underground_view))
            {
                this->invalidate();
            }

            viewport->flags |= ViewportFlags::underground_view;
        }
        else
        {
            if (viewport->flags & ViewportFlags::underground_view)
            {
                this->invalidate();
            }

            viewport->flags &= ~ViewportFlags::underground_view;
        }

        auto pos = gameToScreen(loc, WindowManager::getCurrentRotation());

        pos.x -= viewport->view_width / 2;
        pos.y -= viewport->view_height / 2;

        moveWindowToLocation(pos);
    }

    void Window::viewportCentreMain()
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return;

        auto main = WindowManager::getMainWindow();

        // Unfocus the viewport.
        main->viewportConfigurations[0].viewport_target_sprite = EntityId::null;

        // Centre viewport on tile/thing.
        if (savedView.isThingView())
        {
            auto thing = EntityManager::get<EntityBase>(savedView.thingId);
            main->viewportCentreOnTile(thing->position);
        }
        else
        {
            main->viewportCentreOnTile(savedView.getPos());
        }
    }

    void Window::viewportCentreTileAroundCursor(int16_t map_x, int16_t map_y, int16_t offset_x, int16_t offset_y)
    {
        // Get viewport coordinates centring around the tile.
        auto base_height = TileManager::getHeight({ map_x, map_y }).landHeight;
        Viewport* v = this->viewports[0];
        const auto dest = v->centre2dCoordinates({ map_x, map_y, base_height });

        // Get mouse position to offset against.
        const auto mouse = Ui::getCursorPos();

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int16_t rebased_x = ((this->width >> 1) - mouse.x) * (1 << v->zoom),
                rebased_y = ((this->height >> 1) - mouse.y) * (1 << v->zoom);

        // Apply offset to the viewport.
        ViewportConfig* vc = &this->viewportConfigurations[0];
        vc->saved_view_x = dest.x + rebased_x + (offset_x / (1 << v->zoom));
        vc->saved_view_y = dest.y + rebased_y + (offset_y / (1 << v->zoom));
    }

    void Window::viewportFocusOnEntity(EntityId targetEntity)
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return;

        viewportConfigurations[0].viewport_target_sprite = targetEntity;
    }

    bool Window::viewportIsFocusedOnEntity() const
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return false;

        return viewportConfigurations[0].viewport_target_sprite != EntityId::null;
    }

    void Window::viewportUnfocusFromEntity()
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return;

        if (viewportConfigurations[0].viewport_target_sprite == EntityId::null)
            return;

        auto thing = EntityManager::get<EntityBase>(viewportConfigurations[0].viewport_target_sprite);
        viewportConfigurations[0].viewport_target_sprite = EntityId::null;
        viewportCentreOnTile(thing->position);
    }

    void Window::viewportZoomSet(int8_t zoomLevel, bool toCursor)
    {
        Viewport* v = this->viewports[0];
        ViewportConfig* vc = &this->viewportConfigurations[0];

        zoomLevel = std::clamp<int8_t>(zoomLevel, 0, 3);
        if (v->zoom == zoomLevel)
            return;

        // Zooming to cursor? Remember where we're pointing at the moment.
        int16_t saved_map_x = 0;
        int16_t saved_map_y = 0;
        int16_t offset_x = 0;
        int16_t offset_y = 0;
        if (toCursor && Config::getNew().zoom_to_cursor)
        {
            this->viewportGetMapCoordsByCursor(&saved_map_x, &saved_map_y, &offset_x, &offset_y);
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
        if (toCursor && Config::getNew().zoom_to_cursor)
        {
            this->viewportCentreTileAroundCursor(saved_map_x, saved_map_y, offset_x, offset_y);
        }

        this->invalidate();
    }

    // 0x0045EFDB
    void Window::viewportZoomIn(bool toCursor)
    {
        if (this->viewports[0] == nullptr)
            return;

        this->viewportZoomSet(this->viewports[0]->zoom - 1, toCursor);
    }

    // 0x0045F015
    void Window::viewportZoomOut(bool toCursor)
    {
        if (this->viewports[0] == nullptr)
            return;

        this->viewportZoomSet(this->viewports[0]->zoom + 1, toCursor);
    }

    // 0x0045F04F
    void Window::viewportRotateRight()
    {
        registers regs;
        regs.esi = (uintptr_t)this;
        call(0x0045F04F, regs);
    }

    // 0x0045F0ED
    void Window::viewportRotateLeft()
    {
        registers regs;
        regs.esi = (uintptr_t)this;
        call(0x0045F0ED, regs);
    }

    void Window::viewportRemove(const uint8_t viewportId)
    {
        if (viewports[viewportId] != nullptr)
        {
            viewports[viewportId]->width = 0;
            viewports[viewportId] = nullptr;
        }
    }

    // 0x004421FB
    void Window::viewportFromSavedView(const SavedViewSimple& _savedView)
    {
        auto viewport = viewports[0];
        if (viewport != nullptr)
        {
            auto& config = viewportConfigurations[0];
            config.viewport_target_sprite = EntityId::null;
            config.saved_view_x = _savedView.viewX;
            config.saved_view_y = _savedView.viewY;

            auto zoom = static_cast<int32_t>(_savedView.zoomLevel) - viewport->zoom;
            if (zoom != 0)
            {
                if (zoom < 0)
                {
                    zoom = -zoom;
                    viewport->view_width >>= zoom;
                    viewport->view_height >>= zoom;
                }
                else
                {
                    viewport->view_width <<= zoom;
                    viewport->view_height <<= zoom;
                }
            }
            viewport->zoom = zoom;
            viewport->setRotation(_savedView.rotation);

            config.saved_view_x -= viewport->view_width / 2;
            config.saved_view_y -= viewport->view_height / 2;
        }
    }

    bool Window::move(int16_t dx, int16_t dy)
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

    // 0x004CD320
    void Window::moveInsideScreenEdges()
    {
        Ui::Point offset = { 0, 0 };

        const int16_t xOvershoot = this->x + this->width - Ui::width();

        // Over the edge on the right?
        if (xOvershoot > 0)
            offset.x -= xOvershoot;

        // If not, on the left?
        if (this->x < 0)
            offset.x -= this->x;

        const int16_t yOvershoot = this->y + this->height - (Ui::height() - 27);

        // Over the edge at the bottom?
        if (yOvershoot > 0)
            offset.y -= yOvershoot;

        // Maybe at the top?
        if (this->y - 28 < 0)
            offset.y -= this->y - 28;

        if (offset == Ui::Point(0, 0))
            return;

        this->invalidate();
        this->x += offset.x;
        this->y += offset.y;
        this->invalidate();

        if (this->viewports[0] != nullptr)
        {
            this->viewports[0]->x += offset.x;
            this->viewports[0]->y += offset.y;
        }

        if (this->viewports[1] != nullptr)
        {
            this->viewports[1]->x += offset.x;
            this->viewports[1]->y += offset.y;
        }
    }

    bool Window::moveToCentre()
    {
        int16_t dx = ((Ui::width() - this->width) / 2);
        int16_t dy = ((Ui::height() - this->height) / 2);
        dx = dx - this->x;
        dy = dy - this->y;

        return this->move(dx, dy);
    }

    // 0x004C9513
    WidgetIndex_t Window::findWidgetAt(int16_t xPos, int16_t yPos)
    {
        this->callPrepareDraw();

        WidgetIndex_t activeWidget = -1;

        WidgetIndex_t widgetIndex = -1;
        for (Ui::Widget* widget = &this->widgets[0]; widget->type != WidgetType::end; widget++)
        {
            widgetIndex++;

            if (widget->type == WidgetType::none)
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

        if (this->widgets[activeWidget].type == WidgetType::combobox)
        {
            activeWidget++;
        }

        return activeWidget;
    }

    void Window::callClose()
    {
        if (eventHandlers->on_close == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_close))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->on_close, regs);
            return;
        }

        eventHandlers->on_close(this);
    }

    void Window::callOnPeriodicUpdate()
    {
        if (eventHandlers->on_periodic_update == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_periodic_update))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->on_periodic_update, regs);
            return;
        }

        eventHandlers->on_periodic_update(this);
    }

    void Window::callUpdate()
    {
        if (eventHandlers->on_update == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_update))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uintptr_t)this->eventHandlers->on_update, regs);
            return;
        }

        eventHandlers->on_update(this);
    }

    void Window::call_8()
    {
        if (eventHandlers->event_08 == nullptr)
            return;

        if (isInteropEvent(eventHandlers->event_08))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uintptr_t)this->eventHandlers->event_08, regs);
            return;
        }

        eventHandlers->event_08(this);
    }

    void Window::call_9()
    {
        if (eventHandlers->event_09 == nullptr)
            return;

        if (isInteropEvent(eventHandlers->event_09))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uintptr_t)this->eventHandlers->event_09, regs);
            return;
        }

        eventHandlers->event_09(this);
    }

    void Window::callToolUpdate(int16_t widget_index, int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->on_tool_update == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_tool_update))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            regs.dx = widget_index;
            regs.ax = xPos;
            regs.bx = yPos;
            call((uintptr_t)this->eventHandlers->on_tool_update, regs);
            return;
        }

        eventHandlers->on_tool_update(*this, widget_index, xPos, yPos);
    }

    void Window::callToolDown(int16_t widget_index, int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->on_tool_down == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_tool_down))
        {
            registers regs;
            regs.ax = xPos;
            regs.bx = yPos;
            regs.dx = widget_index;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->on_tool_down, regs);
            return;
        }

        eventHandlers->on_tool_down(*this, widget_index, xPos, yPos);
    }

    void Window::callToolDragContinue(const int16_t widget_index, const int16_t xPos, const int16_t yPos)
    {
        if (eventHandlers->toolDragContinue == nullptr)
            return;

        if (isInteropEvent(eventHandlers->toolDragContinue))
        {
            registers regs;
            regs.ax = xPos;
            regs.bx = yPos;
            regs.dx = widget_index;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->toolDragContinue, regs);
            return;
        }

        eventHandlers->toolDragContinue(*this, widget_index, xPos, yPos);
    }

    void Window::callToolDragEnd(const int16_t widget_index)
    {
        if (eventHandlers->toolDragEnd == nullptr)
            return;

        if (isInteropEvent(eventHandlers->toolDragEnd))
        {
            registers regs;
            regs.dx = widget_index;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->toolDragEnd, regs);
            return;
        }

        eventHandlers->toolDragEnd(*this, widget_index);
    }

    void Window::callToolAbort(int16_t widget_index)
    {
        if (eventHandlers->on_tool_abort == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_tool_abort))
        {
            registers regs;
            regs.dx = widget_index;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->on_tool_abort, regs);
            return;
        }

        eventHandlers->on_tool_abort(*this, widget_index);
    }

    Ui::CursorId Window::call_15(int16_t xPos, int16_t yPos, Ui::CursorId fallback, bool* out)
    {
        if (eventHandlers->event_15 == nullptr)
            return CursorId::pointer;
        if (isInteropEvent(eventHandlers->event_15))
        {
            registers regs;
            regs.ax = xPos;
            regs.bl = *out;
            regs.cx = yPos;
            regs.edi = (int32_t)fallback;
            regs.esi = X86Pointer(this);
            call(reinterpret_cast<uint32_t>(this->eventHandlers->event_15), regs);

            *out = regs.bl;

            return (CursorId)regs.edi;
        }

        return eventHandlers->event_15(*this, xPos, yPos, fallback, *out);
    }

    Ui::CursorId Window::callCursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (eventHandlers->cursor == nullptr)
            return fallback;

        if (isInteropEvent(eventHandlers->cursor))
        {
            registers regs;
            regs.cx = xPos;
            regs.dx = yPos;
            regs.ax = widgetIdx;
            regs.ebx = -1;
            regs.edi = X86Pointer(&this->widgets[widgetIdx]);
            regs.esi = X86Pointer(this);
            call((uintptr_t)this->eventHandlers->cursor, regs);

            if (regs.ebx == -1)
            {
                return fallback;
            }

            return (CursorId)regs.ebx;
        }

        return eventHandlers->cursor(this, widgetIdx, xPos, yPos, fallback);
    }

    void Window::callOnMouseUp(WidgetIndex_t widgetIndex)
    {
        if (eventHandlers->on_mouse_up == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_mouse_up))
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = X86Pointer(this);

            // Not sure if this is used
            regs.edi = X86Pointer(&this->widgets[widgetIndex]);

            call((uintptr_t)this->eventHandlers->on_mouse_up, regs);
            return;
        }

        eventHandlers->on_mouse_up(this, widgetIndex);
    }

    Ui::Window* Window::callOnResize()
    {
        if (eventHandlers->on_resize == nullptr)
            return this;

        if (isInteropEvent(eventHandlers->on_resize))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uint32_t)eventHandlers->on_resize, regs);
            return (Window*)regs.esi;
        }

        eventHandlers->on_resize(this);
        return this;
    }

    void Window::call_3(int8_t widget_index)
    {
        if (eventHandlers->event_03 == nullptr)
            return;

        if (isInteropEvent(eventHandlers->event_03))
        {
            registers regs;
            regs.edx = widget_index;
            regs.esi = X86Pointer(this);
            regs.edi = X86Pointer(&this->widgets[widget_index]);
            call((uint32_t)this->eventHandlers->event_03, regs);
            return;
        }

        eventHandlers->event_03(this, widget_index);
    }

    void Window::callOnMouseDown(Ui::WidgetIndex_t widget_index)
    {
        if (eventHandlers->on_mouse_down == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_mouse_down))
        {
            registers regs;
            regs.edx = widget_index;
            regs.esi = X86Pointer(this);
            regs.edi = X86Pointer(&this->widgets[widget_index]);
            call((uint32_t)this->eventHandlers->on_mouse_down, regs);
            return;
        }

        eventHandlers->on_mouse_down(this, widget_index);
    }

    void Window::callOnDropdown(Ui::WidgetIndex_t widget_index, int16_t item_index)
    {
        if (eventHandlers->on_dropdown == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_dropdown))
        {
            registers regs;
            regs.ax = item_index;
            regs.edx = widget_index;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->on_dropdown, regs);
            return;
        }

        eventHandlers->on_dropdown(this, widget_index, item_index);
    }

    void Window::callGetScrollSize(uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        if (eventHandlers->get_scroll_size == nullptr)
            return;

        if (isInteropEvent(eventHandlers->get_scroll_size))
        {
            registers regs;
            regs.eax = scrollIndex;
            regs.esi = X86Pointer(this);
            call((uint32_t)this->eventHandlers->get_scroll_size, regs);
            *scrollWidth = regs.cx;
            *scrollHeight = regs.dx;
            return;
        }

        eventHandlers->get_scroll_size(this, scrollIndex, scrollWidth, scrollHeight);
    }

    void Window::callScrollMouseDown(int16_t xPos, int16_t yPos, uint8_t scroll_index)
    {
        if (eventHandlers->scroll_mouse_down == nullptr)
            return;

        if (isInteropEvent(eventHandlers->scroll_mouse_down))
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = X86Pointer(this);
            regs.cx = xPos;
            regs.dx = yPos;
            call((uint32_t)this->eventHandlers->scroll_mouse_down, regs);
            return;
        }

        this->eventHandlers->scroll_mouse_down(this, xPos, yPos, scroll_index);
    }

    void Window::callScrollMouseDrag(int16_t xPos, int16_t yPos, uint8_t scroll_index)
    {
        if (eventHandlers->scroll_mouse_drag == nullptr)
            return;

        if (isInteropEvent(eventHandlers->scroll_mouse_drag))
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = X86Pointer(this);
            regs.cx = xPos;
            regs.dx = yPos;
            call((uint32_t)this->eventHandlers->scroll_mouse_drag, regs);
            return;
        }

        this->eventHandlers->scroll_mouse_drag(this, xPos, yPos, scroll_index);
    }

    void Window::callScrollMouseOver(int16_t xPos, int16_t yPos, uint8_t scroll_index)
    {
        if (eventHandlers->scroll_mouse_over == nullptr)
            return;

        if (isInteropEvent(eventHandlers->scroll_mouse_over))
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = X86Pointer(this);
            regs.cx = xPos;
            regs.dx = yPos;
            call((uint32_t)this->eventHandlers->scroll_mouse_over, regs);
            return;
        }

        this->eventHandlers->scroll_mouse_over(this, xPos, yPos, scroll_index);
    }

    void Window::callTextInput(WidgetIndex_t caller, const char* buffer)
    {
        if (eventHandlers->text_input == nullptr)
            return;

        if (isInteropEvent(eventHandlers->text_input))
        {
            registers regs;
            regs.dx = caller;
            regs.esi = X86Pointer(this);
            regs.cl = 1;
            regs.edi = X86Pointer(buffer);
            call((uintptr_t)this->eventHandlers->text_input, regs);
            return;
        }

        this->eventHandlers->text_input(this, caller, buffer);
    }

    void Window::callViewportRotate()
    {
        if (eventHandlers->viewport_rotate == nullptr)
            return;

        if (isInteropEvent(eventHandlers->viewport_rotate))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((uintptr_t)this->eventHandlers->viewport_rotate, regs);
            return;
        }

        this->eventHandlers->viewport_rotate(this);
    }

    std::optional<FormatArguments> Window::callTooltip(int16_t widget_index)
    {
        // We only return std::nullopt when required by the tooltip function
        if (eventHandlers->tooltip == nullptr)
            return FormatArguments();

        if (isInteropEvent(eventHandlers->tooltip))
        {
            registers regs;
            regs.ax = widget_index;
            regs.esi = X86Pointer(this);
            call((int32_t)this->eventHandlers->tooltip, regs);
            auto args = FormatArguments();
            if (regs.ax == (int16_t)StringIds::null)
                return {};
            return args;
        }

        return eventHandlers->tooltip(this, widget_index);
    }

    void Window::callOnMove(int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->on_move == nullptr)
            return;

        if (isInteropEvent(eventHandlers->on_move))
        {
            registers regs;
            regs.cx = xPos;
            regs.dx = yPos;
            regs.esi = X86Pointer(this);
            call(reinterpret_cast<int32_t>(this->eventHandlers->on_move), regs);
        }
        this->eventHandlers->on_move(*this, xPos, yPos);
    }

    void Window::callPrepareDraw()
    {
        if (eventHandlers->prepare_draw == nullptr)
            return;

        if (isInteropEvent(eventHandlers->prepare_draw))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            call((int32_t)this->eventHandlers->prepare_draw, regs);
            return;
        }

        eventHandlers->prepare_draw(this);
    }

    void Window::callDraw(Gfx::Context* context)
    {
        if (eventHandlers->draw == nullptr)
            return;

        if (isInteropEvent(this->eventHandlers->draw))
        {
            registers regs;
            regs.esi = X86Pointer(this);
            regs.edi = X86Pointer(context);
            call((int32_t)this->eventHandlers->draw, regs);
            return;
        }

        eventHandlers->draw(this, context);
    }

    void Window::callDrawScroll(Gfx::Context* context, uint32_t scrollIndex)
    {
        if (eventHandlers->draw_scroll == nullptr)
            return;

        if (isInteropEvent(this->eventHandlers->draw_scroll))
        {
            registers regs;
            regs.ax = scrollIndex;
            regs.esi = X86Pointer(this);
            regs.edi = X86Pointer(context);
            call((int32_t)eventHandlers->draw_scroll, regs);
            return;
        }

        eventHandlers->draw_scroll(*this, *context, scrollIndex);
    }

    // 0x004CA4DF
    void Window::draw(Gfx::Context* context)
    {
        if ((this->flags & WindowFlags::transparent) && !(this->flags & WindowFlags::no_background))
        {
            Gfx::fillRect(*context, this->x, this->y, this->x + this->width - 1, this->y + this->height - 1, 0x2000000 | 52);
        }

        uint64_t pressed_widget = 0;
        if (Input::state() == Input::State::dropdownActive || Input::state() == Input::State::widgetPressed)
        {
            if (Input::isPressed(type, number))
            {
                const WidgetIndex_t widgetIndex = Input::getPressedWidgetIndex();
                pressed_widget = 1ULL << widgetIndex;
            }
        }

        uint64_t tool_widget = 0;
        if (Input::isToolActive(this->type, this->number))
        {
            tool_widget = 1ULL << addr<0x00523394, uint32_t>();
        }

        uint64_t hovered_widget = 0;
        if (Input::isHovering(this->type, this->number))
        {
            hovered_widget = 1ULL << Input::getHoveredWidgetIndex();
        }

        uint8_t scrollviewIndex = 0;
        for (WidgetIndex_t widgetIndex = 0; widgetIndex < 64; widgetIndex++)
        {
            auto widget = &this->widgets[widgetIndex];

            if (widget->type == WidgetType::end)
            {
                break;
            }

            widget->draw(context, this, pressed_widget, tool_widget, hovered_widget, scrollviewIndex);
        }

        if (this->flags & WindowFlags::white_border_mask)
        {
            Gfx::fillRectInset(
                *context,
                this->x,
                this->y,
                this->x + this->width - 1,
                this->y + this->height - 1,
                Colour::white,
                0x10);
        }
    }

    WidgetIndex_t Window::firstActivatedWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex)
    {
        WidgetIndex_t activeIndex = -1;
        for (WidgetIndex_t i = minIndex; i <= maxIndex; i++)
        {
            if (this->isActivated(i))
            {
                activeIndex = i;
                break;
            }
        }
        return activeIndex;
    }

    WidgetIndex_t Window::prevAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex)
    {
        WidgetIndex_t activeIndex = firstActivatedWidgetInRange(minIndex, maxIndex);
        if (activeIndex == -1)
            return activeIndex;

        // Offset, wrapping around if needed.
        activeIndex -= 1;
        if (activeIndex < minIndex)
            activeIndex = maxIndex;

        for (WidgetIndex_t i = activeIndex; i >= minIndex; i--)
        {
            if (this->isDisabled(i) || this->widgets[i].type == WidgetType::none)
            {
                // Wrap around (while compensating for next iteration)
                if (i == minIndex)
                    i = maxIndex + 1;
                continue;
            }

            return i;
        }

        return -1;
    }

    WidgetIndex_t Window::nextAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex)
    {
        WidgetIndex_t activeIndex = firstActivatedWidgetInRange(minIndex, maxIndex);
        if (activeIndex == -1)
            return activeIndex;

        // Offset, wrapping around if needed.
        activeIndex += 1;
        if (activeIndex > maxIndex)
            activeIndex = minIndex;

        for (WidgetIndex_t i = activeIndex; i <= maxIndex; i++)
        {
            if (this->isDisabled(i) || this->widgets[i].type == WidgetType::none)
            {
                // Wrap around (while compensating for next iteration)
                if (i == maxIndex)
                    i = minIndex - 1;
                continue;
            }

            return i;
        }

        return -1;
    }
}
