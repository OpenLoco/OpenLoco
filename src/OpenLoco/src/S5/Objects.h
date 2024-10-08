#pragma once
#include <cstdint>

namespace OpenLoco::S5
{

#pragma pack(push, 1)
    struct AirportObject
    {
        uint16_t name;
        int16_t buildCostFactor; // 0x02
        int16_t sellCostFactor;  // 0x04
        uint8_t costIndex;       // 0x06
        uint8_t var_07;
        uint32_t image; // 0x08
        uint32_t var_0C;
        uint16_t allowedPlaneTypes; // 0x10
        uint8_t numSpriteSets;      // 0x12
        uint8_t numTiles;           // 0x13
        uint32_t var_14ptr;
        uint32_t var_18ptr;
        uint32_t var_1Cptrs[32];
        uint32_t var_9Cptr;
        uint32_t largeTiles;       // 0xA0
        int8_t minX;               // 0xA4
        int8_t minY;               // 0xA5
        int8_t maxX;               // 0xA6
        int8_t maxY;               // 0xA7
        uint16_t designedYear;     // 0xA8
        uint16_t obsoleteYear;     // 0xAA
        uint8_t numMovementNodes;  // 0xAC
        uint8_t numMovementEdges;  // 0xAD
        uint32_t movementNodesPtr; // 0xAE
        uint32_t movementEdgesPtr; // 0xB2
        uint32_t var_B6;
    };
    static_assert(sizeof(AirportObject) == 0xBA);

    struct BridgeObject
    {
        uint16_t name;
        uint8_t noRoof; // 0x02
        uint8_t pad_03;
        uint16_t clearHeight;       // 0x04
        uint16_t var_06;            // 0x06
        uint8_t spanLength;         // 0x08
        uint8_t pillarSpacing;      // 0x09
        uint16_t maxSpeed;          // 0x0A
        int8_t maxHeight;           // 0x0C MicroZ!
        uint8_t costIndex;          // 0x0D
        int16_t baseCostFactor;     // 0x0E
        int16_t heightCostFactor;   // 0x10
        int16_t sellCostFactor;     // 0x12
        uint16_t disabledTrackCfg;  // 0x14
        uint32_t image;             // 0x16
        uint8_t trackNumCompatible; // 0x1A
        uint8_t trackMods[7];       // 0x1B
        uint8_t roadNumCompatible;  // 0x22
        uint8_t roadMods[7];        // 0x23
        uint16_t designedYear;      // 0x2A
    };
    static_assert(sizeof(BridgeObject) == 0x2C);

    struct BuildingObject
    {
        uint16_t name;                           // 0x0
        uint32_t image;                          // 0x2
        uint8_t numParts;                        // 0x6
        uint8_t numVariations;                   // 0x7
        uint32_t partHeightsPtr;                 // 0x8
        uint32_t partAnimationsPtr;              // 0xC
        uint32_t variationPartsPtrs[32];         // 0x10 Access with getBuildingParts helper method
        uint32_t colours;                        // 0x90
        uint16_t designedYear;                   // 0x94
        uint16_t obsoleteYear;                   // 0x96
        uint8_t flags;                           // 0x98
        uint8_t clearCostIndex;                  // 0x99
        uint16_t clearCostFactor;                // 0x9A
        uint8_t scaffoldingSegmentType;          // 0x9C
        uint8_t scaffoldingColour;               // 0x9D
        uint8_t generatorFunction;               // 0x9E
        uint8_t averageNumberOnMap;              // 0x9F
        uint8_t producedQuantity[2];             // 0xA0
        uint8_t producedCargoType[2];            // 0xA2
        uint8_t requiredCargoType[2];            // 0xA4
        uint8_t var_A6[2];                       // 0xA6
        uint8_t var_A8[2];                       // 0xA8
        int16_t demolishRatingReduction;         // 0XAA
        uint8_t var_AC;                          // 0xAC
        uint8_t numElevatorSequences;            // 0XAD
        uint32_t elevatorHeightSequencesPtrs[4]; // 0XAE Access with getElevatorHeightSequence helper method
    };
    static_assert(sizeof(BuildingObject) == 0xBE);

    struct CargoObject
    {
        uint16_t name; // 0x0
        uint16_t var_2;
        uint16_t cargoTransferTime;
        uint16_t unitsAndCargoName;    // 0x6
        uint16_t unitNameSingular;     // 0x8
        uint16_t unitNamePlural;       // 0xA
        uint32_t unitInlineSprite;     // 0xC
        uint16_t cargoCategory;        // 0x10
        uint8_t flags;                 // 0x12
        uint8_t numPlatformVariations; // 0x13
        uint8_t var_14;
        uint8_t premiumDays;       // 0x15
        uint8_t maxNonPremiumDays; // 0x16
        uint16_t nonPremiumRate;   // 0x17
        uint16_t penaltyRate;      // 0x19
        uint16_t paymentFactor;    // 0x1B
        uint8_t paymentIndex;      // 0x1D
        uint8_t unitSize;          // 0x1E
    };
    static_assert(sizeof(CargoObject) == 0x1F);

    struct CliffEdgeObject
    {

        uint16_t name;
        uint32_t image; // 0x02
    };
    static_assert(sizeof(CliffEdgeObject) == 0x6);

    struct ClimateObject
    {
        uint16_t name;           // 0x00
        uint8_t firstSeason;     // 0x02
        uint8_t seasonLength[4]; // 0x03
        uint8_t winterSnowLine;  // 0x07
        uint8_t summerSnowLine;  // 0x08
        uint8_t pad_09;
    };
    static_assert(sizeof(ClimateObject) == 0xA);

    struct CompetitorObject
    {
        uint16_t name;           // 0x00
        uint16_t lastName;       // 0x02
        uint32_t var_04;         // 0x04
        uint32_t var_08;         // 0x08
        uint32_t emotions;       // 0x0C
        uint32_t images[9];      // 0x10
        uint8_t intelligence;    // 0x34
        uint8_t aggressiveness;  // 0x35
        uint8_t competitiveness; // 0x36
        uint8_t var_37;          // 0x37
    };
    static_assert(sizeof(CompetitorObject) == 0x38);

    struct CurrencyObject
    {
        uint16_t name;         // 0x00
        uint16_t prefixSymbol; // 0x02
        uint16_t suffixSymbol; // 0x04
        uint32_t objectIcon;   // 0x06
        uint8_t separator;     // 0x0A
        uint8_t factor;        // 0x0B
    };
    static_assert(sizeof(CurrencyObject) == 0xC);

    struct DockObject
    {
        uint16_t name;
        int16_t buildCostFactor; // 0x02
        int16_t sellCostFactor;  // 0x04
        uint8_t costIndex;       // 0x06
        uint8_t var_07;
        uint32_t image; // 0x08
        uint32_t var_0C;
        uint16_t flags;      // 0x10
        uint8_t numAux01;    // 0x12
        uint8_t numAux02Ent; // 0x13 must be 1 or 0
        uint32_t var_14Ptr;
        uint32_t var_18Ptr;
        uint32_t var_1CPtrs[1]; // odd that this is size 1 but that is how its used
        uint16_t designedYear;  // 0x20
        uint16_t obsoleteYear;  // 0x22
        int16_t boatPositionX;  // 0x24
        int16_t boatPositionY;  // 0x24
    };
    static_assert(sizeof(DockObject) == 0x28);

    struct HillShapesObject
    {
        uint16_t name;
        uint8_t hillHeightMapCount;     // 0x02
        uint8_t mountainHeightMapCount; // 0x03
        uint32_t image;                 // 0x04
        uint32_t imageHills;            // 0x08
        uint16_t flags;                 // 0x0C
    };
    static_assert(sizeof(HillShapesObject) == 0xE);

    struct IndustryObjectProductionRateRange
    {
        uint16_t min;
        uint16_t max;
    };
    static_assert(sizeof(IndustryObjectProductionRateRange) == 0x4);

    struct IndustryObject
    {
        static constexpr auto kObjectType = ObjectType::industry;

        uint16_t name;               // 0x0
        uint16_t var_02;             // 0x2
        uint16_t nameClosingDown;    // 0x4
        uint16_t nameUpProduction;   // 0x6
        uint16_t nameDownProduction; // 0x8
        uint16_t nameSingular;       // 0x0A
        uint16_t namePlural;         // 0x0C
        uint32_t var_0E;             // 0x0E shadows image id base
        uint32_t var_12;             // 0x12 Base image id for building 0
        uint32_t var_16;
        uint32_t var_1A;
        uint8_t numBuildingParts;                // 0x1E
        uint8_t numBuildingVariations;           // 0x1F
        uint32_t buildingPartHeightsPtr;         // 0x20 This is the height of a building image
        uint32_t buildingPartAnimationsPtr;      // 0x24
        uint32_t animationSequencesPtrs[4];      // 0x28 Access with getAnimationSequence helper method
        uint32_t var_38Ptr;                      // 0x38 Access with getUnk38 helper method
        uint32_t buildingVariationPartsPtrs[32]; // 0x3C Access with getBuildingParts helper method
        uint8_t minNumBuildings;                 // 0xBC
        uint8_t maxNumBuildings;                 // 0xBD
        uint32_t buildingsPtr;                   // 0xBE
        uint32_t availableColours;               // 0xC2 bitset
        uint32_t buildingSizeFlags;              // 0xC6 flags indicating the building types size 1:large4x4, 0:small1x1
        uint16_t designedYear;                   // 0xCA start year
        uint16_t obsoleteYear;                   // 0xCC end year
        // Total industries of this type that can be created in a scenario
        // Note: this is not directly comparable to total industries and varies based
        // on scenario total industries cap settings. At low industries cap this value is ~3x the
        // amount of industries in a scenario.
        uint8_t totalOfTypeInScenario;                              // 0xCE
        uint8_t costIndex;                                          // 0xCF
        int16_t costFactor;                                         // 0xD0
        int16_t clearCostFactor;                                    // 0xD2
        uint8_t scaffoldingSegmentType;                             // 0xD4
        uint8_t scaffoldingColour;                                  // 0xD5
        IndustryObjectProductionRateRange initialProductionRate[2]; // 0xD6
        uint8_t producedCargoType[2];                               // 0xDE (0xFF = null)
        uint8_t requiredCargoType[3];                               // 0xE0 (0xFF = null)
        uint8_t mapColour;                                          // 0xE3
        uint32_t flags;                                             // 0xE4
        uint8_t var_E8;
        uint8_t var_E9;
        uint8_t var_EA;
        uint8_t var_EB;
        uint8_t var_EC;       // Used by Livestock cow shed count??
        uint8_t wallTypes[4]; // 0xED There can be up to 4 different wall types for an industry
        // Selection of wall types isn't completely random from the 4 it is biased into 2 groups of 2 (wall and entrance)
        uint8_t buildingWall;         // 0xF1
        uint8_t buildingWallEntrance; // 0xF2 An alternative wall type that looks like a gate placed at random places in building perimeter
        uint8_t var_F3;
    };
    static_assert(sizeof(IndustryObject) == 0xF4);

    struct InterfaceSkinObject
    {
        uint16_t name; // 0x00
        uint32_t img;  // 0x02
        uint8_t colour_06;
        uint8_t colour_07;
        uint8_t tooltipColour; // 0x08
        uint8_t errorColour;   // 0x09
        uint8_t colour_0A;
        uint8_t colour_0B;
        uint8_t colour_0C;
        uint8_t colour_0D;
        uint8_t colour_0E;
        uint8_t colour_0F;
        uint8_t colour_10;
        uint8_t colour_11;
        uint8_t colour_12;
        uint8_t colour_13;
        uint8_t colour_14;
        uint8_t colour_15;
        uint8_t colour_16;
        uint8_t colour_17;
    };
    static_assert(sizeof(InterfaceSkinObject) == 0x18);

    struct LandObject
    {
        uint16_t name;
        uint8_t costIndex; // 0x02
        uint8_t var_03;
        uint8_t var_04;
        uint8_t flags; // 0x05
        uint8_t cliffEdgeHeader1;
        uint8_t cliffEdgeHeader2;
        int8_t costFactor; // 0x08
        uint8_t pad_09;
        uint32_t image; // 0x0A
        uint32_t var_0E;
        uint32_t cliffEdgeImage;
        uint32_t mapPixelImage;      // 0x16
        uint8_t distributionPattern; // 0x1A
        uint8_t numVariations;       // 0x1B
        uint8_t variationLikelihood; // 0x1C
        uint8_t pad_1D;
    };
    static_assert(sizeof(LandObject) == 0x1E);

    struct LevelCrossingObject
    {
        uint16_t name;
        int16_t costFactor;     // 0x02
        int16_t sellCostFactor; // 0x04
        uint8_t costIndex;      // 0x06
        uint8_t animationSpeed; // 0x07
        uint8_t closingFrames;  // 0x08
        uint8_t closedFrames;   // 0x09
        uint8_t pad_0A[0x0C - 0x0A];
        uint16_t designedYear; // 0x0C
        uint32_t image;        // 0x0E
    };
    static_assert(sizeof(LevelCrossingObject) == 0x12);

    struct RegionObject
    {
        uint16_t name;
        uint32_t image; // 0x02
        uint8_t pad_06[0x8 - 0x6];
        uint8_t var_08;
        uint8_t var_09[4];
        uint8_t pad_0D[0x12 - 0xD];
    };
    static_assert(sizeof(RegionObject) == 0x12);

    struct RoadExtraObject
    {
        uint16_t name;           // 0x00
        uint16_t roadPieces;     // 0x02
        uint8_t paintStyle;      // 0x04
        uint8_t costIndex;       // 0x05
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint32_t image;          // 0x0A
        uint32_t var_0E;
    };
    static_assert(sizeof(RoadExtraObject) == 0x12);

    struct RoadObject
    {
        uint16_t name;
        uint16_t roadPieces;      // 0x02
        int16_t buildCostFactor;  // 0x04
        int16_t sellCostFactor;   // 0x06
        int16_t tunnelCostFactor; // 0x08
        uint8_t costIndex;        // 0x0A
        uint8_t tunnel;           // 0x0B
        uint16_t maxSpeed;        // 0x0C
        uint32_t image;           // 0x0E
        uint16_t flags;           // 0x12
        uint8_t numBridges;       // 0x14
        uint8_t bridges[7];       // 0x15
        uint8_t numStations;      // 0x1C
        uint8_t stations[7];      // 0x1D
        uint8_t paintStyle;       // 0x24
        uint8_t numMods;          // 0x25
        uint8_t mods[2];          // 0x26
        uint8_t numCompatible;    // 0x28
        uint8_t pad_29;
        uint16_t compatibleRoads;  // 0x2A
        uint16_t compatibleTracks; // 0x2C
        uint8_t targetTownSize;    // 0x2E
        uint8_t pad_2F;
    };
    static_assert(sizeof(RoadObject) == 0x30);

    struct RoadStationObject
    {
        uint16_t name;           // 0x00
        uint8_t paintStyle;      // 0x02
        uint8_t height;          // 0x03 despite being uint8_t this is bigZ not smallZ
        uint16_t roadPieces;     // 0x04
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint8_t costIndex;       // 0x0A
        uint8_t flags;           // 0x0B
        uint32_t image;          // 0x0C
        uint32_t var_10[4];      // 0x10
        uint8_t numCompatible;   // 0x20
        uint8_t mods[7];         // 0x21
        uint16_t designedYear;   // 0x28
        uint16_t obsoleteYear;   // 0x2A
        uint8_t cargoType;       // 0x2C
        uint8_t pad_2D;
        uint32_t cargoOffsetBytesPtrs[4][4]; // 0x2E
    };
    static_assert(sizeof(RoadStationObject) == 0x6E);

    struct ScaffoldingObject
    {
        uint16_t name;
        uint32_t image;             // 0x02
        uint16_t segmentHeights[3]; // 0x06
        uint16_t roofHeights[3];    // 0x0C
    };
    static_assert(sizeof(ScaffoldingObject) == 0x12);

    struct ScenarioTextObject
    {
        uint16_t name;
        uint16_t details;
        uint8_t pad_04[0x6 - 0x4];
    };
    static_assert(sizeof(ScenarioTextObject) == 0x6);

    struct SnowObject
    {
        uint16_t name;
        uint32_t image; // 0x02
    };
    static_assert(sizeof(SnowObject) == 0x6);

    struct SoundObject
    {
        uint16_t name;
        uint32_t dataPtr;
        uint8_t var_06;
        uint8_t pad_07;
        uint32_t volume; // 0x08
    };
    static_assert(sizeof(SoundObject) == 0xC);

    struct SteamObject
    {
        uint16_t name;                // 0x00
        uint16_t numImages;           // 0x02
        uint8_t numStationaryTicks;   // 0x04 while stationary can be affected by wind
        uint8_t spriteWidth;          // 0x05
        uint8_t spriteHeightNegative; // 0x06
        uint8_t spriteHeightPositive; // 0x07
        uint16_t flags;               // 0x08
        uint32_t var_0A;
        uint32_t baseImageId;         // 0x0E
        uint16_t totalNumFramesType0; // 0x12
        uint16_t totalNumFramesType1; // 0x14
        uint32_t frameInfoType0Ptr;   // 0x16
        uint32_t frameInfoType1Ptr;   // 0x1A
        uint8_t numSoundEffects;      // 0x1E
        uint8_t soundEffects[9];      // 0x1F
    };
    static_assert(sizeof(SteamObject) == 0x28);

    struct StreetLightObject
    {
        uint16_t name;
        uint16_t designedYear[3]; // 0x02
        uint32_t image;           // 0x08
    };
    static_assert(sizeof(StreetLightObject) == 0xC);

    struct TownNamesObject
    {
        struct Unk
        {
            uint8_t count;
            uint8_t fill;
            uint16_t offset;
        };
        uint16_t name; // 0x00
        Unk unks[6];   // 0x02
    };
    static_assert(sizeof(TownNamesObject) == 0x1A);

    struct TrackExtraObject
    {
        uint16_t name;
        uint16_t trackPieces;    // 0x02
        uint8_t paintStyle;      // 0x04
        uint8_t costIndex;       // 0x05
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint32_t image;          // 0x0A
        uint32_t var_0E;
    };
    static_assert(sizeof(TrackExtraObject) == 0x12);

    struct TrackObject
    {
        uint16_t name;
        uint16_t trackPieces;        // 0x02
        uint16_t stationTrackPieces; // 0x04
        uint8_t var_06;
        uint8_t numCompatible;     // 0x07
        uint8_t numMods;           // 0x08
        uint8_t numSignals;        // 0x09
        uint8_t mods[4];           // 0x0A
        uint16_t signals;          // 0x0E bitset
        uint16_t compatibleTracks; // 0x10 bitset
        uint16_t compatibleRoads;  // 0x12 bitset
        int16_t buildCostFactor;   // 0x14
        int16_t sellCostFactor;    // 0x16
        int16_t tunnelCostFactor;  // 0x18
        uint8_t costIndex;         // 0x1A
        uint8_t tunnel;            // 0x1B
        uint16_t curveSpeed;       // 0x1C
        uint32_t image;            // 0x1E
        uint16_t flags;            // 0x22
        uint8_t numBridges;        // 0x24
        uint8_t bridges[7];        // 0x25
        uint8_t numStations;       // 0x2C
        uint8_t stations[7];       // 0x2D
        uint8_t displayOffset;     // 0x34
        uint8_t pad_35;
    };
    static_assert(sizeof(TrackObject) == 0x36);

    struct TrainSignalObject
    {
        uint16_t name;
        uint16_t flags;         // 0x02
        uint8_t animationSpeed; // 0x04
        uint8_t numFrames;      // 0x05
        int16_t costFactor;     // 0x06
        int16_t sellCostFactor; // 0x08
        uint8_t costIndex;      // 0x0A
        uint8_t var_0B;
        uint16_t description;  // 0x0C
        uint32_t image;        // 0x0E
        uint8_t numCompatible; // 0x12
        uint8_t mods[7];       // 0x13
        uint16_t designedYear; // 0x1A
        uint16_t obsoleteYear; // 0x1C
    };
    static_assert(sizeof(TrainSignalObject) == 0x1E);

    struct TrainStationObject
    {
        uint16_t name;
        uint8_t drawStyle;       // 0x02
        uint8_t height;          // 0x03 despite being uint8_t this is bigZ not smallZ
        uint16_t trackPieces;    // 0x04
        int16_t buildCostFactor; // 0x06
        int16_t sellCostFactor;  // 0x08
        uint8_t costIndex;       // 0x0A
        uint8_t var_0B;
        uint8_t flags; // 0x0C
        uint8_t var_0D;
        uint32_t image; // 0x0E
        uint32_t var_12[4];
        uint8_t numCompatible; // 0x22
        uint8_t mods[7];
        uint16_t designedYear;               // 0x2A
        uint16_t obsoleteYear;               // 0x2C
        uint32_t cargoOffsetBytesPtrs[4][4]; // 0x2E
        uint32_t manualPowerPtrs[16];
    };
    static_assert(sizeof(TrainStationObject) == 0xAE);

    struct TreeObject
    {
        uint16_t name;                   // 0x00
        uint8_t initialHeight;           // 0x02
        uint8_t height;                  // 0x03
        uint8_t var_04;                  // 0x04
        uint8_t var_05;                  // 0x05
        uint8_t numRotations;            // 0x06 (1,2,4)
        uint8_t growth;                  // 0x07 (number of tree size images)
        uint16_t flags;                  // 0x08
        uint32_t sprites[6];             // 0x0A
        uint32_t snowSprites[6];         // 0x22
        uint16_t shadowImageOffset;      // 0x3A
        uint8_t var_3C;                  // 0x3C
        uint8_t seasonState;             // 0x3D (index for sprites, seasons + dying)
        uint8_t var_3E;                  // 0x3E
        uint8_t costIndex;               // 0x3F
        int16_t buildCostFactor;         // 0x40
        int16_t clearCostFactor;         // 0x42
        uint32_t colours;                // 0x44
        int16_t rating;                  // 0x48
        int16_t demolishRatingReduction; // 0x4A
    };
    static_assert(sizeof(TreeObject) == 0x4C);

    struct TunnelObject
    {
        uint16_t name;
        uint32_t image; // 0x02
    };
    static_assert(sizeof(TunnelObject) == 0x6);

    struct VehicleObjectFrictionSound
    {
        uint8_t soundObjectId;     // 0x0
        uint32_t minSpeed;         // 0x1 below this speed no sound created
        uint8_t speedFreqFactor;   // 0x5
        uint16_t baseFrequency;    // 0x6
        uint8_t speedVolumeFactor; // 0x8
        uint8_t baseVolume;        // 0x9
        uint8_t maxVolume;         // 0xA
    };
    static_assert(sizeof(VehicleObjectFrictionSound) == 0xB);

    struct VehicleObjectEngine1Sound
    {
        uint8_t soundObjectId;     // 0x0
        uint16_t defaultFrequency; // 0x1
        uint8_t defaultVolume;     // 0x3
        uint16_t var_04;
        uint8_t var_06;
        uint16_t var_07;
        uint8_t var_09;
        uint16_t freqIncreaseStep;  // 0xA
        uint16_t freqDecreaseStep;  // 0xC
        uint8_t volumeIncreaseStep; // 0xE
        uint8_t volumeDecreaseStep; // 0xF
        uint8_t speedFreqFactor;    // 0x10
    };
    static_assert(sizeof(VehicleObjectEngine1Sound) == 0x11);

    struct VehicleObjectEngine2Sound
    {
        uint8_t soundObjectId;         // 0x0
        uint16_t defaultFrequency;     // 0x1
        uint8_t defaultVolume;         // 0x2
        uint16_t firstGearFrequency;   // 0x4 All subsequent gears are based on this frequency
        uint16_t firstGearSpeed;       // 0x6
        uint16_t secondGearFreqFactor; // 0x8
        uint16_t secondGearSpeed;      // 0xA
        uint16_t thirdGearFreqFactor;  // 0xC
        uint16_t thirdGearSpeed;       // 0xE
        uint16_t fourthGearFreqFactor; // 0x10
        uint8_t var_12;
        uint8_t var_13;
        uint16_t freqIncreaseStep;  // 0x14
        uint16_t freqDecreaseStep;  // 0x16
        uint8_t volumeIncreaseStep; // 0x18
        uint8_t volumeDecreaseStep; // 0x19
        uint8_t speedFreqFactor;    // 0x1A
    };
    static_assert(sizeof(VehicleObjectEngine2Sound) == 0x1B);

    struct VehicleObjectSimpleAnimation
    {
        uint8_t objectId; // 0x00 (object loader fills this in)
        uint8_t height;   // 0x01
        uint8_t type;     // 0x02
    };
    static_assert(sizeof(VehicleObjectSimpleAnimation) == 0x3);

    struct VehicleObjectUnk
    {
        uint8_t length; // 0x00
        uint8_t var_01;
        uint8_t frontBogieSpriteInd; // 0x02 index of bogieSprites struct
        uint8_t backBogieSpriteInd;  // 0x03 index of bogieSprites struct
        uint8_t bodySpriteInd;       // 0x04 index of a bodySprites struct
        uint8_t var_05;
    };
    static_assert(sizeof(VehicleObjectUnk) == 0x6);

    struct VehicleObjectBogieSprite
    {
        uint8_t rollStates;      // 0x0 valid values 1, 2, 4 related to bogie->var_46 (identical in value to numRollSprites)
        uint8_t flags;           // 0x1 BogieSpriteFlags
        uint8_t width;           // 0x2 sprite width
        uint8_t heightNegative;  // 0x3 sprite height negative
        uint8_t heightPositive;  // 0x4 sprite height positive
        uint8_t numRollSprites;  // 0x5
        uint32_t flatImageIds;   // 0x6 flat sprites
        uint32_t gentleImageIds; // 0xA gentle sprites
        uint32_t steepImageIds;  // 0xE steep sprites
    };
    static_assert(sizeof(VehicleObjectBogieSprite) == 0x12);

    struct VehicleObjectBodySprite
    {
        uint8_t numFlatRotationFrames;   // 0x00 4, 8, 16, 32, 64?
        uint8_t numSlopedRotationFrames; // 0x01 4, 8, 16, 32?
        uint8_t numAnimationFrames;      // 0x02
        uint8_t numCargoLoadFrames;      // 0x03
        uint8_t numCargoFrames;          // 0x04
        uint8_t numRollFrames;           // 0x05
        uint8_t bogeyPosition;           // 0x06
        uint8_t flags;                   // 0x07
        uint8_t width;                   // 0x08 sprite width
        uint8_t heightNegative;          // 0x09 sprite height negative
        uint8_t heightPositive;          // 0x0A sprite height positive
        uint8_t flatYawAccuracy;         // 0x0B 0 - 4 accuracy of yaw on flat built from numFlatRotationFrames (0 = lowest accuracy 3bits, 4 = highest accuracy 7bits)
        uint8_t slopedYawAccuracy;       // 0x0C 0 - 3 accuracy of yaw on slopes built from numSlopedRotationFrames  (0 = lowest accuracy 3bits, 3 = highest accuracy 6bits)
        uint8_t numFramesPerRotation;    // 0x0D numAnimationFrames * numCargoFrames * numRollFrames + 1 (for braking lights)
        uint32_t flatImageId;            // 0x0E
        uint32_t unkImageId;             // 0x12
        uint32_t gentleImageId;          // 0x16
        uint32_t steepImageId;           // 0x1A
    };

    struct VehicleObject
    {
        static constexpr auto kMaxBodySprites = 4;

        uint16_t name; // 0x00
        uint8_t mode;  // 0x02
        uint8_t type;  // 0x03
        uint8_t var_04;
        uint8_t trackType;              // 0x05
        uint8_t numTrackExtras;         // 0x06
        uint8_t costIndex;              // 0x07
        int16_t costFactor;             // 0x08
        uint8_t reliability;            // 0x0A
        uint8_t runCostIndex;           // 0x0B
        int16_t runCostFactor;          // 0x0C
        uint8_t colourType;             // 0x0E
        uint8_t numCompatibleVehicles;  // 0x0F
        uint16_t compatibleVehicles[8]; // 0x10 array of compatible vehicle_types
        uint8_t requiredTrackExtras[4]; // 0x20
        VehicleObjectUnk var_24[4];
        VehicleObjectBodySprite bodySprites[kMaxBodySprites]; // 0x3C
        VehicleObjectBogieSprite bogieSprites[2];             // 0xB4
        uint16_t power;                                       // 0xD8
        uint16_t speed;                                       // 0xDA
        uint16_t rackSpeed;                                   // 0xDC
        uint16_t weight;                                      // 0xDE
        uint16_t flags;                                       // 0xE0
        uint8_t maxCargo[2];                                  // 0xE2 size is relative to the first compatibleCargoCategories
        uint32_t compatibleCargoCategories[2];                // 0xE4
        uint8_t cargoTypeSpriteOffsets[32];                   // 0xEC
        uint8_t numSimultaneousCargoTypes;                    // 0x10C
        VehicleObjectSimpleAnimation animation[2];            // 0x10D
        uint8_t var_113;
        uint16_t designed;        // 0x114
        uint16_t obsolete;        // 0x116
        uint8_t rackRailType;     // 0x118
        uint8_t drivingSoundType; // 0x119
        union
        {
            VehicleObjectFrictionSound friction;
            VehicleObjectEngine1Sound engine1;
            VehicleObjectEngine2Sound engine2;
        } sound;
        uint8_t pad_135[0x15A - 0x135];
        uint8_t numStartSounds; // 0x15A use mask when accessing kHasCrossingWhistle stuffed in (1 << 7)
        uint8_t startSounds[3]; // 0x15B sound array length numStartSounds highest sound is the crossing whistle
    };
    static_assert(sizeof(VehicleObject) == 0x15E);

    struct WallObject
    {
        uint16_t name;
        uint32_t sprite; // 0x02
        uint8_t var_06;  // 0x06 tool cursor type not used in Locomotion
        uint8_t flags;   // 0x07
        uint8_t height;  // 0x08
        uint8_t var_09;  // 0x09 flags2 None of these are used in Locomotion 0x10 used to be animation
    };
    static_assert(sizeof(WallObject) == 0xA);

    struct WaterObject
    {
        uint16_t name;
        uint8_t costIndex; // 0x02
        uint8_t var_03;
        int8_t costFactor; // 0x04
        uint8_t var_05;
        uint32_t image;         // 0x06
        uint32_t mapPixelImage; // 0x0A
    };
    static_assert(sizeof(WaterObject) == 0xE);
#pragma pack(pop)
}
