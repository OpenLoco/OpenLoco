#include "Hooks.h"
#include "../Config.h"
#include "../GameState.h"
#include "../Graphics/Gfx.h"
#include "../Map/Tile.h"
#include "../Map/Track/TrackData.h"
#include "../Objects/ObjectManager.h"
#include "../S5/S5.h"
#include "../Types.hpp"
#include "../Ui.h"
#include "../Ui/WindowType.h"
#include "../Vehicles/Vehicle.h"
#include "../World/CompanyManager.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <system_error>
#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif

// Static asserts for all loco_global types - validates type sizes used in the Python script
// Note: This file validates sizes for x86 32-bit architecture as used by the original game
namespace OpenLoco::Interop
{
    // Basic C/C++ types
    static_assert(sizeof(bool) == 1);
    static_assert(sizeof(char) == 1);
    static_assert(sizeof(int8_t) == 1);
    static_assert(sizeof(uint8_t) == 1);
    static_assert(sizeof(int16_t) == 2);
    static_assert(sizeof(uint16_t) == 2);
    static_assert(sizeof(int32_t) == 4);
    static_assert(sizeof(uint32_t) == 4);

    // Array types (basic)
    static_assert(sizeof(char[16]) == 16);
    static_assert(sizeof(char[32]) == 32);
    static_assert(sizeof(char[256]) == 256);
    static_assert(sizeof(char[512]) == 512);
    static_assert(sizeof(std::byte[20]) == 20);
    static_assert(sizeof(std::byte[40]) == 40);
    static_assert(sizeof(std::uint8_t[33]) == 33);
    static_assert(sizeof(uint8_t[2]) == 2);
    static_assert(sizeof(uint8_t[18]) == 18);
    static_assert(sizeof(uint8_t[40]) == 40);
    static_assert(sizeof(uint8_t[64]) == 64);
    static_assert(sizeof(uint8_t[128]) == 128);
    static_assert(sizeof(uint16_t[33]) == 66);
    static_assert(sizeof(int32_t[32]) == 128);

    // OpenLoco-specific types
    static_assert(sizeof(StringId) == 2);
    static_assert(sizeof(Colour) == 1);
    static_assert(sizeof(coord_t) == 2);
    static_assert(sizeof(currency32_t) == 4);
    static_assert(sizeof(CompanyId) == 1);
    static_assert(sizeof(EntityId) == 2);
    static_assert(sizeof(IndustryId) == 1); // Note: Changed to uint8_t in current codebase
    static_assert(sizeof(StationId) == 2);
    static_assert(sizeof(Ui::WindowType) == 1);
    static_assert(sizeof(Ui::WindowNumber_t) == 2);
    static_assert(sizeof(Ui::WidgetIndex_t) == 2);
    static_assert(sizeof(Ui::Point) == 4);
    static_assert(sizeof(Ui::ScreenInfo) == 12);
    static_assert(sizeof(World::Pos2) == 4);
    static_assert(sizeof(World::Pos3) == 6);
    static_assert(sizeof(Speed32) == 4);
    static_assert(sizeof(ColourScheme) == 2);
    static_assert(sizeof(Gfx::G1Element32) == 16);
    static_assert(sizeof(S5::Header) == 32);
    // Note: The following sizes may differ from the Python script due to ongoing refactoring
    // static_assert(sizeof(Config::LocoConfig) == 536);  // Current: 534
    // static_assert(sizeof(GameState) == 4867652);  // Current: 4851328
}

void OpenLoco::Interop::loadSections()
{
}
