#pragma once

#include "Point.hpp"
#include "Size.hpp"

#include <algorithm>
#include <cstddef>

namespace OpenLoco::Ui
{
    class Rect
    {
    public:
        Ui::Size size;
        Ui::Point32 origin;
        Rect(int16_t x, int16_t y, uint16_t width, uint16_t height)
            : size(Ui::Size(width, height))
            , origin(Ui::Point32(x, y))
        {
        }

        static Rect fromLTRB(int16_t left, int16_t top, int16_t right, int16_t bottom)
        {
            return Rect(left, top, right - left, bottom - top);
        }

        bool intersects(const Rect& r2) const
        {
            if (origin.x + size.width <= r2.origin.x)
                return false;
            if (origin.y + size.height <= r2.origin.y)
                return false;
            if (origin.x >= r2.origin.x + r2.size.width)
                return false;
            if (origin.y >= r2.origin.y + r2.size.height)
                return false;
            return true;
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
