#include "Map/TileLoop.hpp"

namespace OpenLoco::World
{
    TilePosRangeView getClampedRange(const TilePos2& posA, const TilePos2& posB)
    {
        auto clampedA = TilePos2(clampTileCoord(posA.x), clampTileCoord(posA.y));
        auto clampedB = TilePos2(clampTileCoord(posB.x), clampTileCoord(posB.y));
        return TilePosRangeView(clampedA, clampedB);
    }

    TilePosRangeView getClampedRange(const Pos2& posA, const Pos2& posB)
    {
        auto clampedA = Pos2(clampCoord(posA.x), clampCoord(posA.y));
        auto clampedB = Pos2(clampCoord(posB.x), clampCoord(posB.y));
        return TilePosRangeView(toTileSpace(clampedA), toTileSpace(clampedB));
    }

    TilePosRangeView getDrawableTileRange()
    {
        return TilePosRangeView({ 1, 1 }, { kMapColumns - 2, kMapRows - 2 });
    }

    TilePosRangeView getWorldRange()
    {
        return TilePosRangeView({ 0, 0 }, { kMapColumns - 1, kMapRows - 1 });
    }

    bool advanceTilePos(TilePos2& pos)
    {
        if (pos.x < kMapColumns - 1)
        {
            pos.x++;
            return true;
        }

        pos.x = 0;
        pos.y++;
        return pos.y < kMapRows;
    }
}
