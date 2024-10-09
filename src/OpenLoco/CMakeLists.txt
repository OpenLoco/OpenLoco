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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Economy.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EditorController.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/Effect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/EffectsManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/ExhaustEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/ExplosionEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/ExplosionSmokeEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/FireballEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/MoneyEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/SmokeEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/SplashEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/VehicleCrashEffect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/Entity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityTweener.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Environment.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Game.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Airports/CreateAirport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Airports/RemoveAirport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Buildings/CreateBuilding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Buildings/RemoveBuilding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Cheats/Cheat.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/BuildCompanyHeadquarters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/ChangeCompanyColour.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/ChangeCompanyFace.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/ChangeLoan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/RemoveCompanyHeadquarters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/RenameCompanyName.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/RenameCompanyOwner.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/UpdateOwnerStatus.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Docks/CreatePort.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Docks/RemovePort.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/GameCommands.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/LoadSaveQuit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/RenameStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/SetGameSpeed.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/TogglePause.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Industries/CreateIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Industries/RemoveIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Industries/RenameIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/CreateRoad.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/CreateRoadStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/RemoveRoadStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/ChangeLandMaterial.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/ClearLand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/CreateTree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/CreateWall.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/LowerLand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/LowerRaiseLandMountain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/LowerWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RaiseLand.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RaiseWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RemoveTree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RemoveWall.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Town/CreateTown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Town/RemoveTown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Town/RenameTown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateSignal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateTrack.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateTrackMod.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateTrainStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveSignal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveTrack.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveTrackMod.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveTrainStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/CloneVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/CreateVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/RenameVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleChangeRunningMode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderDelete.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderDown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderInsert.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderReverse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderSkip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderUp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePassSignal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePickup.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePickupAir.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePickupWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePlace.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePlaceAir.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePlaceWater.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleRearrange.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleRefit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleReverse.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleSell.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleSpeedControl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameSaveCompare.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameState.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Colour.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/DrawSprite.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/FPSCounter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Gfx.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/InvalidationGrid.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/PaletteMap.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/RenderTarget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/SoftwareDrawingContext.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/SoftwareDrawingEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/TextRenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Gui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Keyboard.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/MouseInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Shortcuts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Interop/Hooks.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Intro.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Conversion.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Formatting.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/LanguageFiles.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Languages.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringBuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Unicode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/AnimationManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/BuildingElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/IndustryElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/HeightMap.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/MapGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/OriginalTerrainGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/PngTerrainGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/SimplexTerrainGenerator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/RoadElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SignalElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/StationElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SurfaceData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/SurfaceElement.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Tile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileClearance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/TileLoop.cpp"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintAirport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintBuilding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintDocks.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintEffectEntity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintEntity.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintIndustry.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoad.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoadStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintSignal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintSurface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTileDecorations.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrack.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrainStation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTree.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintWall.cpp"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Chart.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Cursor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Dropdown.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/LastMapWindowAttributes.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ProgressBar.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Screenshot.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ScrollView.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/TextInput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ToolManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ViewportInteraction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/ButtonWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/FrameWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/GroupBoxWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/ImageButtonWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/LabelWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/PanelWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/TabWidget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/Wt3Widget.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Window.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/WindowManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/About.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/AboutMusic.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/BuildVehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Cheats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/CompanyFaceSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/CompanyList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/CompanyWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Construction/Common.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Construction/ConstructionTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Construction/OverheadTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Construction/SignalTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Construction/StationTab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/DebugWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/DragVehiclePart.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/EditKeyboardShortcut.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Error.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/IndustryList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/IndustryWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/KeyboardShortcuts.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/LandscapeGeneration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/LandscapeGenerationConfirm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/MapToolTip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/MapWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/MessageWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/MusicSelection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/NetworkStatus.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/News/Common.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/News/News.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/News/Ticker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ObjectLoadError.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ObjectSelectionWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Options.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/PlayerInfoPanel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ProgressBar.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/PromptBrowseWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/PromptOkCancelWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/PromptSaveWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ScenarioOptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ScenarioSelect.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/StationList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/StationWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TerraForm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TextInputWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TileInspector.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TimePanel.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TitleExit.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TitleLogo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TitleMenu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TitleOptions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TitleVersion.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ToolTip.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ToolbarBottomEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ToolbarTop.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ToolbarTopAlt.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ToolbarTopCommon.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TownList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/TownWindow.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Tutorial.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Vehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/VehicleList.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/OrderManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Orders.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Routing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/RoutingManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle1.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleBody.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleBogie.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleDraw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleHead.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleTail.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Version.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Viewport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ViewportManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Company.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/CompanyAi.cpp"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Currency.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Economy.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Economy/Expenditures.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EditorController.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/Effect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/EffectsManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/ExhaustEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/ExplosionEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/ExplosionSmokeEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/FireballEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/MoneyEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/SmokeEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/SplashEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Effects/VehicleCrashEffect.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine/Limits.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/Entity.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/EntityTweener.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Environment.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Game.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Airports/CreateAirport.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Airports/RemoveAirport.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Buildings/CreateBuilding.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Buildings/RemoveBuilding.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Cheats/Cheat.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/BuildCompanyHeadquarters.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/ChangeCompanyColour.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/ChangeCompanyFace.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/ChangeLoan.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/RemoveCompanyHeadquarters.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/RenameCompanyName.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/RenameCompanyOwner.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Company/UpdateOwnerStatus.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Docks/CreatePort.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Docks/RemovePort.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/GameCommands.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/LoadSaveQuit.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/RenameStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/SetGameSpeed.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/General/TogglePause.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Industries/CreateIndustry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Industries/RemoveIndustry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Industries/RenameIndustry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/CreateRoad.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/CreateRoadMod.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/CreateRoadStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/RemoveRoad.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/RemoveRoadMod.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Road/RemoveRoadStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/ChangeLandMaterial.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/ClearLand.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/CreateTree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/CreateWall.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/LowerLand.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/LowerRaiseLandMountain.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/LowerWater.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RaiseLand.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RaiseWater.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RemoveTree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Terraform/RemoveWall.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Town/CreateTown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Town/RemoveTown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Town/RenameTown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateSignal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateTrack.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateTrackMod.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/CreateTrainStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveSignal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveTrack.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveTrackMod.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Track/RemoveTrainStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/CloneVehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/CreateVehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/RenameVehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleChangeRunningMode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderDelete.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderDown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderInsert.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderReverse.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderSkip.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleOrderUp.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePassSignal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePickup.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePickupAir.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePickupWater.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePlace.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePlaceAir.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehiclePlaceWater.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleRearrange.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleRefit.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleReverse.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleSell.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameCommands/Vehicles/VehicleSpeedControl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameException.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameSaveCompare.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GameState.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Colour.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/DrawSprite.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/DrawSpriteBMP.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/DrawSpriteHelper.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/DrawSpriteRLE.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/DrawingContext.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/FPSCounter.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Font.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Gfx.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/ImageId.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/ImageIds.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/InvalidationGrid.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/PaletteMap.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/RenderTarget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/SoftwareDrawingContext.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/SoftwareDrawingEngine.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/TextRenderer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Gui.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/Shortcuts.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Interop/Hooks.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Intro.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LabelFrame.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Conversion.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/FormatArguments.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Formatting.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/LanguageFiles.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Languages.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringBuffer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringIds.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/StringManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Localisation/Unicode.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Location.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Animation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/AnimationManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/BuildingElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/IndustryElement.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/HeightMap.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/HeightMapRange.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/MapGenerator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/OriginalTerrainGenerator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/PngTerrainGenerator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapGenerator/SimplexTerrainGenerator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/MapSelection.h"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Map/Track/TrackEnum.h"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Objects/BuildingCommon.h"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintAirport.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintBuilding.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintDocks.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintEffectEntity.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintEntity.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintIndustry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoad.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoadStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoadCommonData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoadStyle0Data.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoadStyle1Data.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintRoadStyle2Data.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintSignal.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintSurface.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTileDecorations.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrack.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrackAdditionsData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrackData.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTrainStation.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintTree.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintVehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Paint/PaintWall.h"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Chart.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Cursor.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Dropdown.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/LastMapWindowAttributes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ProgressBar.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Screenshot.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ScrollFlags.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ScrollView.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/TextInput.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ToolManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/ViewportInteraction.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/ButtonWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/FrameWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/GroupBoxWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/ImageButtonWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/LabelWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/PanelWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/TabWidget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Widgets/Wt3Widget.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Window.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/WindowManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/WindowType.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/Construction/Construction.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/News/News.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ui/Windows/ToolbarTopCommon.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/OrderManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Orders.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Routing.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/RoutingManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/Vehicle.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleDraw.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vehicles/VehicleManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Viewport.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ViewportManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/Company.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/World/CompanyAi.h"
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
    ${OPENAL_LIBRARIES})

if (WIN32)
    target_link_libraries(OpenLoco winmm ws2_32 Resources)
    # Loco.exe is looking for openloco.dll. Remove this when standalone.
    set_target_properties(OpenLoco PROPERTIES OUTPUT_NAME "openloco")
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
            _WINSOCK_DEPRECATED_NO_WARNINGS)

    target_link_libraries(OpenLoco
        ZLIB::ZLIB) # This shouldn't be required but vcpkg seems to not map the dependencies correctly for png static

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
        )
else ()
    # Dummy target to ease invocation
    add_custom_target(OpenLoco-headers-check)
endif ()
