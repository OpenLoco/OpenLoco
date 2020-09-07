#pragma once

#include "../Graphics/Types.h"
#include <algorithm>
#include <cstddef>

namespace openloco::ui
{
    class Rect
    {
    public:
        gfx::ui_size_t size;
        gfx::point_t origin;
        Rect(int16_t x, int16_t y, uint16_t width, uint16_t height)
            : size(gfx::ui_size_t(width, height))
            , origin(gfx::point_t(x, y))
        {
        }

        static Rect fromLTRB(int16_t left, int16_t top, int16_t right, int16_t bottom)
        {
            return Rect(left, top, right - left, bottom - top);
        }

        Rect intersection(const Rect r2) const
        {
            int left = std::max(this->origin.x, r2.origin.x);
            int top = std::max(this->origin.y, r2.origin.y);
            int right = std::min(this->origin.x + this->size.width, r2.origin.x + r2.size.width);
            int bottom = std::min(this->origin.y + this->size.height, r2.origin.y + r2.size.height);

            return Rect(left, top, right - left, bottom - top);
        }

        uint16_t width() const
        {
            return this->size.width;
        }

        uint16_t height() const
        {
            return this->size.height;
        }

        int32_t left() const
        {
            return this->origin.x;
        }

        int32_t right() const
        {
            return this->origin.x + this->size.width;
        }

        int32_t top() const
        {
            return this->origin.y;
        }

        int32_t bottom() const
        {
            return this->origin.y + this->size.height;
        }
    };
}
