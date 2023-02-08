#pragma once

#include "Graphics/Gfx.h"
#include "Location.hpp"
#include "Map/Map.hpp"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <algorithm>

namespace OpenLoco::Ui
{
    struct SavedViewSimple;

    struct viewport_pos
    {
        int16_t x{};
        int16_t y{};

        viewport_pos()
            : viewport_pos(0, 0)
        {
        }
        viewport_pos(int16_t _x, int16_t _y)
            : x(_x)
            , y(_y)
        {
        }
    };

    struct ViewportRect
    {
        int16_t left = 0;
        int16_t top = 0;
        int16_t bottom = 0;
        int16_t right = 0;

        constexpr bool contains(const viewport_pos& vpos)
        {
            return (left < vpos.x && top < vpos.y && right >= vpos.x && bottom >= vpos.y);
        }
    };

    enum class ViewportFlags : uint16_t
    {
        none = 0U,
        underground_view = 1U << 0,
        hide_foreground_tracks_roads = 1U << 1,
        height_marks_on_tracks_roads = 1U << 2,
        height_marks_on_land = 1U << 3,
        one_way_direction_arrows = 1U << 4,
        gridlines_on_landscape = 1U << 5,
        hide_foreground_scenery_buildings = 1U << 6,
        flag_7 = 1U << 7,
        flag_8 = 1U << 8,
        town_names_displayed = 1U << 9,
        station_names_displayed = 1U << 10,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ViewportFlags);

    struct Viewport;

    namespace ScreenToViewport
    {
        [[nodiscard]] constexpr Point applyTransform(const Point& uiPoint, const Viewport& vp);
    }

    namespace ViewportToScreen
    {
        [[nodiscard]] constexpr Point applyTransform(const Point& vpPoint, const Viewport& vp);
    }

    struct Viewport
    {
        int16_t width;       // 0x00
        int16_t height;      // 0x02
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t viewX;       // 0x08
        int16_t viewY;       // 0x0A
        int16_t viewWidth;   // 0x0C
        int16_t viewHeight;  // 0x0E
        uint8_t zoom;        // 0x10
        uint8_t pad_11;      // 0x11
        ViewportFlags flags; // 0x12

        constexpr bool contains(const viewport_pos& vpos)
        {
            return (vpos.y >= viewY && vpos.y < viewY + viewHeight && vpos.x >= viewX && vpos.x < viewX + viewWidth);
        }

        constexpr bool containsUi(const Point& pos)
        {
            return (pos.x >= x && pos.x < x + width && pos.y >= y && pos.y < y + height);
        }

        Ui::Rect getUiRect() const
        {
            return Ui::Rect::fromLTRB(x, y, x + width, y + height);
        }

        constexpr bool intersects(const ViewportRect& vpos)
        {
            if (vpos.right <= viewX)
                return false;

            if (vpos.bottom <= viewY)
                return false;

            if (vpos.left >= viewX + viewWidth)
                return false;

            if (vpos.top >= viewY + viewHeight)
                return false;

            return true;
        }

        constexpr ViewportRect getIntersection(const ViewportRect& rect)
        {
            auto out = ViewportRect();
            out.left = std::max(rect.left, viewX);
            out.right = std::min<int16_t>(rect.right, viewX + viewWidth);
            out.top = std::max(rect.top, viewY);
            out.bottom = std::min<int16_t>(rect.bottom, viewY + viewHeight);

            return out;
        }

        int getRotation() const;
        void setRotation(int32_t value);

        /**
         * Maps a 2D viewport position to a UI (screen) position.
         */
        Point viewportToScreen(const viewport_pos& vpos) const
        {
            const auto vpPoint = ViewportToScreen::applyTransform({ vpos.x, vpos.y }, *this);
            return vpPoint;
        }

        /**
         * Maps a UI (screen) position to a 2D viewport position.
         */
        viewport_pos screenToViewport(const Point& pos) const
        {
            const auto vpPoint = ScreenToViewport::applyTransform(pos, *this);
            return { vpPoint.x, vpPoint.y };
        }
        /**
         * Maps a UI (screen) rectangle to a 2D viewport rectangle.
         */
        Rect screenToViewport(const Rect& rect)
        {
            auto leftTop = screenToViewport(Point(rect.left(), rect.top()));
            auto rightBottom = screenToViewport(Point(rect.right(), rect.bottom()));
            return Rect::fromLTRB(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
        }

        void render(Gfx::RenderTarget* rt);
        viewport_pos centre2dCoordinates(const Map::Pos3& loc);
        SavedViewSimple toSavedView() const;

        viewport_pos getCentre() const;
        Point getUiCentre() const;
        Map::Pos2 getCentreMapPosition() const;
        std::optional<Map::Pos2> getCentreScreenMapPosition() const;

        constexpr bool hasFlags(ViewportFlags flagsToTest) const
        {
            return (flags & flagsToTest) != ViewportFlags::none;
        }

    private:
        void paint(Gfx::RenderTarget* rt, const Ui::Rect& rect);
    };
    static_assert(sizeof(Viewport) == 0x14);

    struct ViewportConfig
    {
        EntityId viewportTargetSprite; // 0x0
        int16_t savedViewX;            // 0x2
        int16_t savedViewY;            // 0x4
    };

    namespace ScreenToViewport
    {
        [[nodiscard]] constexpr Point uiOffsetTransform(const Point& uiPoint, const Viewport& vp)
        {
            return uiPoint - Point{ vp.x, vp.y };
        }

        [[nodiscard]] constexpr Point scaleTransform(const Point& uiPoint, const Viewport& vp)
        {
            return uiPoint << vp.zoom;
        }

        [[nodiscard]] constexpr Point viewOffsetTransform(const Point& point, const Viewport& vp)
        {
            return point + Point{ vp.viewX, vp.viewY };
        }

        [[nodiscard]] constexpr Point applyTransform(const Point& uiPoint, const Viewport& vp)
        {
            return viewOffsetTransform(scaleTransform(uiOffsetTransform(uiPoint, vp), vp), vp);
        }
    }

    namespace ViewportToScreen
    {
        [[nodiscard]] constexpr Point uiOffsetTransform(const Point& uiPoint, const Viewport& vp)
        {
            return uiPoint + Point{ vp.x, vp.y };
        }

        [[nodiscard]] constexpr Point scaleTransform(const Point& uiPoint, const Viewport& vp)
        {
            return uiPoint >> vp.zoom;
        }

        [[nodiscard]] constexpr Point viewOffsetTransform(const Point& point, const Viewport& vp)
        {
            return point - Point{ vp.viewX, vp.viewY };
        }

        [[nodiscard]] constexpr Point applyTransform(const Point& vpPoint, const Viewport& vp)
        {
            return uiOffsetTransform(scaleTransform(viewOffsetTransform(vpPoint, vp), vp), vp);
        }
    }
}
