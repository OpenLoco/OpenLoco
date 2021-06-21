#pragma once

#include <cstdint>

namespace OpenLoco::Ui
{
    enum class WindowType : uint8_t
    {
        main = 0,
        topToolbar = 1,
        playerInfoToolbar = 2,
        timeToolbar = 3,
        editorToolbar = 4,

        tooltip = 6,
        dropdown = 7,

        about = 9,
        // The Atari credits window is no longer used
        aboutAtari = 10,
        aboutMusic = 11,
        error = 12,
        construction = 13,
        saveGamePrompt = 14,
        terraform = 15,
        titleMenu = 16,
        titleExit = 17,
        scenarioSelect = 18,
        keyboardShortcuts = 19,
        editKeyboardShortcut = 20,
        map = 21,
        title_logo = 22,
        vehicle = 23,
        station = 24,
        dragVehiclePart = 25,
        company = 26,
        vehicleList = 27,
        buildVehicle = 28,
        stationList = 29,
        mapTooltip = 30,
        objectSelection = 31,
        townList = 32,
        town = 33,
        industry = 34,
        industryList = 35,
        news = 36,
        messages = 37,

        multiplayer = 39,
        options = 40,
        musicSelection = 41,
        companyFaceSelection = 42,
        landscapeGeneration = 43,
        landscapeGenerationConfirm = 44,
        scenarioOptions = 45,

        progressBar = 47,
        companyList = 48,
        tutorial = 49,
        confirmDisplayModePrompt = 50,
        textInput = 51,
        fileBrowserPrompt = 52,
        previewImage = 53,
        confirmationPrompt = 54,
        openLocoVersion = 55,
        titleOptions = 56,
        tileInspector = 57,
        cheats = 58,

        undefined = 255
    };
}
