#pragma once

#include "../Core/FileSystem.hpp"
#include "../EditorController.h"
#include "../Objects/Object.h"
#include "Limits.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace OpenLoco
{
    class Stream;
}

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

    enum class LandDistributionPattern : uint8_t
    {
        everywhere,
        nowhere,
        farFromWater,
        nearWater,
        onMountains,
        farFromMountains,
        inSmallRandomAreas,
        inLargeRandomAreas,
        aroundCliffs,
    };

#pragma pack(push, 1)
    struct Options
    {
        EditorController::Step editorStep;                    // 0x00
        uint8_t difficulty;                                   // 0x01
        uint16_t scenarioStartYear;                           // 0x02
        uint8_t pad_4[2];                                     // 0x04
        uint16_t scenarioFlags;                               // 0x06
        uint8_t madeAnyChanges;                               // 0x08
        uint8_t pad_9[1];                                     // 0x09
        LandDistributionPattern landDistributionPatterns[32]; // 0x0A
        char scenarioName[64];                                // 0x2A
        char scenarioDetails[256];                            // 0x6A
        ObjectHeader scenarioText;                            // 0x16A
        uint16_t numberOfForests;                             // 0x17a
        uint8_t minForestRadius;                              // 0x17C
        uint8_t maxForestRadius;                              // 0x17D
        uint8_t minForestDensity;                             // 0x17E
        uint8_t maxForestDensity;                             // 0x17F
        uint16_t numberRandomTrees;                           // 0x180
        uint8_t minAltitudeForTrees;                          // 0x182
        uint8_t maxAltitudeForTrees;                          // 0x183
        uint8_t minLandHeight;                                // 0x184
        TopographyStyle topographyStyle;                      // 0x185
        uint8_t hillDensity;                                  // 0x186
        uint8_t numberOfTowns;                                // 0x187
        uint8_t maxTownSize;                                  // 0x188
        uint8_t numberOfIndustries;                           // 0x189
        uint8_t preview[128][128];                            // 0x18A
        uint8_t maxCompetingCompanies;                        // 0x418A
        uint8_t competitorStartDelay;                         // 0x418B
        uint8_t objectiveType;                                // 0x418C
        uint8_t objectiveFlags;                               // 0x418D
        uint32_t objectiveCompanyValue;                       // 0x418E
        uint32_t objectiveMonthlyVehicleProfit;               // 0x4192
        uint8_t objectivePerformanceIndex;                    // 0x4196
        uint8_t objectiveDeliveredCargoType;                  // 0x4197
        uint32_t objectiveDeliveredCargoAmount;               // 0x4198
        uint8_t objectiveTimeLimitYears;                      // 0x419C
        ObjectHeader objectiveDeliveredCargo;                 // 0x419D
        ObjectHeader currency;                                // 0x41AD

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
    static_assert(0x009C8714 + offsetof(Options, objectiveType) == 0x009CC8A0);
    static_assert(0x009C8714 + offsetof(Options, objectiveFlags) == 0x009CC8A1);
    static_assert(0x009C8714 + offsetof(Options, objectiveCompanyValue) == 0x009CC8A2);
    static_assert(0x009C8714 + offsetof(Options, objectiveMonthlyVehicleProfit) == 0x009CC8A6);
    static_assert(0x009C8714 + offsetof(Options, objectivePerformanceIndex) == 0x009CC8AA);
    static_assert(0x009C8714 + offsetof(Options, objectiveDeliveredCargoType) == 0x009CC8AB);
    static_assert(0x009C8714 + offsetof(Options, objectiveDeliveredCargoAmount) == 0x009CC8AC);
    static_assert(0x009C8714 + offsetof(Options, objectiveTimeLimitYears) == 0x009CC8B0);
    static_assert(0x009C8714 + offsetof(Options, objectiveDeliveredCargo) == 0x009CC8B1);
    static_assert(0x009C8714 + offsetof(Options, currency) == 0x009CC8C1);

    static_assert(0x009CCA54 + offsetof(Options, scenarioFlags) == 0x009CCA5A);
    static_assert(0x009CCA54 + offsetof(Options, preview) == 0x009CCBDE);

#pragma pack(push, 1)
    struct SaveDetails
    {
        char company[256];                   // 0x000
        char owner[256];                     // 0x100
        uint32_t date;                       // 0x200
        uint16_t performanceIndex;           // 0x204 (from [company.performance_index)
        char scenario[0x40];                 // 0x206
        uint8_t challenge_progress;          // 0x246
        std::byte pad_247;                   // 0x247
        uint8_t image[250 * 200];            // 0x248
        uint32_t challengeFlags;             // 0xC598 (from [company.challenge_flags])
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

    struct Message
    {
        uint8_t pad_0[0xD4];
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
        uint32_t rng[2];                                                                 // 0x000000 (0x00525E18)
        uint32_t unkRng[2];                                                              // 0x000008 (0x00525E20)
        uint32_t flags;                                                                  // 0x000010 (0x00525E28)
        uint32_t currentDay;                                                             // 0x000014 (0x00525E2C)
        uint16_t dayCounter;                                                             // 0x000018
        uint16_t currentYear;                                                            // 0x00001A
        uint8_t currentMonth;                                                            // 0x00001C
        uint8_t currentDayOfMonth;                                                       // 0x00001D
        int16_t savedViewX;                                                              // 0x00001E
        int16_t savedViewY;                                                              // 0x000020
        uint8_t savedViewZoom;                                                           // 0x000022
        uint8_t savedViewRotation;                                                       // 0x000023
        uint8_t playerCompanies[2];                                                      // 0x000024 (0x00525E3C)
        uint16_t entityListHeads[Limits::kNumEntityLists];                               // 0x000026 (0x00525E3E)
        uint16_t entityListCounts[Limits::kNumEntityLists];                              // 0x000034 (0x00525E4C)
        uint8_t pad_0042[0x046 - 0x042];                                                 // 0x000042
        uint32_t currencyMultiplicationFactor[32];                                       // 0x000046 (0x00525E5E)
        uint32_t unusedCurrencyMultiplicationFactor[32];                                 // 0x0000C6 (0x00525EDE)
        uint32_t scenarioTicks;                                                          // 0x000146 (0x00525F5E)
        uint16_t var_014A;                                                               // 0x00014A (0x00525F62)
        uint32_t scenarioTicks2;                                                         // 0x00014C (0x00525F64)
        uint32_t magicNumber;                                                            // 0x000150 (0x00525F68)
        uint16_t numMapAnimations;                                                       // 0x000154 (0x00525F6C)
        int16_t tileUpdateStartLocation[2];                                              // 0x000156 (0x00525F6E)
        uint8_t scenarioSignals[8];                                                      // 0x00015A (0x00525F72)
        uint8_t scenarioBridges[8];                                                      // 0x000162 (0x00525F7A)
        uint8_t scenarioTrainStations[8];                                                // 0x00016A (0x00525F82)
        uint8_t scenarioTrackMods[8];                                                    // 0x000172 (0x00525F8A)
        uint8_t var_17A[8];                                                              // 0x00017A (0x00525F92)
        uint8_t scenarioRoadStations[8];                                                 // 0x000182 (0x00525F9A)
        uint8_t scenarioRoadMods[8];                                                     // 0x00018A (0x00525FA2)
        uint8_t lastRailroadOption;                                                      // 0x000192 (0x00525FAA)
        uint8_t lastRoadOption;                                                          // 0x000193 (0x00525FAB)
        uint8_t lastAirport;                                                             // 0x000194 (0x00525FAC)
        uint8_t lastShipPort;                                                            // 0x000195 (0x00525FAD)
        bool trafficHandedness;                                                          // 0x000196 (0x00525FAE)
        uint8_t lastVehicleType;                                                         // 0x000197 (0x00525FAF)
        uint8_t pickupDirection;                                                         // 0x000198 (0x00525FB0)
        uint8_t lastTreeOption;                                                          // 0x000199 (0x00525FB1)
        uint16_t seaLevel;                                                               // 0x00019A (0x00525FB2)
        uint8_t currentSnowLine;                                                         // 0x00019C (0x00525FB4)
        uint8_t currentSeason;                                                           // 0x00019D (0x00525FB5)
        uint8_t lastLandOption;                                                          // 0x00019E (0x00525FB6)
        uint8_t maxCompetingCompanies;                                                   // 0x00019F (0x00525FB7)
        uint32_t numOrders;                                                              // 0x0001A0 (0x00525FB8)
        uint32_t var_1A4;                                                                // 0x0001A4 (0x00525FBC)
        uint32_t var_1A8;                                                                // 0x0001A8 (0x00525FC0)
        uint8_t var_1AC;                                                                 // 0x0001AC (0x00525FC4)
        uint8_t var_1AD;                                                                 // 0x0001AD (0x00525FC5)
        uint8_t loanInterestRate;                                                        // 0x0001AE (0x00525FC6)
        uint8_t lastIndustryOption;                                                      // 0x0001AF (0x00525FC7)
        uint8_t lastBuildingOption;                                                      // 0x0001B0 (0x00525FC8)
        uint8_t lastMiscBuildingOption;                                                  // 0x0001B1 (0x00525FC9)
        uint8_t lastWallOption;                                                          // 0x0001B2 (0x00525FCA)
        uint8_t var_1B3;                                                                 // 0x0001B3 (0x00525FCB)
        uint32_t var_1B4[2];                                                             // 0x0001B4 (0x00525FCC)
        char scenarioFileName[256];                                                      // 0x0001BC (0x00525FD4)
        char scenarioName[64];                                                           // 0x0002BC (0x005260D4)
        char scenarioDetails[256];                                                       // 0x0002FC (0x00526114)
        uint8_t competitorStartDelay;                                                    // 0x0003FC (0x00526214)
        uint8_t preferredAIIntelligence;                                                 // 0x0003FD (0x00526215)
        uint8_t preferredAIAggressiveness;                                               // 0x0003FE (0x00526216)
        uint8_t preferredAICompetitiveness;                                              // 0x0003FF (0x00526217)
        uint16_t startingLoanSize;                                                       // 0x000400 (0x00526218)
        uint16_t maxLoanSize;                                                            // 0x000402 (0x0052621A)
        uint32_t var_404;                                                                // 0x000404 (0x0052621C)
        uint32_t var_408;                                                                // 0x000408 (0x00526220)
        uint32_t var_40C;                                                                // 0x00040C (0x00526224)
        uint32_t var_410;                                                                // 0x000410 (0x00526228)
        uint8_t lastBuildVehiclesOption;                                                 // 0x000414 (0x0052622C)
        uint8_t numberOfIndustries;                                                      // 0x000415 (0x0052622D)
        uint16_t var_416;                                                                // 0x000416 (0x0052622E)
        uint8_t objectiveType;                                                           // 0x000418 (0x00526230)
        uint8_t objectiveFlags;                                                          // 0x000419 (0x00526231)
        uint32_t objectiveCompanyValue;                                                  // 0x00041A (0x00526232)
        uint32_t objectiveMonthlyVehicleProfit;                                          // 0x00041E (0x00526236)
        uint8_t objectivePerformanceIndex;                                               // 0x000422 (0x0052623A)
        uint8_t objectiveDeliveredCargoType;                                             // 0x000423 (0x0052623B)
        uint32_t objectiveDeliveredCargoAmount;                                          // 0x000424 (0x0052623C)
        uint8_t objectiveTimeLimitYears;                                                 // 0x000428 (0x00526240)
        uint16_t objectiveTimeLimitUntilYear;                                            // 0x000429 (0x00526241)
        uint16_t objectiveMonthsInChallenge;                                             // 0x00042B (0x00526243)
        uint16_t objectiveCompletedChallengeInMonths;                                    // 0x00042D (0x00526245)
        uint8_t industryFlags;                                                           // 0x00042F (0x00526247)
        uint16_t forbiddenVehiclesPlayers;                                               // 0x000430 (0x00526248)
        uint16_t forbiddenVehiclesCompetitors;                                           // 0x000432 (0x0052624A)
        uint16_t fixFlags;                                                               // 0x000434 (0x0052624C)
        uint16_t recordSpeed[3];                                                         // 0x000436 (0x0052624E)
        uint8_t recordCompany[4];                                                        // 0x00043C (0x00526254)
        uint32_t recordDate[3];                                                          // 0x000440 (0x00526258)
        uint32_t var_44C;                                                                // 0x00044C (0x00526264)
        uint32_t var_450;                                                                // 0x000450 (0x00526268)
        uint32_t var_454;                                                                // 0x000454 (0x0052626C)
        uint32_t var_458;                                                                // 0x000458 (0x00526270)
        uint32_t var_45C;                                                                // 0x00045C (0x00526274)
        uint32_t var_460;                                                                // 0x000460 (0x00526278)
        uint32_t var_464;                                                                // 0x000464 (0x0052627C)
        uint32_t var_468;                                                                // 0x000468 (0x00526280)
        uint32_t lastMapWindowFlags;                                                     // 0x00046C (0x00526284)
        uint16_t lastMapWindowSize[2];                                                   // 0x000470 (0x00526288)
        uint16_t lastMapWindowVar88A;                                                    // 0x000474 (0x0052628C)
        uint16_t lastMapWindowVar88C;                                                    // 0x000476 (0x0052628E)
        uint32_t var_478;                                                                // 0x000478 (0x00526290)
        uint8_t pad_047C[0x13B6 - 0x47C];                                                // 0x00047C
        uint16_t numMessages;                                                            // 0x0013B6 (0x005271CE)
        uint16_t activeMessageIndex;                                                     // 0x0013B8 (0x005271D0)
        Message messages[S5::Limits::kMaxMessages];                                      // 0x0013BA (0x005271D2)
        uint8_t pad_B886[0xB94C - 0xB886];                                               // 0x00B886
        uint8_t var_B94C;                                                                // 0x00B94C (0x00531774)
        uint8_t pad_B94D[0xB950 - 0xB94D];                                               // 0x00B94D
        uint8_t var_B950;                                                                // 0x00B950 (0x00531778)
        uint8_t pad_B951;                                                                // 0x00B951
        uint8_t var_B952;                                                                // 0x00B952 (0x0053177A)
        uint8_t pad_B953;                                                                // 0x00B953
        uint8_t var_B954;                                                                // 0x00B954 (0x0053177C)
        uint8_t pad_B955;                                                                // 0x00B955
        uint8_t var_B956;                                                                // 0x00B956 (0x0053177E)
        uint8_t pad_B957[0xB968 - 0xB957];                                               // 0x00B957
        uint8_t currentRainLevel;                                                        // 0x00B968 (0x00531780)
        uint8_t pad_B969[0xB96C - 0xB969];                                               // 0x00B969
        Company companies[S5::Limits::kMaxCompanies];                                    // 0x00B96C (0x00531784)
        Town towns[S5::Limits::kMaxTowns];                                               // 0x092444 (0x005B825C)
        Industry industries[S5::Limits::kMaxIndustries];                                 // 0x09E744 (0x005C455C)
        Station stations[S5::Limits::kMaxStations];                                      // 0x0C10C4 (0x005E6EDC)
        Entity entities[S5::Limits::kMaxEntities];                                       // 0x1B58C4 (0x006DB6DC)
        Animation animations[S5::Limits::kMaxAnimations];                                // 0x4268C4 (0x0094C6DC)
        Wave waves[S5::Limits::kMaxWaves];                                               // 0x4328C4 (0x009586DC)
        char userStrings[S5::Limits::kMaxUserStrings][32];                               // 0x432A44 (0x0095885C)
        uint16_t routings[S5::Limits::kMaxVehicles][S5::Limits::kMaxRoutingsPerVehicle]; // 0x442A44 (0x0096885C)
        uint8_t orders[S5::Limits::kMaxOrders];                                          // 0x461E44 (0x00987C5C)
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

    namespace SaveFlags
    {
        constexpr uint32_t none = 0;
        constexpr uint32_t packCustomObjects = 1 << 0;
        constexpr uint32_t scenario = 1 << 1;
        constexpr uint32_t landscape = 1 << 2;
        constexpr uint32_t noWindowClose = 1u << 29;
        constexpr uint32_t raw = 1u << 30;  // Save raw data including pointers with no clean up
        constexpr uint32_t dump = 1u << 31; // Used for dumping the game state when there is a fatal error
    };

    constexpr const char* extensionSC5 = ".SC5";
    constexpr const char* extensionSV5 = ".SV5";

    constexpr const char* filterSC5 = "*.SC5";
    constexpr const char* filterSV5 = "*.SV5";

    Options& getOptions();
    Options& getPreviewOptions();
    bool save(const fs::path& path, uint32_t flags);
    bool save(Stream& stream, uint32_t flags);
    void registerHooks();

    bool load(const fs::path& path, uint32_t flags);
    bool load(Stream& stream, uint32_t flags);

    void sub_4BAEC4();
}
