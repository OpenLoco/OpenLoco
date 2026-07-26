#include "S5/S5LabelFrame.h"
#include "LabelFrame.h"
#include <array>

namespace OpenLoco::S5
{
    static constexpr auto kNumSavedZoomLevels = std::size(S5::LabelFrame{}.left);

    S5::LabelFrame exportLabelFrame(const OpenLoco::LabelFrame& src)
    {
        S5::LabelFrame dst{};
        for (auto zoom = 0U; zoom < kNumSavedZoomLevels; ++zoom)
        {
            const auto index = ZoomLevel(static_cast<int8_t>(zoom)).index();
            dst.left[zoom] = static_cast<int16_t>(src.left[index]);
            dst.right[zoom] = static_cast<int16_t>(src.right[index]);
            dst.top[zoom] = static_cast<int16_t>(src.top[index]);
            dst.bottom[zoom] = static_cast<int16_t>(src.bottom[index]);
        }
        return dst;
    }

    OpenLoco::LabelFrame importLabelFrame(const S5::LabelFrame& src)
    {
        OpenLoco::LabelFrame dst{};
        for (auto zoom = 0U; zoom < kNumSavedZoomLevels; ++zoom)
        {
            const auto index = ZoomLevel(static_cast<int8_t>(zoom)).index();
            dst.left[index] = src.left[zoom];
            dst.right[index] = src.right[zoom];
            dst.top[index] = src.top[zoom];
            dst.bottom[index] = src.bottom[zoom];
        }
        return dst;
    }
}
