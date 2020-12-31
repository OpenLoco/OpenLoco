#include "S5.h"
#include "../CompanyManager.h"
#include "../Date.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Exception.hpp"
#include "../ViewportManager.h"
#include "SawyerStream.h"
#include <fstream>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;

namespace OpenLoco
{
    constexpr uint32_t currentVersion = 0x62262;
    constexpr uint32_t magicNumber = 0x62300;
}

namespace OpenLoco::S5
{
    static loco_global<SaveDetails*, 0x0050AEA8> _saveDetails;
    static loco_global<GameState, 0x00525E18> _gameState;
    static loco_global<char[64], 0x005260D4> _scenarioName;
    static loco_global<Options, 0x009C8714> _activeOptions;
    static loco_global<Header, 0x009CCA34> _header;
    static loco_global<Options, 0x009CCA54> _previewOptions;
    static loco_global<uint32_t, 0x009D1C9C> _saveFlags;
    static loco_global<char[512], 0x0112CE04> _savePath;
    static loco_global<OpenLoco::Ui::SavedViewSimple, 0x00525E36> _savedView;
    static loco_global<uint32_t, 0x00525F68> _525F68;
    static loco_global<uint8_t, 0x009D1CC7> _saveError;
    static loco_global<uint8_t, 0x00F00134> _F00134;

    static bool save(const fs::path& path, const S5File& file, const std::vector<ObjectHeader>& packedObjects);

    Options& getOptions()
    {
        return _activeOptions;
    }

    Options& getPreviewOptions()
    {
        return _previewOptions;
    }

    static void sub_46FF54()
    {
        call(0x0046FF54);
    }

    static void sub_4702F7()
    {
        call(0x004702F7);
    }

    static void sub_4437FC()
    {
        _525F68 = 0x62300;
    }

    static Header prepareHeader(SaveFlags flags, size_t numPackedObjects)
    {
        Header result;
        std::memset(&result, 0, sizeof(result));

        result.type = SType::savedGame;
        if (flags & SaveFlags::landscape)
            result.type = SType::landscape;
        if (flags & SaveFlags::scenario)
            result.type = SType::scenario;

        result.numPackedObjects = static_cast<uint16_t>(numPackedObjects);
        result.version = currentVersion;
        result.magic = magicNumber;

        if (flags & SaveFlags::raw)
        {
            result.flags |= SFlags::isRaw;
        }
        if (flags & SaveFlags::dump)
        {
            result.flags |= SFlags::isDump;
        }
        if (!(flags & SaveFlags::scenario) && !(flags & SaveFlags::raw) && !(flags & SaveFlags::dump))
        {
            result.flags |= SFlags::hasSaveDetails;
        }

        return result;
    }

    // 0x0045A0B3
    static void previewWindowDraw(window* w, Gfx::drawpixelinfo_t* dpi)
    {
        for (auto viewport : w->viewports)
        {
            if (viewport != nullptr)
            {
                viewport->render(dpi);
            }
        }
    }

    static void drawPreviewImage(void* pixels, Gfx::ui_size_t size)
    {
        auto mainViewport = WindowManager::getMainViewport();
        if (mainViewport != nullptr)
        {
            auto mapPosXY = mainViewport->getCentreMapPosition();
            auto mapPosXYZ = map_pos3(mapPosXY, TileManager::getHeight(mapPosXY));

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

                    // Swap screen DPI with our temporary one to draw the window then revert it back
                    auto& dpi = Gfx::screenDpi();
                    auto backupDpi = dpi;
                    dpi.bits = reinterpret_cast<uint8_t*>(pixels);
                    dpi.x = 0;
                    dpi.y = 0;
                    dpi.width = size.width;
                    dpi.height = size.height;
                    dpi.pitch = 0;
                    dpi.zoom_level = 0;
                    Gfx::redrawScreenRect(0, 0, size.width, size.height);
                    dpi = backupDpi;
                }

                WindowManager::close(WindowType::previewImage);
            }
        }
    }

    // 0x004471A4
    static std::unique_ptr<SaveDetails> prepareSaveDetails()
    {
        auto saveDetails = std::make_unique<SaveDetails>();
        auto playerCompany = CompanyManager::getPlayerCompany();
        StringManager::formatString(saveDetails->company, sizeof(saveDetails->company), playerCompany->name);
        StringManager::formatString(saveDetails->owner, sizeof(saveDetails->owner), playerCompany->owner_name);
        saveDetails->date = getCurrentDay();
        saveDetails->performance_index = playerCompany->performance_index;
        saveDetails->challenge_progress = playerCompany->challengeProgress;
        saveDetails->challenge_flags = playerCompany->challenge_flags;
        std::strncpy(saveDetails->scenario, _scenarioName, sizeof(saveDetails->scenario));
        drawPreviewImage(saveDetails->image, { 250, 200 });
        return saveDetails;
    }

    static constexpr SawyerEncoding getBestEncodingForObjectType(object_type type)
    {
        switch (type)
        {
            case object_type::competitor:
                return SawyerEncoding::Uncompressed;
            default:
                return SawyerEncoding::RunLengthSingle;
            case object_type::currency:
                return SawyerEncoding::RunLengthMulti;
            case object_type::town_names:
            case object_type::scenario_text:
                return SawyerEncoding::Rotate;
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
                fs.writeChunk(encodingType, &obj, objSize);
            }
            else
            {
                throw std::runtime_error("Unable to pack object: object not loaded");
            }
        }
    }

    static std::unique_ptr<S5File> prepareSaveFile(SaveFlags flags, const std::vector<ObjectHeader>& requiredObjects, const std::vector<ObjectHeader>& packedObjects)
    {
        auto mainWindow = WindowManager::getMainWindow();
        auto savedView = mainWindow != nullptr ? mainWindow->viewports[0]->toSavedView() : SavedViewSimple();

        auto file = std::make_unique<S5File>();
        file->header = prepareHeader(flags, packedObjects.size());
        if (file->header.type == SType::landscape)
        {
            file->landscapeOptions = std::make_unique<Options>(_activeOptions);
        }
        if (file->header.flags & SFlags::hasSaveDetails)
        {
            file->saveDetails = prepareSaveDetails();
        }
        std::memcpy(file->requiredObjects, requiredObjects.data(), sizeof(file->requiredObjects));
        file->gameState = _gameState;
        file->gameState.savedViewX = savedView.mapX;
        file->gameState.savedViewY = savedView.mapY;
        file->gameState.savedViewZoom = static_cast<uint8_t>(savedView.zoomLevel);
        file->gameState.savedViewRotation = savedView.rotation;

        auto tileElements = TileManager::getElements();
        file->tileElements.resize(tileElements.size());
        std::memcpy(file->tileElements.data(), tileElements.data(), tileElements.size_bytes());
        return file;
    }

    static constexpr bool shouldPackObjects(SaveFlags flags)
    {
        return !(flags & SaveFlags::raw) && !(flags & SaveFlags::dump) && (flags & SaveFlags::packCustomObjects) && !isNetworked();
    }

    // 0x00441C26
    bool save(const fs::path& path, SaveFlags flags)
    {
        if (!(flags & SaveFlags::noWindowClose) || !(flags & SaveFlags::raw) || !(flags & SaveFlags::dump))
        {
            WindowManager::closeConstructionWindows();
        }

        if (!(flags & SaveFlags::raw))
        {
            TileManager::reorganise();
            sub_46FF54();
            ThingManager::zeroUnused();
            StationManager::zeroUnused();
            sub_4702F7();
        }

        sub_4437FC();

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
            _saveError = save(path, *file, packedObjects) ? 0 : 1;
        }

        if (!(flags & SaveFlags::raw) && !(flags & SaveFlags::dump))
        {
            ObjectManager::reloadAll();
        }

        if (_saveError == 0)
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
            fs.writeChunk(SawyerEncoding::Rotate, file.header);
            if (file.header.type == SType::landscape)
            {
                fs.writeChunk(SawyerEncoding::Rotate, *file.landscapeOptions);
            }
            if (file.header.flags & SFlags::hasSaveDetails)
            {
                fs.writeChunk(SawyerEncoding::Rotate, *file.saveDetails);
            }
            if (file.header.numPackedObjects != 0)
            {
                writePackedObjects(fs, packedObjects);
            }
            fs.writeChunk(SawyerEncoding::Rotate, file.requiredObjects, sizeof(file.requiredObjects));

            if (file.header.type == SType::scenario)
            {
                fs.writeChunk(SawyerEncoding::RunLengthSingle, file.gameState.rng, 0xB96C);
                fs.writeChunk(SawyerEncoding::RunLengthSingle, file.gameState.towns, 0x123480);
                fs.writeChunk(SawyerEncoding::RunLengthSingle, file.gameState.animations, 0x79D80);
            }
            else
            {
                fs.writeChunk(SawyerEncoding::RunLengthSingle, file.gameState);
            }

            if (file.header.flags & SaveFlags::raw)
            {
                throw NotImplementedException();
            }
            else
            {
                fs.writeChunk(SawyerEncoding::RunLengthMulti, file.tileElements.data(), file.tileElements.size() * sizeof(TileElement));
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
                auto path = fs::u8path(std::string_view(_savePath));

                registers backup = regs;
                return save(path, static_cast<SaveFlags>(regs.eax)) ? 0x100 : 0;
            });
    }
}
