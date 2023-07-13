include(FindGit)

find_package(Git)
if (Git_FOUND)
    # Describe current version in terms of closest tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe HEAD --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OPENLOCO_VERSION_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        # ERROR_QUIET
    )

    if (OPENLOCO_VERSION_TAG)
        # Use a platform agnostic sed equivalent to strip the commit hash from the version tag
        # i.e. "v22.10-9-g8ff1d207" becomes "v22.10-9 "
        string(REGEX REPLACE "-g.+$" " " OPENLOCO_VERSION_TAG ${OPENLOCO_VERSION_TAG})
        # Has to be in two bits due to empty string not being possible to pass to REGEX REPLACE
        string(STRIP ${OPENLOCO_VERSION_TAG} OPENLOCO_VERSION_TAG)
    else()
        # If a low fetch depth is used then nearest tag for git will fail
        # so just use the project version in that case.
        set(OPENLOCO_VERSION_TAG ${PROJECT_VERSION})
    endif()
    # Define current git branch
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OPENLOCO_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Define short commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE OPENLOCO_COMMIT_SHA1_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # To avoid unnecessary rebuilds, set the current branch and short sha1 hash
    # only for the one file that uses these definitions: Version.cpp
    set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.cpp"
        PROPERTY COMPILE_DEFINITIONS
        OPENLOCO_VERSION_TAG="${OPENLOCO_VERSION_TAG}"
        OPENLOCO_BRANCH="${OPENLOCO_BRANCH}"
        OPENLOCO_COMMIT_SHA1_SHORT="${OPENLOCO_COMMIT_SHA1_SHORT}")
endif()

# add source files Generated originally with: find src -type f -follow -name '*.cpp' -exec echo "\"\${CMAKE_CURRENT_SOURCE_DIR}/{}\"" \;
set(OLOCO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/Audio.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/Channel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/OpenAL.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/VehicleChannel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CommandLine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Date.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/DrawSprite.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/FPSCounter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/SoftwareDrawingContext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/SoftwareDrawingEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Economy.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EditorController.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/EffectsManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/Entity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityTweener.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/Misc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Environment.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Game.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/BuildCompanyHeadquarters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/ChangeCompanyColour.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/ChangeCompanyFace.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/ChangeLandMaterial.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/ChangeLoan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Cheat.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/ClearLand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/CloneVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/CreateTown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/CreateTree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/CreateVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/CreateWall.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/GameCommands.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/LoadSaveQuit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/LowerLand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/LowerWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RaiseLand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RaiseWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RemoveBuilding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RemoveCompanyHeadquarters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RemoveIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RemoveTown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RemoveTree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RemoveWall.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RenameCompanyName.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RenameCompanyOwner.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RenameIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RenameStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RenameTown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/RenameVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/SetGameSpeed.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/TogglePause.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/UpdateOwnerStatus.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleChangeRunningMode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleOrderDelete.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleOrderDown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleOrderInsert.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleOrderReverse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleOrderSkip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleOrderUp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehiclePassSignal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehiclePickup.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehiclePickupWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleRefit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleReverse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleSell.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/VehicleSpeedControl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameState.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Colour.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Gfx.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/PaletteMap.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/RenderTarget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Gui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Keyboard.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/MouseInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Shortcuts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Interop/Hooks.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Intro.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LastGameOptionManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Conversion.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Formatting.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/LanguageFiles.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Languages.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Unicode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/AnimationManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/BuildingElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/IndustryElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/RoadElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SurfaceData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SurfaceElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Tile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileClearance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/SubpositionData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/Track.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/TrackData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Tree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/WaveManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Message.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MessageManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MultiPlayer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/Network.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkBase.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkConnection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkServer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/Socket.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/AirportObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/BridgeObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/BuildingObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CargoObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CliffEdgeObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ClimateObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CompetitorObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CurrencyObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/DockObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/HillShapesObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/IndustryObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/InterfaceSkinObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/LandObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/LevelCrossingObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/Object.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectImageTable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectIndex.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectStringTable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RegionObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RoadExtraObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RoadObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RoadStationObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ScaffoldingObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ScenarioTextObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/SnowObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/SoundObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/SteamObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/StreetLightObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TownNamesObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrackExtraObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrackObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrainSignalObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrainStationObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TreeObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TunnelObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/VehicleObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/WallObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/WaterObject.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/OpenLoco.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/Paint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintEntity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintMiscEntity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintSignal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTileDecorations.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrack.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Random.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/S5/S5.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/S5/SawyerStream.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Scenario.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ScenarioConstruction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ScenarioManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ScenarioObjective.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SceneManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Title.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Tutorial.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Cursor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Dropdown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/LastMapWindowAttributes.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ProgressBar.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Screenshot.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ScrollView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/TextInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ToolManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ViewportInteraction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/WindowManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/OrderManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Orders.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/RoutingManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Routing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle1.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleBody.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleBogie.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleHead.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleTail.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Viewport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ViewportManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Widget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Window.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/About.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/AboutMusic.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/BuildVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Cheats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/CompanyFaceSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/CompanyList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/CompanyWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Construction/Common.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Construction/ConstructionTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Construction/OverheadTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Construction/SignalTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Construction/StationTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Debug/Debug.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/DragVehiclePart.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/EditKeyboardShortcut.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Error.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/IndustryList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/IndustryWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/KeyboardShortcuts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/LandscapeGeneration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/LandscapeGenerationConfirm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/MapToolTip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/MapWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/MessageWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/MusicSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/NetworkStatus.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/News/Common.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/News/News.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/News/Ticker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ObjectLoadError.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ObjectSelectionWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Options.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/PlayerInfoPanel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ProgressBar.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/PromptBrowseWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/PromptOkCancelWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/PromptSaveWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ScenarioOptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ScenarioSelect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/StationList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/StationWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TerraForm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TextInputWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TileInspector.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TimePanel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TitleExit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TitleLogo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TitleMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TitleOptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TitleVersion.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ToolTip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ToolbarBottomEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ToolbarTop.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ToolbarTopAlt.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ToolbarTopCommon.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TownList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/TownWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Tutorial.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Vehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/VehicleList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Company.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/CompanyManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Industry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/IndustryManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Station.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/StationManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Town.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/TownManager.cpp")

set(OLOCO_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/Audio.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/Channel.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/OpenAL.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Audio/VehicleChannel.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CommandLine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigConvert.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Date.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/DrawSprite.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/DrawSpriteBMP.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/DrawSpriteHelper.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/DrawSpriteRLE.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/DrawingContext.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/FPSCounter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/SoftwareDrawingContext.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Drawing/SoftwareDrawingEngine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Currency.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Economy.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Expenditures.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EditorController.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/EffectsManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Limits.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/Entity.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityTweener.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/Misc.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Environment.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Game.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Cheat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/GameCommands.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameException.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameState.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Colour.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Gfx.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/ImageId.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/ImageIds.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/PaletteMap.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/RenderTarget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Gui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Shortcuts.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Interop/Hooks.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Intro.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LabelFrame.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LastGameOptionManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/ArgsWrapper.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Conversion.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/FormatArguments.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Formatting.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/LanguageFiles.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Languages.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringIds.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Unicode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Location.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Animation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/AnimationManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/BuildingElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/IndustryElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/QuarterTile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/RoadElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SignalElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/StationElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SurfaceData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SurfaceElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Tile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileClearance.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileElementBase.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileLoop.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/SubpositionData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/Track.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/TrackData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TrackElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Tree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TreeElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/WallElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Wave.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/WaveManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Message.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MessageManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MultiPlayer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/Network.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkBase.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkClient.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkConnection.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/NetworkServer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/Packet.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Network/Socket.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/AirportObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/BridgeObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/BuildingObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CargoObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CliffEdgeObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ClimateObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CompetitorObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/CurrencyObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/DockObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/HillShapesObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/IndustryObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/InterfaceSkinObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/LandObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/LevelCrossingObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/Object.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectImageTable.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectIndex.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ObjectStringTable.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RegionObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RoadExtraObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RoadObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/RoadStationObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ScaffoldingObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/ScenarioTextObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/SnowObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/SoundObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/SteamObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/StreetLightObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TownNamesObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrackExtraObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrackObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrainSignalObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TrainStationObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TreeObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/TunnelObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/VehicleObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/WallObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/WaterObject.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/OpenLoco.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/Paint.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintEntity.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintIndustry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintMiscEntity.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintSignal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTileDecorations.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrack.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintVehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Random.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/S5/Limits.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/S5/S5.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/S5/SawyerStream.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Scenario.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ScenarioConstruction.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ScenarioManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ScenarioObjective.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SceneManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Speed.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Title.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Tutorial.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Types.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Cursor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Dropdown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/LastMapWindowAttributes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ProgressBar.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Screenshot.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ScrollFlags.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ScrollView.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/TextInput.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ToolManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/WindowManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/WindowType.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/OrderManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Orders.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/RoutingManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Routing.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Viewport.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ViewportManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Widget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Window.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Construction/Construction.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/Debug/Debug.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/News/News.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Windows/ToolbarTopCommon.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Company.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/CompanyManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/CompanyRecords.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Industry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/IndustryManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Station.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/StationManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Town.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/TownManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ZoomLevel.hpp")

if (MINGW)
    loco_add_library(OpenLoco SHARED
        PRIVATE_FILES
            ${OLOCO_SOURCES})
    target_link_libraries(OpenLoco winmm)
else ()
    if (MSVC)
        # Headers added as well so that shown correctly in IDEs
        loco_add_library(OpenLoco SHARED 
            PRIVATE_FILES
                ${OLOCO_SOURCES}
                ${OLOCO_HEADERS})
    else ()
        loco_add_executable(OpenLoco 
            PRIVATE_FILES
                ${OLOCO_SOURCES}
        )
        add_dependencies(OpenLoco linkable_sections)
        # set_source_files_properties has to be set in the CMakeLists that uses them
        set_source_files_properties(
            ${LOCO_SECTIONS}
            PROPERTIES
            EXTERNAL_OBJECT true
            GENERATED true
        )
        target_sources(OpenLoco
            PRIVATE
                ${LOCO_SECTIONS})
        set_target_properties(OpenLoco PROPERTIES LINK_FLAGS ${LOCO_SEGMENT_LINKER_FLAGS})

        if (TARGET SDL2::SDL2main)
            target_link_libraries(OpenLoco SDL2::SDL2main)
        endif ()
    endif ()
endif ()

target_link_libraries(OpenLoco
    Core
    Diagnostics
    Platform
    Interop
    Utility
    Math
    Engine
    ${SDL2_LIB}
    Threads::Threads
    yaml-cpp
    ${PNG_LIBRARY}
    ${OPENAL_LIBRARIES}
    ${IMGUI_LIBRARIES}
)

if (WIN32)
    target_link_libraries(OpenLoco ${BREAKPAD_LIBRARIES})
    target_link_libraries(OpenLoco winmm ws2_32 Resources)
endif ()

if (NOT WIN32)
    add_dependencies(OpenLoco segfiles)
endif()

if (MINGW)
    target_compile_definitions(OpenLoco
        PRIVATE
            DEBUG=${DEBUG_LEVEL}
            SDL_MAIN_HANDLED=1)

    target_link_libraries(OpenLoco imm32 winmm version)
    # newer GCC gets confused about our use of hardcoded address shenanigans
elseif (MSVC)
    target_compile_definitions(OpenLoco
        PRIVATE
            DEBUG=${DEBUG_LEVEL}
            _WINDLL _MBCS DEBUG_WIN32=1
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE
            _USE_MATH_DEFINES
            SDL_MAIN_HANDLED
            _WINSOCK_DEPRECATED_NO_WARNINGS
            USE_BREAKPAD)
else ()
    target_compile_definitions(OpenLoco
        PRIVATE
            DEBUG=${DEBUG_LEVEL}
            _NO_LOCO_WIN32_=1)
endif()

if (APPLE AND FALSE) # TODO: This is broken after moving resources into src/Resources
    target_link_libraries(OpenLoco "-framework Cocoa")

    set_target_properties(OpenLoco PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/distribution/macos/Info.plist.in)

    set(MACOSX_BUNDLE_BUNDLE_NAME "OpenLoco")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${OPENRCT2_COMMIT_SHA1_SHORT}")
    set(MACOSX_BUNDLE_COPYRIGHT "OpenLoco is licensed under the MIT License")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.openloco.OpenLoco")
    set(MACOSX_BUNDLE_ICON_FILE "AppIcon")

    if(${OPENLOCO_BRANCH} EQUAL master)
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG}")
    else()
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENLOCO_VERSION_TAG} ${OPENLOCO_BRANCH}")
    endif()

    set(BUNDLE_RESOURCES "")

    find_program(ICONUTIL iconutil)
    if (ICONUTIL)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/distribution/macos/AppIcon.iconset DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        set(ICON_TARGET "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.iconset")
        set(ICON_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns")
        set(SOURCE_ICON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo")
        add_custom_command(OUTPUT ${ICON_OUTPUT}
                COMMAND cp icon_x16.png ${ICON_TARGET}/icon_16x16.png
                COMMAND cp icon_x32.png ${ICON_TARGET}/icon_16x16@2x.png
                COMMAND cp icon_x32.png ${ICON_TARGET}/icon_32x32.png
                COMMAND cp icon_x64.png ${ICON_TARGET}/icon_32x32@2x.png
                COMMAND cp icon_x128.png ${ICON_TARGET}/icon_128x128.png
                COMMAND cp icon_x256.png ${ICON_TARGET}/icon_128x128@2x.png
                COMMAND cp icon_x256.png ${ICON_TARGET}/icon_256x256.png
                COMMAND cp icon_x512.png ${ICON_TARGET}/icon_256x256@2x.png
                COMMAND cp icon_x512.png ${ICON_TARGET}/icon_512x512.png
                COMMAND cp icon_x1024.png ${ICON_TARGET}/icon_512x512@2x.png
                COMMAND ${ICONUTIL} --convert icns --output ${ICON_OUTPUT} ${ICON_TARGET}
                WORKING_DIRECTORY ${SOURCE_ICON_DIR})
        list(APPEND BUNDLE_RESOURCES ${ICON_OUTPUT})
    endif ()

    list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md)
    list(APPEND BUNDLE_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTORS.md)
    set_target_properties(OpenLoco PROPERTIES RESOURCE "${BUNDLE_RESOURCES}")
    target_sources(OpenLoco PUBLIC ${BUNDLE_RESOURCES})

    file(GLOB BUNDLE_LANGUAGES "${CMAKE_CURRENT_SOURCE_DIR}/data/language/*")
    target_sources(OpenLoco PUBLIC ${BUNDLE_LANGUAGES})
    set_property(
        SOURCE ${BUNDLE_LANGUAGES}
        PROPERTY MACOSX_PACKAGE_LOCATION "Resources/language"
    )
endif ()

# Add headers check to verify all headers carry their dependencies.
# Only valid for Clang for now:
# - GCC 8 does not support -Wno-pragma-once-outside-header
# - Other compilers status unknown
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_library(OpenLoco-headers-check OBJECT ${OLOCO_HEADERS})
    set_target_properties(OpenLoco-headers-check PROPERTIES LINKER_LANGUAGE CXX)
    set_source_files_properties(${OLOCO_HEADERS} PROPERTIES LANGUAGE CXX)
    loco_target_compile_link_flags(OpenLoco-headers-check)
    target_compile_options(OpenLoco-headers-check PUBLIC -x c++ -Wno-pragma-once-outside-header -Wno-unused-const-variable)
    target_include_directories(OpenLoco-headers-check PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(OpenLoco-headers-check PUBLIC
        Core
        Diagnostics
        Platform
        Interop
        Utility
        Math
        Engine
        ${SDL2_LIB} # These libraries are used in some of our headers try to minimize doing this
        yaml-cpp
        nonstd::span-lite
        ${IMGUI_LIBRARIES}
    )
else ()
    # Dummy target to ease invocation
    add_custom_target(OpenLoco-headers-check)
endif ()
