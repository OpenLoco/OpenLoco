#pragma once

#include "../Core/FileSystem.hpp"
#include "../Objects/ObjectManager.h"
#include <cstdint>
#include <memory>

namespace OpenLoco::S5
{
    enum class S5Type : uint8_t
    {
        savedGame = 0,
        scenario = 1,
        objects = 2,
        landscape = 3,
    };

    namespace S5Flags
    {
        constexpr uint8_t isRaw = 1 << 0;
        constexpr uint8_t isDump = 1 << 1;
        constexpr uint8_t isTitleSequence = 1 << 2;
        constexpr uint8_t hasSaveDetails = 1 << 3;
    }

#pragma pack(push, 1)
    struct Header
    {
        S5Type type;
        uint8_t flags;
        uint16_t numPackedObjects;
        uint32_t version;
        uint32_t magic;
        std::byte padding[20];
    };
#pragma pack(pop)
    static_assert(sizeof(Header) == 0x20);

    enum class TopographyStyle : uint8_t
    {
        flatLand,
        smallHills,
        mountains,
        halfMountainsHills,
        halfMountainsFlat,
    };

    enum class LandGeneratorType : uint8_t
    {
        Original,
        Improved,
    };

#pragma pack(push, 1)
    struct Options
    {
        uint8_t editorStep;                     // 0x00
        uint8_t difficulty;                     // 0x01
        uint16_t scenarioStartYear;             // 0x02
        uint8_t pad_4[2];                       // 0x04
        uint16_t scenarioFlags;                 // 0x06
        uint8_t madeAnyChanges;                 // 0x08
        uint8_t pad_9[1];                       // 0x09
        uint8_t landDistributionPatterns[32];   // 0x0A
        char scenarioName[64];                  // 0x2A
        char scenarioDetails[256];              // 0x6A
        ObjectHeader scenarioText;              // 0x16A
        uint16_t numberOfForests;               // 0x17a
        uint8_t minForestRadius;                // 0x17C
        uint8_t maxForestRadius;                // 0x17D
        uint8_t minForestDensity;               // 0x17E
        uint8_t maxForestDensity;               // 0x17F
        uint16_t numberRandomTrees;             // 0x180
        uint8_t minAltitudeForTrees;            // 0x182
        uint8_t maxAltitudeForTrees;            // 0x183
        uint8_t minLandHeight;                  // 0x184
        TopographyStyle topographyStyle;        // 0x185
        uint8_t hillDensity;                    // 0x186
        uint8_t numberOfTowns;                  // 0x187
        uint8_t maxTownSize;                    // 0x188
        uint8_t numberOfIndustries;             // 0x189
        uint8_t preview[128][128];              // 0x18A
        uint8_t maxCompetingCompanies;          // 0x418A
        uint8_t competitorStartDelay;           // 0x418B
        uint8_t objectiveType;                  // 0x418C
        uint8_t objectiveFlags;                 // 0x418D
        uint32_t objectiveCompanyValue;         // 0x418E
        uint32_t objectiveMonthlyVehicleProfit; // 0x4192
        uint8_t objectivePerformanceIndex;      // 0x4196
        uint8_t objectiveDeliveredCargoType;    // 0x4197
        uint32_t objectiveDeliveredCargoAmount; // 0x4198
        uint8_t objectiveTimeLimitYears;        // 0x419C
        ObjectHeader objectiveDeliveredCargo;   // 0x419D
        ObjectHeader currency;                  // 0x41AD

        // new fields:
        LandGeneratorType generator;

        std::byte pad_41BD[348];
    };
#pragma pack(pop)

    static_assert(sizeof(Options) == 0x431A);
    static_assert(0x009C8714 + offsetof(Options, editorStep) == 0x009C8714);
    static_assert(0x009C8714 + offsetof(Options, difficulty) == 0x009C8715);
    static_assert(0x009C8714 + offsetof(Options, scenarioStartYear) == 0x009C8716);
    static_assert(0x009C8714 + offsetof(Options, scenarioFlags) == 0x009C871A);
    static_assert(0x009C8714 + offsetof(Options, madeAnyChanges) == 0x009C871C);
    static_assert(0x009C8714 + offsetof(Options, landDistributionPatterns) == 0x009C871E);
    static_assert(0x009C8714 + offsetof(Options, numberOfForests) == 0x009C888E);
    static_assert(0x009C8714 + offsetof(Options, minForestRadius) == 0x009C8890);
    static_assert(0x009C8714 + offsetof(Options, maxForestRadius) == 0x009C8891);
    static_assert(0x009C8714 + offsetof(Options, minForestDensity) == 0x009C8892);
    static_assert(0x009C8714 + offsetof(Options, maxForestDensity) == 0x009C8893);
    static_assert(0x009C8714 + offsetof(Options, numberRandomTrees) == 0x009C8894);
    static_assert(0x009C8714 + offsetof(Options, minAltitudeForTrees) == 0x009C8896);
    static_assert(0x009C8714 + offsetof(Options, maxAltitudeForTrees) == 0x009C8897);
    static_assert(0x009C8714 + offsetof(Options, minLandHeight) == 0x009C8898);
    static_assert(0x009C8714 + offsetof(Options, topographyStyle) == 0x009C8899);
    static_assert(0x009C8714 + offsetof(Options, hillDensity) == 0x009C889A);
    static_assert(0x009C8714 + offsetof(Options, numberOfTowns) == 0x009C889B);
    static_assert(0x009C8714 + offsetof(Options, maxTownSize) == 0x009C889C);
    static_assert(0x009C8714 + offsetof(Options, numberOfIndustries) == 0x009C889D);
    static_assert(0x009C8714 + offsetof(Options, scenarioName) == 0x009C873E);
    static_assert(0x009C8714 + offsetof(Options, scenarioDetails) == 0x009C877E);

    static_assert(0x009CCA54 + offsetof(Options, scenarioFlags) == 0x009CCA5A);
    static_assert(0x009CCA54 + offsetof(Options, preview) == 0x009CCBDE);

#pragma pack(push, 1)
    struct SaveDetails
    {
        char company[256];                   // 0x000
        char owner[256];                     // 0x100
        uint32_t date;                       // 0x200
        uint16_t performance_index;          // 0x204 (from [company.performance_index)
        char scenario[0x40];                 // 0x206
        uint8_t challenge_progress;          // 0x246
        std::byte pad_247;                   // 0x247
        uint8_t image[250 * 200];            // 0x248
        uint32_t challenge_flags;            // 0xC598 (from [company.challenge_flags])
        std::byte pad_C59C[0xC618 - 0xC59C]; // 0xC59C
    };
#pragma pack(pop)
    static_assert(sizeof(SaveDetails) == 0xC618);

#pragma pack(push, 1)
    struct Company
    {
        uint16_t name;                 // 0x0000
        uint16_t ownerName;            // 0x0002
        uint32_t challengeFlags;       // 0x0004
        uint8_t cash[6];               // 0x0008
        uint32_t currentLoan;          // 0x000E
        uint32_t updateCounter;        // 0x0012
        int16_t performanceIndex;      // 0x0016
        uint8_t pad_18[0x8C4E - 0x18]; // 0x0018
        uint8_t challengeProgress;     // 0x8C4E
        uint8_t pad_8C4F[0x8FA8 - 0x8C4F];
    };
    static_assert(sizeof(Company) == 0x8FA8);

    struct Town
    {
        uint8_t pad_000[0x270];
    };

    struct Industry
    {
        uint8_t pad_000[0x453];
    };

    struct Station
    {
        uint8_t pad_000[0x3D2];
    };

    struct Entity
    {
        uint8_t pad_00[0x80];
    };

    struct Animation
    {
        uint8_t pad_0[0x6];
    };

    struct Wave
    {
        uint8_t pad_0[0x6];
    };

    struct TileElement
    {
    private:
        static constexpr uint8_t FLAG_GHOST = 1 << 4;
        static constexpr uint8_t FLAG_LAST = 1 << 7;

    public:
        uint8_t type;
        uint8_t flags;
        uint8_t baseZ;
        uint8_t clearZ;
        uint8_t pad_4[4];

        void setLast(bool value)
        {
            if (value)
                flags |= FLAG_LAST;
            else
                flags &= ~FLAG_LAST;
        }

        constexpr bool isGhost() const
        {
            return flags & FLAG_GHOST;
        }

        constexpr bool isLast() const
        {
            return flags & FLAG_LAST;
        }
    };
    static_assert(sizeof(TileElement) == 8);

    namespace S5FixFlags
    {
        constexpr uint16_t fixFlag0 = 1 << 0;
        constexpr uint16_t fixFlag1 = 1 << 1;
    }

    struct GameState
    {
        uint32_t rng[2];                  // 0x000000 (0x00525E18)
        uint32_t pad_0008[3];             // 0x000008
        uint32_t currentDay;              // 0x000014 (0x00525E2C)
        uint16_t dayCounter;              // 0x000018
        uint16_t currentYear;             // 0x00001A
        uint8_t currentMonth;             // 0x00001C
        uint8_t currentDayOfMonth;        // 0x00001D
        int16_t savedViewX;               // 0x00001E
        int16_t savedViewY;               // 0x000020
        uint8_t savedViewZoom;            // 0x000022
        uint8_t savedViewRotation;        // 0x000023
        uint8_t playerCompanyId;          // 0x000024
        uint8_t pad_0025[0x146 - 0x25];   // 0x000025
        uint32_t scenarioTicks;           // 0x000146 (0x00525F5E)
        uint16_t pad_014A;                // 0x00014A (0x00525F62)
        uint32_t scenarioTicks2;          // 0x00014C (0x00525F64)
        uint32_t magicNumber;             // 0x000150 (0x00525F68)
        uint16_t numMapAnimations;        // 0x000154 (0x00525F6C)
        uint8_t pad_0156[0x02BC - 0x156]; // 0x000156
        char scenarioName[64];            // 0x0002BC (0x005260D4)
        char scenarioDetails[256];        // 0x0002FC (0x00526114)
        uint8_t pad_03FC[0x434 - 0x3FC];  // 0x0003FC
        uint16_t fixFlags;                // 0x000434 (0x0052624C)
        uint8_t pad_0436[0xB96C - 0x436]; // 0x0003FC
        Company companies[15];            // 0x00B96C (0x00531784)
        Town towns[80];                   // 0x092444 (0x005B825C)
        Industry industries[128];         // 0x09E744 (0x005C455C)
        Station stations[1024];           // 0x0C10C4 (0x005E6EDC)
        Entity entities[20000];           // 0x1B58C4 (0x006DB6DC)
        Animation animations[8192];       // 0x4268C4 (0x0094C6DC)
        Wave waves[64];                   // 0x4328C4 (0x009586DC)
        uint8_t userStrings[2048][32];    // 0x432A44 (0x0095885C)
        uint16_t routings[1000][64];      // 0x442A44 (0x0096885C)
        uint8_t orders[256000];           // 0x461E44 (0x0096885C)
    };
#pragma pack(pop)
    static_assert(sizeof(GameState) == 0x4A0644);

    struct S5File
    {
        Header header;
        std::unique_ptr<Options> landscapeOptions;
        std::unique_ptr<SaveDetails> saveDetails;
        ObjectHeader requiredObjects[859];
        GameState gameState;
        std::vector<TileElement> tileElements;
    };

    namespace LoadFlags
    {
        constexpr uint32_t titleSequence = 1 << 0;
        constexpr uint32_t twoPlayer = 1 << 1;
    }

    enum SaveFlags : uint32_t
    {
        packCustomObjects = 1 << 0,
        scenario = 1 << 1,
        landscape = 1 << 2,
        noWindowClose = 1u << 29,
        raw = 1u << 30,  // Save raw data including pointers with no clean up
        dump = 1u << 31, // Used for dumping the game state when there is a fatal error
    };

    constexpr const char* extensionSC5 = ".SC5";
    constexpr const char* extensionSV5 = ".SV5";

    constexpr const char* filterSC5 = "*.SC5";
    constexpr const char* filterSV5 = "*.SV5";

    Options& getOptions();
    Options& getPreviewOptions();
    bool save(const fs::path& path, SaveFlags flags);
    void registerHooks();

    bool load(const fs::path& path, uint32_t flags);
}
