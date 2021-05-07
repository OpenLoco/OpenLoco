#include "S5.h"
#include "../CompanyManager.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../StationManager.h"
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
    static void previewWindowDraw(window* w, Gfx::Context* context)
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

            static window_event_list eventList; // 0x4FB3F0
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

    static constexpr SawyerEncoding getBestEncodingForObjectType(object_type type)
    {
        switch (type)
        {
            case object_type::competitor:
                return SawyerEncoding::uncompressed;
            default:
                return SawyerEncoding::runLengthSingle;
            case object_type::currency:
                return SawyerEncoding::runLengthMulti;
            case object_type::town_names:
            case object_type::scenario_text:
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
                auto obj = ObjectManager::get<object>(*index);
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

    void registerHooks()
    {
        registerHook(
            0x00441C26,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                auto path = fs::u8path(std::string(_savePath));
                return save(path, static_cast<SaveFlags>(regs.eax)) ? 0 : X86_FLAG_CARRY;
            });
    }
}
