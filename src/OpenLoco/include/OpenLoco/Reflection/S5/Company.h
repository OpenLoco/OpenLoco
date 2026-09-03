#pragma once

#include <OpenLoco/Core/Reflection.hpp>
#include <OpenLoco/S5/S5Company.h>

namespace OpenLoco
{
    template<>
    struct Reflection<S5::AiThought::Station>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::AiThought::Station, id),
            REFL_FIELD(S5::AiThought::Station, var_02),
            REFL_FIELD(S5::AiThought::Station, rotation),
            REFL_FIELD(S5::AiThought::Station, pos),
            REFL_FIELD(S5::AiThought::Station, baseZ),
            REFL_FIELD(S5::AiThought::Station, var_9),
            REFL_FIELD(S5::AiThought::Station, var_A),
            REFL_FIELD(S5::AiThought::Station, var_B),
            REFL_FIELD(S5::AiThought::Station, var_C),
            REFL_FIELD(S5::AiThought::Station, pad_D)>;
    };

    template<>
    struct Reflection<S5::AiThought>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::AiThought, type),
            REFL_FIELD(S5::AiThought, destinationA),
            REFL_FIELD(S5::AiThought, destinationB),
            REFL_FIELD(S5::AiThought, numStations),
            REFL_FIELD(S5::AiThought, stationLength),
            REFL_FIELD(S5::AiThought, pad_05),
            REFL_FIELD(S5::AiThought, stations),
            REFL_FIELD(S5::AiThought, trackObjId),
            REFL_FIELD(S5::AiThought, rackRailType),
            REFL_FIELD(S5::AiThought, mods),
            REFL_FIELD(S5::AiThought, cargoType),
            REFL_FIELD(S5::AiThought, var_43),
            REFL_FIELD(S5::AiThought, numVehicles),
            REFL_FIELD(S5::AiThought, var_45),
            REFL_FIELD(S5::AiThought, var_46),
            REFL_FIELD(S5::AiThought, vehicles),
            REFL_FIELD(S5::AiThought, var_76),
            REFL_FIELD(S5::AiThought, pad_7A),
            REFL_FIELD(S5::AiThought, var_7C),
            REFL_FIELD(S5::AiThought, var_80),
            REFL_FIELD(S5::AiThought, var_84),
            REFL_FIELD(S5::AiThought, var_88),
            REFL_FIELD(S5::AiThought, stationObjId),
            REFL_FIELD(S5::AiThought, signalObjId),
            REFL_FIELD(S5::AiThought, purchaseFlags)>;
    };

    template<>
    struct Reflection<S5::Company::HashTableEntry>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Company::HashTableEntry, var_00),
            REFL_FIELD(S5::Company::HashTableEntry, var_02),
            REFL_FIELD(S5::Company::HashTableEntry, var_04),
            REFL_FIELD(S5::Company::HashTableEntry, var_05)>;
    };

    template<>
    struct Reflection<S5::Company>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Company, name),
            REFL_FIELD(S5::Company, ownerName),
            REFL_FIELD(S5::Company, challengeFlags),
            REFL_FIELD(S5::Company, cash),
            REFL_FIELD(S5::Company, currentLoan),
            REFL_FIELD(S5::Company, updateCounter),
            REFL_FIELD(S5::Company, performanceIndex),
            REFL_FIELD(S5::Company, competitorId),
            REFL_FIELD(S5::Company, ownerEmotion),
            REFL_FIELD(S5::Company, mainColours),
            REFL_FIELD(S5::Company, vehicleColours),
            REFL_FIELD(S5::Company, customVehicleColoursSet),
            REFL_FIELD(S5::Company, unlockedVehicles),
            REFL_FIELD(S5::Company, availableVehicles),
            REFL_FIELD(S5::Company, aiPlaystyleFlags),
            REFL_FIELD(S5::Company, aiPlaystyleTownId),
            REFL_FIELD(S5::Company, numExpenditureYears),
            REFL_FIELD(S5::Company, expenditures),
            REFL_FIELD(S5::Company, startedDate),
            REFL_FIELD(S5::Company, var_49C),
            REFL_FIELD(S5::Company, var_4A0),
            REFL_FIELD(S5::Company, var_4A4),
            REFL_FIELD(S5::Company, var_4A5),
            REFL_FIELD(S5::Company, var_4A6),
            REFL_FIELD(S5::Company, var_4A7),
            REFL_FIELD(S5::Company, aiThoughts),
            REFL_FIELD(S5::Company, activeThoughtId),
            REFL_FIELD(S5::Company, headquartersZ),
            REFL_FIELD(S5::Company, headquartersX),
            REFL_FIELD(S5::Company, headquartersY),
            REFL_FIELD(S5::Company, activeThoughtRevenueEstimate),
            REFL_FIELD(S5::Company, var_2582),
            REFL_FIELD(S5::Company, pad_2586),
            REFL_FIELD(S5::Company, var_2596),
            REFL_FIELD(S5::Company, var_259A),
            REFL_FIELD(S5::Company, var_259B),
            REFL_FIELD(S5::Company, var_259C),
            REFL_FIELD(S5::Company, pad_259D),
            REFL_FIELD(S5::Company, aiPlaceVehicleIndex),
            REFL_FIELD(S5::Company, pad_25A2),
            REFL_FIELD(S5::Company, var_25BE),
            REFL_FIELD(S5::Company, currentRating),
            REFL_FIELD(S5::Company, var_25C0),
            REFL_FIELD(S5::Company, var_25C0_length),
            REFL_FIELD(S5::Company, var_85C2),
            REFL_FIELD(S5::Company, var_85C3),
            REFL_FIELD(S5::Company, var_85C4),
            REFL_FIELD(S5::Company, var_85C8),
            REFL_FIELD(S5::Company, var_85C9),
            REFL_FIELD(S5::Company, var_85CD),
            REFL_FIELD(S5::Company, var_85CE),
            REFL_FIELD(S5::Company, var_85CF),
            REFL_FIELD(S5::Company, var_85D0),
            REFL_FIELD(S5::Company, var_85D4),
            REFL_FIELD(S5::Company, var_85D5),
            REFL_FIELD(S5::Company, var_85D7),
            REFL_FIELD(S5::Company, var_85DB),
            REFL_FIELD(S5::Company, var_85DC),
            REFL_FIELD(S5::Company, var_85DE),
            REFL_FIELD(S5::Company, var_85E2),
            REFL_FIELD(S5::Company, var_85E6),
            REFL_FIELD(S5::Company, var_85E8),
            REFL_FIELD(S5::Company, var_85EA),
            REFL_FIELD(S5::Company, var_85EE),
            REFL_FIELD(S5::Company, var_85EF),
            REFL_FIELD(S5::Company, var_85F0),
            REFL_FIELD(S5::Company, var_85F2),
            REFL_FIELD(S5::Company, var_85F6),
            REFL_FIELD(S5::Company, cargoUnitsTotalDelivered),
            REFL_FIELD(S5::Company, cargoUnitsDeliveredHistory),
            REFL_FIELD(S5::Company, performanceIndexHistory),
            REFL_FIELD(S5::Company, historySize),
            REFL_FIELD(S5::Company, companyValueHistory),
            REFL_FIELD(S5::Company, vehicleProfit),
            REFL_FIELD(S5::Company, transportTypeCount),
            REFL_FIELD(S5::Company, activeEmotions),
            REFL_FIELD(S5::Company, observationStatus),
            REFL_FIELD(S5::Company, observationTownId),
            REFL_FIELD(S5::Company, observationEntity),
            REFL_FIELD(S5::Company, observationX),
            REFL_FIELD(S5::Company, observationY),
            REFL_FIELD(S5::Company, observationObject),
            REFL_FIELD(S5::Company, observationTimeout),
            REFL_FIELD(S5::Company, ownerStatus),
            REFL_FIELD(S5::Company, pad_8BCA),
            REFL_FIELD(S5::Company, cargoDelivered),
            REFL_FIELD(S5::Company, challengeProgress),
            REFL_FIELD(S5::Company, numMonthsInTheRed),
            REFL_FIELD(S5::Company, cargoUnitsTotalDistance),
            REFL_FIELD(S5::Company, cargoUnitsDistanceHistory),
            REFL_FIELD(S5::Company, jailStatus),
            REFL_FIELD(S5::Company, pad_8E36)>;
    };

    template<>
    struct Reflection<S5::CompanyType2::HashTableEntry>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::CompanyType2::HashTableEntry, var_00),
            REFL_FIELD(S5::CompanyType2::HashTableEntry, var_02),
            REFL_FIELD(S5::CompanyType2::HashTableEntry, var_04),
            REFL_FIELD(S5::CompanyType2::HashTableEntry, var_05)>;
    };

    template<>
    struct Reflection<S5::CompanyType2>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::CompanyType2, name),
            REFL_FIELD(S5::CompanyType2, ownerName),
            REFL_FIELD(S5::CompanyType2, challengeFlags),
            REFL_FIELD(S5::CompanyType2, cash),
            REFL_FIELD(S5::CompanyType2, currentLoan),
            REFL_FIELD(S5::CompanyType2, updateCounter),
            REFL_FIELD(S5::CompanyType2, performanceIndex),
            REFL_FIELD(S5::CompanyType2, competitorId),
            REFL_FIELD(S5::CompanyType2, ownerEmotion),
            REFL_FIELD(S5::CompanyType2, mainColours),
            REFL_FIELD(S5::CompanyType2, vehicleColours),
            REFL_FIELD(S5::CompanyType2, customVehicleColoursSet),
            REFL_FIELD(S5::CompanyType2, unlockedVehicles),
            REFL_FIELD(S5::CompanyType2, availableVehicles),
            REFL_FIELD(S5::CompanyType2, aiPlaystyleFlags),
            REFL_FIELD(S5::CompanyType2, aiPlaystyleTownId),
            REFL_FIELD(S5::CompanyType2, numExpenditureYears),
            REFL_FIELD(S5::CompanyType2, expenditures),
            REFL_FIELD(S5::CompanyType2, startedDate),
            REFL_FIELD(S5::CompanyType2, var_49C),
            REFL_FIELD(S5::CompanyType2, var_4A0),
            REFL_FIELD(S5::CompanyType2, var_4A4),
            REFL_FIELD(S5::CompanyType2, var_4A5),
            REFL_FIELD(S5::CompanyType2, var_4A6),
            REFL_FIELD(S5::CompanyType2, var_4A7),
            REFL_FIELD(S5::CompanyType2, aiThoughts),
            REFL_FIELD(S5::CompanyType2, activeThoughtId),
            REFL_FIELD(S5::CompanyType2, headquartersZ),
            REFL_FIELD(S5::CompanyType2, headquartersX),
            REFL_FIELD(S5::CompanyType2, headquartersY),
            REFL_FIELD(S5::CompanyType2, activeThoughtRevenueEstimate),
            REFL_FIELD(S5::CompanyType2, var_2582),
            REFL_FIELD(S5::CompanyType2, pad_2586),
            REFL_FIELD(S5::CompanyType2, var_2596),
            REFL_FIELD(S5::CompanyType2, var_259A),
            REFL_FIELD(S5::CompanyType2, var_259B),
            REFL_FIELD(S5::CompanyType2, var_259C),
            REFL_FIELD(S5::CompanyType2, pad_259D),
            REFL_FIELD(S5::CompanyType2, aiPlaceVehicleIndex),
            REFL_FIELD(S5::CompanyType2, pad_25A2),
            REFL_FIELD(S5::CompanyType2, var_25BE),
            REFL_FIELD(S5::CompanyType2, currentRating),
            REFL_FIELD(S5::CompanyType2, var_25C0),
            REFL_FIELD(S5::CompanyType2, var_25C0_length),
            REFL_FIELD(S5::CompanyType2, var_85C2),
            REFL_FIELD(S5::CompanyType2, var_85C3),
            REFL_FIELD(S5::CompanyType2, var_85C4),
            REFL_FIELD(S5::CompanyType2, var_85C8),
            REFL_FIELD(S5::CompanyType2, var_85C9),
            REFL_FIELD(S5::CompanyType2, var_85CD),
            REFL_FIELD(S5::CompanyType2, var_85CE),
            REFL_FIELD(S5::CompanyType2, var_85CF),
            REFL_FIELD(S5::CompanyType2, var_85D0),
            REFL_FIELD(S5::CompanyType2, var_85D4),
            REFL_FIELD(S5::CompanyType2, var_85D5),
            REFL_FIELD(S5::CompanyType2, var_85D7),
            REFL_FIELD(S5::CompanyType2, var_85DB),
            REFL_FIELD(S5::CompanyType2, var_85DC),
            REFL_FIELD(S5::CompanyType2, var_85DE),
            REFL_FIELD(S5::CompanyType2, var_85E2),
            REFL_FIELD(S5::CompanyType2, var_85E6),
            REFL_FIELD(S5::CompanyType2, var_85E8),
            REFL_FIELD(S5::CompanyType2, var_85EA),
            REFL_FIELD(S5::CompanyType2, var_85EE),
            REFL_FIELD(S5::CompanyType2, var_85EF),
            REFL_FIELD(S5::CompanyType2, var_85F0),
            REFL_FIELD(S5::CompanyType2, var_85F2),
            REFL_FIELD(S5::CompanyType2, var_85F6),
            REFL_FIELD(S5::CompanyType2, cargoUnitsTotalDelivered),
            REFL_FIELD(S5::CompanyType2, cargoUnitsDeliveredHistory),
            REFL_FIELD(S5::CompanyType2, performanceIndexHistory),
            REFL_FIELD(S5::CompanyType2, historySize),
            REFL_FIELD(S5::CompanyType2, companyValueHistory),
            REFL_FIELD(S5::CompanyType2, vehicleProfit),
            REFL_FIELD(S5::CompanyType2, transportTypeCount),
            REFL_FIELD(S5::CompanyType2, activeEmotions),
            REFL_FIELD(S5::CompanyType2, observationStatus),
            REFL_FIELD(S5::CompanyType2, observationTownId),
            REFL_FIELD(S5::CompanyType2, observationEntity),
            REFL_FIELD(S5::CompanyType2, observationX),
            REFL_FIELD(S5::CompanyType2, observationY),
            REFL_FIELD(S5::CompanyType2, observationObject),
            REFL_FIELD(S5::CompanyType2, observationTimeout),
            REFL_FIELD(S5::CompanyType2, ownerStatus),
            REFL_FIELD(S5::CompanyType2, pad_8BCA),
            REFL_FIELD(S5::CompanyType2, cargoDelivered),
            REFL_FIELD(S5::CompanyType2, challengeProgress),
            REFL_FIELD(S5::CompanyType2, numMonthsInTheRed),
            REFL_FIELD(S5::CompanyType2, cargoUnitsTotalDistance),
            REFL_FIELD(S5::CompanyType2, jailStatus),
            REFL_FIELD(S5::CompanyType2, pad_8E36)>;
    };

    template<>
    struct Reflection<S5::Records>
    {
        using Fields = FieldList<
            REFL_FIELD(S5::Records, speed),
            REFL_FIELD(S5::Records, company),
            REFL_FIELD(S5::Records, pad_43A),
            REFL_FIELD(S5::Records, date)>;
    };
}
