#include "TileLoop.hpp"

namespace OpenLoco::World
{
    TilePosRangeView getClampedRange(const TilePos2& posA, const TilePos2& posB)
    {
        auto clampedA = World::TileManager::clampTileCoords(posA);
        auto clampedB = World::TileManager::clampTileCoords(posB);
        return TilePosRangeView(clampedA, clampedB);
    }

    TilePosRangeView getClampedRange(const Pos2& posA, const Pos2& posB)
    {
        auto clampedA = World::TileManager::clampCoords(posA);
        auto clampedB = World::TileManager::clampCoords(posB);
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
