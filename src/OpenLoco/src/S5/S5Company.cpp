#include "S5/S5Company.h"
#include "World/Company.h"
#include "World/CompanyRecords.h"
#include <cstring>

namespace OpenLoco::S5
{
    S5::Company exportCompany(const OpenLoco::Company& src)
    {
        S5::Company dst{};
        dst.name = src.name;
        dst.ownerName = src.ownerName;
        dst.challengeFlags = enumValue(src.challengeFlags);
        dst.cash = src.cash;
        dst.currentLoan = src.currentLoan;
        dst.updateCounter = src.updateCounter;
        dst.performanceIndex = src.performanceIndex;
        dst.competitorId = src.competitorId;
        dst.ownerEmotion = enumValue(src.ownerEmotion);
        dst.mainColours[0] = enumValue(src.mainColours.primary);
        dst.mainColours[1] = enumValue(src.mainColours.secondary);
        for (auto i = 0U; i < std::size(src.vehicleColours); ++i)
        {
            dst.vehicleColours[i][0] = enumValue(src.vehicleColours[i].primary);
            dst.vehicleColours[i][1] = enumValue(src.vehicleColours[i].secondary);
        }
        dst.customVehicleColoursSet = src.customVehicleColoursSet;
        for (auto i = 0U; i < 224; ++i)
        {
            dst.unlockedVehicles[i / 32] |= src.unlockedVehicles[i] << static_cast<uint32_t>((i % 32));
        }
        dst.availableVehicles = src.availableVehicles;
        dst.aiPlaystyleFlags = enumValue(src.aiPlaystyleFlags);
        dst.aiPlaystyleTownId = src.aiPlaystyleTownId;
        dst.numExpenditureYears = src.numExpenditureYears;
        std::memcpy(dst.expenditures, src.expenditures, kExpenditureHistoryCapacity * ExpenditureType::Count * sizeof(currency32_t));
        dst.startedDate = src.startedDate;
        dst.var_49C = src.var_49C;
        dst.var_4A0 = src.var_4A0;
        dst.var_4A4 = enumValue(src.var_4A4);
        dst.var_4A5 = src.var_4A5;
        dst.var_4A6 = enumValue(src.var_4A6);
        dst.var_4A7 = src.var_4A7;

        // Copy AI thoughts
        for (auto i = 0U; i < kMaxAiThoughts; ++i)
        {
            auto& srcThought = src.aiThoughts[i];
            auto& dstThought = dst.aiThoughts[i];

            dstThought.type = enumValue(srcThought.type);
            dstThought.destinationA = srcThought.destinationA;
            dstThought.destinationB = srcThought.destinationB;
            dstThought.numStations = srcThought.numStations;
            dstThought.stationLength = srcThought.stationLength;

            for (auto j = 0U; j < std::size(srcThought.stations); ++j)
            {
                dstThought.stations[j].id = enumValue(srcThought.stations[j].id);
                dstThought.stations[j].var_02 = enumValue(srcThought.stations[j].var_02);
                dstThought.stations[j].rotation = srcThought.stations[j].rotation;
                dstThought.stations[j].pos = srcThought.stations[j].pos;
                dstThought.stations[j].baseZ = srcThought.stations[j].baseZ;
                dstThought.stations[j].var_9 = srcThought.stations[j].var_9;
                dstThought.stations[j].var_A = srcThought.stations[j].var_A;
                dstThought.stations[j].var_B = srcThought.stations[j].var_B;
                dstThought.stations[j].var_C = srcThought.stations[j].var_C;
            }

            dstThought.trackObjId = srcThought.trackObjId;
            dstThought.rackRailType = srcThought.rackRailType;
            dstThought.mods = srcThought.mods;
            dstThought.cargoType = srcThought.cargoType;
            dstThought.var_43 = srcThought.var_43;
            dstThought.numVehicles = srcThought.numVehicles;
            dstThought.var_45 = srcThought.var_45;
            std::ranges::copy(srcThought.var_46, dstThought.var_46);

            for (auto j = 0U; j < std::size(srcThought.vehicles); ++j)
            {
                dstThought.vehicles[j] = enumValue(srcThought.vehicles[j]);
            }

            dstThought.var_76 = srcThought.var_76;
            dstThought.var_7C = srcThought.var_7C;
            dstThought.var_80 = srcThought.var_80;
            dstThought.var_84 = srcThought.var_84;
            dstThought.var_88 = srcThought.var_88;
            dstThought.stationObjId = srcThought.stationObjId;
            dstThought.signalObjId = srcThought.signalObjId;
            dstThought.purchaseFlags = enumValue(srcThought.purchaseFlags);
        }

        dst.activeThoughtId = src.activeThoughtId;
        dst.headquartersZ = src.headquartersZ;
        dst.headquartersX = src.headquartersX;
        dst.headquartersY = src.headquartersY;
        dst.activeThoughtRevenueEstimate = src.activeThoughtRevenueEstimate;
        dst.var_2582 = src.var_2582;
        dst.var_2596 = src.var_2596;
        dst.var_259A = src.var_259A;
        dst.var_259B = src.var_259B;
        dst.var_259C = src.var_259C;
        dst.aiPlaceVehicleIndex = src.aiPlaceVehicleIndex;
        dst.var_25BE = enumValue(src.var_25BE);
        dst.currentRating = enumValue(src.currentRating);

        // Copy hash table
        for (auto i = 0U; i < std::size(src.var_25C0); ++i)
        {
            dst.var_25C0[i].var_00 = src.var_25C0[i].var_00;
            dst.var_25C0[i].var_02 = src.var_25C0[i].var_02;
            dst.var_25C0[i].var_04 = src.var_25C0[i].var_04;
            dst.var_25C0[i].var_05 = src.var_25C0[i].var_05;
        }

        dst.var_25C0_length = src.var_25C0_length;
        dst.var_85C2 = src.var_85C2;
        dst.var_85C3 = src.var_85C3;
        dst.var_85C4 = src.var_85C4;
        dst.var_85C8 = src.var_85C8;
        dst.var_85C9 = src.var_85C9;
        dst.var_85CD = src.var_85CD;
        dst.var_85CE = src.var_85CE;
        dst.var_85CF = src.var_85CF;
        dst.var_85D0 = src.var_85D0;
        dst.var_85D4 = src.var_85D4;
        dst.var_85D5 = src.var_85D5;
        dst.var_85D7 = src.var_85D7;
        dst.var_85DB = src.var_85DB;
        dst.var_85DC = src.var_85DC;
        dst.var_85DE = src.var_85DE;
        dst.var_85E2 = src.var_85E2;
        dst.var_85E6 = src.var_85E6;
        dst.var_85E8 = src.var_85E8;
        dst.var_85EA = src.var_85EA;
        dst.var_85EE = src.var_85EE;
        dst.var_85EF = src.var_85EF;
        dst.var_85F0 = src.var_85F0;
        dst.var_85F2 = src.var_85F2;
        dst.var_85F6 = src.var_85F6;
        dst.cargoUnitsTotalDelivered = src.cargoUnitsTotalDelivered;
        std::ranges::copy(src.cargoUnitsDeliveredHistory, dst.cargoUnitsDeliveredHistory);
        std::ranges::copy(src.performanceIndexHistory, dst.performanceIndexHistory);
        dst.historySize = src.historySize;
        std::ranges::copy(src.companyValueHistory, dst.companyValueHistory);
        dst.vehicleProfit = src.vehicleProfit;
        std::ranges::copy(src.transportTypeCount, dst.transportTypeCount);
        std::ranges::copy(src.activeEmotions, dst.activeEmotions);
        dst.observationStatus = src.observationStatus;
        dst.observationTownId = enumValue(src.observationTownId);
        dst.observationEntity = enumValue(src.observationEntity);
        dst.observationX = src.observationX;
        dst.observationY = src.observationY;
        dst.observationObject = src.observationObject;
        dst.observationTimeout = src.observationTimeout;
        dst.ownerStatus[0] = src.ownerStatus.data[0];
        dst.ownerStatus[1] = src.ownerStatus.data[1];
        std::ranges::copy(src.cargoDelivered, dst.cargoDelivered);
        dst.challengeProgress = src.challengeProgress;
        dst.numMonthsInTheRed = src.numMonthsInTheRed;
        dst.cargoUnitsTotalDistance = src.cargoUnitsTotalDistance;
        std::ranges::copy(src.cargoUnitsDistanceHistory, dst.cargoUnitsDistanceHistory);
        dst.jailStatus = src.jailStatus;

        return dst;
    }

    S5::Company importCompanyType2(const S5::CompanyType2& src)
    {
        S5::Company dst{};
        dst.name = src.name;
        dst.ownerName = src.ownerName;
        dst.challengeFlags = src.challengeFlags;
        dst.cash = src.cash;
        dst.currentLoan = src.currentLoan;
        dst.updateCounter = src.updateCounter;
        dst.performanceIndex = src.performanceIndex;
        dst.competitorId = src.competitorId;
        dst.ownerEmotion = src.ownerEmotion;
        dst.mainColours[0] = src.mainColours[0];
        dst.mainColours[1] = src.mainColours[1];
        for (auto i = 0U; i < std::size(src.vehicleColours); ++i)
        {
            dst.vehicleColours[i][0] = src.vehicleColours[i][0];
            dst.vehicleColours[i][1] = src.vehicleColours[i][1];
        }
        dst.customVehicleColoursSet = src.customVehicleColoursSet;
        std::ranges::copy(src.unlockedVehicles, dst.unlockedVehicles);
        dst.availableVehicles = src.availableVehicles;
        dst.aiPlaystyleFlags = src.aiPlaystyleFlags;
        dst.aiPlaystyleTownId = src.aiPlaystyleTownId;
        dst.numExpenditureYears = src.numExpenditureYears;
        std::memcpy(dst.expenditures, src.expenditures, kExpenditureHistoryCapacity * ExpenditureType::Count * sizeof(currency32_t));
        dst.startedDate = src.startedDate;
        dst.var_49C = src.var_49C;
        dst.var_4A0 = src.var_4A0;
        dst.var_4A4 = src.var_4A4;
        dst.var_4A5 = src.var_4A5;
        dst.var_4A6 = src.var_4A6;
        dst.var_4A7 = src.var_4A7;

        std::ranges::copy(src.aiThoughts, dst.aiThoughts);

        dst.activeThoughtId = src.activeThoughtId;
        dst.headquartersZ = src.headquartersZ;
        dst.headquartersX = src.headquartersX;
        dst.headquartersY = src.headquartersY;
        dst.activeThoughtRevenueEstimate = src.activeThoughtRevenueEstimate;
        dst.var_2582 = src.var_2582;
        dst.var_2596 = src.var_2596;
        dst.var_259A = src.var_259A;
        dst.var_259B = src.var_259B;
        dst.var_259C = src.var_259C;
        dst.aiPlaceVehicleIndex = src.aiPlaceVehicleIndex;
        dst.var_25BE = src.var_25BE;
        dst.currentRating = src.currentRating;

        // Copy hash table
        for (auto i = 0U; i < std::size(src.var_25C0); ++i)
        {
            dst.var_25C0[i].var_00 = src.var_25C0[i].var_00;
            dst.var_25C0[i].var_02 = src.var_25C0[i].var_02;
            dst.var_25C0[i].var_04 = src.var_25C0[i].var_04;
            dst.var_25C0[i].var_05 = src.var_25C0[i].var_05;
        }

        dst.var_25C0_length = src.var_25C0_length;
        dst.var_85C2 = src.var_85C2;
        dst.var_85C3 = src.var_85C3;
        dst.var_85C4 = src.var_85C4;
        dst.var_85C8 = src.var_85C8;
        dst.var_85C9 = src.var_85C9;
        dst.var_85CD = src.var_85CD;
        dst.var_85CE = src.var_85CE;
        dst.var_85CF = src.var_85CF;
        dst.var_85D0 = src.var_85D0;
        dst.var_85D4 = src.var_85D4;
        dst.var_85D5 = src.var_85D5;
        dst.var_85D7 = src.var_85D7;
        dst.var_85DB = src.var_85DB;
        dst.var_85DC = src.var_85DC;
        dst.var_85DE = src.var_85DE;
        dst.var_85E2 = src.var_85E2;
        dst.var_85E6 = src.var_85E6;
        dst.var_85E8 = src.var_85E8;
        dst.var_85EA = src.var_85EA;
        dst.var_85EE = src.var_85EE;
        dst.var_85EF = src.var_85EF;
        dst.var_85F0 = src.var_85F0;
        dst.var_85F2 = src.var_85F2;
        dst.var_85F6 = src.var_85F6;
        dst.cargoUnitsTotalDelivered = src.cargoUnitsTotalDelivered;
        std::ranges::copy(src.cargoUnitsDeliveredHistory, dst.cargoUnitsDeliveredHistory);
        std::ranges::copy(src.performanceIndexHistory, dst.performanceIndexHistory);
        dst.historySize = src.historySize;
        std::ranges::copy(src.companyValueHistory, dst.companyValueHistory);
        dst.vehicleProfit = src.vehicleProfit;
        std::ranges::copy(src.transportTypeCount, dst.transportTypeCount);
        std::ranges::copy(src.activeEmotions, dst.activeEmotions);
        dst.observationStatus = src.observationStatus;
        dst.observationTownId = src.observationTownId;
        dst.observationEntity = src.observationEntity;
        dst.observationX = src.observationX;
        dst.observationY = src.observationY;
        dst.observationObject = src.observationObject;
        dst.observationTimeout = src.observationTimeout;
        dst.ownerStatus[0] = src.ownerStatus[0];
        dst.ownerStatus[1] = src.ownerStatus[1];
        std::ranges::copy(src.cargoDelivered, dst.cargoDelivered);
        dst.challengeProgress = src.challengeProgress;
        dst.numMonthsInTheRed = src.numMonthsInTheRed;
        dst.cargoUnitsTotalDistance = src.cargoUnitsTotalDistance;
        dst.jailStatus = src.jailStatus;

        return dst;
    }

    S5::Records exportRecords(const CompanyManager::Records& src)
    {
        S5::Records dst{};
        for (auto i = 0; i < 3; i++)
        {
            dst.speed[i] = src.speed[i].getRaw();
            dst.company[i] = enumValue(src.company[i]);
            dst.date[i] = src.date[i];
        }
        return dst;
    }

    OpenLoco::CompanyManager::Records importRecords(const S5::Records& src)
    {
        OpenLoco::CompanyManager::Records dst{};
        for (auto i = 0; i < 3; i++)
        {
            dst.speed[i] = Speed16{ src.speed[i] };
            dst.company[i] = static_cast<CompanyId>(src.company[i]);
            dst.date[i] = src.date[i];
        }
        return dst;
    }

    OpenLoco::Company importCompany(const S5::Company& src)
    {
        OpenLoco::Company dst{};
        dst.name = src.name;
        dst.ownerName = src.ownerName;
        dst.challengeFlags = static_cast<CompanyFlags>(src.challengeFlags);
        dst.cash = src.cash;
        dst.currentLoan = src.currentLoan;
        dst.updateCounter = src.updateCounter;
        dst.performanceIndex = src.performanceIndex;
        dst.competitorId = src.competitorId;
        dst.ownerEmotion = static_cast<Emotion>(src.ownerEmotion);
        dst.mainColours.primary = static_cast<Colour>(src.mainColours[0]);
        dst.mainColours.secondary = static_cast<Colour>(src.mainColours[1]);
        for (auto i = 0U; i < std::size(src.vehicleColours); ++i)
        {
            dst.vehicleColours[i].primary = static_cast<Colour>(src.vehicleColours[i][0]);
            dst.vehicleColours[i].secondary = static_cast<Colour>(src.vehicleColours[i][1]);
        }
        dst.customVehicleColoursSet = src.customVehicleColoursSet;
        for (auto i = 0U; i < 224; ++i)
        {
            dst.unlockedVehicles[i] = (src.unlockedVehicles[i / 32] >> static_cast<uint32_t>(i % 32)) & 0x1;
        }
        dst.availableVehicles = src.availableVehicles;
        dst.aiPlaystyleFlags = static_cast<AiPlaystyleFlags>(src.aiPlaystyleFlags);
        dst.aiPlaystyleTownId = src.aiPlaystyleTownId;
        dst.numExpenditureYears = src.numExpenditureYears;
        std::memcpy(dst.expenditures, src.expenditures, kExpenditureHistoryCapacity * ExpenditureType::Count * sizeof(currency32_t));
        dst.startedDate = src.startedDate;
        dst.var_49C = src.var_49C;
        dst.var_4A0 = src.var_4A0;
        dst.var_4A4 = static_cast<AiThinkState>(src.var_4A4);
        dst.var_4A5 = src.var_4A5;
        dst.var_4A6 = static_cast<AiPlaceVehicleState>(src.var_4A6);
        dst.var_4A7 = src.var_4A7;
        // Copy AI thoughts
        for (auto i = 0U; i < kMaxAiThoughts; ++i)
        {
            auto& srcThought = src.aiThoughts[i];
            auto& dstThought = dst.aiThoughts[i];
            dstThought.type = static_cast<AiThoughtType>(srcThought.type);
            dstThought.destinationA = srcThought.destinationA;
            dstThought.destinationB = srcThought.destinationB;
            dstThought.numStations = srcThought.numStations;
            dstThought.stationLength = srcThought.stationLength;
            for (auto j = 0U; j < std::size(srcThought.stations); ++j)
            {
                dstThought.stations[j].id = static_cast<StationId>(srcThought.stations[j].id);
                dstThought.stations[j].var_02 = static_cast<AiThoughtStationFlags>(srcThought.stations[j].var_02);
                dstThought.stations[j].rotation = srcThought.stations[j].rotation;
                dstThought.stations[j].pos = srcThought.stations[j].pos;
                dstThought.stations[j].baseZ = srcThought.stations[j].baseZ;
                dstThought.stations[j].var_9 = srcThought.stations[j].var_9;
                dstThought.stations[j].var_A = srcThought.stations[j].var_A;
                dstThought.stations[j].var_B = srcThought.stations[j].var_B;
                dstThought.stations[j].var_C = srcThought.stations[j].var_C;
            }
            dstThought.trackObjId = srcThought.trackObjId;
            dstThought.rackRailType = srcThought.rackRailType;
            dstThought.mods = srcThought.mods;
            dstThought.cargoType = srcThought.cargoType;
            dstThought.var_43 = srcThought.var_43;
            dstThought.numVehicles = srcThought.numVehicles;
            dstThought.var_45 = srcThought.var_45;
            std::ranges::copy(srcThought.var_46, dstThought.var_46);
            for (auto j = 0U; j < std::size(srcThought.vehicles); ++j)
            {
                dstThought.vehicles[j] = static_cast<EntityId>(srcThought.vehicles[j]);
            }
            dstThought.var_76 = srcThought.var_76;
            dstThought.var_7C = srcThought.var_7C;
            dstThought.var_80 = srcThought.var_80;
            dstThought.var_84 = srcThought.var_84;
            dstThought.var_88 = srcThought.var_88;
            dstThought.stationObjId = srcThought.stationObjId;
            dstThought.signalObjId = srcThought.signalObjId;
            dstThought.purchaseFlags = static_cast<AiPurchaseFlags>(srcThought.purchaseFlags);
        }
        dst.activeThoughtId = src.activeThoughtId;
        dst.headquartersZ = src.headquartersZ;
        dst.headquartersX = src.headquartersX;
        dst.headquartersY = src.headquartersY;
        dst.activeThoughtRevenueEstimate = src.activeThoughtRevenueEstimate;
        dst.var_2582 = src.var_2582;
        dst.var_2596 = src.var_2596;
        dst.var_259A = src.var_259A;
        dst.var_259B = src.var_259B;
        dst.var_259C = src.var_259C;
        dst.aiPlaceVehicleIndex = src.aiPlaceVehicleIndex;
        dst.var_25BE = static_cast<AiThoughtType>(src.var_25BE);
        dst.currentRating = static_cast<CorporateRating>(src.currentRating);
        // Copy hash table
        for (auto i = 0U; i < std::size(src.var_25C0); ++i)
        {
            dst.var_25C0[i].var_00 = src.var_25C0[i].var_00;
            dst.var_25C0[i].var_02 = src.var_25C0[i].var_02;
            dst.var_25C0[i].var_04 = src.var_25C0[i].var_04;
            dst.var_25C0[i].var_05 = src.var_25C0[i].var_05;
        }
        dst.var_25C0_length = src.var_25C0_length;
        dst.var_85C2 = src.var_85C2;
        dst.var_85C3 = src.var_85C3;
        dst.var_85C4 = src.var_85C4;
        dst.var_85C8 = src.var_85C8;
        dst.var_85C9 = src.var_85C9;
        dst.var_85CD = src.var_85CD;
        dst.var_85CE = src.var_85CE;
        dst.var_85CF = src.var_85CF;
        dst.var_85D0 = src.var_85D0;
        dst.var_85D4 = src.var_85D4;
        dst.var_85D5 = src.var_85D5;
        dst.var_85D7 = src.var_85D7;
        dst.var_85DB = src.var_85DB;
        dst.var_85DC = src.var_85DC;
        dst.var_85DE = src.var_85DE;
        dst.var_85E2 = src.var_85E2;
        dst.var_85E6 = src.var_85E6;
        dst.var_85E8 = src.var_85E8;
        dst.var_85EA = src.var_85EA;
        dst.var_85EE = src.var_85EE;
        dst.var_85EF = src.var_85EF;
        dst.var_85F0 = src.var_85F0;
        dst.var_85F2 = src.var_85F2;
        dst.var_85F6 = src.var_85F6;
        dst.cargoUnitsTotalDelivered = src.cargoUnitsTotalDelivered;
        std::ranges::copy(src.cargoUnitsDeliveredHistory, dst.cargoUnitsDeliveredHistory);
        std::ranges::copy(src.performanceIndexHistory, dst.performanceIndexHistory);
        dst.historySize = src.historySize;
        std::ranges::copy(src.companyValueHistory, dst.companyValueHistory);
        dst.vehicleProfit = src.vehicleProfit;
        std::ranges::copy(src.transportTypeCount, dst.transportTypeCount);
        std::ranges::copy(src.activeEmotions, dst.activeEmotions);
        dst.observationStatus = static_cast<ObservationStatus>(src.observationStatus);
        dst.observationTownId = static_cast<TownId>(src.observationTownId);
        dst.observationEntity = static_cast<EntityId>(src.observationEntity);
        dst.observationX = src.observationX;
        dst.observationY = src.observationY;
        dst.observationObject = src.observationObject;
        dst.observationTimeout = src.observationTimeout;
        dst.ownerStatus.data[0] = src.ownerStatus[0];
        dst.ownerStatus.data[1] = src.ownerStatus[1];
        std::ranges::copy(src.cargoDelivered, dst.cargoDelivered);
        dst.challengeProgress = src.challengeProgress;
        dst.numMonthsInTheRed = src.numMonthsInTheRed;
        dst.cargoUnitsTotalDistance = src.cargoUnitsTotalDistance;
        std::ranges::copy(src.cargoUnitsDistanceHistory, dst.cargoUnitsDistanceHistory);
        dst.jailStatus = src.jailStatus;

        return dst;
    }
}
