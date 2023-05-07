#pragma once

#include "HeightMapRange.h"

namespace OpenLoco::World::MapGenerator
{
    class PngTerrainGenerator
    {
    public:
        void generate(HeightMapRange heightMap);

    private:
        void openUiPngBrowser();
        int generateFromHeightmapPng(HeightMapRange heightMap);
    };
}
