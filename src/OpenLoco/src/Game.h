#pragma once

#include <cstdint>
#include <optional>
#include <string>

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
    [[nodiscard]] std::optional<std::string> loadSaveGameOpen();
    [[nodiscard]] std::optional<std::string> loadLandscapeOpen();
    [[nodiscard]] std::optional<std::string> loadHeightmapOpen();
    [[nodiscard]] std::optional<std::string> saveSaveGameOpen();
    [[nodiscard]] std::optional<std::string> saveScenarioOpen();
    [[nodiscard]] std::optional<std::string> saveLandscapeOpen();

    void loadGame();
    void quitGame();
    void returnToTitle();
    void confirmSaveGame();
    bool saveLandscape(std::string filename);

    GameStateFlags getFlags();
    void setFlags(GameStateFlags flags);
    bool hasFlags(GameStateFlags flags);
    void removeFlags(GameStateFlags flags);
}
