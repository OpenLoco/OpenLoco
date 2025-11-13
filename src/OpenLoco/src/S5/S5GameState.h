#pragma once

#include "Limits.h"
#include "S5Animation.h"
#include "S5Company.h"
#include "S5Entity.h"
#include "S5Industry.h"
#include "S5LabelFrame.h"
#include "S5Message.h"
#include "S5Station.h"
#include "S5Town.h"
#include "S5Wave.h"

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct Construction
    {
        uint8_t signals[8];       // 0x00015A (0x00525F72)
        uint8_t bridges[8];       // 0x000162 (0x00525F7A)
        uint8_t trainStations[8]; // 0x00016A (0x00525F82)
        uint8_t trackMods[8];     // 0x000172 (0x00525F8A)
        uint8_t var_17A[8];       // 0x00017A (0x00525F92) NOTE: not used always filled with 0xFF
        uint8_t roadStations[8];  // 0x000182 (0x00525F9A)
        uint8_t roadMods[8];      // 0x00018A (0x00525FA2)
    };
#pragma pack(pop)
    static_assert(sizeof(Construction) == 0x38);

#pragma pack(push, 1)
    // We are only splitting this up to make handling the GameState2 struct easier
    struct GeneralState
    {
        uint32_t rng[2];                                    // 0x000000 (0x00525E18)
        uint32_t unkRng[2];                                 // 0x000008 (0x00525E20)
        uint32_t flags;                                     // 0x000010 (0x00525E28)
        uint32_t currentDay;                                // 0x000014 (0x00525E2C)
        uint16_t dayCounter;                                // 0x000018
        uint16_t currentYear;                               // 0x00001A
        uint8_t currentMonth;                               // 0x00001C
        uint8_t currentDayOfMonth;                          // 0x00001D
        int16_t savedViewX;                                 // 0x00001E
        int16_t savedViewY;                                 // 0x000020
        uint8_t savedViewZoom;                              // 0x000022
        uint8_t savedViewRotation;                          // 0x000023
        uint8_t playerCompanies[2];                         // 0x000024 (0x00525E3C)
        uint16_t entityListHeads[Limits::kNumEntityLists];  // 0x000026 (0x00525E3E)
        uint16_t entityListCounts[Limits::kNumEntityLists]; // 0x000034 (0x00525E4C)
        uint8_t pad_0042[0x046 - 0x042];                    // 0x000042
        uint32_t currencyMultiplicationFactor[32];          // 0x000046 (0x00525E5E)
        uint32_t unusedCurrencyMultiplicationFactor[32];    // 0x0000C6 (0x00525EDE)
        uint32_t scenarioTicks;                             // 0x000146 (0x00525F5E)
        uint16_t var_014A;                                  // 0x00014A (0x00525F62)
        uint32_t scenarioTicks2;                            // 0x00014C (0x00525F64)
        uint32_t magicNumber;                               // 0x000150 (0x00525F68)
        uint16_t numMapAnimations;                          // 0x000154 (0x00525F6C)
        World::Pos2 tileUpdateStartLocation;                // 0x000156 (0x00525F6E)
        Construction scenarioConstruction;                  // 0x00015A (0x00525F72)
        uint8_t lastRailroadOption;                         // 0x000192 (0x00525FAA)
        uint8_t lastRoadOption;                             // 0x000193 (0x00525FAB)
        uint8_t lastAirport;                                // 0x000194 (0x00525FAC)
        uint8_t lastShipPort;                               // 0x000195 (0x00525FAD)
        bool trafficHandedness;                             // 0x000196 (0x00525FAE)
        uint8_t lastVehicleType;                            // 0x000197 (0x00525FAF)
        uint8_t pickupDirection;                            // 0x000198 (0x00525FB0)
        uint8_t lastTreeOption;                             // 0x000199 (0x00525FB1)
        uint16_t seaLevel;                                  // 0x00019A (0x00525FB2)
        uint8_t currentSnowLine;                            // 0x00019C (0x00525FB4)
        uint8_t currentSeason;                              // 0x00019D (0x00525FB5)
        uint8_t lastLandOption;                             // 0x00019E (0x00525FB6)
        uint8_t maxCompetingCompanies;                      // 0x00019F (0x00525FB7)
        uint32_t orderTableLength;                          // 0x0001A0 (0x00525FB8)
        uint32_t roadObjectIdIsNotTram;                     // 0x0001A4 (0x00525FBC)
        uint32_t roadObjectIdIsFlag7;                       // 0x0001A8 (0x00525FC0)
        uint8_t currentDefaultLevelCrossingType;            // 0x0001AC (0x00525FC4)
        uint8_t lastTrackTypeOption;                        // 0x0001AD (0x00525FC5)
        uint8_t loanInterestRate;                           // 0x0001AE (0x00525FC6)
        uint8_t lastIndustryOption;                         // 0x0001AF (0x00525FC7)
        uint8_t lastBuildingOption;                         // 0x0001B0 (0x00525FC8)
        uint8_t lastMiscBuildingOption;                     // 0x0001B1 (0x00525FC9)
        uint8_t lastWallOption;                             // 0x0001B2 (0x00525FCA)
        uint8_t produceAICompanyTimeout;                    // 0x0001B3 (0x00525FCB)
        uint32_t tickStartPrngState[2];                     // 0x0001B4 (0x00525FCC)
        char scenarioFileName[256];                         // 0x0001BC (0x00525FD4)
        char scenarioName[64];                              // 0x0002BC (0x005260D4)
        char scenarioDetails[256];                          // 0x0002FC (0x00526114)
        uint8_t competitorStartDelay;                       // 0x0003FC (0x00526214)
        uint8_t preferredAIIntelligence;                    // 0x0003FD (0x00526215)
        uint8_t preferredAIAggressiveness;                  // 0x0003FE (0x00526216)
        uint8_t preferredAICompetitiveness;                 // 0x0003FF (0x00526217)
        uint16_t startingLoanSize;                          // 0x000400 (0x00526218)
        uint16_t maxLoanSize;                               // 0x000402 (0x0052621A)
        uint32_t multiplayerPrng[2];                        // 0x000404 (0x0052621C)
        uint32_t multiplayerChecksumA;                      // 0x00040C (0x00526224)
        uint32_t multiplayerChecksumB;                      // 0x000410 (0x00526228)
        uint8_t lastBuildVehiclesOption;                    // 0x000414 (0x0052622C)
        uint8_t numberOfIndustries;                         // 0x000415 (0x0052622D)
        uint16_t vehiclePreviewRotationFrame;               // 0x000416 (0x0052622E)
        uint8_t objectiveType;                              // 0x000418 (0x00526230)
        uint8_t objectiveFlags;                             // 0x000419 (0x00526231)
        uint32_t objectiveCompanyValue;                     // 0x00041A (0x00526232)
        uint32_t objectiveMonthlyVehicleProfit;             // 0x00041E (0x00526236)
        uint8_t objectivePerformanceIndex;                  // 0x000422 (0x0052623A)
        uint8_t objectiveDeliveredCargoType;                // 0x000423 (0x0052623B)
        uint32_t objectiveDeliveredCargoAmount;             // 0x000424 (0x0052623C)
        uint8_t objectiveTimeLimitYears;                    // 0x000428 (0x00526240)
        uint16_t objectiveTimeLimitUntilYear;               // 0x000429 (0x00526241)
        uint16_t objectiveMonthsInChallenge;                // 0x00042B (0x00526243)
        uint16_t objectiveCompletedChallengeInMonths;       // 0x00042D (0x00526245)
        uint8_t industryFlags;                              // 0x00042F (0x00526247)
        uint16_t forbiddenVehiclesPlayers;                  // 0x000430 (0x00526248)
        uint16_t forbiddenVehiclesCompetitors;              // 0x000432 (0x0052624A)
        uint16_t fixFlags;                                  // 0x000434 (0x0052624C)
        Records companyRecords;                             // 0x000436 (0x0052624E)
        uint32_t var_44C;                                   // 0x00044C (0x00526264)
        uint32_t var_450;                                   // 0x000450 (0x00526268)
        uint32_t var_454;                                   // 0x000454 (0x0052626C)
        uint32_t var_458;                                   // 0x000458 (0x00526270)
        uint32_t var_45C;                                   // 0x00045C (0x00526274)
        uint32_t var_460;                                   // 0x000460 (0x00526278)
        uint32_t var_464;                                   // 0x000464 (0x0052627C)
        uint32_t var_468;                                   // 0x000468 (0x00526280)
        uint32_t lastMapWindowFlags;                        // 0x00046C (0x00526284)
        uint16_t lastMapWindowSize[2];                      // 0x000470 (0x00526288)
        uint16_t lastMapWindowVar88A;                       // 0x000474 (0x0052628C)
        uint16_t lastMapWindowVar88C;                       // 0x000476 (0x0052628E)
        uint32_t var_478;                                   // 0x000478 (0x00526290)
        uint8_t pad_047C[0x13B6 - 0x47C];                   // 0x00047C
        uint16_t numMessages;                               // 0x0013B6 (0x005271CE)
        uint16_t activeMessageIndex;                        // 0x0013B8 (0x005271D0)
        Message messages[S5::Limits::kMaxMessages];         // 0x0013BA (0x005271D2)
        uint8_t pad_B95A[0xB95C - 0xB95A];                  // 0x00B95A
        uint8_t var_B95C;                                   // 0x00B95C (0x00531774)
        uint8_t pad_B95D[0xB960 - 0xB95D];                  // 0x00B95D
        uint8_t var_B960;                                   // 0x00B960 (0x00531778)
        uint8_t pad_B961;                                   // 0x00B961
        uint8_t var_B962;                                   // 0x00B962 (0x0053177A)
        uint8_t pad_B963;                                   // 0x00B963
        uint8_t var_B964;                                   // 0x00B964 (0x0053177C)
        uint8_t pad_B965;                                   // 0x00B965
        uint8_t var_B966;                                   // 0x00B966 (0x0053177E)
        uint8_t pad_B967;                                   // 0x00B967
        uint8_t currentRainLevel;                           // 0x00B968 (0x00531780)
        uint8_t pad_B969[0xB96C - 0xB969];                  // 0x00B969
    };
    static_assert(sizeof(GeneralState) == 0x00B96C);

    struct GameState
    {
        GeneralState general;                                                            // 0x000000 (0x00525E18)
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
    static_assert(sizeof(GameState) == 0x4A0644);

    struct GameStateType2
    {
        GeneralState general;                                                            // 0x000000
        CompanyType2 companies[S5::Limits::kMaxCompanies];                               // 0x00B96C
        Town towns[S5::Limits::kMaxTowns];                                               // 0x090824
        Industry industries[S5::Limits::kMaxIndustries];                                 // 0x09CB24
        Station stations[S5::Limits::kMaxStations];                                      // 0x0BF4A4
        Entity entities[S5::Limits::kMaxEntities];                                       // 0x1B3CA4
        Animation animations[S5::Limits::kMaxAnimations];                                // 0x424CA4
        Wave waves[S5::Limits::kMaxWaves];                                               // 0x430CA4
        char userStrings[S5::Limits::kMaxUserStrings][32];                               // 0x430E24
        uint16_t routings[S5::Limits::kMaxVehicles][S5::Limits::kMaxRoutingsPerVehicle]; // 0x440E24
        uint8_t orders[S5::Limits::kMaxOrders];                                          // 0x460224
    };
    static_assert(sizeof(GameStateType2) == 0x49EA24);
#pragma pack(pop)
}
