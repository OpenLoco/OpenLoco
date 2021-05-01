#pragma once

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
}
