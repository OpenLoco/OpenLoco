#include "TileLoop.hpp"

namespace OpenLoco::World
{
    TilePosRangeView getClampedRange(const TilePos2& posA, const TilePos2& posB)
    {
        auto clampedA = TilePos2(World::TileManager::clampTileCoord(posA.x), World::TileManager::clampTileCoord(posA.y));
        auto clampedB = TilePos2(World::TileManager::clampTileCoord(posB.x), World::TileManager::clampTileCoord(posB.y));
        return TilePosRangeView(clampedA, clampedB);
    }

    TilePosRangeView getClampedRange(const Pos2& posA, const Pos2& posB)
    {
        auto clampedA = Pos2(World::TileManager::clampCoord(posA.x), World::TileManager::clampCoord(posA.y));
        auto clampedB = Pos2(World::TileManager::clampCoord(posB.x), World::TileManager::clampCoord(posB.y));
        return TilePosRangeView(toTileSpace(clampedA), toTileSpace(clampedB));
    }

    TilePosRangeView getDrawableTileRange()
    {
        return TilePosRangeView({ 1, 1 }, { World::TileManager::getMapColumns() - 2, World::TileManager::getMapRows() - 2 });
    }

    TilePosRangeView getWorldRange()
    {
        return TilePosRangeView({ 0, 0 }, { World::TileManager::getMapColumns() - 1, World::TileManager::getMapRows() - 1 });
    }
}
