#define DO_TITLE_SEQUENCE_CHECKS

#include "S5.h"
#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Entities/EntityManager.h"
#include "../Game.h"
#include "../GameException.hpp"
#include "../Gui.h"
#include "../IndustryManager.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Localisation/StringManager.h"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Exception.hpp"
#include "../Vehicles/Orders.h"
#include "../ViewportManager.h"
#include "SawyerStream.h"
#include <fstream>

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
    static loco_global<Options, 0x009CCA54> _previewOptions;
    static loco_global<char[512], 0x0112CE04> _savePath;
    static loco_global<uint8_t, 0x00508F1A> _gameSpeed;
    static loco_global<uint8_t, 0x0050C197> _loadErrorCode;
    static loco_global<string_id, 0x0050C198> _loadErrorMessage;

    static bool save(const fs::path& path, const S5File& file, const std::vector<ObjectHeader>& packedObjects);

    Options& getOptions()
    {
        return _activeOptions;
    }

    Options& getPreviewOptions()
    {
        return _previewOptions;
    }

    static Header prepareHeader(SaveFlags flags, size_t numPackedObjects)
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
    static void previewWindowDraw(Window* w, Gfx::Context* context)
    {
        for (auto viewport : w->viewports)
        {
            if (viewport != nullptr)
            {
                viewport->render(context);
            }
        }
    }

    static void drawPreviewImage(void* pixels, Gfx::ui_size_t size)
    {
        auto mainViewport = WindowManager::getMainViewport();
        if (mainViewport != nullptr)
        {
            auto mapPosXY = mainViewport->getCentreMapPosition();
            auto mapPosXYZ = Pos3(mapPosXY.x, mapPosXY.y, TileManager::getHeight(mapPosXY));

            static WindowEventList eventList; // 0x4FB3F0
            eventList.draw = previewWindowDraw;

            auto tempWindow = WindowManager::createWindow(
                WindowType::previewImage,
                { 0, 0 },
                size,
                WindowFlags::stick_to_front,
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
                    auto& context = Gfx::screenContext();
                    auto backupContext = context;
                    context.bits = reinterpret_cast<uint8_t*>(pixels);
                    context.x = 0;
                    context.y = 0;
                    context.width = size.width;
                    context.height = size.height;
                    context.pitch = 0;
                    context.zoom_level = 0;
                    Gfx::redrawScreenRect(0, 0, size.width, size.height);
                    context = backupContext;
                }

                WindowManager::close(WindowType::previewImage);
            }
        }
    }

    // 0x004471A4
    static std::unique_ptr<SaveDetails> prepareSaveDetails(GameState& gameState)
    {
        auto saveDetails = std::make_unique<SaveDetails>();
        const auto& playerCompany = gameState.companies[gameState.playerCompanyId];
        StringManager::formatString(saveDetails->company, sizeof(saveDetails->company), playerCompany.name);
        StringManager::formatString(saveDetails->owner, sizeof(saveDetails->owner), playerCompany.ownerName);
        saveDetails->date = gameState.currentDay;
        saveDetails->performance_index = playerCompany.performanceIndex;
        saveDetails->challenge_progress = playerCompany.challengeProgress;
        saveDetails->challenge_flags = playerCompany.challengeFlags;
        std::strncpy(saveDetails->scenario, gameState.scenarioName, sizeof(saveDetails->scenario));
        drawPreviewImage(saveDetails->image, { 250, 200 });
        return saveDetails;
    }

    static constexpr SawyerEncoding getBestEncodingForObjectType(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::competitor:
                return SawyerEncoding::uncompressed;
            default:
                return SawyerEncoding::runLengthSingle;
            case ObjectType::currency:
                return SawyerEncoding::runLengthMulti;
            case ObjectType::townNames:
            case ObjectType::scenarioText:
                return SawyerEncoding::rotate;
        }
    }

    // 0x00472633
    // 0x004722FF
    static void writePackedObjects(SawyerStreamWriter& fs, const std::vector<ObjectHeader>& packedObjects)
    {
        // TODO at some point, change this to just pack the object file directly from
        //      disc rather than using the in-memory version. This then avoids having
        //      to unload the object temporarily to save the S5.
        for (const auto& header : packedObjects)
        {
            auto index = ObjectManager::findIndex(header);
            if (index)
            {
                // Unload the object so that the object data is restored to
                // its original file state
                ObjectManager::unload(*index);

                auto encodingType = getBestEncodingForObjectType(header.getType());
                auto obj = ObjectManager::get<Object>(*index);
                auto objSize = ObjectManager::getByteLength(*index);

                fs.write(header);
                fs.writeChunk(encodingType, obj, objSize);
            }
            else
            {
                throw std::runtime_error("Unable to pack object: object not loaded");
            }
        }
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

    static std::unique_ptr<S5File> prepareSaveFile(SaveFlags flags, const std::vector<ObjectHeader>& requiredObjects, const std::vector<ObjectHeader>& packedObjects)
    {
        auto mainWindow = WindowManager::getMainWindow();
        auto savedView = mainWindow != nullptr ? mainWindow->viewports[0]->toSavedView() : SavedViewSimple();

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
        file->gameState.savedViewX = savedView.mapX;
        file->gameState.savedViewY = savedView.mapY;
        file->gameState.savedViewZoom = static_cast<uint8_t>(savedView.zoomLevel);
        file->gameState.savedViewRotation = savedView.rotation;
        file->gameState.magicNumber = magicNumber; // Match implementation at 0x004437FC

        auto tileElements = TileManager::getElements();
        file->tileElements.resize(tileElements.size());
        std::memcpy(file->tileElements.data(), tileElements.data(), tileElements.size_bytes());
        removeGhostElements(file->tileElements);
        return file;
    }

    static constexpr bool shouldPackObjects(SaveFlags flags)
    {
        return !(flags & SaveFlags::raw) && !(flags & SaveFlags::dump) && (flags & SaveFlags::packCustomObjects) && !isNetworked();
    }

    // 0x00441C26
    bool save(const fs::path& path, SaveFlags flags)
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

            auto file = prepareSaveFile(flags, requiredObjects, packedObjects);
            saveResult = save(path, *file, packedObjects);
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

    static bool save(const fs::path& path, const S5File& file, const std::vector<ObjectHeader>& packedObjects)
    {
        try
        {
            SawyerStreamWriter fs(path);
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
                writePackedObjects(fs, packedObjects);
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
            auto dst = src + 0x1C20;
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
    static std::unique_ptr<S5File> load(const fs::path& path)
    {
        SawyerStreamReader fs(path);
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

        // Read packed objects
        if (file->header.numPackedObjects > 0)
        {
            bool objectInstalled = false;
            for (auto i = 0; i < file->header.numPackedObjects; ++i)
            {
                ObjectHeader object;
                fs.read(&object, sizeof(ObjectHeader));
                if (ObjectManager::tryInstallObject(object, fs.readChunk()))
                {
                    objectInstalled = true;
                }
            }

            if (objectInstalled)
            {
                ObjectManager::loadIndex();
            }
            // 0x004420B2
        }

        if (file->header.type == S5Type::objects)
        {
            addr<0x00525F62, uint16_t>() = 0;
            _loadErrorCode = 254;
            _loadErrorMessage = StringIds::new_objects_installed_successfully;
            // Throws!
            Game::returnToTitle();
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
    static void object_create_identifier_name(char* dst, const ObjectHeader& header)
    {
        registers regs;
        regs.edi = reinterpret_cast<int32_t>(dst);
        regs.ebp = reinterpret_cast<int32_t>(&header);
        call(0x00473BC7, regs);
    }

    // 0x00444D76
    static void setObjectErrorMessage(const ObjectHeader& header)
    {
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2040));
        StringManager::formatString(buffer, sizeof(buffer), StringIds::missing_object_data_id_x);
        object_create_identifier_name(strchr(buffer, 0), header);
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

    static void sub_42F7F8()
    {
        call(0x0042F7F8);
    }

    static void sub_4BAEC4()
    {
        addr<0x001136496, uint8_t>() = 2;
        addr<0x00525FB1, uint8_t>() = 255;
        addr<0x00525FCA, uint8_t>() = 255;
    }

    // 0x00441FA7
    bool load(const fs::path& path, uint32_t flags)
    {
        _gameSpeed = 0;
        if (!(flags & LoadFlags::titleSequence) && !(flags & LoadFlags::twoPlayer))
        {
            WindowManager::closeConstructionWindows();
            WindowManager::closeAllFloatingWindows();
        }

        try
        {
            auto file = load(path);

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

            if (file->header.type == S5Type::scenario)
            {
                throw LoadException("File is a scenario, not a saved game", StringIds::error_file_contains_invalid_data);
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
            else
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
                    sub_42F7F8();
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
            TileManager::setElements(stdx::span<Map::TileElement>(reinterpret_cast<Map::TileElement*>(file->tileElements.data()), file->tileElements.size()));

            EntityManager::resetSpatialIndex();
            CompanyManager::updateColours();
            call(0x004748FA);
            TileManager::resetSurfaceClearance();
            IndustryManager::createAllMapAnimations();

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
                savedView.mapX = file->gameState.savedViewX;
                savedView.mapY = file->gameState.savedViewY;
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
                addr<0x00525F5E, uint32_t>()--; // _scenario_ticks
                addr<0x00525F64, uint32_t>()--; // _scenario_ticks2
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

    void registerHooks()
    {
        registerHook(
            0x00441C26,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto path = fs::u8path(std::string(_savePath));
                return save(path, static_cast<SaveFlags>(regs.eax)) ? 0 : X86_FLAG_CARRY;
            });
        registerHook(
            0x00441FA7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto path = fs::u8path(std::string(_savePath));
                return load(path, regs.eax) ? X86_FLAG_CARRY : 0;
            });
    }
}
