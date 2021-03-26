#include "MapGenerator.h"
#include "../Interop/Interop.hpp"
#include "../S5/S5.h"
#include "../Scenario.h"
#include "../Ui/ProgressBar.h"
#include "../Ui/WindowManager.h"
#include "Tile.h"
#include "TileLoop.hpp"
#include "TileManager.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <random>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;
using namespace OpenLoco::S5;
using namespace OpenLoco::Map::MapGenerator;

namespace OpenLoco::Map::MapGenerator
{
    static loco_global<uint8_t*, 0x00F00160> _heightMap;

    struct Point
    {
        int32_t x{};
        int32_t y{};
    };

    class HeightMap
    {
    private:
        std::unique_ptr<uint8_t[]> _height;

    public:
        int32_t const width;
        int32_t const height;
        int32_t const pitch;

        HeightMap(int32_t width, int32_t height, int32_t pitch, uint8_t baseHeight)
            : width(width)
            , height(height)
            , pitch(pitch)
        {
            _height = std::make_unique<uint8_t[]>(width * pitch);
        }

        HeightMap(const HeightMap& src)
            : width(src.width)
            , height(src.height)
            , pitch(src.pitch)
        {
            _height = std::make_unique<uint8_t[]>(width * pitch);
            std::memcpy(&_height[0], &src._height[0], width * pitch);
        }

        uint8_t& operator[](Point pos)
        {
            assert(pos.x >= 0 || pos.y >= 0 || pos.x < width || pos.y < height);
            return _height[pos.y * pitch + pos.x];
        }

        const uint8_t& operator[](Point pos) const
        {
            assert(pos.x >= 0 || pos.y >= 0 || pos.x < width || pos.y < height);
            return _height[pos.y * pitch + pos.x];
        }

        uint8_t* data()
        {
            return _height.get();
        }

        const uint8_t* data() const
        {
            return _height.get();
        }
    };

    class HeightMapRange
    {
    private:
        HeightMap& _heightMap;
        int32_t const _minX;
        int32_t const _minY;

    public:
        int32_t const width;
        int32_t const height;

        HeightMapRange(HeightMap& heightMap)
            : _heightMap(heightMap)
            , _minX(0)
            , _minY(0)
            , width(heightMap.width)
            , height(heightMap.height)
        {
        }

        HeightMapRange(HeightMap& heightMap, int32_t minX, int32_t minY, int32_t width, int32_t height)
            : _heightMap(heightMap)
            , _minX(minX)
            , _minY(minY)
            , width(width)
            , height(height)
        {
        }

        uint8_t& operator[](Point pos)
        {
            pos.x += _minX;
            pos.y += _minY;
            return _heightMap[pos];
        }

        const uint8_t& operator[](Point pos) const
        {
            pos.x += _minX;
            pos.y += _minY;
            return _heightMap[pos];
        }

        HeightMapRange slice(int32_t l, int32_t t, int32_t w, int32_t h)
        {
            return HeightMapRange(_heightMap, _minX + l, _minY + t, w, h);
        }
    };

    class OriginalTerrainGenerator
    {
    public:
        // 0x004624F0
        void generate([[maybe_unused]] const S5::Options& options, HeightMap& heightMap)
        {
            _heightMap = heightMap.data();
            call(0x004624F0);
            _heightMap = nullptr;
        }
    };

    class ModernTerrainGenerator
    {
    public:
        void generate(const S5::Options& options, HeightMapRange heightMap, uint32_t seed)
        {
            initialiseRng(seed);

            auto hillDensity = std::clamp<uint8_t>(options.hillDensity, 0, 100) / 100.0f;

            SimplexSettings settings;
            settings.low = options.minLandHeight;

            switch (options.topographyStyle)
            {
                case TopographyStyle::flatLand:
                    settings.high = options.minLandHeight + 8;
                    settings.baseFreq = 4.0f * hillDensity;
                    settings.octaves = 5;
                    settings.smooth = 2;
                    generate(settings, heightMap);
                    break;
                case TopographyStyle::smallHills:
                    settings.high = options.minLandHeight + 14;
                    settings.baseFreq = 6.0f * hillDensity;
                    settings.octaves = 6;
                    settings.smooth = 1;
                    generate(settings, heightMap);
                    break;
                case TopographyStyle::mountains:
                    settings.high = 32;
                    settings.baseFreq = 4.0f * hillDensity;
                    settings.octaves = 6;
                    settings.smooth = 1;
                    generate(settings, heightMap);
                    break;
                case TopographyStyle::halfMountainsHills:
                    settings.high = 32;
                    settings.baseFreq = 8.0f * hillDensity;
                    settings.octaves = 6;
                    settings.smooth = 2;
                    generate(settings, heightMap);
                    break;
                case TopographyStyle::halfMountainsFlat:
                    settings.high = 32;
                    settings.baseFreq = 6.0f * hillDensity;
                    settings.octaves = 5;
                    settings.smooth = 5;
                    generate(settings, heightMap);
                    break;
            }
        }

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

        void initialiseRng(uint32_t seed)
        {
            _pprng.seed(seed);
        }

        void generate(const SimplexSettings& settings, HeightMapRange heightMap)
        {
            generateSimplex(settings, heightMap);
            smooth(settings.smooth, heightMap);
        }

        void generateSimplex(const SimplexSettings& settings, HeightMapRange heightMap)
        {
            auto freq = settings.baseFreq * (1.0f / std::max(heightMap.width, heightMap.height));
            uint8_t perm[512];
            noise(perm, std::size(perm));
            for (int32_t y = 0; y < heightMap.height; y++)
            {
                for (int32_t x = 0; x < heightMap.width; x++)
                {
                    auto noiseValue = std::clamp(noiseFractal(perm, x, y, freq, settings.octaves, 2.0f, 0.65f), -1.0f, 1.0f);
                    auto normalisedNoiseValue = (noiseValue + 1.0f) / 2.0f;
                    auto height = settings.low + static_cast<int32_t>(normalisedNoiseValue * settings.high);
                    heightMap[{ x, y }] = height;
                }
            }
        }

        static void smooth(int32_t iterations, HeightMapRange heightMap)
        {
            for (int32_t i = 0; i < iterations; i++)
            {
                auto copyHeight = heightMap;
                for (int32_t y = 1; y < heightMap.width - 1; y++)
                {
                    for (int32_t x = 1; x < heightMap.height - 1; x++)
                    {
                        int32_t total = 0;
                        for (int32_t yy = -1; yy <= 1; yy++)
                        {
                            for (int32_t xx = -1; xx <= 1; xx++)
                            {
                                total += copyHeight[{ x + xx, y + yy }];
                            }
                        }
                        heightMap[{ x, y }] = total / 9;
                    }
                }
            }
        }

        static float noiseFractal(uint8_t* perm, int32_t x, int32_t y, float frequency, int32_t octaves, float lacunarity, float persistence)
        {
            float total = 0.0f;
            float amplitude = persistence;
            for (int32_t i = 0; i < octaves; i++)
            {
                total += generateNoise(perm, x * frequency, y * frequency) * amplitude;
                frequency *= lacunarity;
                amplitude *= persistence;
            }
            return total;
        }

        static float generateNoise(uint8_t* perm, float x, float y)
        {
            const float F2 = 0.366025403f; // F2 = 0.5*(sqrt(3.0)-1.0)
            const float G2 = 0.211324865f; // G2 = (3.0-sqrt(3.0))/6.0

            float n0, n1, n2; // Noise contributions from the three corners

            // Skew the input space to determine which simplex cell we're in
            float s = (x + y) * F2; // Hairy factor for 2D
            float xs = x + s;
            float ys = y + s;
            int32_t i = fastFloor(xs);
            int32_t j = fastFloor(ys);

            float t = static_cast<float>(i + j) * G2;
            float X0 = i - t; // Unskew the cell origin back to (x,y) space
            float Y0 = j - t;
            float x0 = x - X0; // The x,y distances from the cell origin
            float y0 = y - Y0;

            // For the 2D case, the simplex shape is an equilateral triangle.
            // Determine which simplex we are in.
            int32_t i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
            if (x0 > y0)
            {
                i1 = 1;
                j1 = 0;
            } // lower triangle, XY order: (0,0)->(1,0)->(1,1)
            else
            {
                i1 = 0;
                j1 = 1;
            } // upper triangle, YX order: (0,0)->(0,1)->(1,1)

            // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
            // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
            // c = (3-sqrt(3))/6

            float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
            float y1 = y0 - j1 + G2;
            float x2 = x0 - 1.0f + 2.0f * G2; // Offsets for last corner in (x,y) unskewed coords
            float y2 = y0 - 1.0f + 2.0f * G2;

            // Wrap the integer indices at 256, to avoid indexing perm[] out of bounds
            int32_t ii = i % 256;
            int32_t jj = j % 256;

            // Calculate the contribution from the three corners
            float t0 = 0.5f - x0 * x0 - y0 * y0;
            if (t0 < 0.0f)
            {
                n0 = 0.0f;
            }
            else
            {
                t0 *= t0;
                n0 = t0 * t0 * grad(perm[ii + perm[jj]], x0, y0);
            }

            float t1 = 0.5f - x1 * x1 - y1 * y1;
            if (t1 < 0.0f)
            {
                n1 = 0.0f;
            }
            else
            {
                t1 *= t1;
                n1 = t1 * t1 * grad(perm[ii + i1 + perm[jj + j1]], x1, y1);
            }

            float t2 = 0.5f - x2 * x2 - y2 * y2;
            if (t2 < 0.0f)
            {
                n2 = 0.0f;
            }
            else
            {
                t2 *= t2;
                n2 = t2 * t2 * grad(perm[ii + 1 + perm[jj + 1]], x2, y2);
            }

            // Add contributions from each corner to get the final noise value.
            // The result is scaled to return values in the interval [-1,1].
            return 40.0f * (n0 + n1 + n2); // TODO: The scale factor is preliminary!
        }

        void noise(uint8_t* perm, size_t len)
        {
            for (size_t i = 0; i < len; i++)
            {
                perm[i] = randomNext() & 0xFF;
            }
        }

        static float grad(int32_t hash, float x, float y)
        {
            int32_t h = hash & 7;    // Convert low 3 bits of hash code
            float u = h < 4 ? x : y; // into 8 simple gradient directions,
            float v = h < 4 ? y : x; // and compute the dot product with (x,y).
            return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -2.0f * v : 2.0f * v);
        }

        static int32_t fastFloor(float x)
        {
            return (x > 0) ? (static_cast<int32_t>(x)) : ((static_cast<int32_t>(x)) - 1);
        }

        uint32_t randomNext()
        {
            return _pprng();
        }

        int32_t randomNext(int32_t min, int32_t max)
        {
            return min + (randomNext() % (max - min));
        }
    };

    // 0x004624F0
    static void generateHeightMap(const S5::Options& options, HeightMap& heightMap)
    {
        if (options.generator == LandGeneratorType::Original)
        {
            OriginalTerrainGenerator generator;
            generator.generate(options, heightMap);
        }
        else
        {
            ModernTerrainGenerator generator;
            generator.generate(options, heightMap, std::random_device{}());
        }
    }

    // 0x004625D0
    static void generateLand(HeightMap& heightMap)
    {
        _heightMap = heightMap.data();
        call(0x004625D0);
        _heightMap = nullptr;
    }

    // 0x004C4BD7
    static void generateWater(HeightMap& heightMap)
    {
        static loco_global<uint16_t, 0x00525FB2> seaLevel;

        Map::tile_loop tileLoop;
        for (uint32_t posId = 0; posId < map_size; posId++)
        {
            auto pos = tileLoop.current();
            auto tile = TileManager::get(pos);
            auto* surface = tile.surface();

            if (surface != nullptr && surface->baseZ() < (seaLevel << 2))
                surface->setWater(seaLevel);

            tileLoop.next();
        }
    }

    // 0x0046A021
    static void generateTerrain(HeightMap& heightMap)
    {
        _heightMap = heightMap.data();
        call(0x0046A021);
        _heightMap = nullptr;
    }

    // 0x004BDA49
    static void generateTrees()
    {
        call(0x004BDA49);
    }

    // 0x00496BBC
    static void generateTowns(uint32_t minProgress, uint32_t maxProgress)
    {
        registers regs;
        regs.eax = minProgress;
        regs.ebx = maxProgress;
        call(0x00496BBC, regs);
    }

    // 0x004597FD
    static void generateIndustries(uint32_t minProgress, uint32_t maxProgress)
    {
        registers regs;
        regs.eax = minProgress;
        regs.ebx = maxProgress;
        call(0x004597FD, regs);
    }

    // 0x0042E6F2
    static void generateMiscBuildings()
    {
        call(0x0042E6F2);
    }

    static void miniMessageLoop()
    {
        call(0x004072EC);
    }

    static void updateProgress(uint8_t value)
    {
        miniMessageLoop();
        Ui::ProgressBar::setProgress(value);
    }

    // 0x0043C90C
    void generate(const S5::Options& options)
    {
        miniMessageLoop();

        WindowManager::close(WindowType::town);
        WindowManager::close(WindowType::industry);
        Ui::ProgressBar::begin(StringIds::generating_landscape);

        auto rotation = WindowManager::getCurrentRotation();
        Scenario::reset();
        WindowManager::setCurrentRotation(rotation);

        updateProgress(5);

        Scenario::initialiseDate(options.scenarioStartYear);
        call(0x00496A18);
        TileManager::initialise();
        updateProgress(10);

        {
            // Should be 384x384 (but generateLandscape goes out of bounds?)
            HeightMap heightMap(512, 512, 512, options.minLandHeight);

            generateHeightMap(options, heightMap);
            updateProgress(17);

            generateLand(heightMap);
            updateProgress(17);

            generateWater(heightMap);
            updateProgress(25);

            generateTerrain(heightMap);
            updateProgress(35);
        }

        call(0x004611DF);
        updateProgress(40);

        generateTrees();
        updateProgress(45);

        generateTowns(45, 225);
        updateProgress(225);

        generateIndustries(225, 245);
        updateProgress(245);

        generateMiscBuildings();
        updateProgress(250);

        call(0x004611DF);
        updateProgress(255);

        call(0x004969E0);
        call(0x004748D4);
        Ui::ProgressBar::end();
    }
}
