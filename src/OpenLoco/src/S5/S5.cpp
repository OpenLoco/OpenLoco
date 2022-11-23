#define DO_TITLE_SEQUENCE_CHECKS

#include "S5.h"
#include "Audio/Audio.h"
#include "CompanyManager.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameException.hpp"
#include "Gui.h"
#include "IndustryManager.h"
#include "Interop/Interop.hpp"
#include "LastGameOptionManager.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/TileManager.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "SawyerStream.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "StationManager.h"
#include "TownManager.h"
#include "Ui/WindowManager.h"
#include "Utility/Exception.hpp"
#include "Utility/Stream.hpp"
#include "Vehicles/Orders.h"
#include "ViewportManager.h"
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;

namespace OpenLoco::S5
{
    constexpr uint32_t currentVersion = 0x62262;
    constexpr uint32_t magicNumber = 0x62300;

    static loco_global<GameState, 0x00525E18> _gameState;
    static loco_global<Options, 0x009C8714> _activeOptions;
    static loco_global<Header, 0x009CCA34> _header;
    static loco_global<char[512], 0x0112CE04> _savePath;
    static loco_global<uint8_t, 0x00508F1A> _gameSpeed;
    static loco_global<uint8_t, 0x0050C197> _loadErrorCode;
    static loco_global<string_id, 0x0050C198> _loadErrorMessage;

    static bool exportGameState(Stream& stream, const S5File& file, const std::vector<ObjectHeader>& packedObjects);

    Options& getOptions()
    {
        return _activeOptions;
    }

    static Header prepareHeader(uint32_t flags, size_t numPackedObjects)
    {
        Header result;
        std::memset(&result, 0, sizeof(result));

        result.type = S5Type::savedGame;
        if (flags & SaveFlags::landscape)
            result.type = S5Type::landscape;
        if (flags & SaveFlags::scenario)
            result.type = S5Type::scenario;

        result.numPackedObjects = static_cast<uint16_t>(numPackedObjects);
        result.version = currentVersion;
        result.magic = magicNumber;

        if (flags & SaveFlags::raw)
        {
            result.flags |= S5Flags::isRaw;
        }
        if (flags & SaveFlags::dump)
        {
            result.flags |= S5Flags::isDump;
        }
        if (!(flags & SaveFlags::scenario) && !(flags & SaveFlags::raw) && !(flags & SaveFlags::dump))
        {
            result.flags |= S5Flags::hasSaveDetails;
        }

        return result;
    }

    // 0x0045A0B3
    static void previewWindowDraw(Window& w, Gfx::RenderTarget* rt)
    {
        for (auto viewport : w.viewports)
        {
            if (viewport != nullptr)
            {
                viewport->render(rt);
            }
        }
    }

    static void drawPreviewImage(void* pixels, Ui::Size size)
    {
        auto mainViewport = WindowManager::getMainViewport();
        if (mainViewport != nullptr)
        {
            auto mapPosXY = mainViewport->getCentreMapPosition();
            auto mapPosXYZ = Pos3(mapPosXY.x, mapPosXY.y, coord_t{ TileManager::getHeight(mapPosXY) });

            static WindowEventList eventList; // 0x4FB3F0
            eventList.draw = previewWindowDraw;

            auto tempWindow = WindowManager::createWindow(
                WindowType::previewImage,
                { 0, 0 },
                size,
                WindowFlags::stickToFront,
                &eventList);
            if (tempWindow != nullptr)
            {
                auto tempViewport = ViewportManager::create(
                    tempWindow,
                    0,
                    { tempWindow->x, tempWindow->y },
                    { tempWindow->width, tempWindow->height },
                    ZoomLevel::half,
                    mapPosXYZ);
                if (tempViewport != nullptr)
                {
                    tempViewport->flags = ViewportFlags::town_names_displayed | ViewportFlags::station_names_displayed;

                    // Swap screen Context with our temporary one to draw the window then revert it back
                    auto& rt = Gfx::getScreenRT();
                    auto backupContext = rt;
                    rt.bits = reinterpret_cast<uint8_t*>(pixels);
                    rt.x = 0;
                    rt.y = 0;
                    rt.width = size.width;
                    rt.height = size.height;
                    rt.pitch = 0;
                    rt.zoomLevel = 0;
                    Gfx::redrawScreenRect(0, 0, size.width, size.height);
                    rt = backupContext;
                }

                WindowManager::close(WindowType::previewImage);
            }
        }
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
        drawPreviewImage(saveDetails->image, { 250, 200 });
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

    static std::unique_ptr<S5File> prepareGameState(uint32_t flags, const std::vector<ObjectHeader>& requiredObjects, const std::vector<ObjectHeader>& packedObjects)
    {
        auto mainWindow = WindowManager::getMainWindow();
        auto savedView = mainWindow != nullptr && mainWindow->viewports[0] != nullptr ? mainWindow->viewports[0]->toSavedView() : SavedViewSimple{ 0, 0, 0, 0 };

        auto file = std::make_unique<S5File>();
        file->header = prepareHeader(flags, packedObjects.size());
        if (file->header.type == S5Type::scenario || file->header.type == S5Type::landscape)
        {
            file->landscapeOptions = std::make_unique<Options>(_activeOptions);
        }
        if (file->header.flags & S5Flags::hasSaveDetails)
        {
            file->saveDetails = prepareSaveDetails(_gameState);
        }
        std::memcpy(file->requiredObjects, requiredObjects.data(), sizeof(file->requiredObjects));
        file->gameState = _gameState;
        file->gameState.savedViewX = savedView.viewX;
        file->gameState.savedViewY = savedView.viewY;
        file->gameState.savedViewZoom = static_cast<uint8_t>(savedView.zoomLevel);
        file->gameState.savedViewRotation = savedView.rotation;
        file->gameState.magicNumber = magicNumber; // Match implementation at 0x004437FC

        auto tileElements = TileManager::getElements();
        file->tileElements.resize(tileElements.size());
        std::memcpy(file->tileElements.data(), tileElements.data(), tileElements.size_bytes());
        removeGhostElements(file->tileElements);
        return file;
    }

    static constexpr bool shouldPackObjects(uint32_t flags)
    {
        return !(flags & SaveFlags::raw) && !(flags & SaveFlags::dump) && (flags & SaveFlags::packCustomObjects) && !isNetworked();
    }

    // 0x00441C26
    bool exportGameStateToFile(const fs::path& path, uint32_t flags)
    {
        FileStream fs(path, StreamFlags::write);
        return exportGameStateToFile(fs, flags);
    }

    bool exportGameStateToFile(Stream& stream, uint32_t flags)
    {
        if (!(flags & SaveFlags::noWindowClose) && !(flags & SaveFlags::raw) && !(flags & SaveFlags::dump))
        {
            WindowManager::closeConstructionWindows();
        }

        if (!(flags & SaveFlags::raw))
        {
            TileManager::reorganise();
            EntityManager::resetSpatialIndex();
            EntityManager::zeroUnused();
            StationManager::zeroUnused();
            Vehicles::zeroOrderTable();
        }

        bool saveResult;
        {
            auto requiredObjects = ObjectManager::getHeaders();
            std::vector<ObjectHeader> packedObjects;
            if (shouldPackObjects(flags))
            {
                std::copy_if(requiredObjects.begin(), requiredObjects.end(), std::back_inserter(packedObjects), [](ObjectHeader& header) {
                    return header.isCustom();
                });
            }

            auto file = prepareGameState(flags, requiredObjects, packedObjects);
            saveResult = exportGameState(stream, *file, packedObjects);
        }

        if (!(flags & SaveFlags::raw) && !(flags & SaveFlags::dump))
        {
            ObjectManager::reloadAll();
        }

        if (saveResult)
        {
            Gfx::invalidateScreen();
            if (!(flags & SaveFlags::raw))
            {
                resetScreenAge();
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
                fs.writeChunk(SawyerEncoding::rotate, *file.landscapeOptions);
            }
            if (file.header.flags & S5Flags::hasSaveDetails)
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

            if (file.header.flags & SaveFlags::raw)
            {
                throw NotImplementedException();
            }
            else
            {
                fs.writeChunk(SawyerEncoding::runLengthMulti, file.tileElements.data(), file.tileElements.size() * sizeof(TileElement));
            }

            fs.writeChecksum();
            fs.close();
            return true;
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to save S5: %s\n", e.what());
            return false;
        }
    }

    // 0x00445A4A
    static void fixState(GameState& state)
    {
        if (state.fixFlags & S5FixFlags::fixFlag0)
        {
            state.fixFlags |= S5FixFlags::fixFlag1;
        }
        if (!(state.fixFlags & S5FixFlags::fixFlag1))
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
    static std::unique_ptr<S5File> importSave(Stream& stream)
    {
        SawyerStreamReader fs(stream);
        if (!fs.validateChecksum())
        {
            throw std::runtime_error("Invalid checksum");
        }

        auto file = std::make_unique<S5File>();

        // Read header
        fs.readChunk(&file->header, sizeof(file->header));

        // Read saved details 0x00442087
        if (file->header.flags & S5Flags::hasSaveDetails)
        {
            file->saveDetails = std::make_unique<SaveDetails>();
            fs.readChunk(file->saveDetails.get(), sizeof(file->saveDetails));
        }
        if (file->header.type == S5Type::scenario)
        {
            file->landscapeOptions = std::make_unique<S5::Options>();
            fs.readChunk(&*file->landscapeOptions, sizeof(S5::Options));
        }
        // Read packed objects
        if (file->header.numPackedObjects > 0)
        {
            for (auto i = 0; i < file->header.numPackedObjects; ++i)
            {
                ObjectHeader object;
                fs.read(&object, sizeof(ObjectHeader));
                auto unownedObjectData = fs.readChunk();
                std::vector<uint8_t> objectData;
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

            if (file->gameState.flags & (1 << 0))
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

    // 0x00473BC7
    static void objectCreateIdentifierName(char* dst, const ObjectHeader& header)
    {
        for (auto& c : header.name)
        {
            if (c != ' ')
            {
                *dst++ = c;
            }
        }
        *dst++ = '/';
        *dst = '\0';
        std::stringstream ss;
        ss << std::uppercase << std::setfill('0') << std::hex << std::setw(8) << header.flags << std::setw(8) << header.checksum;
        const auto flagsChecksum = ss.str();
        strcat(dst, flagsChecksum.c_str());
    }

    // 0x00444D76
    static void setObjectErrorMessage(const ObjectHeader& header)
    {
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2040));
        StringManager::formatString(buffer, sizeof(buffer), StringIds::missing_object_data_id_x);
        objectCreateIdentifierName(strchr(buffer, 0), header);
        _loadErrorCode = 255;
        _loadErrorMessage = StringIds::buffer_2040;
    }

    class LoadException : public std::runtime_error
    {
    private:
        string_id _localisedMessage;

    public:
        LoadException(const char* message, string_id localisedMessage)
            : std::runtime_error(message)
            , _localisedMessage(localisedMessage)
        {
        }

        string_id getLocalisedMessage() const
        {
            return _localisedMessage;
        }
    };

    void sub_4BAEC4() // TerraformConfig
    {
        addr<0x001136496, uint8_t>() = 2;
        LastGameOptionManager::setLastTree(LastGameOptionManager::kNoLastOption);
        LastGameOptionManager::setLastWall(LastGameOptionManager::kNoLastOption);
    }

    // 0x00441FA7
    bool importSaveToGameState(const fs::path& path, uint32_t flags)
    {
        FileStream fs(path, StreamFlags::read);
        return importSaveToGameState(fs, flags);
    }

    bool importSaveToGameState(Stream& stream, uint32_t flags)
    {
        _gameSpeed = 0;
        if (!(flags & LoadFlags::titleSequence) && !(flags & LoadFlags::twoPlayer))
        {
            WindowManager::closeConstructionWindows();
            WindowManager::closeAllFloatingWindows();
        }

        try
        {
            auto file = importSave(stream);

            if (file->header.version != currentVersion)
            {
                throw LoadException("Unsupported S5 version", StringIds::error_file_contains_invalid_data);
            }

#ifdef DO_TITLE_SEQUENCE_CHECKS
            if (flags & LoadFlags::titleSequence)
            {
                if (!(file->header.flags & S5Flags::isTitleSequence))
                {
                    throw LoadException("File was not a title sequence", StringIds::error_file_contains_invalid_data);
                }
            }
            else
            {
                if (file->header.flags & S5Flags::isTitleSequence)
                {
                    throw LoadException("File is a title sequence", StringIds::error_file_contains_invalid_data);
                }
            }
#endif
            if (!file->packedObjects.empty())
            {
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
            }
            if (file->header.type == S5Type::objects)
            {
                addr<0x00525F62, uint16_t>() = 0;
                _loadErrorCode = 254;
                _loadErrorMessage = StringIds::new_objects_installed_successfully;
                // Throws!
                Game::returnToTitle();
            }
            if (!(flags & LoadFlags::scenario))
            {
                if (file->header.type == S5Type::scenario)
                {
                    throw LoadException("File is a scenario, not a saved game", StringIds::error_file_contains_invalid_data);
                }
            }

            if ((file->header.flags & S5Flags::isRaw) || (file->header.flags & S5Flags::isDump))
            {
                throw LoadException("Unsupported S5 format", StringIds::error_file_contains_invalid_data);
            }

            if (flags & LoadFlags::twoPlayer)
            {
                if (file->header.type != S5Type::landscape)
                {
                    throw LoadException("Not a two player saved game", StringIds::error_file_is_not_two_player_save);
                }
            }
            else if (!(flags & LoadFlags::scenario))
            {
                if (file->header.type != S5Type::savedGame)
                {
                    throw LoadException("Not a single player saved game", StringIds::error_file_is_not_single_player_save);
                }
            }

            auto loadObjectResult = ObjectManager::loadAll(file->requiredObjects);
            if (!loadObjectResult.success)
            {
                setObjectErrorMessage(loadObjectResult.problemObject);
                if (flags & LoadFlags::twoPlayer)
                {
                    CompanyManager::reset();
                    addr<0x00525F62, uint16_t>() = 0;
                    return false;
                }
                else
                {
                    Game::returnToTitle();
                    return false;
                }
            }

            ObjectManager::reloadAll();

            _gameState = file->gameState;
            if (flags & LoadFlags::scenario)
            {
                _activeOptions = *file->landscapeOptions;
            }
            if (file->gameState.flags & (1 << 0))
            {
                TileManager::setElements(stdx::span<Map::TileElement>(reinterpret_cast<Map::TileElement*>(file->tileElements.data()), file->tileElements.size()));
            }
            else
            {
                Map::TileManager::initialise();
                Scenario::sub_46115C();
            }
            if (flags & LoadFlags::scenario)
            {
                CompanyManager::reset();
                EntityManager::reset();
            }

            EntityManager::resetSpatialIndex();
            CompanyManager::updateColours();
            call(0x004748FA);
            TileManager::resetSurfaceClearance();
            IndustryManager::createAllMapAnimations();
            Audio::resetSoundObjects();

            if (flags & LoadFlags::scenario)
            {
                _gameState->var_014A = 0;
                return true;
            }
            if (!(flags & LoadFlags::titleSequence))
            {
                clearScreenFlag(ScreenFlags::title);
                initialiseViewports();
                Gui::init();
                Audio::resetMusic();
            }

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
            call(0x004C153B);
            call(0x0046E07B); // load currency gfx
            addr<0x00525F62, uint16_t>() = 0;

            if (flags & LoadFlags::titleSequence)
            {
                ScenarioManager::setScenarioTicks(ScenarioManager::getScenarioTicks() - 1);
                ScenarioManager::setScenarioTicks2(ScenarioManager::getScenarioTicks2() - 1);
                addr<0x0050BF6C, uint8_t>() = 1;
            }

            if (!(flags & LoadFlags::titleSequence) && !(flags & LoadFlags::twoPlayer))
            {
                resetScreenAge();
                throw GameException::Interrupt;
            }

            return true;
        }
        catch (const LoadException& e)
        {
            std::fprintf(stderr, "Unable to load S5: %s\n", e.what());
            _loadErrorCode = 255;
            _loadErrorMessage = e.getLocalisedMessage();
            return false;
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to load S5: %s\n", e.what());
            _loadErrorCode = 255;
            _loadErrorMessage = StringIds::null;
            return false;
        }
    }

    // 0x00442403
    std::unique_ptr<SaveDetails> peekSaveDetails(const fs::path& path)
    {
        FileStream stream(path, StreamFlags::read);
        SawyerStreamReader fs(stream);
        if (!fs.validateChecksum())
        {
            return nullptr;
        }

        Header s5Header{};

        // Read header
        fs.readChunk(&s5Header, sizeof(s5Header));

        if (s5Header.version != currentVersion)
        {
            return nullptr;
        }

        if (s5Header.flags & (S5Flags::isTitleSequence | S5Flags::isDump | S5Flags::isRaw))
        {
            return nullptr;
        }

        if (s5Header.flags & S5Flags::hasSaveDetails)
        {
            // 0x0050AEA8
            auto ret = std::make_unique<SaveDetails>();
            fs.readChunk(ret.get(), sizeof(*ret));
            return ret;
        }
        return nullptr;
    }

    // 0x00442AFC
    std::unique_ptr<Options> peekScenarioOptions(const fs::path& path)
    {
        FileStream stream(path, StreamFlags::read);
        SawyerStreamReader fs(stream);
        if (!fs.validateChecksum())
        {
            return nullptr;
        }

        Header s5Header{};

        // Read header
        fs.readChunk(&s5Header, sizeof(s5Header));

        if (s5Header.version != currentVersion)
        {
            return nullptr;
        }

        if (s5Header.type == S5Type::scenario)
        {
            // 0x009DA285 = 1
            // 0x009CCA54 _previewOptions
            auto ret = std::make_unique<Options>();
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
                auto path = fs::u8path(std::string(_savePath));
                return exportGameStateToFile(path, regs.eax) ? 0 : X86_FLAG_CARRY;
            });
        registerHook(
            0x00441FA7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto path = fs::u8path(std::string(_savePath));
                return importSaveToGameState(path, regs.eax) ? X86_FLAG_CARRY : 0;
            });
    }
}
