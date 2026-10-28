#include "GameCommands/Terraform/ClearLand.h"
#include "GameCommands/Terraform/CreateTree.h"
#include "GameCommands/Terraform/CreateWall.h"
#include "GameCommands/Terraform/RaiseLand.h"
#include "GameCommands/Terraform/RaiseWater.h"
#include "GameCommands/Vehicles/VehiclePlace.h"
#include <gtest/gtest.h>

using namespace OpenLoco;
using namespace OpenLoco::GameCommands;

namespace
{
    // Game commands pass their arguments through the register block, which used to hold world
    // coordinates as 16 bit values. Maps larger than 1024 tiles have coordinates beyond that range.
    constexpr World::Pos3 kFarPos3{ 100000, 80000, 48 };
    constexpr World::Pos2 kFarPosA{ 70000, 50000 };
    constexpr World::Pos2 kFarPosB{ 90000, 110000 };
    constexpr World::Pos2 kFarPosC{ 120000, 30000 };
}

TEST(GameCommandArgsTest, WallPlacementArgsRoundTripFarPosition)
{
    WallPlacementArgs args;
    args.pos = kFarPos3;
    args.rotation = 2;
    args.type = 3;
    args.primaryColour = static_cast<Colour>(1);
    args.secondaryColour = static_cast<Colour>(2);
    args.tertiaryColour = static_cast<Colour>(3);

    const WallPlacementArgs roundTrip{ static_cast<registers>(args) };
    EXPECT_EQ(roundTrip.pos, args.pos);
    EXPECT_EQ(roundTrip.rotation, args.rotation);
    EXPECT_EQ(roundTrip.type, args.type);
    EXPECT_EQ(roundTrip.primaryColour, args.primaryColour);
    EXPECT_EQ(roundTrip.secondaryColour, args.secondaryColour);
    EXPECT_EQ(roundTrip.tertiaryColour, args.tertiaryColour);
}

TEST(GameCommandArgsTest, TreePlacementArgsRoundTripFarPosition)
{
    TreePlacementArgs args;
    args.pos = kFarPosA;
    args.rotation = 1;
    args.type = 4;
    args.quadrant = 2;
    args.colour = static_cast<Colour>(5);
    args.buildImmediately = true;
    args.requiresFullClearance = false;

    const TreePlacementArgs roundTrip{ static_cast<registers>(args) };
    EXPECT_EQ(roundTrip.pos, args.pos);
    EXPECT_EQ(roundTrip.rotation, args.rotation);
    EXPECT_EQ(roundTrip.type, args.type);
    EXPECT_EQ(roundTrip.quadrant, args.quadrant);
    EXPECT_EQ(roundTrip.colour, args.colour);
    EXPECT_EQ(roundTrip.buildImmediately, args.buildImmediately);
    EXPECT_EQ(roundTrip.requiresFullClearance, args.requiresFullClearance);
}

TEST(GameCommandArgsTest, ClearLandArgsRoundTripThreeFarPositions)
{
    ClearLandArgs args;
    args.centre = kFarPosA;
    args.pointA = kFarPosB;
    args.pointB = kFarPosC;

    const ClearLandArgs roundTrip{ static_cast<registers>(args) };
    EXPECT_EQ(roundTrip.centre, args.centre);
    EXPECT_EQ(roundTrip.pointA, args.pointA);
    EXPECT_EQ(roundTrip.pointB, args.pointB);
}

TEST(GameCommandArgsTest, RaiseLandArgsRoundTripThreeFarPositions)
{
    RaiseLandArgs args;
    args.centre = kFarPosA;
    args.pointA = kFarPosB;
    args.pointB = kFarPosC;
    args.corner = World::MapSelectionType::corner0;

    const RaiseLandArgs roundTrip{ static_cast<registers>(args) };
    EXPECT_EQ(roundTrip.centre, args.centre);
    EXPECT_EQ(roundTrip.pointA, args.pointA);
    EXPECT_EQ(roundTrip.pointB, args.pointB);
    EXPECT_EQ(roundTrip.corner, args.corner);
}

TEST(GameCommandArgsTest, RaiseWaterArgsRoundTripTwoFarPositions)
{
    RaiseWaterArgs args;
    args.pointA = kFarPosA;
    args.pointB = kFarPosC;

    const RaiseWaterArgs roundTrip{ static_cast<registers>(args) };
    EXPECT_EQ(roundTrip.pointA, args.pointA);
    EXPECT_EQ(roundTrip.pointB, args.pointB);
}

TEST(GameCommandArgsTest, VehiclePlacementArgsRoundTripFarPosition)
{
    VehiclePlacementArgs args;
    args.pos = kFarPos3;
    args.trackAndDirection = 0x1234;
    args.trackProgress = 0x5678;
    args.head = EntityId(42);
    args.convertGhost = false;

    const VehiclePlacementArgs roundTrip{ static_cast<registers>(args) };
    EXPECT_EQ(roundTrip.pos, args.pos);
    EXPECT_EQ(roundTrip.trackAndDirection, args.trackAndDirection);
    EXPECT_EQ(roundTrip.trackProgress, args.trackProgress);
    EXPECT_EQ(roundTrip.head, args.head);
    EXPECT_EQ(roundTrip.convertGhost, args.convertGhost);
}
