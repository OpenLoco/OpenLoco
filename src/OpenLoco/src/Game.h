#pragma once

#include <cstdint>

namespace OpenLoco::Scenario
{
    enum class Flags : uint16_t;
}

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
    Scenario::Flags getFlags();
    void setFlags(Scenario::Flags flags);
    bool hasFlags(Scenario::Flags flags);
    void removeFlags(Scenario::Flags flags);
    void sub_46DB4C();
}
