#include "S5/S5LabelFrame.h"
#include "LabelFrame.h"

namespace OpenLoco::S5
{
    S5::LabelFrame exportLabelFrame(const OpenLoco::LabelFrame& src)
    {
        S5::LabelFrame dst{};
        std::ranges::copy(src.left, dst.left);
        std::ranges::copy(src.right, dst.right);
        std::ranges::copy(src.top, dst.top);
        std::ranges::copy(src.bottom, dst.bottom);
        return dst;
    }
}
