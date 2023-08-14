#pragma once

#include "Types.hpp"
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

    namespace Ui::Windows::PromptBrowse
    {
        enum browse_type : uint8_t;
    }
}

namespace OpenLoco::Game
{
    using Ui::Windows::PromptBrowse::browse_type;

    bool openBrowsePrompt(StringId titleId, browse_type type, const char* filter);
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
