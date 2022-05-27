#include "Map.hpp"
#include <optional>

namespace OpenLoco::Map
{
    std::optional<uint8_t> getRandomTreeTypeFromSurface(const Map::TilePos2& loc, bool unk);
    bool placeTreeCluster(const Map::TilePos2& centreLoc, const uint16_t range, const uint16_t density, const std::optional<uint8_t> treeType);
}
