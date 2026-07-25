#pragma once
#include "ZoomLevel.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>

namespace OpenLoco
{

#pragma pack(push, 1)
    struct LabelFrame
    {
        int32_t left[ZoomLevel::count]{};
        int32_t right[ZoomLevel::count]{};
        int32_t top[ZoomLevel::count]{};
        int32_t bottom[ZoomLevel::count]{};

        [[nodiscard]] bool contains(const Ui::Rect& rec, ZoomLevel zoom) const
        {
            const auto index = zoom.index();
            if (rec.top() > bottom[index])
            {
                return false;
            }
            if (rec.bottom() < top[index])
            {
                return false;
            }
            if (rec.left() > right[index])
            {
                return false;
            }
            if (rec.right() < left[index])
            {
                return false;
            }
            return true;
        }
    };
#pragma pack(pop)
}
