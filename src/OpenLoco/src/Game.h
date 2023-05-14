#pragma once

#include <cstdint>

namespace OpenLoco
{
    enum class GameStateFlags : uint32_t;

    enum class LoadOrQuitMode : uint16_t
    {
        loadGamePrompt,
        returnToTitlePrompt,
        quitGamePrompt,
    };
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
    GameStateFlags getFlags();
    void setFlags(GameStateFlags flags);
    bool hasFlags(GameStateFlags flags);
    void removeFlags(GameStateFlags flags);
    void sub_46DB4C();
}
