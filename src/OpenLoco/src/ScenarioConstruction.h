#pragma once

#include <cstdint>

namespace OpenLoco::Scenario
{
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

    Construction& getConstruction();
    void resetRoadObjects();
    void resetTrackObjects();

    void initialiseDefaultTrackRoadMods();
}
