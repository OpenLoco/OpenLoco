#pragma once

#include "Objects/Object.h"
#include "S5/S5.h"
#include "S5/S5GameState.h"
#include "S5/S5TileElement.h"

namespace OpenLoco::S5
{
    struct S5File
    {
        Header header;
        std::unique_ptr<Options> scenarioOptions;
        std::unique_ptr<SaveDetails> saveDetails;
        ObjectHeader requiredObjects[859];
        GameState gameState;
        std::vector<TileElement> tileElements;
        std::vector<std::pair<ObjectHeader, std::vector<std::byte>>> packedObjects;
    };
}
