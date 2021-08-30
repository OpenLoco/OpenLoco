#pragma once
#include "Company.h"
#include "Entities/Entity.h"
#include "Industry.h"
#include "Limits.h"
#include "Map/Animation.h"
#include "Map/Wave.h"
#include "Station.h"
#include "Town.h"

namespace OpenLoco
{
#pragma pack(push, 1)
    struct GameState
    {
        Utility::prng rng;                                                     // 0x000000 (0x00525E18)
        uint32_t pad_0008[3];                                                  // 0x000008 (0x00525E20)
        uint32_t currentDay;                                                   // 0x000014 (0x00525E2C)
        uint16_t dayCounter;                                                   // 0x000018 (0x00525E30)
        uint16_t currentYear;                                                  // 0x00001A (0x00525E32)
        uint8_t currentMonth;                                                  // 0x00001C (0x00525E34)
        uint8_t currentDayOfMonth;                                             // 0x00001D (0x00525E35)
        int16_t savedViewX;                                                    // 0x00001E (0x00525E36)
        int16_t savedViewY;                                                    // 0x000020 (0x00525E38)
        uint8_t savedViewZoom;                                                 // 0x000022 (0x00525E3A)
        uint8_t savedViewRotation;                                             // 0x000023 (0x00525E3B)
        CompanyId_t playerCompanies[2];                                        // 0x000024 (0x00525E3C)
        EntityId_t entityListHeads[Limits::numEntityLists];                    // 0x000026 (0x00525E3E)
        uint16_t entityListCounts[Limits::numEntityLists];                     // 0x000034 (0x00525E4C)
        uint8_t pad_0042[0x046 - 0x042];                                       // 0x000042
        uint32_t currencyMultiplicationFactor[32];                             // 0x000046 (0x00525E5E)
        uint32_t unusedCurrencyMultiplicationFactor[32];                       // 0x0000C6 (0x00525EDE)
        uint32_t scenarioTicks;                                                // 0x000146 (0x00525F5E)
        uint16_t pad_014A;                                                     // 0x00014A (0x00525F62)
        uint32_t scenarioTicks2;                                               // 0x00014C (0x00525F64)
        uint32_t magicNumber;                                                  // 0x000150 (0x00525F68)
        uint16_t numMapAnimations;                                             // 0x000154 (0x00525F6C)
        uint16_t var_0156;                                                     // 0x000156 (0x00525F6E)
        uint16_t var_0158;                                                     // 0x000158 (0x00525F70)
        uint8_t var_15A[8];                                                    // 0x00015A (0x00525F72)
        uint8_t var_162[8];                                                    // 0x000162 (0x00525F7A)
        uint8_t var_16A[8];                                                    // 0x00016A (0x00525F82)
        uint8_t var_172[8];                                                    // 0x000172 (0x00525F8A)
        uint8_t var_17A[8];                                                    // 0x00017A (0x00525F92)
        uint8_t var_182[8];                                                    // 0x000182 (0x00525F9A)
        uint8_t var_18A[8];                                                    // 0x00018A (0x00525FA2)
        uint8_t var_192;                                                       // 0x000192 (0x00525FAA)
        uint8_t lastRoadOption;                                                // 0x000193 (0x00525FAB)
        uint8_t var_194;                                                       // 0x000194 (0x00525FAC)
        uint8_t var_195;                                                       // 0x000195 (0x00525FAD)
        uint8_t var_196;                                                       // 0x000196 (0x00525FAE)
        uint8_t var_197;                                                       // 0x000197 (0x00525FAF)
        uint8_t var_198;                                                       // 0x000198 (0x00525FB0)
        uint8_t var_199;                                                       // 0x000199 (0x00525FB1)
        uint16_t seaLevel;                                                     // 0x00019A (0x00525FB2)
        uint8_t var_19C;                                                       // 0x00019C (0x00525FB4)
        uint8_t var_19D;                                                       // 0x00019D (0x00525FB5)
        uint8_t var_19E;                                                       // 0x00019E (0x00525FB6)
        uint8_t maxCompetingCompanies;                                         // 0x00019F (0x00525FB7)
        uint8_t pad_01A0[0x02BC - 0x1A0];                                      // 0x0001A0
        char scenarioName[64];                                                 // 0x0002BC (0x005260D4)
        char scenarioDetails[256];                                             // 0x0002FC (0x00526114)
        uint8_t pad_03FC[0x434 - 0x3FC];                                       // 0x0003FC
        uint16_t fixFlags;                                                     // 0x000434 (0x0052624C)
        uint8_t pad_0436[0xB96C - 0x436];                                      // 0x0003FC
        Company companies[Limits::maxCompanies];                               // 0x00B96C (0x00531784)
        Town towns[Limits::maxTowns];                                          // 0x092444 (0x005B825C)
        Industry industries[Limits::maxIndustries];                            // 0x09E744 (0x005C455C)
        Station stations[Limits::maxStations];                                 // 0x0C10C4 (0x005E6EDC)
        Entity entities[Limits::maxEntities];                                  // 0x1B58C4 (0x006DB6DC)
        Map::Animation animations[Limits::maxAnimations];                      // 0x4268C4 (0x0094C6DC)
        Map::Wave waves[Limits::maxWaves];                                     // 0x4328C4 (0x009586DC)
        uint8_t userStrings[Limits::maxUserStrings][32];                       // 0x432A44 (0x0095885C)
        uint16_t routings[Limits::maxVehicles][Limits::maxRoutingsPerVehicle]; // 0x442A44 (0x0096885C)
        uint8_t orders[Limits::maxOrders];                                     // 0x461E44 (0x00987C5C)
    };
#pragma pack(pop)
    static_assert(sizeof(GameState) == 0x4A0644);

    GameState& getGameState();
}
