#include "TileLoop.hpp"

namespace OpenLoco::World
{
    TilePosRangeView getClampedRange(const TilePos2& posA, const TilePos2& posB)
    {
        auto clampedA = TilePos2(TileManager::clampTileCoord(posA.x), TileManager::clampTileCoord(posA.y));
        auto clampedB = TilePos2(TileManager::clampTileCoord(posB.x), TileManager::clampTileCoord(posB.y));
        return TilePosRangeView(clampedA, clampedB);
    }

    TilePosRangeView getClampedRange(const Pos2& posA, const Pos2& posB)
    {
        auto clampedA = Pos2(TileManager::clampCoord(posA.x), TileManager::clampCoord(posA.y));
        auto clampedB = Pos2(TileManager::clampCoord(posB.x), TileManager::clampCoord(posB.y));
        return TilePosRangeView(World::toTileSpace(clampedA), World::toTileSpace(clampedB));
    }

    TilePosRangeView getDrawableTileRange()
    {
        return TilePosRangeView({ 1, 1 }, { TileManager::getMapColumns() - 2, TileManager::getMapRows() - 2 });
    }

    TilePosRangeView getWorldRange()
    {
        return TilePosRangeView({ 0, 0 }, { TileManager::getMapColumns() - 1, TileManager::getMapRows() - 1 });
    }
}
