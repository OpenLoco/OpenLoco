#define DO_TITLE_SEQUENCE_CHECKS

#include "S5.h"

#include "Audio/Audio.h"
#include "EditorController.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameException.hpp"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Gui.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Objects/ScenarioTextObject.h"
#include "Objects/WaterObject.h"
#include "OpenLoco.h"
#include "SawyerStream.h"
#include "ScenarioManager.h"
#include "ScenarioOptions.h"
#include "SceneManager.h"
#include "Ui/ProgressBar.h"
#include "Ui/WindowManager.h"
#include "Vehicles/OrderManager.h"
#include "ViewportManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Core/Stream.hpp>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <fstream>
#include <iomanip>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::S5
{
    constexpr uint32_t kCurrentVersion = 0x62262;
    constexpr uint32_t kMagicNumber = 0x62300;

    static loco_global<GameState, 0x00525E18> _gameState;
    static loco_global<Options, 0x009C8714> _activeOptions;
    static loco_global<Header, 0x009CCA34> _header;
    static loco_global<char[512], 0x0112CE04> _savePath;
    static loco_global<uint8_t, 0x0050C197> _loadErrorCode;
    static loco_global<StringId, 0x0050C198> _loadErrorMessage;

    // TODO: move this?
    static std::vector<ObjectHeader> _loadErrorObjectsList;

    static bool exportGameState(Stream& stream, const S5File& file, const std::vector<ObjectHeader>& packedObjects);

    constexpr bool hasSaveFlags(SaveFlags flags, SaveFlags flagsToTest)
    {
        return (flags & flagsToTest) != SaveFlags::none;
    }

    constexpr bool hasLoadFlags(LoadFlags flags, LoadFlags flagsToTest)
    {
        return (flags & flagsToTest) != LoadFlags::none;
    }

    static Header prepareHeader(SaveFlags flags, size_t numPackedObjects)
    {
        Header result;
        std::memset(&result, 0, sizeof(result));

        result.type = S5Type::savedGame;
        if (hasSaveFlags(flags, SaveFlags::landscape))
        {
            result.type = S5Type::landscape;
        }
        if (hasSaveFlags(flags, SaveFlags::scenario))
        {
            result.type = S5Type::scenario;
        }

        result.numPackedObjects = static_cast<uint16_t>(numPackedObjects);
        result.version = kCurrentVersion;
        result.magic = kMagicNumber;

        if (hasSaveFlags(flags, SaveFlags::raw))
        {
            result.flags |= HeaderFlags::isRaw;
        }
        if (hasSaveFlags(flags, SaveFlags::dump))
        {
            result.flags |= HeaderFlags::isDump;
        }
        if (!hasSaveFlags(flags, SaveFlags::scenario)
            && !hasSaveFlags(flags, SaveFlags::raw)
            && !hasSaveFlags(flags, SaveFlags::dump))
        {
            result.flags |= HeaderFlags::hasSaveDetails;
        }

        return result;
    }

    static PaletteIndex_t getPreviewColourByTilePos(const TilePos2& pos)
    {
        PaletteIndex_t colour = PaletteIndex::transparent;
        auto tile = TileManager::get(pos);

        for (auto& el : tile)
        {
            switch (el.type())
            {
                case ElementType::surface:
                {
                    auto* surfaceEl = el.as<SurfaceElement>();
                    if (surfaceEl == nullptr)
                    {
                        continue;
                    }

                    if (surfaceEl->water() == 0)
                    {
                        const auto* landObj = ObjectManager::get<LandObject>(surfaceEl->terrain());
                        const auto* landImage = Gfx::getG1Element(landObj->mapPixelImage);
                        auto offset = surfaceEl->baseZ() / kMicroToSmallZStep * 2;
                        colour = landImage->offset[offset];
                    }
                    else
                    {
                        const auto* waterObj = ObjectManager::get<WaterObject>();
                        const auto* waterImage = Gfx::getG1Element(waterObj->mapPixelImage);
                        auto offset = (surfaceEl->water() * kMicroToSmallZStep - surfaceEl->baseZ()) / 2;
                        colour = waterImage->offset[offset - 2];
                    }
                    break;
                }

                case ElementType::building:
                case ElementType::road:
                    colour = PaletteIndex::mutedDarkRed7;
                    break;

                case ElementType::industry:
                    colour = PaletteIndex::mutedPurple7;
                    break;

                case ElementType::tree:
                    colour = PaletteIndex::green6;
                    break;

                default:
                    break;
            }
        }

        return colour;
    }

    // 0x0046DB4C
    void drawScenarioPreviewImage()
    {
        auto& options = *_activeOptions;
        const auto kPreviewSize = sizeof(options.preview[0]);
        const auto kMapSkipFactor = TileManager::getMapRows() / kPreviewSize;

        for (auto y = 0U; y < kPreviewSize; y++)
        {
            for (auto x = 0U; x < kPreviewSize; x++)
            {
                auto pos = TilePos2(TileManager::getMapColumns() - (x + 1) * kMapSkipFactor + 1, y * kMapSkipFactor + 1);
                options.preview[y][x] = getPreviewColourByTilePos(pos);
            }
        }
    }

    static void drawSavePreviewImage(void* pixels, Ui::Size size)
    {
        auto mainViewport = WindowManager::getMainViewport();
        if (mainViewport == nullptr)
        {
            return;
        }

        const auto mapPosXY = mainViewport->getCentreMapPosition();
        const auto mapPosXYZ = Pos3(mapPosXY.x, mapPosXY.y, coord_t{ TileManager::getHeight(mapPosXY) });

        Viewport saveVp{};
        saveVp.x = 0;
        saveVp.y = 0;
        saveVp.width = size.width;
        saveVp.height = size.height;
        saveVp.flags = ViewportFlags::town_names_displayed | ViewportFlags::station_names_displayed;
        saveVp.zoom = ZoomLevel::half;
        saveVp.viewWidth = size.width << saveVp.zoom;
        saveVp.viewHeight = size.height << saveVp.zoom;

        const auto viewPos = saveVp.centre2dCoordinates(mapPosXYZ);
        saveVp.viewX = viewPos.x;
        saveVp.viewY = viewPos.y;

        Gfx::RenderTarget rt{};
        rt.bits = static_cast<uint8_t*>(pixels);
        rt.x = 0;
        rt.y = 0;
        rt.width = size.width;
        rt.height = size.height;
        rt.pitch = 0;
        rt.zoomLevel = saveVp.zoom;

        auto& drawingEngine = Gfx::getDrawingEngine();
        auto& drawingCtx = drawingEngine.getDrawingContext();

        drawingCtx.pushRenderTarget(rt);
        saveVp.render(drawingCtx);
        drawingCtx.popRenderTarget();
    }

    // 0x004471A4
    static std::unique_ptr<SaveDetails> prepareSaveDetails(GameState& gameState)
    {
        auto saveDetails = std::make_unique<SaveDetails>();
        const auto& playerCompany = gameState.companies[gameState.playerCompanies[0]];
        StringManager::formatString(saveDetails->company, sizeof(saveDetails->company), playerCompany.name);
        StringManager::formatString(saveDetails->owner, sizeof(saveDetails->owner), playerCompany.ownerName);
        saveDetails->date = gameState.currentDay;
        saveDetails->performanceIndex = playerCompany.performanceIndex;
        saveDetails->challengeProgress = playerCompany.challengeProgress;
        saveDetails->challengeFlags = playerCompany.challengeFlags;
        std::strncpy(saveDetails->scenario, gameState.scenarioName, sizeof(saveDetails->scenario));
        drawSavePreviewImage(saveDetails->image, { 250, 200 });
        return saveDetails;
    }

    /**
     * Removes all tile elements that have the ghost flag set.
     * Assumes all elements are organised in tile order.
     */
    static void removeGhostElements(std::vector<TileElement>& elements)
    {
        for (size_t i = 0; i < elements.size(); i++)
        {
            if (elements[i].isGhost())
            {
                if (elements[i].isLast())
                {
                    if (i == 0 || elements[i - 1].isLast())
                    {
                        // First element of tile, can not remove...
                    }
                    else
                    {
                        elements[i - 1].setLast(true);
                        elements.erase(elements.begin() + i);
                        i--;
                    }
                }
                else
                {
                    elements.erase(elements.begin() + i);
                    i--;
                }
            }
        }
    }

    static std::unique_ptr<S5File> prepareGameState(SaveFlags flags, const std::vector<ObjectHeader>& requiredObjects, const std::vector<ObjectHeader>& packedObjects)
    {
        auto mainWindow = WindowManager::getMainWindow();
        auto savedView = mainWindow != nullptr && mainWindow->viewports[0] != nullptr ? mainWindow->viewports[0]->toSavedView() : SavedViewSimple{ 0, 0, 0, 0 };

        auto file = std::make_unique<S5File>();
        file->header = prepareHeader(flags, packedObjects.size());
        if (file->header.type == S5Type::scenario || file->header.type == S5Type::landscape)
        {
            file->scenarioOptions = std::make_unique<Options>(_activeOptions);
        }
        if (file->header.hasFlags(HeaderFlags::hasSaveDetails))
        {
            file->saveDetails = prepareSaveDetails(_gameState);
        }
        std::memcpy(file->requiredObjects, requiredObjects.data(), sizeof(file->requiredObjects));
        file->gameState = _gameState;
        file->gameState.savedViewX = savedView.viewX;
        file->gameState.savedViewY = savedView.viewY;
        file->gameState.savedViewZoom = static_cast<uint8_t>(savedView.zoomLevel);
        file->gameState.savedViewRotation = savedView.rotation;
        file->gameState.magicNumber = kMagicNumber; // Match implementation at 0x004437FC

        auto tileElements = TileManager::getElements();
        file->tileElements.resize(tileElements.size());
        std::memcpy(file->tileElements.data(), tileElements.data(), tileElements.size_bytes());
        removeGhostElements(file->tileElements);
        return file;
    }

    static constexpr bool shouldPackObjects(SaveFlags flags)
    {
        return (flags & SaveFlags::raw) == SaveFlags::none
            && (flags & SaveFlags::dump) == SaveFlags::none
            && (flags & SaveFlags::packCustomObjects) != SaveFlags::none
            && !SceneManager::isNetworked();
    }

    // 0x00441C26
    bool exportGameStateToFile(const fs::path& path, SaveFlags flags)
    {
        FileStream fs(path, StreamMode::write);
        return exportGameStateToFile(fs, flags);
    }

    bool exportGameStateToFile(Stream& stream, SaveFlags flags)
    {
        if ((flags & SaveFlags::isAutosave) == SaveFlags::none)
        {
            Ui::ProgressBar::begin(StringIds::please_wait);
            Ui::ProgressBar::setProgress(20);
        }

        if ((flags & SaveFlags::noWindowClose) == SaveFlags::none
            && (flags & SaveFlags::raw) == SaveFlags::none
            && (flags & SaveFlags::dump) == SaveFlags::none)
        {
            WindowManager::closeConstructionWindows();
        }

        if ((flags & SaveFlags::raw) == SaveFlags::none)
        {
            TileManager::reorganise();
            EntityManager::resetSpatialIndex();
            EntityManager::zeroUnused();
            StationManager::zeroUnused();
            Vehicles::OrderManager::zeroUnusedOrderTable();
        }

        if ((flags & SaveFlags::isAutosave) == SaveFlags::none)
        {
            Ui::ProgressBar::setProgress(40);
        }

        bool saveResult;
        {
            auto requiredObjects = ObjectManager::getHeaders();
            std::vector<ObjectHeader> packedObjects;
            if (shouldPackObjects(flags))
            {
                std::copy_if(requiredObjects.begin(), requiredObjects.end(), std::back_inserter(packedObjects), [](ObjectHeader& header) {
                    return !header.isEmpty() && !header.isVanilla();
                });
            }

            auto file = prepareGameState(flags, requiredObjects, packedObjects);
            saveResult = exportGameState(stream, *file, packedObjects);
        }

        if ((flags & SaveFlags::isAutosave) == SaveFlags::none)
        {
            Ui::ProgressBar::setProgress(230);
        }

        if ((flags & SaveFlags::raw) == SaveFlags::none
            && (flags & SaveFlags::dump) == SaveFlags::none)
        {
            ObjectManager::reloadAll();
        }

        if ((flags & SaveFlags::isAutosave) == SaveFlags::none)
        {
            Ui::ProgressBar::end();
        }

        if (saveResult)
        {
            Gfx::invalidateScreen();
            if ((flags & SaveFlags::raw) == SaveFlags::none)
            {
                SceneManager::resetSceneAge();
            }

            return true;
        }

        return false;
    }

    static bool exportGameState(Stream& stream, const S5File& file, const std::vector<ObjectHeader>& packedObjects)
    {
        try
        {
            SawyerStreamWriter fs(stream);
            fs.writeChunk(SawyerEncoding::rotate, file.header);
            if (file.header.type == S5Type::scenario || file.header.type == S5Type::landscape)
            {
                fs.writeChunk(SawyerEncoding::rotate, *file.scenarioOptions);
            }
            if (file.header.hasFlags(HeaderFlags::hasSaveDetails))
            {
                fs.writeChunk(SawyerEncoding::rotate, *file.saveDetails);
            }
            if (file.header.numPackedObjects != 0)
            {
                ObjectManager::writePackedObjects(fs, packedObjects);
            }
            fs.writeChunk(SawyerEncoding::rotate, file.requiredObjects, sizeof(file.requiredObjects));

            if (file.header.type == S5Type::scenario)
            {
                fs.writeChunk(SawyerEncoding::runLengthSingle, file.gameState.rng, 0xB96C);
                fs.writeChunk(SawyerEncoding::runLengthSingle, file.gameState.towns, 0x123480);
                fs.writeChunk(SawyerEncoding::runLengthSingle, file.gameState.animations, 0x79D80);
            }
            else
            {
                fs.writeChunk(SawyerEncoding::runLengthSingle, file.gameState);
            }

            if (file.header.hasFlags(HeaderFlags::isRaw))
            {
                throw Exception::NotImplemented();
            }
            else
            {
                fs.writeChunk(SawyerEncoding::runLengthMulti, file.tileElements.data(), file.tileElements.size() * sizeof(TileElement));
            }

            fs.writeChecksum();
            return true;
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to save S5: {}", e.what());
            return false;
        }
    }

    // 0x00445A4A
    static void fixState(GameState& state)
    {
        if (state.hasFixFlags(S5FixFlags::fixFlag0))
        {
            state.fixFlags |= S5FixFlags::fixFlag1;
        }
        if (!state.hasFixFlags(S5FixFlags::fixFlag1))
        {
            // Shift data after companies to correct location
            auto src = reinterpret_cast<uint8_t*>(&state) + 0x49EA24;
            auto dst = src + 0x1C20; // std::size(Company::cargoUnitsDistanceHistory) * std::size(state.companies)
            for (size_t i = 0; i < 0x40E200; i++)
            {
                *--dst = *--src;
            }

            // Convert each company format from old to new
            for (size_t i = 0; i < std::size(state.companies); i++)
            {
                for (size_t j = 0; j < 372; j++)
                {
                    *--dst = *--src;
                }

                // Make space for cargoUnitsDistanceHistory
                for (size_t j = 0; j < 480; j++)
                {
                    *--dst = 0;
                }

                for (size_t j = 0; j < 35924; j++)
                {
                    *--dst = *--src;
                }
            }
        }
    }

    // 0x00441FC9
    std::unique_ptr<S5File> importSave(Stream& stream)
    {
        SawyerStreamReader fs(stream);
        if (!fs.validateChecksum())
        {
            throw Exception::RuntimeError("Invalid checksum");
        }

        auto file = std::make_unique<S5File>();

        // Read header
        fs.readChunk(&file->header, sizeof(file->header));

        // Read saved details 0x00442087
        if (file->header.hasFlags(HeaderFlags::hasSaveDetails))
        {
            file->saveDetails = std::make_unique<SaveDetails>();
            fs.readChunk(file->saveDetails.get(), sizeof(file->saveDetails));
        }
        if (file->header.type == S5Type::scenario)
        {
            file->scenarioOptions = std::make_unique<S5::Options>();
            fs.readChunk(&*file->scenarioOptions, sizeof(S5::Options));
        }
        // Read packed objects
        if (file->header.numPackedObjects > 0)
        {
            for (auto i = 0; i < file->header.numPackedObjects; ++i)
            {
                ObjectHeader object;
                fs.read(&object, sizeof(ObjectHeader));
                auto unownedObjectData = fs.readChunk();
                std::vector<std::byte> objectData;
                objectData.resize(unownedObjectData.size());
                std::copy(std::begin(unownedObjectData), std::end(unownedObjectData), std::begin(objectData));
                file->packedObjects.push_back(std::make_pair(object, std::move(objectData)));
            }
            // 0x004420B2
        }

        if (file->header.type == S5Type::scenario)
        {
            // Load required objects
            fs.readChunk(file->requiredObjects, sizeof(file->requiredObjects));

            // Load game state up to just before companies
            fs.readChunk(&file->gameState, sizeof(file->gameState));
            // Load game state towns industry and stations
            fs.readChunk(&file->gameState.towns, sizeof(file->gameState));
            // Load the rest of gamestate after animations
            fs.readChunk(&file->gameState.animations, sizeof(file->gameState));
            file->gameState.fixFlags |= S5FixFlags::fixFlag1;
            fixState(file->gameState);

            if ((file->gameState.flags & GameStateFlags::tileManagerLoaded) != GameStateFlags::none)
            {
                // Load tile elements
                auto tileElements = fs.readChunk();
                auto numTileElements = tileElements.size() / sizeof(TileElement);
                file->tileElements.resize(numTileElements);
                std::memcpy(file->tileElements.data(), tileElements.data(), numTileElements * sizeof(TileElement));
            }
        }
        else
        {
            // Load required objects
            fs.readChunk(file->requiredObjects, sizeof(file->requiredObjects));

            // Load game state
            fs.readChunk(&file->gameState, sizeof(file->gameState));
            fixState(file->gameState);

            // Load tile elements
            auto tileElements = fs.readChunk();
            auto numTileElements = tileElements.size() / sizeof(TileElement);
            file->tileElements.resize(numTileElements);
            std::memcpy(file->tileElements.data(), tileElements.data(), numTileElements * sizeof(TileElement));
        }

        return file;
    }

    // 0x00444D76
    static void setObjectErrorMessage(const ObjectHeader& header)
    {
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2040));
        StringManager::formatString(buffer, 512, StringIds::missing_object_data_id_x);
        objectCreateIdentifierName(strchr(buffer, 0), header);
        _loadErrorCode = 255;
        _loadErrorMessage = StringIds::buffer_2040;
    }

    static void setObjectErrorList(const std::vector<ObjectHeader>& list)
    {
        _loadErrorObjectsList = list;
    }

    const std::vector<ObjectHeader>& getObjectErrorList()
    {
        return _loadErrorObjectsList;
    }

    class LoadException : public std::runtime_error
    {
    private:
        StringId _localisedMessage;

    public:
        LoadException(const char* message, StringId localisedMessage)
            : std::runtime_error(message)
            , _localisedMessage(localisedMessage)
        {
        }

        StringId getLocalisedMessage() const
        {
            return _localisedMessage;
        }
    };

    void sub_4BAEC4() // TerraformConfig
    {
        addr<0x001136496, uint8_t>() = 2; // last tree rotation
        getGameState().lastTreeOption = 0xFF;
        getGameState().lastWallOption = 0xFF;
    }

    // 0x00441FA7
    bool importSaveToGameState(const fs::path& path, LoadFlags flags)
    {
        FileStream fs(path, StreamMode::read);
        return importSaveToGameState(fs, flags);
    }

    bool importSaveToGameState(Stream& stream, LoadFlags flags)
    {
        SceneManager::setGameSpeed(GameSpeed::Normal);
        if ((flags & LoadFlags::titleSequence) == LoadFlags::none
            && (flags & LoadFlags::twoPlayer) == LoadFlags::none)
        {
            WindowManager::closeConstructionWindows();
            WindowManager::closeAllFloatingWindows();
        }

        try
        {
            Ui::ProgressBar::begin(StringIds::loading);
            Ui::ProgressBar::setProgress(10);

            auto file = importSave(stream);

            Ui::ProgressBar::setProgress(90);

            if (file->header.version != kCurrentVersion)
            {
                throw LoadException("Unsupported S5 version", StringIds::error_file_contains_invalid_data);
            }

#ifdef DO_TITLE_SEQUENCE_CHECKS
            if ((flags & LoadFlags::titleSequence) != LoadFlags::none)
            {
                if (!file->header.hasFlags(HeaderFlags::isTitleSequence))
                {
                    throw LoadException("File was not a title sequence", StringIds::error_file_contains_invalid_data);
                }
            }
            else
            {
                if (file->header.hasFlags(HeaderFlags::isTitleSequence))
                {
                    throw LoadException("File is a title sequence", StringIds::error_file_contains_invalid_data);
                }
            }
#endif
            if (hasLoadFlags(flags, LoadFlags::landscape))
            {
                if (file->header.type != S5Type::scenario)
                {
                    _loadErrorCode = 255;
                    _loadErrorMessage = StringIds::error_file_contains_invalid_data;
                    Ui::ProgressBar::end();
                    return false;
                }
                if (static_cast<EditorController::Step>(file->scenarioOptions->editorStep) == EditorController::Step::null)
                {
                    file->scenarioOptions->editorStep = enumValue(EditorController::Step::landscapeEditor);
                }
            }

            Ui::ProgressBar::setProgress(100);

            // Any packed objects to install?
            if (!file->packedObjects.empty())
            {
                // For now installing objects can't be done with a progress bar
                // revert this when objects do not change the current game state
                Ui::ProgressBar::end();

                bool objectInstalled = false;

                for (auto [object, data] : file->packedObjects)
                {
                    if (ObjectManager::tryInstallObject(object, data))
                    {
                        objectInstalled = true;
                    }
                }

                if (objectInstalled)
                {
                    ObjectManager::loadIndex();
                }

                // See above. restart progress bar
                Ui::ProgressBar::begin(StringIds::loading);
            }

            Ui::ProgressBar::setProgress(150);

            if (file->header.type == S5Type::objects)
            {
                _gameState->var_014A = 0;
                _loadErrorCode = 254;
                _loadErrorMessage = StringIds::new_objects_installed_successfully;
                Ui::ProgressBar::end();
                // Throws!
                Game::returnToTitle();
            }

            if (!hasLoadFlags(flags, LoadFlags::scenario | LoadFlags::landscape))
            {
                if (file->header.type == S5Type::scenario)
                {
                    throw LoadException("File is a scenario, not a saved game", StringIds::error_file_contains_invalid_data);
                }
            }

            if (file->header.hasFlags(HeaderFlags::isRaw) || file->header.hasFlags(HeaderFlags::isDump))
            {
                throw LoadException("Unsupported S5 format", StringIds::error_file_contains_invalid_data);
            }

            if (hasLoadFlags(flags, LoadFlags::twoPlayer))
            {
                if (file->header.type != S5Type::landscape)
                {
                    throw LoadException("Not a two player saved game", StringIds::error_file_is_not_two_player_save);
                }
            }
            else if (!hasLoadFlags(flags, LoadFlags::scenario) && !hasLoadFlags(flags, LoadFlags::landscape))
            {
                if (file->header.type != S5Type::savedGame)
                {
                    throw LoadException("Not a single player saved game", StringIds::error_file_is_not_single_player_save);
                }
            }

            // Load required objects
            auto loadObjectResult = ObjectManager::loadAll(file->requiredObjects);
            if (!loadObjectResult.success)
            {
                setObjectErrorMessage(loadObjectResult.problemObjects[0]);
                setObjectErrorList(loadObjectResult.problemObjects);
                if (hasLoadFlags(flags, LoadFlags::twoPlayer))
                {
                    CompanyManager::reset();
                    _gameState->var_014A = 0;
                    Ui::ProgressBar::end();
                    return false;
                }
                else
                {
                    Ui::ProgressBar::end();
                    Game::returnToTitle();
                    return false;
                }
            }

            ObjectManager::reloadAll();
            Ui::ProgressBar::setProgress(200);

            _gameState = file->gameState;
            if (hasLoadFlags(flags, LoadFlags::scenario | LoadFlags::landscape))
            {
                _activeOptions = *file->scenarioOptions;
            }
            if ((file->gameState.flags & GameStateFlags::tileManagerLoaded) != GameStateFlags::none)
            {
                TileManager::setElements(std::span<World::TileElement>(reinterpret_cast<World::TileElement*>(file->tileElements.data()), file->tileElements.size()));
            }
            else
            {
                World::TileManager::initialise();
                Scenario::sub_46115C();
            }
            if (hasLoadFlags(flags, LoadFlags::landscape))
            {
                EntityManager::freeUserStrings();
            }
            if (hasLoadFlags(flags, LoadFlags::scenario | LoadFlags::landscape))
            {
                CompanyManager::reset();
                EntityManager::reset();
            }

            Audio::stopVehicleNoise();
            EntityManager::resetSpatialIndex();
            CompanyManager::updateColours();
            ObjectManager::sub_4748FA();
            TileManager::resetSurfaceClearance();
            IndustryManager::createAllMapAnimations();

            Ui::ProgressBar::setProgress(225);

            if (hasLoadFlags(flags, LoadFlags::landscape))
            {
                Scenario::initialiseSnowLine();
                auto* stexObj = ObjectManager::get<ScenarioTextObject>();
                if (stexObj != nullptr)
                {
                    auto header = ObjectManager::getHeader(LoadedObjectHandle{ ObjectType::scenarioText, 0 });
                    ObjectManager::unload(header);
                    ObjectManager::reloadAll();
                    ObjectManager::sub_4748FA();
                    _activeOptions->editorStep = enumValue(EditorController::Step::landscapeEditor);
                    _activeOptions->difficulty = 3;
                    StringManager::formatString(_activeOptions->scenarioDetails, StringIds::no_details_yet);
                    _activeOptions->scenarioName[0] = '\0';
                }
            }
            Audio::resetSoundObjects();

            if (hasLoadFlags(flags, LoadFlags::scenario))
            {
                _gameState->var_014A = 0;
                Ui::ProgressBar::end();
                return true;
            }
            if (!hasLoadFlags(flags, LoadFlags::titleSequence))
            {
                SceneManager::removeSceneFlags(SceneManager::Flags::title);
                initialiseViewports();
                Audio::resetMusic();
                if (hasLoadFlags(flags, LoadFlags::landscape))
                {
                    SceneManager::addSceneFlags(SceneManager::Flags::editor);
                    EditorController::showEditor();
                }
                else
                {
                    Gui::init();
                }
            }

            Ui::ProgressBar::setProgress(245);

            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                SavedViewSimple savedView;
                savedView.viewX = file->gameState.savedViewX;
                savedView.viewY = file->gameState.savedViewY;
                savedView.zoomLevel = static_cast<ZoomLevel>(file->gameState.savedViewZoom);
                savedView.rotation = file->gameState.savedViewRotation;
                mainWindow->viewportFromSavedView(savedView);
                mainWindow->invalidate();
            }

            EntityManager::updateSpatialIndex();
            TownManager::updateLabels();
            StationManager::updateLabels();
            sub_4BAEC4();
            addr<0x0052334E, uint16_t>() = 0; // _thousandthTickCounter
            Gfx::invalidateScreen();
            if (!hasLoadFlags(flags, LoadFlags::landscape))
            {
                Scenario::loadPreferredCurrencyAlways();
            }
            Gfx::loadCurrency();
            _gameState->var_014A = 0;

            if (hasLoadFlags(flags, LoadFlags::titleSequence))
            {
                ScenarioManager::setScenarioTicks(ScenarioManager::getScenarioTicks() - 1);
                ScenarioManager::setScenarioTicks2(ScenarioManager::getScenarioTicks2() - 1);
                World::TileManager::disablePeriodicDefrag();
            }

            Ui::ProgressBar::end();

            if (!hasLoadFlags(flags, LoadFlags::titleSequence) && !hasLoadFlags(flags, LoadFlags::twoPlayer) && !hasLoadFlags(flags, LoadFlags::landscape))
            {
                SceneManager::resetSceneAge();
                throw GameException::Interrupt;
            }

            return true;
        }
        catch (const LoadException& e)
        {
            Logging::error("Unable to load S5: {}", e.what());
            _loadErrorCode = 255;
            _loadErrorMessage = e.getLocalisedMessage();
            Ui::ProgressBar::end();
            return false;
        }
        catch (const std::exception& e)
        {
            Logging::error("Unable to load S5: {}", e.what());
            _loadErrorCode = 255;
            _loadErrorMessage = StringIds::null;
            Ui::ProgressBar::end();
            return false;
        }
    }

    // 0x00442403
    std::unique_ptr<SaveDetails> readSaveDetails(const fs::path& path)
    {
        FileStream stream(path, StreamMode::read);
        SawyerStreamReader fs(stream);
        if (!fs.validateChecksum())
        {
            return nullptr;
        }

        Header s5Header{};

        // Read header
        fs.readChunk(&s5Header, sizeof(s5Header));

        if (s5Header.version != kCurrentVersion)
        {
            return nullptr;
        }

        if (s5Header.hasFlags(HeaderFlags::isTitleSequence | HeaderFlags::isDump | HeaderFlags::isRaw))
        {
            return nullptr;
        }

        if (s5Header.hasFlags(HeaderFlags::hasSaveDetails))
        {
            // 0x0050AEA8
            auto ret = std::make_unique<SaveDetails>();
            fs.readChunk(ret.get(), sizeof(*ret));
            return ret;
        }
        return nullptr;
    }

    // 0x00442AFC
    std::unique_ptr<Scenario::Options> readScenarioOptions(const fs::path& path)
    {
        FileStream stream(path, StreamMode::read);
        SawyerStreamReader fs(stream);
        if (!fs.validateChecksum())
        {
            return nullptr;
        }

        Header s5Header{};

        // Read header
        fs.readChunk(&s5Header, sizeof(s5Header));

        if (s5Header.version != kCurrentVersion)
        {
            return nullptr;
        }

        if (s5Header.type == S5Type::scenario)
        {
            // 0x009DA285 = 1
            // 0x009CCA54 _previewOptions

            // TODO: For now OpenLoco::Options and S5::Options are identical in the future
            // there should be an import and validation step that converts from one to the
            // other
            auto ret = std::make_unique<Scenario::Options>();
            fs.readChunk(ret.get(), sizeof(*ret));
            return ret;
        }
        return nullptr;
    }

    void registerHooks()
    {
        registerHook(
            0x00441C26,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto path = fs::u8path(std::string(_savePath));
                const auto res = exportGameStateToFile(path, static_cast<SaveFlags>(regs.eax)) ? 0 : X86_FLAG_CARRY;
                regs = backup;
                return res;
            });
        registerHook(
            0x00441FA7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto path = fs::u8path(std::string(_savePath));
                const auto res = importSaveToGameState(path, static_cast<LoadFlags>(regs.eax)) ? X86_FLAG_CARRY : 0;
                regs = backup;
                return res;
            });
    }
}
