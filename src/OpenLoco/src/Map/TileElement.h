#pragma once

#include "TileElementBase.h"

namespace OpenLoco::World
{
#pragma pack(push, 1)
    struct TileElement : public TileElementBase
    {
    private:
        uint8_t pad[4];
    };
#pragma pack(pop)
    static_assert(sizeof(TileElement) == kTileElementSize);
}
