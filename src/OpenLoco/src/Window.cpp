#include "Window.h"
#include "Config.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Entities/EntityManager.h"
#include "Graphics/Colour.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Widget.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>
#include <cinttypes>

using namespace OpenLoco;
using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::Ui
{
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
        return this->hasFlags(WindowFlags::resizable) && (this->minWidth != this->maxWidth || this->minHeight != this->maxHeight);
    }

    void Window::capSize(int32_t newMinWidth, int32_t newMinHeight, int32_t newMaxWidth, int32_t newMaxHeight)
    {
        auto w = this->width;
        auto h = this->height;
        auto shouldInvalidateBefore = false;
        auto shouldInvalidateAfter = false;
        if (w < newMinWidth)
        {
            w = newMinWidth;
            shouldInvalidateAfter = true;
        }
        if (h < newMinHeight)
        {
            h = newMinHeight;
            shouldInvalidateAfter = true;
        }
        if (w > newMaxWidth)
        {
            shouldInvalidateBefore = true;
            w = newMaxWidth;
        }
        if (h > newMaxHeight)
        {
            shouldInvalidateBefore = true;
            h = newMaxHeight;
        }

        if (shouldInvalidateBefore)
        {
            invalidate();
        }
        this->width = w;
        this->height = h;
        this->minWidth = newMinWidth;
        this->minHeight = newMinHeight;
        this->maxWidth = newMaxWidth;
        this->maxHeight = newMaxHeight;
        if (shouldInvalidateAfter)
        {
            invalidate();
        }
    }

    bool Window::isEnabled(int8_t widgetIndex)
    {
        return (this->enabledWidgets & (1ULL << widgetIndex)) != 0;
    }

    bool Window::isDisabled(int8_t widgetIndex)
    {
        return (this->disabledWidgets & (1ULL << widgetIndex)) != 0;
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
    void Window::drawViewports(Gfx::RenderTarget* rt)
    {
        if (viewports[0] != nullptr)
            viewports[0]->render(rt);

        if (viewports[1] != nullptr)
            viewports[1]->render(rt);
    }

    // 0x0045FCE6
    // Input:
    // regs.ax:  x
    // regs.bx:  y
    // regs.bp:  z
    // Output:
    // {x: regs.ax, y: regs.bx}
    std::optional<World::Pos2> screenGetMapXyWithZ(const Point& mouse, const int16_t z)
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
            World::Pos2 position = viewportCoordToMapCoord(vpos.x, vpos.y, z, WindowManager::getCurrentRotation());
            if (World::validCoords(position))
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
    World::Pos2 viewportCoordToMapCoord(int16_t x, int16_t y, int16_t z, int32_t rotation)
    {
        constexpr uint8_t inverseRotationMapping[4] = { 0, 3, 2, 1 };
        const auto result = World::Pos2(y - (x >> 1) + z, y + (x >> 1) + z);
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
            shouldInvalidate = !vp->hasFlags(ViewportFlags::underground_view);
            vp->flags &= ~ViewportFlags::underground_view;
        }
        else
        {
            shouldInvalidate = vp->hasFlags(ViewportFlags::underground_view);
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
        int origX = vp->viewX >> vp->zoom;
        int origY = vp->viewY >> vp->zoom;
        int newX = x >> vp->zoom;
        int newY = y >> vp->zoom;
        int diffX = origX - newX;
        int diffY = origY - newY;

        vp->viewX = x;
        vp->viewY = y;

        // If no change in viewing area
        if (diffX == 0 && diffY == 0)
            return;

        if (vp->hasFlags(ViewportFlags::hide_foreground_tracks_roads) || vp->hasFlags(ViewportFlags::hide_foreground_scenery_buildings) || w->hasFlags(WindowFlags::flag_8))
        {
            auto rect = Ui::Rect(vp->x, vp->y, vp->width, vp->height);
            Gfx::render(rect);
            return;
        }

        uint8_t zoom = (1 << vp->zoom);
        Viewport backup = *vp;

        if (vp->x < 0)
        {
            vp->width += vp->x;
            vp->viewWidth += vp->x * zoom;
            vp->viewX -= vp->x * zoom;
            vp->x = 0;
        }

        int32_t eax = vp->x + vp->width - Ui::width();
        if (eax > 0)
        {
            vp->width -= eax;
            vp->viewWidth -= eax * zoom;
        }

        if (vp->width <= 0)
        {
            *vp = backup;
            return;
        }

        if (vp->y < 0)
        {
            vp->height += vp->y;
            vp->viewHeight += vp->y * zoom;
            vp->viewY -= vp->y * zoom;
            vp->y = 0;
        }

        eax = vp->y + vp->height - Ui::height();
        if (eax > 0)
        {
            vp->height -= eax;
            vp->viewHeight -= eax * zoom;
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

            if (config->viewportTargetSprite != EntityId::null)
            {
                auto entity = EntityManager::get<EntityBase>(config->viewportTargetSprite);

                int z = (TileManager::getHeight(entity->position).landHeight) - 16;
                bool underground = (entity->position.z < z);

                viewportSetUndergroundFlag(underground, viewport);

                centre = viewport->centre2dCoordinates(entity->position + Pos3{ 0, 0, 12 });
            }
            else
            {
                int16_t midX = config->savedViewX + (viewport->viewWidth / 2);
                int16_t midY = config->savedViewY + (viewport->viewHeight / 2);

                World::Pos2 mapCoord = viewportCoordToMapCoord(midX, midY, 128, viewport->getRotation());
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

                    config->savedViewX = coord_2d.x - viewport->viewWidth / 2;
                    config->savedViewY = coord_2d.y - viewport->viewHeight / 2;
                }

                centre.x = config->savedViewX;
                centre.y = config->savedViewY;

                if (this->hasFlags(WindowFlags::scrollingToLocation))
                {
                    bool flippedX = false;
                    centre.x -= viewport->viewX;
                    if (centre.x < 0)
                    {
                        centre.x = -centre.x;
                        flippedX = true;
                    }

                    bool flippedY = false;
                    centre.y -= viewport->viewY;
                    if (centre.y < 0)
                    {
                        centre.y = -centre.y;
                        flippedY = true;
                    }

                    centre.x = (centre.x + 7) / 8; // ceil(centreX / 8.0);
                    centre.y = (centre.y + 7) / 8; // ceil(centreX / 8.0);

                    if (centre.x == 0 && centre.y == 0)
                    {
                        this->flags &= ~WindowFlags::scrollingToLocation;
                    }

                    if (flippedX)
                    {
                        centre.x = -centre.x;
                    }

                    if (flippedY)
                    {
                        centre.y = -centre.y;
                    }

                    centre.x += viewport->viewX;
                    centre.y += viewport->viewY;
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
        Gfx::invalidateRegion(x, y, x + width, y + height);
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

            this->scrollAreas[s].flags = ScrollFlags::none;

            uint16_t scrollWidth = 0, scrollHeight = 0;
            this->callGetScrollSize(s, &scrollWidth, &scrollHeight);
            this->scrollAreas[s].contentOffsetX = 0;
            this->scrollAreas[s].contentWidth = scrollWidth + 1;
            this->scrollAreas[s].contentOffsetY = 0;
            this->scrollAreas[s].contentHeight = scrollHeight + 1;

            if (widget->content & Scrollbars::horizontal)
            {
                this->scrollAreas[s].flags |= Ui::ScrollFlags::hscrollbarVisible;
            }
            if (widget->content & Scrollbars::vertical)
            {
                this->scrollAreas[s].flags |= Ui::ScrollFlags::vscrollbarVisible;
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

    // 0x004CC7CB
    void Window::setDisabledWidgetsAndInvalidate(uint32_t _disabledWidgets)
    {
        const auto oldDisabled = disabledWidgets;
        if (oldDisabled == _disabledWidgets)
        {
            return;
        }
        disabledWidgets = _disabledWidgets;
        auto changedWidgets = oldDisabled ^ _disabledWidgets;
        for (auto widx = Numerics::bitScanForward(changedWidgets); widx != -1; widx = Numerics::bitScanForward(changedWidgets))
        {
            changedWidgets &= ~(1ULL << widx);
            WindowManager::invalidateWidget(type, number, widx);
        }
    }

    void Window::viewportGetMapCoordsByCursor(int16_t* mapX, int16_t* mapY, int16_t* offsetX, int16_t* offsetY)
    {
        // Get mouse position to offset against.
        const auto mouse = Ui::getCursorPos();

        // Compute map coordinate by mouse position.
        auto res = ViewportInteraction::getMapCoordinatesFromPos(mouse.x, mouse.y, ViewportInteraction::InteractionItemFlags::none);
        auto& interaction = res.first;
        *mapX = interaction.pos.x;
        *mapY = interaction.pos.y;

        // Get viewport coordinates centring around the tile.
        auto baseHeight = TileManager::getHeight({ *mapX, *mapY }).landHeight;
        Viewport* v = this->viewports[0];
        const auto dest = v->centre2dCoordinates({ *mapX, *mapY, baseHeight });

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int16_t rebasedX = ((this->width >> 1) - mouse.x) * (1 << v->zoom),
                rebasedY = ((this->height >> 1) - mouse.y) * (1 << v->zoom);

        // Compute cursor offset relative to tile.
        ViewportConfig* vc = &this->viewportConfigurations[0];
        *offsetX = (vc->savedViewX - (dest.x + rebasedX)) * (1 << v->zoom);
        *offsetY = (vc->savedViewY - (dest.y + rebasedY)) * (1 << v->zoom);
    }

    // 0x004C6801
    void Window::moveWindowToLocation(viewport_pos pos)
    {
        if (this->viewportConfigurations->viewportTargetSprite != EntityId::null)
            return;

        if (this->hasFlags(WindowFlags::viewportNoScrolling))
            return;

        this->viewportConfigurations->savedViewX = pos.x;
        this->viewportConfigurations->savedViewY = pos.y;
        this->flags |= WindowFlags::scrollingToLocation;
    }

    // 0x004C6827
    void Window::viewportCentreOnTile(const World::Pos3& loc)
    {
        auto viewport = this->viewports[0];
        if (viewport == nullptr)
            return;

        auto tileHeight = TileManager::getHeight(loc).landHeight;
        tileHeight -= 16;

        if (loc.z < tileHeight)
        {
            if (!viewport->hasFlags(ViewportFlags::underground_view))
            {
                this->invalidate();
            }

            viewport->flags |= ViewportFlags::underground_view;
        }
        else
        {
            if (viewport->hasFlags(ViewportFlags::underground_view))
            {
                this->invalidate();
            }

            viewport->flags &= ~ViewportFlags::underground_view;
        }

        auto pos = gameToScreen(loc, WindowManager::getCurrentRotation());

        pos.x -= viewport->viewWidth / 2;
        pos.y -= viewport->viewHeight / 2;

        moveWindowToLocation(pos);
    }

    void Window::viewportCentreMain()
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return;

        auto main = WindowManager::getMainWindow();

        // Unfocus the viewport.
        main->viewportConfigurations[0].viewportTargetSprite = EntityId::null;

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

    void Window::viewportCentreTileAroundCursor(int16_t mapX, int16_t mapY, int16_t offsetX, int16_t offsetY)
    {
        // Get viewport coordinates centring around the tile.
        auto baseHeight = TileManager::getHeight({ mapX, mapY }).landHeight;
        Viewport* v = this->viewports[0];
        const auto dest = v->centre2dCoordinates({ mapX, mapY, baseHeight });

        // Get mouse position to offset against.
        const auto mouse = Ui::getCursorPos();

        // Rebase mouse position onto centre of window, and compensate for zoom level.
        int16_t rebasedX = ((this->width >> 1) - mouse.x) * (1 << v->zoom),
                rebasedY = ((this->height >> 1) - mouse.y) * (1 << v->zoom);

        // Apply offset to the viewport.
        ViewportConfig* vc = &this->viewportConfigurations[0];
        vc->savedViewX = dest.x + rebasedX + (offsetX / (1 << v->zoom));
        vc->savedViewY = dest.y + rebasedY + (offsetY / (1 << v->zoom));
    }

    void Window::viewportFocusOnEntity(EntityId targetEntity)
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return;

        viewportConfigurations[0].viewportTargetSprite = targetEntity;
    }

    bool Window::viewportIsFocusedOnEntity() const
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return false;

        return viewportConfigurations[0].viewportTargetSprite != EntityId::null;
    }

    void Window::viewportUnfocusFromEntity()
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
            return;

        if (viewportConfigurations[0].viewportTargetSprite == EntityId::null)
            return;

        auto thing = EntityManager::get<EntityBase>(viewportConfigurations[0].viewportTargetSprite);
        viewportConfigurations[0].viewportTargetSprite = EntityId::null;
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
        int16_t savedMapX = 0;
        int16_t savedMapY = 0;
        int16_t offsetX = 0;
        int16_t offsetY = 0;
        if (toCursor && Config::get().zoomToCursor)
        {
            this->viewportGetMapCoordsByCursor(&savedMapX, &savedMapY, &offsetX, &offsetY);
        }

        // Zoom in
        while (v->zoom > zoomLevel)
        {
            v->zoom--;
            vc->savedViewX += v->viewWidth / 4;
            vc->savedViewY += v->viewHeight / 4;
            v->viewWidth /= 2;
            v->viewHeight /= 2;
        }

        // Zoom out
        while (v->zoom < zoomLevel)
        {
            v->zoom++;
            vc->savedViewX -= v->viewWidth / 2;
            vc->savedViewY -= v->viewHeight / 2;
            v->viewWidth *= 2;
            v->viewHeight *= 2;
        }

        // Zooming to cursor? Centre around the tile we were hovering over just now.
        if (toCursor && Config::get().zoomToCursor)
        {
            this->viewportCentreTileAroundCursor(savedMapX, savedMapY, offsetX, offsetY);
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
        viewportRotate(true);
    }

    // 0x0045F0ED
    void Window::viewportRotateLeft()
    {
        viewportRotate(false);
    }

    void Window::viewportRotate(bool directionRight)
    {
        auto* viewport = viewports[0];
        if (viewport == nullptr)
        {
            return;
        }

        const auto uiCentre = viewport->getUiCentre();
        auto res = ViewportInteraction::getSurfaceLocFromUi(uiCentre);

        World::Pos3 target = [&]() {
            if (!res.has_value() || res->second != viewport)
            {
                const auto centre = viewport->getCentreMapPosition();
                const auto height = World::TileManager::getHeight(centre).landHeight;
                return World::Pos3{ centre, height };
            }
            else
            {
                auto height = World::TileManager::getHeight(res->first);
                return World::Pos3{ res->first.x, res->first.y, height.landHeight };
            }
        }();

        viewport->setRotation((viewport->getRotation() + (directionRight ? 1 : -1)) & 3);
        const auto newCentre = viewport->centre2dCoordinates(target);
        viewportConfigurations->savedViewX = newCentre.x;
        viewportConfigurations->savedViewY = newCentre.y;
        viewport->viewX = newCentre.x;
        viewport->viewY = newCentre.y;
        invalidate();
        WindowManager::callViewportRotateEventOnAllWindows();
        EntityManager::updateSpatialIndex();
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
    void Window::viewportFromSavedView(const SavedViewSimple& newSavedView)
    {
        auto viewport = viewports[0];
        if (viewport != nullptr)
        {
            auto& config = viewportConfigurations[0];
            config.viewportTargetSprite = EntityId::null;
            config.savedViewX = newSavedView.viewX;
            config.savedViewY = newSavedView.viewY;

            auto zoom = static_cast<int32_t>(newSavedView.zoomLevel) - viewport->zoom;
            if (zoom != 0)
            {
                if (zoom < 0)
                {
                    zoom = -zoom;
                    viewport->viewWidth >>= zoom;
                    viewport->viewHeight >>= zoom;
                }
                else
                {
                    viewport->viewWidth <<= zoom;
                    viewport->viewHeight <<= zoom;
                }
            }
            viewport->zoom = zoom;
            viewport->setRotation(newSavedView.rotation);

            config.savedViewX -= viewport->viewWidth / 2;
            config.savedViewY -= viewport->viewHeight / 2;
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
        if (eventHandlers->onClose == nullptr)
            return;

        eventHandlers->onClose(*this);
    }

    void Window::callOnPeriodicUpdate()
    {
        if (eventHandlers->onPeriodicUpdate == nullptr)
            return;

        eventHandlers->onPeriodicUpdate(*this);
    }

    void Window::callUpdate()
    {
        if (eventHandlers->onUpdate == nullptr)
            return;

        eventHandlers->onUpdate(*this);
    }

    void Window::call_8()
    {
        if (eventHandlers->event_08 == nullptr)
            return;

        eventHandlers->event_08(*this);
    }

    void Window::call_9()
    {
        if (eventHandlers->event_09 == nullptr)
            return;

        eventHandlers->event_09(*this);
    }

    void Window::callToolUpdate(int16_t widgetIndex, int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->onToolUpdate == nullptr)
            return;

        eventHandlers->onToolUpdate(*this, widgetIndex, xPos, yPos);
    }

    void Window::callToolDown(int16_t widgetIndex, int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->onToolDown == nullptr)
            return;

        eventHandlers->onToolDown(*this, widgetIndex, xPos, yPos);
    }

    void Window::callToolDragContinue(const int16_t widgetIndex, const int16_t xPos, const int16_t yPos)
    {
        if (eventHandlers->toolDragContinue == nullptr)
            return;

        eventHandlers->toolDragContinue(*this, widgetIndex, xPos, yPos);
    }

    void Window::callToolDragEnd(const int16_t widgetIndex)
    {
        if (eventHandlers->toolDragEnd == nullptr)
            return;

        eventHandlers->toolDragEnd(*this, widgetIndex);
    }

    void Window::callToolAbort(int16_t widgetIndex)
    {
        if (eventHandlers->onToolAbort == nullptr)
            return;

        eventHandlers->onToolAbort(*this, widgetIndex);
    }

    Ui::CursorId Window::callToolCursor(int16_t xPos, int16_t yPos, Ui::CursorId fallback, bool* out)
    {
        if (eventHandlers->toolCursor == nullptr)
            return fallback;

        return eventHandlers->toolCursor(*this, xPos, yPos, fallback, *out);
    }

    Ui::CursorId Window::callCursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (eventHandlers->cursor == nullptr)
            return fallback;

        return eventHandlers->cursor(*this, widgetIdx, xPos, yPos, fallback);
    }

    void Window::callOnMouseUp(WidgetIndex_t widgetIndex)
    {
        if (eventHandlers->onMouseUp == nullptr)
            return;

        eventHandlers->onMouseUp(*this, widgetIndex);
    }

    Ui::Window* Window::callOnResize()
    {
        if (eventHandlers->onResize == nullptr)
            return this;

        eventHandlers->onResize(*this);
        return this;
    }

    void Window::call_3(int8_t widgetIndex)
    {
        if (eventHandlers->event_03 == nullptr)
            return;

        eventHandlers->event_03(*this, widgetIndex);
    }

    void Window::callOnMouseDown(Ui::WidgetIndex_t widgetIndex)
    {
        if (eventHandlers->onMouseDown == nullptr)
            return;

        eventHandlers->onMouseDown(*this, widgetIndex);
    }

    void Window::callOnDropdown(Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (eventHandlers->onDropdown == nullptr)
            return;

        eventHandlers->onDropdown(*this, widgetIndex, itemIndex);
    }

    void Window::callGetScrollSize(uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        if (eventHandlers->getScrollSize == nullptr)
            return;

        eventHandlers->getScrollSize(*this, scrollIndex, scrollWidth, scrollHeight);
    }

    void Window::callScrollMouseDown(int16_t xPos, int16_t yPos, uint8_t scrollIndex)
    {
        if (eventHandlers->scrollMouseDown == nullptr)
            return;

        this->eventHandlers->scrollMouseDown(*this, xPos, yPos, scrollIndex);
    }

    void Window::callScrollMouseDrag(int16_t xPos, int16_t yPos, uint8_t scrollIndex)
    {
        if (eventHandlers->scrollMouseDrag == nullptr)
            return;

        this->eventHandlers->scrollMouseDrag(*this, xPos, yPos, scrollIndex);
    }

    void Window::callScrollMouseOver(int16_t xPos, int16_t yPos, uint8_t scrollIndex)
    {
        if (eventHandlers->scrollMouseOver == nullptr)
            return;

        this->eventHandlers->scrollMouseOver(*this, xPos, yPos, scrollIndex);
    }

    void Window::callTextInput(WidgetIndex_t caller, const char* buffer)
    {
        if (eventHandlers->textInput == nullptr)
            return;

        this->eventHandlers->textInput(*this, caller, buffer);
    }

    void Window::callViewportRotate()
    {
        if (eventHandlers->viewportRotate == nullptr)
            return;

        this->eventHandlers->viewportRotate(*this);
    }

    std::optional<FormatArguments> Window::callTooltip(int16_t widgetIndex)
    {
        // We only return std::nullopt when required by the tooltip function
        if (eventHandlers->tooltip == nullptr)
            return FormatArguments();

        return eventHandlers->tooltip(*this, widgetIndex);
    }

    void Window::callOnMove(int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->onMove == nullptr)
            return;

        this->eventHandlers->onMove(*this, xPos, yPos);
    }

    void Window::callPrepareDraw()
    {
        if (eventHandlers->prepareDraw == nullptr)
            return;

        eventHandlers->prepareDraw(*this);
    }

    void Window::callDraw(Gfx::RenderTarget* rt)
    {
        if (eventHandlers->draw == nullptr)
            return;

        eventHandlers->draw(*this, rt);
    }

    void Window::callDrawScroll(Gfx::RenderTarget* rt, uint32_t scrollIndex)
    {
        if (eventHandlers->drawScroll == nullptr)
            return;

        eventHandlers->drawScroll(*this, *rt, scrollIndex);
    }

    // 0x004CA4DF
    void Window::draw(Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        if (this->hasFlags(WindowFlags::transparent) && !this->hasFlags(WindowFlags::noBackground))
        {
            drawingCtx.fillRect(*rt, this->x, this->y, this->x + this->width - 1, this->y + this->height - 1, enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);
        }

        uint64_t pressedWidget = 0;
        if (Input::state() == Input::State::dropdownActive || Input::state() == Input::State::widgetPressed)
        {
            if (Input::isPressed(type, number))
            {
                const WidgetIndex_t widgetIndex = Input::getPressedWidgetIndex();
                pressedWidget = 1ULL << widgetIndex;
            }
        }

        uint64_t tool_widget = 0;
        if (Input::isToolActive(this->type, this->number))
        {
            tool_widget = 1ULL << ToolManager::getToolWidgetIndex();
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

            widget->draw(rt, this, pressedWidget, tool_widget, hovered_widget, scrollviewIndex);
        }

        if (this->hasFlags(WindowFlags::whiteBorderMask))
        {
            drawingCtx.fillRectInset(
                *rt,
                this->x,
                this->y,
                this->x + this->width - 1,
                this->y + this->height - 1,
                Colour::white,
                Drawing::RectInsetFlags::fillNone);
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
