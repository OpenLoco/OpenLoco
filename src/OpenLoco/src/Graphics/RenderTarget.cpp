#include "RenderTarget.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Gfx
{
    Ui::Rect RenderTarget::getDrawableRect() const
    {
        auto zoom = zoomLevel;
        auto left = x >> zoom;
        auto top = y >> zoom;
        auto right = (width >> zoom) + left;
        auto bottom = (height >> zoom) + top;
        return Ui::Rect::fromLTRB(left, top, right, bottom);
    }

    Ui::Rect RenderTarget::getUiRect() const
    {
        return Ui::Rect::fromLTRB(x, y, x + width, y + height);
    }

    // 0x004CEC50
    std::optional<RenderTarget> clipRenderTarget(const RenderTarget& src, const Ui::Rect& newRect)
    {
        const Ui::Rect oldRect = src.getUiRect();
        Ui::Rect intersect = oldRect.intersection(newRect);
        const auto stride = oldRect.size.width + src.pitch;
        const int16_t newPitch = stride - intersect.size.width;
        auto* newBits = src.bits + (stride * (intersect.origin.y - oldRect.origin.y) + (intersect.origin.x - oldRect.origin.x));
        intersect.origin.x = std::max(0, oldRect.origin.x - newRect.origin.x);
        intersect.origin.y = std::max(0, oldRect.origin.y - newRect.origin.y);
        RenderTarget newRT{ newBits, static_cast<int16_t>(intersect.origin.x), static_cast<int16_t>(intersect.origin.y), static_cast<int16_t>(intersect.size.width), static_cast<int16_t>(intersect.size.height), newPitch, 0 };

        if (newRT.width <= 0 || newRT.height <= 0)
        {
            return {};
        }
        return { newRT };
    }
}
