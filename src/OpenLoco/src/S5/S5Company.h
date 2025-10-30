#pragma once

#include "Economy/Currency.h"
#include <OpenLoco/Engine/World.hpp>

namespace OpenLoco
{
    struct Company;
    namespace CompanyManager
    {
        struct Records;
    }
}

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct AiThought
    {
        struct Station
        {
            uint16_t id;      // 0x0
            uint8_t var_02;   // 0x2 flags?
            uint8_t rotation; // 0x3
            World::Pos2 pos;  // 0x4
            uint8_t baseZ;    // 0x8
            uint8_t var_9;    // 0x9 aiStationIndex
            uint8_t var_A;    // 0xA aiStationIndex
            uint8_t var_B;    // 0xB
            uint8_t var_C;    // 0xC
            uint8_t pad_D[0xE - 0xD];
        };
        static_assert(sizeof(Station) == 0xE);
        uint8_t type;          // 0x00 0x4A8
        uint8_t destinationA;  // 0x01 0x4A9 either a TownId or IndustryId
        uint8_t destinationB;  // 0x02 0x4AA either a TownId or IndustryId
        uint8_t numStations;   // 0x03 0x4AB size of stations
        uint8_t stationLength; // 0x04 0x4AC station length
        uint8_t pad_05;
        Station stations[4];  // 0x06 0x4AE Will lists stations created that vehicles will route to
        uint8_t trackObjId;   // 0x3E 0x4E6 track or road (with high bit set)
        uint8_t rackRailType; // 0x3F 0x4E7 Is 0xFFU for no rack rail
        uint16_t mods;        // 0x40 0x4E8 track or road
        uint8_t cargoType;    // 0x42 0x4EA
        uint8_t var_43;       // 0x4EB
        uint8_t numVehicles;  // 0x44 0x4EC size of var_66
        uint8_t var_45;       // 0x4ED size of var_46
        uint16_t var_46[16];  // 0x4EF array of uint16_t object id
        uint16_t vehicles[8]; // 0x66 0x50E see also numVehicles for current size
        currency32_t var_76;  // 0x51E
        uint8_t pad_7A[0x7C - 0x7A];
        currency32_t var_7C;   // 0x524
        currency32_t var_80;   // 0x528
        currency32_t var_84;   // 0x52C
        uint8_t var_88;        // 0x530
        uint8_t stationObjId;  // 0x89 0x531 Could be either Airport/Dock/TrainStation/RoadStation
        uint8_t signalObjId;   // 0x8A 0x532 Can be 0xFFU for n
        uint8_t purchaseFlags; // 0x8B 0x533
    };

    struct Company
    {
        struct HashTableEntry
        {
            uint16_t var_00; // x
            uint16_t var_02; // y + flags
            uint8_t var_04;  // z
            uint8_t var_05;  // trackId | (direction << 6)
        };
        uint16_t name;
        uint16_t ownerName;
        uint32_t challengeFlags;                                                        // 0x04
        currency48_t cash;                                                              // 0x08
        currency32_t currentLoan;                                                       // 0x0E
        uint32_t updateCounter;                                                         // 0x12
        int16_t performanceIndex;                                                       // 0x16
        uint8_t competitorId;                                                           // 0x18
        Emotion ownerEmotion;                                                           // 0x19
        uint8_t mainColours[2];                                                         // 0x1A
        uint8_t vehicleColours[10][2];                                                  // 0x1C
        uint32_t customVehicleColoursSet;                                               // 0x30
        uint32_t unlockedVehicles[7];                                                   // 0x34
        uint16_t availableVehicles;                                                     // 0x50
        uint32_t aiPlaystyleFlags;                                                      // 0x52
        uint8_t aiPlaystyleTownId;                                                      // 0x56
        uint8_t numExpenditureYears;                                                    // 0x57
        currency32_t expenditures[kExpenditureHistoryCapacity][ExpenditureType::Count]; // 0x58
        uint32_t startedDate;                                                           // 0x0498
        uint32_t var_49C;
        uint32_t var_4A0;
        uint8_t var_4A4; // 0x04A4
        uint8_t var_4A5;
        uint8_t var_4A6;
        uint8_t var_4A7;
        AiThought aiThoughts[kMaxAiThoughts];      // 0x04A8
        uint8_t activeThoughtId;                   // 0x2578
        World::SmallZ headquartersZ;               // 0x2579
        coord_t headquartersX;                     // 0x257A -1 on no headquarter placed
        coord_t headquartersY;                     // 0x257C
        currency32_t activeThoughtRevenueEstimate; // 0x257E Also used for thoughtState2AiStationIdx in sub_430CEC TODO: Don't do this
        uint32_t var_2582;
        uint8_t pad_2586[0x2596 - 0x2586];
        uint32_t var_2596;
        uint8_t var_259A;
        uint8_t var_259B;
        uint8_t var_259C;
        uint8_t pad_259D;
        uint32_t aiPlaceVehicleIndex;
        uint8_t pad_25A2[0x25BE - 0x25A2];
        uint8_t var_25BE;
        uint8_t currentRating;           // 0x25BF
        HashTableEntry var_25C0[0x1000]; // 0x25C0 Hash table entries
        uint16_t var_25C0_length;        // 0x85C0 Hash table length
        uint8_t var_85C2;
        uint8_t var_85C3;
        World::Pos2 var_85C4;
        World::SmallZ var_85C8;
        World::Pos2 var_85C9;
        World::SmallZ var_85CD;
        uint8_t var_85CE;
        uint8_t var_85CF;
        World::Pos2 var_85D0;
        World::SmallZ var_85D4;
        uint16_t var_85D5;
        World::Pos2 var_85D7;
        World::SmallZ var_85DB;
        uint16_t var_85DC;
        uint32_t var_85DE;
        uint32_t var_85E2;
        uint16_t var_85E6;
        uint16_t var_85E8;
        uint32_t var_85EA;
        uint8_t var_85EE;
        uint8_t var_85EF;
        uint16_t var_85F0;
        currency32_t var_85F2;
        uint16_t var_85F6;
        uint32_t cargoUnitsTotalDelivered;        // 0x85F8
        uint32_t cargoUnitsDeliveredHistory[120]; // 0x85FC
        int16_t performanceIndexHistory[120];     // 0x87DC
        uint16_t historySize;                     // 0x88CC
        currency48_t companyValueHistory[120];    // 0x88CE
        currency48_t vehicleProfit;               // 0x8B9E
        uint16_t transportTypeCount[6];           // 0x8BA4
        uint8_t activeEmotions[9];                // 0x8BB0 duration in days that emotion is active 0 == not active
        uint8_t observationStatus;                // 0x8BB9;
        uint16_t observationTownId;               // 0x8BBA;
        uint16_t observationEntity;               // 0x8BBC;
        int16_t observationX;                     // 0x8BBE;
        int16_t observationY;                     // 0x8BC0;
        uint16_t observationObject;               // 0x8BC2;
        uint16_t observationTimeout;              // 0x8BC4
        uint16_t ownerStatus[2];                  // 0x8BC6
        uint8_t pad_8BCA[0x8BCE - 0x8BCA];
        uint32_t cargoDelivered[32];             // 0x8BCE;
        uint8_t challengeProgress;               // 0x8C4E - percent completed on challenge
        uint8_t numMonthsInTheRed;               // 0x8C4F
        uint32_t cargoUnitsTotalDistance;        // 0x8C50
        uint32_t cargoUnitsDistanceHistory[120]; // 0x8C54
        uint16_t jailStatus;                     // 0x8E34
        uint8_t pad_8E36[0x8FA8 - 0x8E36];
    };
    static_assert(sizeof(Company) == 0x8FA8);

    struct Records
    {
        uint16_t speed[3];  // 0x000436 (0x0052624E)
        uint8_t company[3]; // 0x00043C (0x00526254)
        uint8_t pad_43A;
        uint32_t date[3]; // 0x000440 (0x00526258)
    };
#pragma pack(pop)

    S5::Company exportCompany(OpenLoco::Company& src);
    S5::Records exportRecords(OpenLoco::CompanyManager::Records& src);
}
