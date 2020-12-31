#include "S5.h"
#include "../CompanyManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Objects/ObjectManager.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Exception.hpp"
#include "../ViewportManager.h"
#include <OpenLoco/Date.h>
#include <fstream>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Ui;

namespace OpenLoco
{
    constexpr uint32_t currentVersion = 0x62262;
    constexpr uint32_t magicNumber = 0x62300;

    enum class SawyerEncoding : uint8_t
    {
        Uncompressed,
        RunLengthSingle,
        RunLengthMulti,
        Rotate,
    };

    class SawyerStreamWriter
    {
    private:
        std::ofstream _stream;
        uint32_t _checksum{};

    public:
        SawyerStreamWriter(const fs::path& path)
        {
            _stream.exceptions(std::ifstream::failbit);
            _stream.open(path, std::ios::out | std::ios::binary);
        }

        template<typename T>
        void writeChunk(SawyerEncoding chunkType, const T& data)
        {
            writeChunk(chunkType, &data, sizeof(T));
        }

        void writeChunk(SawyerEncoding chunkType, const void* data, size_t dataLen)
        {
            // Force uncompressed for now
            chunkType = SawyerEncoding::Uncompressed;

            write(&chunkType, sizeof(chunkType));
            write(&dataLen, sizeof(dataLen));
            write(data, dataLen);
        }

        void write(const void* data, size_t dataLen)
        {
            _stream.write(reinterpret_cast<const char*>(data), dataLen);
            auto data8 = reinterpret_cast<const uint8_t*>(data);
            for (size_t i = 0; i < dataLen; i++)
            {
                _checksum += data8[i];
            }
        }

        void writeChecksum()
        {
            _stream.write(reinterpret_cast<const char*>(&_checksum), sizeof(_checksum));
        }

        void close()
        {
            _stream.close();
        }
    };

}

namespace OpenLoco::S5
{
    static loco_global<SaveDetails*, 0x0050AEA8> _saveDetails;
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

    Options& getOptions()
    {
        return _activeOptions;
    }

    Options& getPreviewOptions()
    {
        return _previewOptions;
    }

    // 0x00441C26
    [[maybe_unused]] bool saveLegacy(const fs::path& path, SaveFlags flags)
    {
        // Copy UTF-8 path into filename buffer
        auto path8 = path.u8string();
        if (path8.size() >= std::size(_savePath))
        {
            std::fprintf(stderr, "Save path is too long: %s\n", path8.c_str());
            return false;
        }
        std::strncpy(_savePath, path8.c_str(), std::size(_savePath));

        if (flags & SaveFlags::noWindowClose)
        {
            // TODO: Remove ghost elements before saving to file

            // Skip the close construction window call
            // We have skipped the _saveFlags = eax instruction, so do this here
            _saveFlags = flags;
            return call(0x00441C3C) & (1 << 8);
        }
        else
        {
            // Normal entry
            registers regs;
            regs.eax = flags;
            return call(0x00441C26, regs) & (1 << 8);
        }
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

    static void writeSaveDetails(SawyerStreamWriter& fs)
    {
        auto saveDetails = prepareSaveDetails();
        fs.writeChunk(SawyerEncoding::Rotate, *saveDetails);
    }

    // 0x00472633
    static void writePackedObjects(SawyerStreamWriter& fs)
    {
        throw NotImplementedException();
    }

    // 0x004723F1
    static void writeRequiredObjects(SawyerStreamWriter& fs)
    {
        std::vector<ObjectManager::header> entries;
        entries.reserve(ObjectManager::maxObjects);

        for (size_t i = 0; i < ObjectManager::maxObjects; i++)
        {
            auto obj = ObjectManager::get<object>(i);
            if (obj != nullptr)
            {
                auto entry = ObjectManager::getObjectEntry(i);
                entries.push_back(*entry);
            }
            else
            {
                entries.push_back(ObjectManager::header::empty());
            }
        }

        fs.writeChunk(SawyerEncoding::Rotate, entries.data(), entries.size() * sizeof(ObjectManager::header));
    }

    static Header prepareHeader(SaveFlags flags)
    {
        Header result;
        std::memset(&result, 0, sizeof(result));

        result.type = SType::savedGame;
        if (flags & SaveFlags::landscape)
            result.type = SType::landscape;
        if (flags & SaveFlags::scenario)
            result.type = SType::scenario;

        if (!(flags & SaveFlags::raw) && !(flags & SaveFlags::dump) && (flags & SaveFlags::savedGame))
        {
            result.numPackedObjects = static_cast<uint16_t>(ObjectManager::getNumCustomObjects());
        }

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

        auto mainWindow = WindowManager::getMainWindow();
        _savedView = mainWindow != nullptr ? mainWindow->viewports[0]->toSavedView() : SavedViewSimple();
        _header = prepareHeader(flags);

        try
        {
            SawyerStreamWriter fs(path);
            fs.writeChunk(SawyerEncoding::Rotate, *_header);
            if (flags & SaveFlags::landscape)
            {
                fs.writeChunk(SawyerEncoding::Rotate, _activeOptions);
            }
            if (_header->flags & SFlags::hasSaveDetails)
            {
                writeSaveDetails(fs);
            }
            if (_header->numPackedObjects != 0)
            {
                writePackedObjects(fs);
            }
            writeRequiredObjects(fs);

            if (flags & SaveFlags::scenario)
            {
                fs.writeChunk(SawyerEncoding::RunLengthSingle, (const void*)0x00525E18, 0xB96C);
                fs.writeChunk(SawyerEncoding::RunLengthSingle, (const void*)0x005B825C, 0x123480);
                fs.writeChunk(SawyerEncoding::RunLengthSingle, (const void*)0x0094C6DC, 0x79D80);
            }
            else
            {
                fs.writeChunk(SawyerEncoding::RunLengthSingle, (const void*)0x00525E18, 0x4A0644);
            }

            if (flags & SaveFlags::raw)
            {
                throw NotImplementedException();
            }
            else
            {
                auto elements = TileManager::getElements();
                fs.writeChunk(SawyerEncoding::RunLengthMulti, elements.data(), elements.size() * sizeof(tile_element));
            }

            fs.writeChecksum();
            fs.close();
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to save S5: %s\n", e.what());
            _saveError = 1;
        }

        if (!(flags & SaveFlags::raw) && !(flags & SaveFlags::dump))
        {
            ObjectManager::resetLoadedObjects();
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
