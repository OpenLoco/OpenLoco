#pragma once

#include "HeightMapRange.h"
#include <cstdint>
#include <random>

namespace OpenLoco::S5
{
    struct Options;
}

namespace OpenLoco::World::MapGenerator
{
    class ModernTerrainGenerator
    {
    public:
        void generate(const S5::Options& options, HeightMapRange heightMap, uint32_t seed);

    private:
        struct SimplexSettings
        {
            int32_t low = 2;
            int32_t high = 24;
            float baseFreq = 1.25f;
            int32_t octaves = 4;
            int32_t smooth = 2;
        };

        std::mt19937 _pprng;

        void initialiseRng(uint32_t seed);

        void generate(const SimplexSettings& settings, HeightMapRange heightMap);

        void generateSimplex(const SimplexSettings& settings, HeightMapRange heightMap);

        static void smooth(int32_t iterations, HeightMapRange heightMap);

        static float noiseFractal(uint8_t* perm, int32_t x, int32_t y, float frequency, int32_t octaves, float lacunarity, float persistence);

        static float generateNoise(uint8_t* perm, float x, float y);

        void noise(uint8_t* perm, size_t len);

        static float grad(int32_t hash, float x, float y);

        static int32_t fastFloor(float x);

        uint32_t randomNext();

        int32_t randomNext(int32_t min, int32_t max);
    };
}
