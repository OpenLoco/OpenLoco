#pragma once

#include "Tile.h"

namespace OpenLoco::Map
{
#pragma pack(push, 1)
    struct tile_loop
    {
    private:
        Pos2 _pos;

    public:
        Pos2 current() const { return _pos; }
        Pos2 next()
        {
            _pos.x += tile_size;
            if (_pos.x >= map_width - 1)
            {
                _pos.x = 0;
                _pos.y += tile_size;
                if (_pos.y >= map_height - 1)
                {
                    _pos.y = 0;
                }
            }
            return _pos;
        }
    };
#pragma pack(pop)
}
