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
        uint32_t pad_0008[3];                                                  // 0x000008
        uint32_t currentDay;                                                   // 0x000014 (0x00525E2C)
        uint16_t dayCounter;                                                   // 0x000018
        uint16_t currentYear;                                                  // 0x00001A
        uint8_t currentMonth;                                                  // 0x00001C
        uint8_t currentDayOfMonth;                                             // 0x00001D
        int16_t savedViewX;                                                    // 0x00001E
        int16_t savedViewY;                                                    // 0x000020
        uint8_t savedViewZoom;                                                 // 0x000022
        uint8_t savedViewRotation;                                             // 0x000023
        uint8_t playerCompanyId;                                               // 0x000024
        uint8_t pad_0025[0x104 - 0x25];                                        // 0x000025
        EntityId_t entityListHeads[Limits::numEntityLists];                    // 0x000104 (0x00525E3E)
        uint16_t entityListCounts[Limits::numEntityLists];                     // 0x000112 (0x00525E4C)
        uint8_t pad_0120[0x146 - 0x120];                                       // 0x000120
        uint32_t scenarioTicks;                                                // 0x000146 (0x00525F5E)
        uint16_t pad_014A;                                                     // 0x00014A (0x00525F62)
        uint32_t scenarioTicks2;                                               // 0x00014C (0x00525F64)
        uint32_t magicNumber;                                                  // 0x000150 (0x00525F68)
        uint16_t numMapAnimations;                                             // 0x000154 (0x00525F6C)
        uint8_t pad_0156[0x02BC - 0x156];                                      // 0x000156
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
        uint8_t orders[Limits::maxOrders];                                     // 0x461E44 (0x0096885C)
    };
#pragma pack(pop)
    static_assert(sizeof(GameState) == 0x4A0644);

    GameState& getGameState();
}
