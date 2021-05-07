#pragma once

#include "../Localisation/StringManager.h"

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct RockObject
    {
        string_id name;
        uint32_t image; // 0x02

        void drawPreviewImage(Gfx::Context& dpi, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
