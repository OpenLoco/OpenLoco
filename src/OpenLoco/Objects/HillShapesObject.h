#pragma once

#include "../Types.hpp"
#include "Object.h"
namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

#pragma pack(push, 1)
    struct HillShapesObject
    {
        static constexpr auto _objectType = ObjectType::hillShapes;

        string_id name;
        uint8_t hillHeightMapCount;     // 0x02
        uint8_t mountainHeightMapCount; // 0x03
        uint32_t image;                 // 0x04
        uint8_t pad_08[0x0E - 0x08];

        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
}
