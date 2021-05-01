#pragma once

namespace OpenLoco::Game
{
    bool loadSaveGameOpen();
    bool saveSaveGameOpen();
    bool loadLandscapeOpen();
    bool saveLandscapeOpen();
    void loadGame();
    void quitGame();
    void returnToTitle();
    void confirmSaveGame();
}
