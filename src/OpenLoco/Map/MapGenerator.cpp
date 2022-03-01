#include "MapGenerator.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../S5/S5.h"
#include "../Scenario.h"
#include "../Ui/ProgressBar.h"
#include "../Ui/WindowManager.h"
#include "Tile.h"
#include "TileLoop.hpp"
#include "TileManager.h"
#include <cassert>
#include <cstdint>
#include <random>
#include <vector>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;
using namespace OpenLoco::S5;
using namespace OpenLoco::Map::MapGenerator;

namespace OpenLoco::Map::MapGenerator
{
    static loco_global<uint8_t*, 0x00F00160> _heightMap;
    static loco_global<uint8_t, 0x00525FB6> _primaryLandObjectIndex;

    struct Point
    {
        int32_t x{};
        int32_t y{};
    };

    class HeightMap
    {
    private:
        std::vector<uint8_t> _height;

    public:
        int32_t const width;
        int32_t const height;
        int32_t const pitch;

        HeightMap(int32_t width, int32_t height, int32_t pitch, uint8_t baseHeight)
            : _height(width * pitch)
            , width(width)
            , height(height)
            , pitch(pitch)
        {
        }

        HeightMap(const HeightMap& src)
            : _height(src._height)
            , width(src.width)
            , height(src.height)
            , pitch(src.pitch)
        {
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
            return _height.data();
        }

        const uint8_t* data() const
        {
            return _height.data();
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

        Map::TileLoop tileLoop;
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

    static std::optional<uint8_t> getEverywhereSurfaceStyle()
    {
        for (uint8_t landObjectIdx = 0; landObjectIdx < ObjectManager::getMaxObjects(ObjectType::land); ++landObjectIdx)
        {
            auto* landObj = ObjectManager::get<LandObject>(landObjectIdx);
            if (landObj == nullptr)
            {
                continue;
            }
            if (S5::getOptions().landDistributionPatterns[landObjectIdx] == LandDistributionPattern::everywhere)
            {
                return landObjectIdx;
            }
        }
        if (*_primaryLandObjectIndex != 0xFF)
        {
            return *_primaryLandObjectIndex;
        }
        return std::nullopt;
    }

    // 0x00469FC8
    std::optional<uint8_t> getRandomTerrainVariation(const SurfaceElement& surface)
    {
        if (surface.water())
        {
            return std::nullopt;
        }
        if (surface.hasHighTypeFlag())
        {
            return std::nullopt;
        }

        auto* landObj = ObjectManager::get<LandObject>(surface.terrain());
        if (landObj == nullptr)
        {
            return std::nullopt;
        }

        if (landObj->numVariations == 0 || surface.slope())
        {
            return 0;
        }

        // TODO: split into two randNext calls
        uint16_t randVal = gPrng().randNext();
        if (landObj->variationLikelihood <= (randVal >> 8))
        {
            return 0;
        }
        return ((randVal & 0xFF) * landObj->numVariations) >> 8;
    }

    // 0x0046A379
    static void generateTerrainFarFromWater(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A379, regs);
        _heightMap = nullptr;
    }

    // 0x0046A439
    static void generateTerrainNearWater(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A439, regs);
        _heightMap = nullptr;
    }

    // 0x0046A5B3
    static void generateTerrainOnMountains(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A5B3, regs);
        _heightMap = nullptr;
    }

    // 0x0046A4F9
    static void generateTerrainFarFromMountains(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A4F9, regs);
        _heightMap = nullptr;
    }

    // 0x0046A0D8
    static void generateTerrainInSmallRandomAreas(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A0D8, regs);
        _heightMap = nullptr;
    }

    // 0x0046A227
    static void generateTerrainInLargeRandomAreas(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A227, regs);
        _heightMap = nullptr;
    }

    // 0x0046A66D
    static void generateTerrainAroundCliffs(HeightMap& heightMap, uint8_t surfaceStyle)
    {
        _heightMap = heightMap.data();
        registers regs;
        regs.ebx = surfaceStyle;
        call(0x0046A66D, regs);
        _heightMap = nullptr;
    }

    static void generateTerrainNull(HeightMap& heightMap, uint8_t surfaceStyle) {}

    using GenerateTerrainFunc = void (*)(HeightMap&, uint8_t);
    static const GenerateTerrainFunc _generateFuncs[] = {
        generateTerrainNull,               // LandDistributionPattern::everywhere This is null as it is a special function performed separately
        generateTerrainNull,               // LandDistributionPattern::nowhere
        generateTerrainFarFromWater,       // LandDistributionPattern::farFromWater
        generateTerrainNearWater,          // LandDistributionPattern::nearWater
        generateTerrainOnMountains,        // LandDistributionPattern::onMountains
        generateTerrainFarFromMountains,   // LandDistributionPattern::farFromMountains
        generateTerrainInSmallRandomAreas, // LandDistributionPattern::inSmallRandomAreas
        generateTerrainInLargeRandomAreas, // LandDistributionPattern::inLargeRandomAreas
        generateTerrainAroundCliffs        // LandDistributionPattern::aroundCliffs
    };

    // 0x0046A021
    static void generateTerrain(HeightMap& heightMap)
    {
        const auto style = getEverywhereSurfaceStyle();
        if (!style.has_value())
        {
            return;
        }

        TilePosRangeView tileLoop{ { 1, 1 }, { map_columns - 1, map_rows - 1 } };
        for (const auto& tilePos : tileLoop)
        {
            auto* surface = Map::TileManager::get(tilePos).surface();
            if (surface == nullptr)
            {
                continue;
            }
            surface->setTerrain(*style);
            surface->setVar6SLR5(0);

            const auto variation = getRandomTerrainVariation(*surface);
            if (variation.has_value())
            {
                surface->setVariation(*variation);
            }
        }

        for (const auto pattern : {
                 LandDistributionPattern::farFromWater,
                 LandDistributionPattern::nearWater,
                 LandDistributionPattern::onMountains,
                 LandDistributionPattern::farFromMountains,
                 LandDistributionPattern::inSmallRandomAreas,
                 LandDistributionPattern::inLargeRandomAreas,
                 LandDistributionPattern::aroundCliffs,
             })
        {
            for (uint8_t landObjectIdx = 0; landObjectIdx < ObjectManager::getMaxObjects(ObjectType::land); ++landObjectIdx)
            {
                const auto* landObj = ObjectManager::get<LandObject>(landObjectIdx);
                if (landObj == nullptr)
                {
                    continue;
                }
                const auto typePattern = S5::getOptions().landDistributionPatterns[landObjectIdx];
                if (typePattern != pattern)
                {
                    continue;
                }
                _generateFuncs[enumValue(pattern)](heightMap, landObjectIdx);
            }
        }
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

    static void updateProgress(uint8_t value)
    {
        Ui::processMessagesMini();
        Ui::ProgressBar::setProgress(value);
    }

    // 0x0043C90C
    void generate(const S5::Options& options)
    {
        Ui::processMessagesMini();

        WindowManager::close(WindowType::town);
        WindowManager::close(WindowType::industry);
        Ui::ProgressBar::begin(StringIds::generating_landscape);

        auto rotation = WindowManager::getCurrentRotation();
        Scenario::reset();
        WindowManager::setCurrentRotation(rotation);

        updateProgress(5);

        Scenario::initialiseDate(options.scenarioStartYear);
        Scenario::initialiseSnowLine();
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
        Scenario::sub_4748D4();
        Ui::ProgressBar::end();
    }
}
