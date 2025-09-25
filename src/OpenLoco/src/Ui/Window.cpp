#include "Window.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "Graphics/Colour.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/Tile.h"
#include "Map/TileManager.h"
#include "Ui.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "ViewportManager.h"
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
    Window::Window(Ui::Point32 position, Ui::Size32 size)
        : x(static_cast<int16_t>(position.x))
        , y(static_cast<int16_t>(position.y))
        , width(static_cast<uint16_t>(size.width))
        , height(static_cast<uint16_t>(size.height))
        , minWidth(static_cast<uint16_t>(size.width))
        , maxWidth(static_cast<uint16_t>(size.width))
        , minHeight(static_cast<uint16_t>(size.height))
        , maxHeight(static_cast<uint16_t>(size.height))
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

    bool Window::isEnabled(WidgetIndex_t widgetIndex)
    {
        return (this->disabledWidgets & (1ULL << widgetIndex)) == 0;
    }

    bool Window::isDisabled(WidgetIndex_t widgetIndex)
    {
        return (this->disabledWidgets & (1ULL << widgetIndex)) != 0;
    }

    bool Window::isActivated(WidgetIndex_t index)
    {
        return (this->activatedWidgets & (1ULL << index)) != 0;
    }

    bool Window::isHoldable(WidgetIndex_t index)
    {
        return (this->holdableWidgets & (1ULL << index)) != 0;
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

        if (vp->containsUi(mouse - w->position()))
        {
            viewport_pos vpos = vp->screenToViewport(mouse - w->position());
            World::Pos2 position = viewportCoordToMapCoord(vpos.x, vpos.y, z, WindowManager::getCurrentRotation());
            if (World::TileManager::validCoords(position))
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
        {
            return;
        }

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
        {
            w->invalidate();
        }
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
        {
            return;
        }

        if (vp->hasFlags(ViewportFlags::seeThroughTracks | ViewportFlags::seeThroughScenery | ViewportFlags::seeThroughRoads | ViewportFlags::seeThroughBuildings | ViewportFlags::seeThroughTrees | ViewportFlags::seeThroughBridges) || w->hasFlags(WindowFlags::flag_8))
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
        WidgetIndex_t pressedWidgetIndex = kWidgetIndexNull;
        if (Input::isPressed(type, number) || Input::isDropdownActive(type, number))
        {
            pressedWidgetIndex = Input::getPressedWidgetIndex();
        }

        int16_t toolWidgetIndex = -1;
        if (ToolManager::isToolActive(type, number))
        {
            toolWidgetIndex = ToolManager::getToolWidgetIndex();
        }

        WidgetIndex_t widx{};
        for (auto& widget : widgets)
        {
            const bool activated = isActivated(widx);
            // This might be the remap flag, not entirely sure.
            const bool hasBit31 = (widget.content & (1U << 31)) != 0;
            if ((widget.type == WidgetType::slider || widget.type == WidgetType::wt_3) && hasBit31)
            {
                if (activated || pressedWidgetIndex == widx || toolWidgetIndex == widx)
                {
                    Gfx::invalidateRegion(
                        x + widget.left,
                        y + widget.top,
                        x + widget.right + 1,
                        y + widget.bottom + 1);
                }
            }
            widx++;
        }
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
        WidgetIndex_t widx = -1;
        for (auto& widget : widgets)
        {
            widx++;
            if (widget.type != WidgetType::scrollview)
            {
                continue;
            }

            int32_t scrollWidth = 0, scrollHeight = 0;
            this->callGetScrollSize(s, scrollWidth, scrollHeight);

            bool invalidate = false;

            if (widget.content & Scrollbars::horizontal)
            {
                if (this->scrollAreas[s].contentWidth != scrollWidth + 1)
                {
                    this->scrollAreas[s].contentWidth = scrollWidth + 1;
                    invalidate = true;
                }
            }

            if (widget.content & Scrollbars::vertical)
            {
                if (this->scrollAreas[s].contentHeight != scrollHeight + 1)
                {
                    this->scrollAreas[s].contentHeight = scrollHeight + 1;
                    invalidate = true;
                }
            }

            if (invalidate)
            {
                Ui::ScrollView::updateThumbs(*this, widx);
                this->invalidate();
            }

            s++;
        }
    }

    // 0x004CA17F
    void Window::initScrollWidgets()
    {
        uint32_t s = 0;
        WidgetIndex_t widx = -1;
        for (auto& widget : widgets)
        {
            widx++;

            if (widget.type != WidgetType::scrollview)
            {
                continue;
            }

            this->scrollAreas[s].flags = ScrollFlags::none;

            int32_t scrollWidth = 0, scrollHeight = 0;
            this->callGetScrollSize(s, scrollWidth, scrollHeight);
            this->scrollAreas[s].contentOffsetX = 0;
            this->scrollAreas[s].contentWidth = scrollWidth + 1;
            this->scrollAreas[s].contentOffsetY = 0;
            this->scrollAreas[s].contentHeight = scrollHeight + 1;

            if (widget.content & Scrollbars::horizontal)
            {
                this->scrollAreas[s].flags |= Ui::ScrollFlags::hscrollbarVisible;
            }
            if (widget.content & Scrollbars::vertical)
            {
                this->scrollAreas[s].flags |= Ui::ScrollFlags::vscrollbarVisible;
            }

            Ui::ScrollView::updateThumbs(*this, widx);
            s++;
        }
    }

    int8_t Window::getScrollDataIndex(WidgetIndex_t targetIndex)
    {
        if (widgets[targetIndex].type != WidgetType::scrollview)
        {
            assert(false);
            return -1;
        }

        auto widgetIndex = 0;
        auto scrollIndex = 0;
        for (auto& widget : widgets)
        {
            widgetIndex++;
            if (widgetIndex == targetIndex)
            {
                return scrollIndex;
            }

            if (widget.type == WidgetType::scrollview)
            {
                scrollIndex++;
            }
        }

        assert(false);
        return -2;
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

    // 0x004C6801
    void Window::moveWindowToLocation(viewport_pos pos)
    {
        if (this->viewportConfigurations->viewportTargetSprite != EntityId::null)
        {
            return;
        }

        if (this->hasFlags(WindowFlags::viewportNoScrolling))
        {
            return;
        }

        this->viewportConfigurations->savedViewX = pos.x;
        this->viewportConfigurations->savedViewY = pos.y;
        this->flags |= WindowFlags::scrollingToLocation;
    }

    // 0x004C6827
    void Window::viewportCentreOnTile(const World::Pos3& loc)
    {
        auto viewport = this->viewports[0];
        if (viewport == nullptr)
        {
            return;
        }

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
        {
            return;
        }

        auto main = WindowManager::getMainWindow();

        // Unfocus the viewport.
        main->viewportConfigurations[0].viewportTargetSprite = EntityId::null;

        // Centre viewport on tile/entity.
        if (savedView.isEntityView())
        {
            auto entity = EntityManager::get<EntityBase>(savedView.entityId);
            main->viewportCentreOnTile(entity->position);
        }
        else
        {
            main->viewportCentreOnTile(savedView.getPos());
        }
    }

    void Window::viewportFocusOnEntity(EntityId targetEntity)
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
        {
            return;
        }

        viewportConfigurations[0].viewportTargetSprite = targetEntity;
    }

    bool Window::viewportIsFocusedOnEntity(EntityId targetEntity) const
    {
        if (targetEntity == EntityId::null || viewports[0] == nullptr || savedView.isEmpty())
        {
            return false;
        }

        return viewportConfigurations[0].viewportTargetSprite == targetEntity;
    }

    bool Window::viewportIsFocusedOnAnyEntity() const
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
        {
            return false;
        }

        return viewportConfigurations[0].viewportTargetSprite != EntityId::null;
    }

    void Window::viewportUnfocusFromEntity()
    {
        if (viewports[0] == nullptr || savedView.isEmpty())
        {
            return;
        }

        if (viewportConfigurations[0].viewportTargetSprite == EntityId::null)
        {
            return;
        }

        auto entity = EntityManager::get<EntityBase>(viewportConfigurations[0].viewportTargetSprite);
        viewportConfigurations[0].viewportTargetSprite = EntityId::null;
        viewportCentreOnTile(entity->position);
    }

    void Window::viewportZoomSet(int8_t zoomLevel, bool toCursor)
    {
        Viewport* v = this->viewports[0];
        ViewportConfig* vc = &this->viewportConfigurations[0];

        zoomLevel = std::clamp<int8_t>(zoomLevel, 0, 3);
        if (v->zoom == zoomLevel)
        {
            return;
        }

        const auto previousZoomLevel = v->zoom;

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

        if (toCursor && Config::get().zoomToCursor)
        {
            const auto mouseCoords = Ui::getCursorPosScaled() - Point32(v->x, v->y);
            const int32_t diffX = mouseCoords.x - ((v->viewWidth >> zoomLevel) / 2);
            const int32_t diffY = mouseCoords.y - ((v->viewHeight >> zoomLevel) / 2);
            if (previousZoomLevel > zoomLevel)
            {
                vc->savedViewX += diffX << zoomLevel;
                vc->savedViewY += diffY << zoomLevel;
            }
            else
            {
                vc->savedViewX -= diffX << previousZoomLevel;
                vc->savedViewY -= diffY << previousZoomLevel;
            }
        }

        v->viewX = vc->savedViewX;
        v->viewY = vc->savedViewY;

        this->invalidate();
    }

    // 0x0045EFDB
    void Window::viewportZoomIn(bool toCursor)
    {
        if (this->viewports[0] == nullptr)
        {
            return;
        }

        this->viewportZoomSet(this->viewports[0]->zoom - 1, toCursor);
    }

    // 0x0045F015
    void Window::viewportZoomOut(bool toCursor)
    {
        if (this->viewports[0] == nullptr)
        {
            return;
        }

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
        if (auto* vp = viewports[viewportId]; vp != nullptr)
        {
            ViewportManager::destroy(vp);
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
        {
            offset.x -= xOvershoot;
        }

        // If not, on the left?
        if (this->x < 0)
        {
            offset.x -= this->x;
        }

        const int16_t yOvershoot = this->y + this->height - (Ui::height() - 27);

        // Over the edge at the bottom?
        if (yOvershoot > 0)
        {
            offset.y -= yOvershoot;
        }

        // Maybe at the top?
        if (this->y - 28 < 0)
        {
            offset.y -= this->y - 28;
        }

        if (offset == Ui::Point(0, 0))
        {
            return;
        }

        this->invalidate();
        this->x += offset.x;
        this->y += offset.y;
        this->invalidate();
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

        WidgetIndex_t activeWidget = kWidgetIndexNull;

        WidgetIndex_t widgetIndex = kWidgetIndexNull;
        for (auto& widget : widgets)
        {
            widgetIndex++;

            if (widget.type == WidgetType::empty || widget.hidden)
            {
                continue;
            }

            if (xPos < this->x + widget.left)
            {
                continue;
            }

            if (xPos > this->x + widget.right)
            {
                continue;
            }

            if (yPos < this->y + widget.top)
            {
                continue;
            }

            if (yPos > this->y + widget.bottom)
            {
                continue;
            }

            activeWidget = widgetIndex;
        }

        if (activeWidget == kWidgetIndexNull)
        {
            return kWidgetIndexNull;
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
        {
            return;
        }

        eventHandlers->onClose(*this);
    }

    void Window::callOnPeriodicUpdate()
    {
        if (eventHandlers->onPeriodicUpdate == nullptr)
        {
            return;
        }

        eventHandlers->onPeriodicUpdate(*this);
    }

    void Window::callUpdate()
    {
        if (eventHandlers->onUpdate == nullptr)
        {
            return;
        }

        eventHandlers->onUpdate(*this);
    }

    void Window::call_8()
    {
        if (eventHandlers->event_08 == nullptr)
        {
            return;
        }

        eventHandlers->event_08(*this);
    }

    void Window::call_9()
    {
        if (eventHandlers->event_09 == nullptr)
        {
            return;
        }

        eventHandlers->event_09(*this);
    }

    void Window::callToolUpdate(WidgetIndex_t widgetIndex, const WidgetId id, int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->onToolUpdate == nullptr)
        {
            return;
        }

        eventHandlers->onToolUpdate(*this, widgetIndex, id, xPos, yPos);
    }

    void Window::callToolDown(WidgetIndex_t widgetIndex, const WidgetId id, int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->onToolDown == nullptr)
        {
            return;
        }

        eventHandlers->onToolDown(*this, widgetIndex, id, xPos, yPos);
    }

    void Window::callToolDrag(const WidgetIndex_t widgetIndex, const WidgetId id, const int16_t xPos, const int16_t yPos)
    {
        if (eventHandlers->toolDrag == nullptr)
        {
            return;
        }

        eventHandlers->toolDrag(*this, widgetIndex, id, xPos, yPos);
    }

    void Window::callToolUp(const WidgetIndex_t widgetIndex, const WidgetId id, const int16_t xPos, const int16_t yPos)
    {
        if (eventHandlers->toolUp == nullptr)
        {
            return;
        }

        eventHandlers->toolUp(*this, widgetIndex, id, xPos, yPos);
    }

    void Window::callToolAbort(WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (eventHandlers->onToolAbort == nullptr)
        {
            return;
        }

        eventHandlers->onToolAbort(*this, widgetIndex, id);
    }

    Ui::CursorId Window::callToolCursor(int16_t xPos, int16_t yPos, Ui::CursorId fallback, bool* out)
    {
        if (eventHandlers->toolCursor == nullptr)
        {
            return fallback;
        }

        return eventHandlers->toolCursor(*this, xPos, yPos, fallback, *out);
    }

    Ui::CursorId Window::callCursor(WidgetIndex_t widgetIdx, const WidgetId id, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (eventHandlers->cursor == nullptr)
        {
            return fallback;
        }

        return eventHandlers->cursor(*this, widgetIdx, id, xPos, yPos, fallback);
    }

    void Window::callOnMouseUp(WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (eventHandlers->onMouseUp == nullptr)
        {
            return;
        }

        eventHandlers->onMouseUp(*this, widgetIndex, id);
    }

    Ui::Window* Window::callOnResize()
    {
        if (eventHandlers->onResize == nullptr)
        {
            return this;
        }

        eventHandlers->onResize(*this);
        return this;
    }

    void Window::callOnMouseHover(WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (eventHandlers->onMouseHover == nullptr)
        {
            return;
        }

        eventHandlers->onMouseHover(*this, widgetIndex, id);
    }

    void Window::callOnMouseDown(WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (eventHandlers->onMouseDown == nullptr)
        {
            return;
        }

        eventHandlers->onMouseDown(*this, widgetIndex, id);
    }

    void Window::callOnDropdown(WidgetIndex_t widgetIndex, const WidgetId id, int16_t itemIndex)
    {
        if (eventHandlers->onDropdown == nullptr)
        {
            return;
        }

        eventHandlers->onDropdown(*this, widgetIndex, id, itemIndex);
    }

    void Window::callGetScrollSize(uint32_t scrollIndex, int32_t& scrollWidth, int32_t& scrollHeight)
    {
        if (eventHandlers->getScrollSize == nullptr)
        {
            return;
        }

        eventHandlers->getScrollSize(*this, scrollIndex, scrollWidth, scrollHeight);
    }

    void Window::callScrollMouseDown(int16_t xPos, int16_t yPos, uint8_t scrollIndex)
    {
        if (eventHandlers->scrollMouseDown == nullptr)
        {
            return;
        }

        this->eventHandlers->scrollMouseDown(*this, xPos, yPos, scrollIndex);
    }

    void Window::callScrollMouseDrag(int16_t xPos, int16_t yPos, uint8_t scrollIndex)
    {
        if (eventHandlers->scrollMouseDrag == nullptr)
        {
            return;
        }

        this->eventHandlers->scrollMouseDrag(*this, xPos, yPos, scrollIndex);
    }

    void Window::callScrollMouseOver(int16_t xPos, int16_t yPos, uint8_t scrollIndex)
    {
        if (eventHandlers->scrollMouseOver == nullptr)
        {
            return;
        }

        this->eventHandlers->scrollMouseOver(*this, xPos, yPos, scrollIndex);
    }

    void Window::callTextInput(WidgetIndex_t caller, const WidgetId id, const char* buffer)
    {
        if (eventHandlers->textInput == nullptr)
        {
            return;
        }

        this->eventHandlers->textInput(*this, caller, id, buffer);
    }

    void Window::callViewportRotate()
    {
        if (eventHandlers->viewportRotate == nullptr)
        {
            return;
        }

        this->eventHandlers->viewportRotate(*this);
    }

    std::optional<FormatArguments> Window::callTooltip(WidgetIndex_t widgetIndex, const WidgetId id)
    {
        // We only return std::nullopt when required by the tooltip function
        if (eventHandlers->tooltip == nullptr)
        {
            return FormatArguments();
        }

        return eventHandlers->tooltip(*this, widgetIndex, id);
    }

    void Window::callOnMove(int16_t xPos, int16_t yPos)
    {
        if (eventHandlers->onMove == nullptr)
        {
            return;
        }

        this->eventHandlers->onMove(*this, xPos, yPos);
    }

    void Window::callPrepareDraw()
    {
        if (eventHandlers->prepareDraw == nullptr)
        {
            return;
        }

        eventHandlers->prepareDraw(*this);
    }

    void Window::callDraw(Gfx::DrawingContext& ctx)
    {
        if (eventHandlers->draw == nullptr)
        {
            return;
        }

        eventHandlers->draw(*this, ctx);
    }

    void Window::callDrawScroll(Gfx::DrawingContext& drawingCtx, uint32_t scrollIndex)
    {
        if (eventHandlers->drawScroll == nullptr)
        {
            return;
        }

        eventHandlers->drawScroll(*this, drawingCtx, scrollIndex);
    }

    bool Window::callKeyUp(uint32_t charCode, uint32_t keyCode)
    {
        if (eventHandlers->keyUp == nullptr)
        {
            return false;
        }

        return eventHandlers->keyUp(*this, charCode, keyCode);
    }

    // 0x004CA4DF
    void Window::draw(Gfx::DrawingContext& drawingCtx)
    {
        if (this->hasFlags(WindowFlags::transparent) && !this->hasFlags(WindowFlags::noBackground))
        {
            drawingCtx.fillRect(0, 0, this->width - 1, this->height - 1, enumValue(ExtColour::unk34), Gfx::RectFlags::transparent);
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
        if (ToolManager::isToolActive(this->type, this->number))
        {
            tool_widget = 1ULL << ToolManager::getToolWidgetIndex();
        }

        uint64_t hovered_widget = 0;
        if (Input::isHovering(this->type, this->number))
        {
            hovered_widget = 1ULL << Input::getHoveredWidgetIndex();
        }

        uint8_t scrollviewIndex = 0;
        for (auto& widget : widgets)
        {
            widget.draw(drawingCtx, this, pressedWidget, tool_widget, hovered_widget, scrollviewIndex);

            // FIXME: This is ugly and error prone, put the ScrollArea data in the widget,
            //        previously it was passed as reference to draw where it incremented it.
            if (widget.type == WidgetType::scrollview)
            {
                scrollviewIndex++;
            }
        }

        if (this->hasFlags(WindowFlags::whiteBorderMask))
        {
            drawingCtx.fillRectInset(
                0,
                0,
                this->width - 1,
                this->height - 1,
                Colour::white,
                Gfx::RectInsetFlags::fillNone);
        }
    }

    WidgetIndex_t Window::firstActivatedWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex)
    {
        WidgetIndex_t activeIndex = kWidgetIndexNull;
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
        {
            return activeIndex;
        }

        // Offset, wrapping around if needed.
        activeIndex -= 1;
        if (activeIndex < minIndex)
        {
            activeIndex = maxIndex;
        }

        for (WidgetIndex_t i = activeIndex; i >= minIndex; i--)
        {
            if (this->isDisabled(i) || this->widgets[i].type == WidgetType::empty || this->widgets[i].hidden)
            {
                // Wrap around (while compensating for next iteration)
                if (i == minIndex)
                {
                    i = maxIndex + 1;
                }
                continue;
            }

            return i;
        }

        return -1;
    }

    WidgetIndex_t Window::nextAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex)
    {
        WidgetIndex_t activeIndex = firstActivatedWidgetInRange(minIndex, maxIndex);
        if (activeIndex == kWidgetIndexNull)
        {
            return activeIndex;
        }

        // Offset, wrapping around if needed.
        activeIndex += 1;
        if (activeIndex > maxIndex)
        {
            activeIndex = minIndex;
        }

        for (WidgetIndex_t i = activeIndex; i <= maxIndex; i++)
        {
            if (this->isDisabled(i) || this->widgets[i].type == WidgetType::empty || this->widgets[i].hidden)
            {
                // Wrap around (while compensating for next iteration)
                if (i == maxIndex)
                {
                    i = minIndex - 1;
                }
                continue;
            }

            return i;
        }

        return -1;
    }
}
