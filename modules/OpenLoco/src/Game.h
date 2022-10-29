#pragma once

#include <cstdint>

namespace OpenLoco::Game
{
    bool loadSaveGameOpen();
    bool loadLandscapeOpen();
    bool saveSaveGameOpen();
    bool saveScenarioOpen();
    bool saveLandscapeOpen();
    void loadGame();
    void quitGame();
    void returnToTitle();
    void confirmSaveGame();
    bool saveLandscape();
    uint32_t getFlags();
    void setFlags(uint32_t flags);
    bool hasFlags(uint32_t flags);
    void removeFlags(uint32_t flags);
    void sub_46DB4C();
}
