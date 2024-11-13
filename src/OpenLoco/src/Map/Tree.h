#pragma once

#include <OpenLoco/Engine/World.hpp>
#include <optional>
#include <vector>
#include "GameCommands/Terraform/CreateTree.h"

namespace OpenLoco::World
{
    std::optional<uint8_t> getRandomTreeTypeFromSurface(const World::TilePos2& loc, bool unk);
    bool placeRandomTree(const World::Pos2& pos, std::optional<uint8_t> treeType);
    bool placeTreeCluster(const World::TilePos2& centreLoc, const uint16_t range, const uint16_t density, const std::optional<uint8_t> treeType);
    std::vector<GameCommands::TreePlacementArgs> placeTreeClusterPre(const World::TilePos2& centreLoc, const uint16_t range, const uint16_t density, const std::optional<uint8_t> treeType);
    bool placeTreeCluster(std::vector<GameCommands::TreePlacementArgs> argsVector);
}
