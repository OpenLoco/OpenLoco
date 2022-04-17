#pragma once
#include "Company.h"
#include "Entities/Entity.h"
#include "Industry.h"
#include "Limits.h"
#include "Map/Animation.h"
#include "Map/Wave.h"
#include "Message.h"
#include "Station.h"
#include "Town.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct GameState
    {
        Utility::prng rng;                                                       // 0x000000 (0x00525E18)
        Utility::prng unkRng;                                                    // 0x000008 (0x00525E20)
        uint32_t flags;                                                          // 0x000010 (0x00525E28)
        uint32_t currentDay;                                                     // 0x000014 (0x00525E2C)
        uint16_t dayCounter;                                                     // 0x000018 (0x00525E30)
        uint16_t currentYear;                                                    // 0x00001A (0x00525E32)
        uint8_t currentMonth;                                                    // 0x00001C (0x00525E34)
        uint8_t currentDayOfMonth;                                               // 0x00001D (0x00525E35)
        int16_t savedViewX;                                                      // 0x00001E (0x00525E36)
        int16_t savedViewY;                                                      // 0x000020 (0x00525E38)
        uint8_t savedViewZoom;                                                   // 0x000022 (0x00525E3A)
        uint8_t savedViewRotation;                                               // 0x000023 (0x00525E3B)
        CompanyId playerCompanies[2];                                            // 0x000024 (0x00525E3C)
        EntityId entityListHeads[Limits::kNumEntityLists];                       // 0x000026 (0x00525E3E)
        uint16_t entityListCounts[Limits::kNumEntityLists];                      // 0x000034 (0x00525E4C)
        uint8_t pad_0042[0x046 - 0x042];                                         // 0x000042
        uint32_t currencyMultiplicationFactor[32];                               // 0x000046 (0x00525E5E)
        uint32_t unusedCurrencyMultiplicationFactor[32];                         // 0x0000C6 (0x00525EDE)
        uint32_t scenarioTicks;                                                  // 0x000146 (0x00525F5E)
        uint16_t var_014A;                                                       // 0x00014A (0x00525F62)
        uint32_t scenarioTicks2;                                                 // 0x00014C (0x00525F64)
        uint32_t magicNumber;                                                    // 0x000150 (0x00525F68)
        uint16_t numMapAnimations;                                               // 0x000154 (0x00525F6C)
        Map::Pos2 tileUpdateStartLocation;                                       // 0x000156 (0x00525F6E)
        uint8_t scenarioSignals[8];                                              // 0x00015A (0x00525F72)
        uint8_t scenarioBridges[8];                                              // 0x000162 (0x00525F7A)
        uint8_t scenarioTrainStations[8];                                        // 0x00016A (0x00525F82)
        uint8_t scenarioTrackMods[8];                                            // 0x000172 (0x00525F8A)
        uint8_t var_17A[8];                                                      // 0x00017A (0x00525F92)
        uint8_t scenarioRoadStations[8];                                         // 0x000182 (0x00525F9A)
        uint8_t scenarioRoadMods[8];                                             // 0x00018A (0x00525FA2)
        uint8_t lastRailroadOption;                                              // 0x000192 (0x00525FAA)
        uint8_t lastRoadOption;                                                  // 0x000193 (0x00525FAB)
        uint8_t lastAirport;                                                     // 0x000194 (0x00525FAC)
        uint8_t lastShipPort;                                                    // 0x000195 (0x00525FAD)
        bool trafficHandedness;                                                  // 0x000196 (0x00525FAE)
        VehicleType lastVehicleType;                                             // 0x000197 (0x00525FAF)
        uint8_t pickupDirection;                                                 // 0x000198 (0x00525FB0)
        uint8_t lastTreeOption;                                                  // 0x000199 (0x00525FB1)
        uint16_t seaLevel;                                                       // 0x00019A (0x00525FB2)
        uint8_t currentSnowLine;                                                 // 0x00019C (0x00525FB4)
        uint8_t currentSeason;                                                   // 0x00019D (0x00525FB5)
        uint8_t lastLandOption;                                                  // 0x00019E (0x00525FB6)
        uint8_t maxCompetingCompanies;                                           // 0x00019F (0x00525FB7)
        uint32_t numOrders;                                                      // 0x0001A0 (0x00525FB8)
        uint32_t var_1A4;                                                        // 0x0001A4 (0x00525FBC)
        uint32_t var_1A8;                                                        // 0x0001A8 (0x00525FC0)
        uint8_t var_1AC;                                                         // 0x0001AC (0x00525FC4)
        uint8_t var_1AD;                                                         // 0x0001AD (0x00525FC5)
        uint8_t loanInterestRate;                                                // 0x0001AE (0x00525FC6)
        uint8_t lastIndustryOption;                                              // 0x0001AF (0x00525FC7)
        uint8_t lastBuildingOption;                                              // 0x0001B0 (0x00525FC8)
        uint8_t lastMiscBuildingOption;                                          // 0x0001B1 (0x00525FC9)
        uint8_t lastWallOption;                                                  // 0x0001B2 (0x00525FCA)
        uint8_t var_1B3;                                                         // 0x0001B3 (0x00525FCB)
        Utility::prng var_1B4;                                                   // 0x0001B4 (0x00525FCC)
        char scenarioFileName[256];                                              // 0x0001BC (0x00525FD4)
        char scenarioName[64];                                                   // 0x0002BC (0x005260D4)
        char scenarioDetails[256];                                               // 0x0002FC (0x00526114)
        uint8_t competitorStartDelay;                                            // 0x0003FC (0x00526214)
        uint8_t preferredAIIntelligence;                                         // 0x0003FD (0x00526215)
        uint8_t preferredAIAggressiveness;                                       // 0x0003FE (0x00526216)
        uint8_t preferredAICompetitiveness;                                      // 0x0003FF (0x00526217)
        uint16_t startingLoanSize;                                               // 0x000400 (0x00526218)
        uint16_t maxLoanSize;                                                    // 0x000402 (0x0052621A)
        uint32_t var_404;                                                        // 0x000404 (0x0052621C)
        uint32_t var_408;                                                        // 0x000408 (0x00526220)
        uint32_t var_40C;                                                        // 0x00040C (0x00526224)
        uint32_t var_410;                                                        // 0x000410 (0x00526228)
        uint8_t lastBuildVehiclesOption;                                         // 0x000414 (0x0052622C)
        uint8_t numberOfIndustries;                                              // 0x000415 (0x0052622D)
        uint16_t var_416;                                                        // 0x000416 (0x0052622E)
        uint8_t objectiveType;                                                   // 0x000418 (0x00526230)
        uint8_t objectiveFlags;                                                  // 0x000419 (0x00526231)
        uint32_t objectiveCompanyValue;                                          // 0x00041A (0x00526232)
        uint32_t objectiveMonthlyVehicleProfit;                                  // 0x00041E (0x00526236)
        uint8_t objectivePerformanceIndex;                                       // 0x000422 (0x0052623A)
        uint8_t objectiveDeliveredCargoType;                                     // 0x000423 (0x0052623B)
        uint32_t objectiveDeliveredCargoAmount;                                  // 0x000424 (0x0052623C)
        uint8_t objectiveTimeLimitYears;                                         // 0x000428 (0x00526240)
        uint16_t objectiveTimeLimitUntilYear;                                    // 0x000429 (0x00526241)
        uint16_t objectiveMonthsInChallenge;                                     // 0x00042B (0x00526243)
        uint16_t objectiveCompletedChallengeInMonths;                            // 0x00042D (0x00526245)
        uint8_t industryFlags;                                                   // 0x00042F (0x00526247)
        uint16_t forbiddenVehiclesPlayers;                                       // 0x000430 (0x00526248)
        uint16_t forbiddenVehiclesCompetitors;                                   // 0x000432 (0x0052624A)
        uint16_t fixFlags;                                                       // 0x000434 (0x0052624C)
        Speed16 recordSpeed[3];                                                  // 0x000436 (0x0052624E)
        uint8_t recordCompany[4];                                                // 0x00043C (0x00526254)
        uint32_t recordDate[3];                                                  // 0x000440 (0x00526258)
        uint32_t var_44C;                                                        // 0x00044C (0x00526264)
        uint32_t var_450;                                                        // 0x000450 (0x00526268)
        uint32_t var_454;                                                        // 0x000454 (0x0052626C)
        uint32_t var_458;                                                        // 0x000458 (0x00526270)
        uint32_t var_45C;                                                        // 0x00045C (0x00526274)
        uint32_t var_460;                                                        // 0x000460 (0x00526278)
        uint32_t var_464;                                                        // 0x000464 (0x0052627C)
        uint32_t var_468;                                                        // 0x000468 (0x00526280)
        uint32_t lastMapWindowFlags;                                             // 0x00046C (0x00526284)
        Ui::Size lastMapWindowSize;                                              // 0x000470 (0x00526288)
        uint16_t lastMapWindowVar88A;                                            // 0x000474 (0x0052628C)
        uint16_t lastMapWindowVar88C;                                            // 0x000476 (0x0052628E)
        uint32_t var_478;                                                        // 0x000478 (0x00526290)
        uint8_t pad_047C[0x13B6 - 0x47C];                                        // 0x00047C
        uint16_t numMessages;                                                    // 0x0013B6 (0x005271CE)
        MessageId activeMessageIndex;                                            // 0x0013B8 (0x005271D0)
        Message messages[Limits::kMaxMessages];                                  // 0x0013BA (0x005271D2)
        uint8_t pad_B886[0xB94C - 0xB886];                                       // 0x00B886
        uint8_t var_B94C;                                                        // 0x00B94C (0x00531774)
        uint8_t pad_B94D[0xB950 - 0xB94D];                                       // 0x00B94D
        uint8_t var_B950;                                                        // 0x00B950 (0x00531778)
        uint8_t pad_B951;                                                        // 0x00B951
        uint8_t var_B952;                                                        // 0x00B952 (0x0053177A)
        uint8_t pad_B953;                                                        // 0x00B953
        uint8_t var_B954;                                                        // 0x00B954 (0x0053177C)
        uint8_t pad_B955;                                                        // 0x00B955
        uint8_t var_B956;                                                        // 0x00B956 (0x0053177E)
        uint8_t pad_B957[0xB968 - 0xB957];                                       // 0x00B957
        uint8_t currentRainLevel;                                                // 0x00B968 (0x00531780)
        uint8_t pad_B969[0xB96C - 0xB969];                                       // 0x00B969
        Company companies[Limits::kMaxCompanies];                                // 0x00B96C (0x00531784)
        Town towns[Limits::kMaxTowns];                                           // 0x092444 (0x005B825C)
        Industry industries[Limits::kMaxIndustries];                             // 0x09E744 (0x005C455C)
        Station stations[Limits::kMaxStations];                                  // 0x0C10C4 (0x005E6EDC)
        Entity entities[Limits::kMaxEntities];                                   // 0x1B58C4 (0x006DB6DC)
        Map::Animation animations[Limits::kMaxAnimations];                       // 0x4268C4 (0x0094C6DC)
        Map::Wave waves[Limits::kMaxWaves];                                      // 0x4328C4 (0x009586DC)
        char userStrings[Limits::kMaxUserStrings][32];                           // 0x432A44 (0x0095885C)
        uint16_t routings[Limits::kMaxVehicles][Limits::kMaxRoutingsPerVehicle]; // 0x442A44 (0x0096885C)
        uint8_t orders[Limits::kMaxOrders];                                      // 0x461E44 (0x00987C5C)
    };
#pragma pack(pop)
    static_assert(sizeof(GameState) == 0x4A0644);

    GameState& getGameState();

}
