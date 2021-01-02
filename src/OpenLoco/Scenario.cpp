#include "Scenario.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Map/TileManager.h"
#include "Objects/CargoObject.h"
#include "ProgressBar.h"
#include "S5/S5.h"
#include "Ui/WindowManager.h"
#include <random>

#pragma warning(disable : 4505)

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;

namespace OpenLoco::Scenario
{
    static loco_global<cargo_object*, 0x0050D15C> _50D15C;
    static loco_global<uint8_t, 0x00526230> objectiveType;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint32_t, 0x00526232> objectiveCompanyValue;
    static loco_global<uint32_t, 0x00526236> objectiveMonthlyVehicleProfit;
    static loco_global<uint8_t, 0x0052623A> objectivePerformanceIndex;
    static loco_global<uint8_t, 0x0052623B> objectiveDeliveredCargoType;
    static loco_global<uint32_t, 0x0052623C> objectiveDeliveredCargoAmount;
    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526241> objectiveTimeLimitUntilYear;
    static loco_global<uint8_t*, 0x00F00160> _heightMap;

    // 0x0043C88C
    void reset()
    {
        call(0x0043C88C);
    }

    // 0x004748D4
    // ?Set default types for building. Like the initial track type and vehicle type and such.?
    void sub_4748D4()
    {
        call(0x004748D4);
    }

    // 0x0043EDAD
    void eraseLandscape()
    {
        S5::getOptions().scenarioFlags &= ~(Scenario::flags::landscape_generation_done);
        Ui::WindowManager::invalidate(Ui::WindowType::landscapeGeneration, 0);
        call(0x0043C88C);
        S5::getOptions().madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
        Gfx::invalidateScreen();
    }

    static void miniMessageLoop()
    {
        call(0x004072EC);
    }

    static void initialiseDate(uint16_t year)
    {
        registers regs;
        regs.ax = year;
        call(0x0049685C, regs);
    }

    // 0x004624DD
    static void allocateHeightMap()
    {
        _heightMap = new (std::nothrow) uint8_t[512 * 512];
    }

    // 0x004626A8
    static void freeHeightMap()
    {
        delete[] _heightMap;
        _heightMap = nullptr;
    }

    struct SimplexSettings
    {
        int32_t mapSize = 512;
        int32_t low = 2;
        int32_t high = 24;
        float baseFreq = 1.25f;
        int32_t octaves = 4;
        int32_t smooth = 2;
    };

    static uint32_t randomNext()
    {
        thread_local std::mt19937 prng(std::random_device{}());
        return prng();
    }

    static int32_t randomNext(int32_t min, int32_t max)
    {
        return min + (randomNext() % (max - min));
    }

    static void noise(uint8_t* perm, size_t len)
    {
        for (size_t i = 0; i < len; i++)
        {
            perm[i] = randomNext() & 0xFF;
        }
    }

    static int32_t fast_floor(float x)
    {
        return (x > 0) ? (static_cast<int32_t>(x)) : ((static_cast<int32_t>(x)) - 1);
    }

    static float grad(int32_t hash, float x, float y)
    {
        int32_t h = hash & 7;    // Convert low 3 bits of hash code
        float u = h < 4 ? x : y; // into 8 simple gradient directions,
        float v = h < 4 ? y : x; // and compute the dot product with (x,y).
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -2.0f * v : 2.0f * v);
    }

    static float generate(uint8_t* perm, float x, float y)
    {
        const float F2 = 0.366025403f; // F2 = 0.5*(sqrt(3.0)-1.0)
        const float G2 = 0.211324865f; // G2 = (3.0-sqrt(3.0))/6.0

        float n0, n1, n2; // Noise contributions from the three corners

        // Skew the input space to determine which simplex cell we're in
        float s = (x + y) * F2; // Hairy factor for 2D
        float xs = x + s;
        float ys = y + s;
        int32_t i = fast_floor(xs);
        int32_t j = fast_floor(ys);

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

    static float noiseFractal(uint8_t* perm, int32_t x, int32_t y, float frequency, int32_t octaves, float lacunarity, float persistence)
    {
        float total = 0.0f;
        float amplitude = persistence;
        for (int32_t i = 0; i < octaves; i++)
        {
            total += generate(perm, x * frequency, y * frequency) * amplitude;
            frequency *= lacunarity;
            amplitude *= persistence;
        }
        return total;
    }

    static void generateSimplex(const SimplexSettings& settings, uint8_t* heightMap)
    {
        auto freq = settings.baseFreq * (1.0f / settings.mapSize);
        uint8_t perm[512];
        noise(perm, std::size(perm));
        for (int32_t y = 0; y < settings.mapSize; y++)
        {
            for (int32_t x = 0; x < settings.mapSize; x++)
            {
                auto noiseValue = std::clamp(noiseFractal(perm, x, y, freq, settings.octaves, 2.0f, 0.65f), -1.0f, 1.0f);
                auto normalisedNoiseValue = (noiseValue + 1.0f) / 2.0f;
                auto height = settings.low + static_cast<int32_t>(normalisedNoiseValue * settings.high);
                heightMap[(y * settings.mapSize) + x] = height;
            }
        }
    }

    static void smooth(const SimplexSettings& settings, uint8_t* heightMap)
    {
        auto arraySize = settings.mapSize * settings.mapSize;
        auto copyHeight = std::make_unique<uint8_t[]>(arraySize);
        for (int32_t i = 0; i < settings.smooth; i++)
        {
            std::memcpy(&copyHeight[0], heightMap, arraySize);
            for (int32_t y = 1; y < settings.mapSize - 1; y++)
            {
                for (int32_t x = 1; x < settings.mapSize - 1; x++)
                {
                    int32_t total = 0;
                    for (int32_t yy = -1; yy <= 1; yy++)
                    {
                        for (int32_t xx = -1; xx <= 1; xx++)
                        {
                            total += copyHeight[(y + yy) * settings.mapSize + (x + xx)];
                        }
                    }
                    heightMap[(y * settings.mapSize) + x] = total / 9;
                }
            }
        }
    }

    // 0x004624F0
    static void generateHeightMap(const S5::Options& options)
    {
        SimplexSettings settings;
        settings.mapSize = 512;
        settings.low = options.minLandHeight;
        settings.high = randomNext(8, 32);
        settings.baseFreq = 1.25f;
        settings.octaves = 5;
        settings.smooth = 2;
        generateSimplex(settings, _heightMap);
        smooth(settings, _heightMap);

        // call(0x004624F0);
    }

    // 0x004625D0
    static void generateLand()
    {
        call(0x004625D0);
    }

    // 0x004C4BD7
    static void generateWater()
    {
        call(0x004C4BD7);
    }

    // 0x0046A021
    static void generateTerrain()
    {
        call(0x0046A021);
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

    // 0x0043C90C
    void generateLandscape()
    {
        miniMessageLoop();

        WindowManager::close(WindowType::town);
        WindowManager::close(WindowType::industry);
        ProgressBar::begin(StringIds::generating_landscape, 0);

        auto rotation = WindowManager::getCurrentRotation();
        reset();
        WindowManager::setCurrentRotation(rotation);

        miniMessageLoop();
        ProgressBar::setProgress(5);

        auto& options = S5::getOptions();

        initialiseDate(options.scenarioStartYear);
        call(0x00496A18);
        TileManager::initialise();
        miniMessageLoop();
        ProgressBar::setProgress(10);

        allocateHeightMap();
        generateHeightMap(options);
        miniMessageLoop();
        ProgressBar::setProgress(17);

        generateLand();
        miniMessageLoop();
        ProgressBar::setProgress(17);

        generateWater();
        miniMessageLoop();
        ProgressBar::setProgress(25);

        generateTerrain();
        miniMessageLoop();
        ProgressBar::setProgress(35);

        freeHeightMap();
        call(0x004611DF);
        miniMessageLoop();
        ProgressBar::setProgress(40);

        generateTrees();
        miniMessageLoop();
        ProgressBar::setProgress(45);

        generateTowns(45, 225);
        miniMessageLoop();
        ProgressBar::setProgress(225);

        generateIndustries(225, 245);
        miniMessageLoop();
        ProgressBar::setProgress(245);

        generateMiscBuildings();
        miniMessageLoop();
        ProgressBar::setProgress(250);

        call(0x004611DF);
        miniMessageLoop();
        ProgressBar::setProgress(255);

        call(0x004969E0);
        call(0x004748D4);
        ProgressBar::end();

        options.madeAnyChanges = 0;
        addr<0x00F25374, uint8_t>() = 0;
    }

    // 0x0044400C
    void start(const char* filename)
    {
        if (filename == nullptr)
            filename = reinterpret_cast<const char*>(-1);

        registers regs;
        regs.ebx = reinterpret_cast<int32_t>(filename);
        call(0x0044400C, regs);
    }

    // this will prepare _commonFormatArgs array before drawing the StringIds::challenge_value
    // after that for example it will draw this string: Achieve a performance index of 10.0% ("Engineer")
    // Note: no input and output parameters are in the original assembly, update is done in the memory
    // in this implementation we return FormatArguments so in the future it will be not depending on global variables
    // 0x004384E9
    void formatChallengeArguments(FormatArguments& args)
    {
        switch (objectiveType)
        {
            case Scenario::objective_type::company_value:
                args.push(StringIds::achieve_a_company_value_of);
                args.push(*objectiveCompanyValue);
                break;

            case Scenario::objective_type::vehicle_profit:
                args.push(StringIds::achieve_a_monthly_profit_from_vehicles_of);
                args.push(*objectiveMonthlyVehicleProfit);
                break;

            case Scenario::objective_type::performance_index:
            {
                args.push(StringIds::achieve_a_performance_index_of);
                int16_t performanceIndex = objectivePerformanceIndex * 10;
                formatPerformanceIndex(performanceIndex, args);
                break;
            }

            case Scenario::objective_type::cargo_delivery:
            {
                args.push(StringIds::deliver);
                cargo_object* cargoObject = _50D15C;
                if (objectiveDeliveredCargoType != 0xFF)
                {
                    cargoObject = ObjectManager::get<cargo_object>(objectiveDeliveredCargoType);
                }
                args.push(cargoObject->unit_name_plural);
                args.push(*objectiveDeliveredCargoAmount);
                break;
            }
        }

        if ((objectiveFlags & Scenario::objective_flags::be_top_company) != 0)
        {
            args.push(StringIds::and_be_the_top_performing_company);
        }
        if ((objectiveFlags & Scenario::objective_flags::be_within_top_three_companies) != 0)
        {
            args.push(StringIds::and_be_one_of_the_top_3_performing_companies);
        }
        if ((objectiveFlags & Scenario::objective_flags::within_time_limit) != 0)
        {
            if (isTitleMode() || isEditorMode())
            {
                args.push(StringIds::within_years);
                args.push<uint16_t>(*objectiveTimeLimitYears);
            }
            else
            {
                args.push(StringIds::by_the_end_of);
                args.push(*objectiveTimeLimitUntilYear);
            }
        }

        args.push<uint16_t>(0);
        args.push<uint16_t>(0);
    }
}
