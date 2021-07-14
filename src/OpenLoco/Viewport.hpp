#pragma once

#include "Graphics/Gfx.h"
#include "Location.hpp"
#include "Map/Map.hpp"
#include "Types.hpp"
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

    namespace ViewportFlags
    {
        constexpr uint32_t underground_view = 1 << 0;
        constexpr uint32_t hide_foreground_tracks_roads = 1 << 1;
        constexpr uint32_t height_marks_on_tracks_roads = 1 << 2;
        constexpr uint32_t height_marks_on_land = 1 << 3;
        constexpr uint32_t one_way_direction_arrows = 1 << 4;
        constexpr uint32_t gridlines_on_landscape = 1 << 5;
        constexpr uint32_t hide_foreground_scenery_buildings = 1 << 6;
        constexpr uint32_t flag_7 = 1 << 7;
        constexpr uint32_t flag_8 = 1 << 8;
        constexpr uint32_t town_names_displayed = 1 << 9;
        constexpr uint32_t station_names_displayed = 1 << 10;
    }

    struct Viewport
    {
        int16_t width;       // 0x00
        int16_t height;      // 0x02
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t view_x;      // 0x08
        int16_t view_y;      // 0x0A
        int16_t view_width;  // 0x0C
        int16_t view_height; // 0x0E
        uint8_t zoom;        // 0x10
        uint8_t pad_11;
        uint16_t flags; // 0x12

        constexpr bool contains(const viewport_pos& vpos)
        {
            return (vpos.y >= view_y && vpos.y < view_y + view_height && vpos.x >= view_x && vpos.x < view_x + view_width);
        }

        constexpr bool containsUi(const xy32& pos)
        {
            return (pos.x >= x && pos.x < x + width && pos.y >= y && pos.y < y + height);
        }

        Ui::Rect getUiRect() const
        {
            return Ui::Rect::fromLTRB(x, y, x + width, y + height);
        }

        constexpr bool intersects(const ViewportRect& vpos)
        {
            if (vpos.right <= view_x)
                return false;

            if (vpos.bottom <= view_y)
                return false;

            if (vpos.left >= view_x + view_width)
                return false;

            if (vpos.top >= view_y + view_height)
                return false;

            return true;
        }

        constexpr ViewportRect getIntersection(const ViewportRect& rect)
        {
            auto out = ViewportRect();
            out.left = std::max(rect.left, view_x);
            out.right = std::min<int16_t>(rect.right, view_x + view_width);
            out.top = std::max(rect.top, view_y);
            out.bottom = std::min<int16_t>(rect.bottom, view_y + view_height);

            return out;
        }

        int getRotation() const;
        void setRotation(int32_t value);

        /**
         * Maps a 2D viewport position to a UI (screen) position.
         */
        xy32 mapToUi(const viewport_pos& vpos)
        {
            auto uiX = x + ((vpos.x - view_x) >> zoom);
            auto uiY = y + ((vpos.y - view_y) >> zoom);
            return { uiX, uiY };
        }

        /**
         * Maps a UI (screen) position to a 2D viewport position.
         */
        viewport_pos uiToMap(const xy32& pos)
        {
            int16_t viewport_x = ((pos.x - x) << zoom) + view_x;
            int16_t viewport_y = ((pos.y - y) << zoom) + view_y;
            return { viewport_x, viewport_y };
        }
        /**
         * Maps a UI (screen) rectangle to a 2D viewport rectangle.
         */
        Rect uiToMap(const Rect& rect)
        {
            auto leftTop = uiToMap({ rect.left(), rect.top() });
            auto rightBottom = uiToMap({ rect.right(), rect.bottom() });
            return Rect::fromLTRB(leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
        }

        void render(Gfx::Context* context);
        void centre2dCoordinates(int16_t x, int16_t y, int16_t z, int16_t* outX, int16_t* outY);
        SavedViewSimple toSavedView() const;
        Map::Pos2 getCentreMapPosition() const;
        Map::Pos2 getCentreScreenMapPosition() const;

    private:
        void paint(Gfx::Context* context, const Ui::Rect& rect);
    };

    struct ViewportConfig
    {
        uint16_t viewport_target_sprite; // 0x0
        int16_t saved_view_x;            // 0x2
        int16_t saved_view_y;            // 0x4
    };
}
