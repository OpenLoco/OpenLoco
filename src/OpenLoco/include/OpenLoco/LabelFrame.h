#pragma once
#include "ZoomLevel.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>

namespace OpenLoco
{

#pragma pack(push, 1)
    struct LabelFrame
    {
        int16_t left[ZoomLevel::max]{};
        int16_t right[ZoomLevel::max]{};
        int16_t top[ZoomLevel::max]{};
        int16_t bottom[ZoomLevel::max]{};

        [[nodiscard]] bool contains(const Ui::Rect& rec, uint8_t zoom) const
        {
            if (rec.top() > bottom[zoom])
            {
                return false;
            }
            if (rec.bottom() < top[zoom])
            {
                return false;
            }
            if (rec.left() > right[zoom])
            {
                return false;
            }
            if (rec.right() < left[zoom])
            {
                return false;
            }
            return true;
        }
    };
#pragma pack(pop)
}
