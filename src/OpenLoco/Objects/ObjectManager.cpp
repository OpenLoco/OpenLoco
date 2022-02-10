#include "ObjectManager.h"
#include "../Core/FileSystem.hpp"
#include "../Environment.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../S5/SawyerStream.h"
#include "../Ui.h"
#include "../Ui/ProgressBar.h"
#include "../Utility/Numeric.hpp"
#include "../Utility/Stream.hpp"
#include "AirportObject.h"
#include "BridgeObject.h"
#include "BuildingObject.h"
#include "CargoObject.h"
#include "ClimateObject.h"
#include "CompetitorObject.h"
#include "CurrencyObject.h"
#include "DockObject.h"
#include "HillShapesObject.h"
#include "IndustryObject.h"
#include "InterfaceSkinObject.h"
#include "LandObject.h"
#include "LevelCrossingObject.h"
#include "RegionObject.h"
#include "RoadExtraObject.h"
#include "RoadObject.h"
#include "RoadStationObject.h"
#include "RockObject.h"
#include "ScaffoldingObject.h"
#include "ScenarioTextObject.h"
#include "SnowObject.h"
#include "SoundObject.h"
#include "SteamObject.h"
#include "StreetLightObject.h"
#include "TownNamesObject.h"
#include "TrackExtraObject.h"
#include "TrackObject.h"
#include "TrainSignalObject.h"
#include "TrainStationObject.h"
#include "TreeObject.h"
#include "TunnelObject.h"
#include "VehicleObject.h"
#include "WallObject.h"
#include "WaterObject.h"
#include <iterator>
#include <vector>

using namespace OpenLoco::Interop;

namespace OpenLoco::ObjectManager
{
    constexpr auto objectChecksumMagic = 0xF369A75B;
#pragma pack(push, 1)
    struct ObjectEntry2 : public ObjectHeader
    {
        uint32_t dataSize;
        ObjectEntry2(ObjectHeader head, uint32_t size)
            : ObjectHeader(head)
            , dataSize(size)
        {
        }
    };
    static_assert(sizeof(ObjectEntry2) == 0x14);

    struct ObjectRepositoryItem
    {
        Object** objects;
        ObjectEntry2* object_entry_extendeds;
    };
    assert_struct_size(ObjectRepositoryItem, 8);
#pragma pack(pop)

    loco_global<ObjectEntry2[maxObjects], 0x1125A90> objectEntries;
    loco_global<ObjectRepositoryItem[64], 0x4FE0B8> object_repository;
    loco_global<Object* [maxObjects], 0x0050C3D0> _allObjects;
    loco_global<InterfaceSkinObject* [1], 0x0050C3D0> _interfaceObjects;
    loco_global<SoundObject* [128], 0x0050C3D4> _soundObjects;
    loco_global<CurrencyObject* [1], 0x0050C5D4> _currencyObjects;
    loco_global<SteamObject* [32], 0x0050C5D8> _steamObjects;
    loco_global<RockObject* [8], 0x0050C658> _rockObjects;
    loco_global<WaterObject* [1], 0x0050C678> _waterObjects;
    loco_global<LandObject* [32], 0x0050C67C> _landObjects;
    loco_global<TownNamesObject* [1], 0x0050C6FC> _townNamesObjects;
    loco_global<CargoObject* [32], 0x0050C700> _cargoObjects;
    loco_global<WallObject* [32], 0x0050C780> _wallObjects;
    loco_global<TrainSignalObject* [16], 0x0050C800> _trainSignalObjects;
    loco_global<LevelCrossingObject* [4], 0x0050C840> _levelCrossingObjects;
    loco_global<StreetLightObject* [1], 0x0050C850> _streetLightObjects;
    loco_global<TunnelObject* [16], 0x0050C854> _tunnelObjects;
    loco_global<BridgeObject* [8], 0x0050C894> _bridgeObjects;
    loco_global<TrainStationObject* [16], 0x0050C8B4> _trainStationObjects;
    loco_global<TrackExtraObject* [8], 0x0050C8F4> _trackExtraObjects;
    loco_global<TrackObject* [8], 0x0050C914> _trackObjects;
    loco_global<RoadStationObject* [16], 0x0050C934> _roadStationObjects;
    loco_global<RoadExtraObject* [4], 0x0050C974> _roadExtraObjects;
    loco_global<RoadObject* [8], 0x0050C984> _roadObjects;
    loco_global<AirportObject* [8], 0x0050C9A4> _airportObjects;
    loco_global<DockObject* [8], 0x0050C9C4> _dockObjects;
    loco_global<VehicleObject* [224], 0x0050C9E4> _vehicleObjects;
    loco_global<TreeObject* [64], 0x0050CD64> _treeObjects;
    loco_global<SnowObject* [1], 0x0050CE64> _snowObjects;
    loco_global<ClimateObject* [1], 0x0050CE68> _climateObjects;
    loco_global<HillShapesObject* [1], 0x0050CE6C> _hillShapeObjects;
    loco_global<BuildingObject* [128], 0x0050CE70> _buildingObjects;
    loco_global<ScaffoldingObject* [1], 0x0050D070> _scaffoldingObjects;
    loco_global<IndustryObject* [16], 0x0050D074> _industryObjects;
    loco_global<RegionObject* [1], 0x0050D0B4> _regionObjects;
    loco_global<CompetitorObject* [32], 0x0050D0B8> _competitorObjects;
    loco_global<ScenarioTextObject* [1], 0x0050D138> _scenarioTextObjects;

    loco_global<uint32_t, 0x0050D154> _totalNumImages;

    static loco_global<std::byte*, 0x0050D13C> _installedObjectList;
    static loco_global<std::byte*, 0x0050D158> _50D158;
    static loco_global<std::byte[0x2002], 0x0112A17F> _112A17F;
    static loco_global<uint32_t, 0x0112A110> _installedObjectCount;
    static loco_global<bool, 0x0112A17E> _customObjectsInIndex;
    static loco_global<bool, 0x0050AEAD> _isFirstTime;
    static loco_global<bool, 0x0050D161> _isPartialLoaded;
    static loco_global<uint32_t, 0x009D9D52> _decodedSize;    // return of getScenarioText (badly named)
    static loco_global<uint32_t, 0x0112A168> _numImages;      // return of getScenarioText (badly named)
    static loco_global<uint8_t, 0x0112C211> _intelligence;    // return of getScenarioText (badly named)
    static loco_global<uint8_t, 0x0112C212> _aggressiveness;  // return of getScenarioText (badly named)
    static loco_global<uint8_t, 0x0112C213> _competitiveness; // return of getScenarioText (badly named)

#pragma pack(push, 1)
    struct ObjectFolderState
    {
        uint32_t numObjects = 0;
        uint32_t totalFileSize = 0;
        uint32_t dateHash = 0;
        constexpr bool operator==(const ObjectFolderState& rhs) const
        {
            return (numObjects == rhs.numObjects) && (totalFileSize == rhs.totalFileSize) && (dateHash == rhs.dateHash);
        }
        constexpr bool operator!=(const ObjectFolderState& rhs) const
        {
            return !(*this == rhs);
        }
    };
    static_assert(sizeof(ObjectFolderState) == 0xC);
    struct IndexHeader
    {
        ObjectFolderState state;
        uint32_t fileSize;
        uint32_t numObjects; // duplicates ObjectFolderState.numObjects but without high 1 and includes corrupted .dat's
    };
    static_assert(sizeof(IndexHeader) == 0x14);
#pragma pack(pop)
    // 0x00470F3C
    static ObjectFolderState getCurrentObjectFolderState()
    {
        ObjectFolderState currentState;
        const auto objectPath = Environment::getPathNoWarning(Environment::PathId::objects);
        for (const auto& file : fs::directory_iterator(objectPath, fs::directory_options::skip_permission_denied))
        {
            if (!file.is_regular_file())
            {
                continue;
            }
            const auto extension = file.path().extension().u8string();
            if (!Utility::iequals(extension, ".DAT"))
            {
                continue;
            }
            currentState.numObjects++;
            const auto lastWrite = file.last_write_time().time_since_epoch().count();
            currentState.dateHash ^= ((lastWrite >> 32) ^ (lastWrite & 0xFFFFFFFF));
            currentState.dateHash = Utility::ror(currentState.dateHash, 5);
            currentState.totalFileSize += file.file_size();
        }
        currentState.numObjects |= (1 << 24);
        return currentState;
    }

    // 0x00471712
    static bool hasCustomObjectsInIndex()
    {
        auto* ptr = *_installedObjectList;
        for (uint32_t i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);
            if (entry._header->isCustom())
                return true;
        }
        return false;
    }

    static void saveIndex(const IndexHeader& header)
    {
        std::ofstream stream;
        const auto indexPath = Environment::getPathNoWarning(Environment::PathId::plugin1);
        stream.open(indexPath, std::ios::out | std::ios::binary);
        if (!stream.is_open())
        {
            return;
        }
        stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
        stream.write(reinterpret_cast<const char*>(*_installedObjectList), header.fileSize);
    }

    static std::pair<ObjectIndexEntry, size_t> createPartialNewEntry(std::byte* entryBuffer, const ObjectHeader& objHeader, const fs::path filename)
    {
        ObjectIndexEntry entry{};
        size_t newEntrySize = 0;

        // Header
        std::memcpy(&entryBuffer[newEntrySize], &objHeader, sizeof(objHeader));
        entry._header = reinterpret_cast<ObjectHeader*>(&entryBuffer[newEntrySize]);
        newEntrySize += sizeof(objHeader);

        // Filename
        std::strcpy(reinterpret_cast<char*>(&entryBuffer[newEntrySize]), filename.u8string().c_str());
        entry._filename = reinterpret_cast<char*>(&entryBuffer[newEntrySize]);
        newEntrySize += strlen(reinterpret_cast<char*>(&entryBuffer[newEntrySize])) + 1;

        // Header2 NULL
        ObjectHeader2 objHeader2;
        objHeader2.var_00 = std::numeric_limits<uint32_t>::max();
        std::memcpy(&entryBuffer[newEntrySize], &objHeader2, sizeof(objHeader2));
        newEntrySize += sizeof(objHeader2);

        // Name NULL
        entryBuffer[newEntrySize] = std::byte(0);
        entry._name = reinterpret_cast<char*>(&entryBuffer[newEntrySize]);
        newEntrySize += strlen(reinterpret_cast<char*>(&entryBuffer[newEntrySize])) + 1;

        // Header3 NULL
        ObjectHeader3 objHeader3{};
        std::memcpy(&entryBuffer[newEntrySize], &objHeader3, sizeof(objHeader3));
        newEntrySize += sizeof(objHeader3);

        // ObjectList1
        const uint8_t size1 = 0;
        entryBuffer[newEntrySize++] = std::byte(size1);

        // ObjectList2
        const uint8_t size2 = 0;
        entryBuffer[newEntrySize++] = std::byte(size2);

        return std::make_pair(entry, newEntrySize);
    }

    static std::pair<ObjectIndexEntry, size_t> createNewEntry(std::byte* entryBuffer, const ObjectHeader& objHeader, const fs::path filename)
    {
        ObjectIndexEntry entry{};
        size_t newEntrySize = 0;

        // Header
        std::memcpy(&entryBuffer[newEntrySize], &objHeader, sizeof(objHeader));
        entry._header = reinterpret_cast<ObjectHeader*>(&entryBuffer[newEntrySize]);
        newEntrySize += sizeof(objHeader);

        // Filename
        std::strcpy(reinterpret_cast<char*>(&entryBuffer[newEntrySize]), filename.u8string().c_str());
        entry._filename = reinterpret_cast<char*>(&entryBuffer[newEntrySize]);
        newEntrySize += strlen(reinterpret_cast<char*>(&entryBuffer[newEntrySize])) + 1;

        // Header2
        ObjectHeader2 objHeader2;
        objHeader2.var_00 = _decodedSize;
        std::memcpy(&entryBuffer[newEntrySize], &objHeader2, sizeof(objHeader2));
        newEntrySize += sizeof(objHeader2);

        // Name
        strcpy(reinterpret_cast<char*>(&entryBuffer[newEntrySize]), StringManager::getString(0x2000));
        entry._name = reinterpret_cast<char*>(&entryBuffer[newEntrySize]);
        newEntrySize += strlen(reinterpret_cast<char*>(&entryBuffer[newEntrySize])) + 1;

        // Header3
        ObjectHeader3 objHeader3{};
        objHeader3.var_00 = _numImages;
        objHeader3.intelligence = _intelligence;
        objHeader3.aggressiveness = _aggressiveness;
        objHeader3.competitiveness = _competitiveness;
        std::memcpy(&entryBuffer[newEntrySize], &objHeader3, sizeof(objHeader3));
        newEntrySize += sizeof(objHeader3);

        auto* ptr = &_112A17F[0];

        // ObjectList1 (required objects)
        const uint8_t size1 = uint8_t(*ptr++);
        entryBuffer[newEntrySize++] = std::byte(size1);
        std::memcpy(&entryBuffer[newEntrySize], ptr, sizeof(ObjectHeader) * size1);
        newEntrySize += sizeof(ObjectHeader) * size1;
        ptr += sizeof(ObjectHeader) * size1;

        // ObjectList2 (also loads objects {used for category selection})
        const uint8_t size2 = uint8_t(*ptr++);
        entryBuffer[newEntrySize++] = std::byte(size2);
        std::memcpy(&entryBuffer[newEntrySize], ptr, sizeof(ObjectHeader) * size2);
        newEntrySize += sizeof(ObjectHeader) * size2;
        ptr += sizeof(ObjectHeader) * size2;

        return std::make_pair(entry, newEntrySize);
    }

    // Adds a new object to the index by: 1. creating a partial index, 2. validating, 3. creating a full index entry
    static void addObjectToIndex(const fs::path filepath, size_t& usedBufferSize)
    {
        ObjectHeader objHeader{};
        {
            std::ifstream stream;
            stream.open(filepath, std::ios::in | std::ios::binary);
            Utility::readData(stream, objHeader);
            if (stream.gcount() != sizeof(objHeader))
            {
                return;
            }
        }

        const auto curObjPos = usedBufferSize;
        const auto partialNewEntry = createPartialNewEntry(&_installedObjectList[usedBufferSize], objHeader, filepath.filename());
        usedBufferSize += partialNewEntry.second;
        _installedObjectCount++;

        _isPartialLoaded = true;
        _50D158 = _112A17F;
        getScenarioText(objHeader);
        _50D158 = reinterpret_cast<std::byte*>(-1);
        _isPartialLoaded = false;
        if (_installedObjectCount == 0)
        {
            return;
        }
        _installedObjectCount--;

        // Rewind as it is only a partial object loaded
        usedBufferSize = curObjPos;

        // Load full entry into temp buffer.
        // 0x009D1CC8
        std::byte newEntryBuffer[0x2000] = {};
        const auto [newEntry, newEntrySize] = createNewEntry(newEntryBuffer, objHeader, filepath.filename());

        freeScenarioText();

        auto* indexPtr = *_installedObjectList;
        // This ptr will be pointing at where in the object list to install the new entry
        auto* installPtr = indexPtr;
        for (uint32_t i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&indexPtr);
            if (strcmp(newEntry._name, entry._name) < 0)
            {
                break;
            }
            // If cmp < 0 then we will want to install at the previous indexPtr location
            installPtr = indexPtr;
        }

        auto moveSize = usedBufferSize - (installPtr - *_installedObjectList);
        std::memmove(installPtr + newEntrySize, installPtr, moveSize);
        std::memcpy(installPtr, newEntryBuffer, newEntrySize);
        usedBufferSize += newEntrySize;

        _installedObjectCount++;
    }

    // 0x0047118B
    static void createIndex(const ObjectFolderState& currentState)
    {
        Ui::processMessagesMini();
        const auto progressString = _isFirstTime ? StringIds::starting_for_the_first_time : StringIds::checking_object_files;
        Ui::ProgressBar::begin(progressString);

        // Reset
        reloadAll();
        if (reinterpret_cast<int32_t>(*_installedObjectList) != -1)
        {
            free(*_installedObjectList);
        }

        // Prepare initial object list buffer (we will grow this as required)
        size_t bufferSize = 0x4000;
        _installedObjectList = static_cast<std::byte*>(malloc(bufferSize));
        if (_installedObjectList == nullptr)
        {
            _installedObjectList = reinterpret_cast<std::byte*>(-1);
            exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
            return;
        }

        // Create new index by iterating all DAT files and processing
        IndexHeader header{};
        uint8_t progress = 0;      // Progress is used for the ProgressBar Ui element
        size_t usedBufferSize = 0; // Keep track of used space to allow for growth and for final sizing
        const auto objectPath = Environment::getPathNoWarning(Environment::PathId::objects);
        for (const auto& file : fs::directory_iterator(objectPath, fs::directory_options::skip_permission_denied))
        {
            if (!file.is_regular_file())
            {
                continue;
            }
            const auto extension = file.path().extension().u8string();
            if (!Utility::iequals(extension, ".DAT"))
            {
                continue;
            }
            Ui::processMessagesMini();
            header.state.numObjects++;

            // Cheap calculation of (curObjectCount / totalObjectCount) * 256
            const auto newProgress = (header.state.numObjects << 8) / ((currentState.numObjects & 0xFFFFFF) + 1);
            if (progress != newProgress)
            {
                progress = newProgress;
                Ui::ProgressBar::setProgress(newProgress);
            }

            // Grow object list buffer if near limit
            const auto remainingBuffer = bufferSize - usedBufferSize;
            if (remainingBuffer < 0x231E)
            {
                // Original grew buffer at slower rate. Memory is cheap though
                bufferSize *= 2;
                _installedObjectList = static_cast<std::byte*>(realloc(*_installedObjectList, bufferSize));
                if (_installedObjectList == nullptr)
                {
                    exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
                    return;
                }
            }

            addObjectToIndex(file.path(), usedBufferSize);
        }

        // New index creation completed. Reset and save result.
        reloadAll();
        header.fileSize = usedBufferSize;
        header.numObjects = _installedObjectCount;
        header.state = currentState;
        saveIndex(header);

        Ui::ProgressBar::end();
    }

    static bool tryLoadIndex(const ObjectFolderState& currentState)
    {
        const auto indexPath = Environment::getPathNoWarning(Environment::PathId::plugin1);
        if (!fs::exists(indexPath))
        {
            return false;
        }
        std::ifstream stream;
        stream.open(indexPath, std::ios::in | std::ios::binary);
        if (!stream.is_open())
        {
            return false;
        }
        // 0x00112A14C -> 160
        IndexHeader header{};
        Utility::readData(stream, header);
        if ((header.state != currentState) || (stream.gcount() != sizeof(header)))
        {
            return false;
        }
        else
        {
            if (reinterpret_cast<int32_t>(*_installedObjectList) != -1)
            {
                free(*_installedObjectList);
            }
            _installedObjectList = static_cast<std::byte*>(malloc(header.fileSize));
            if (_installedObjectList == nullptr)
            {
                exitWithError(StringIds::unable_to_allocate_enough_memory, StringIds::game_init_failure);
                return false;
            }
            Utility::readData(stream, *_installedObjectList, header.fileSize);
            if (stream.gcount() != static_cast<int32_t>(header.fileSize))
            {
                return false;
            }
            _installedObjectCount = header.numObjects;
            reloadAll();
        }
        return true;
    }

    // 0x00470F3C
    void loadIndex()
    {
        // 0x00112A138 -> 144
        const auto currentState = getCurrentObjectFolderState();

        if (!tryLoadIndex(currentState))
        {
            createIndex(currentState);
        }

        _customObjectsInIndex = hasCustomObjectsInIndex();
    }

    ObjectHeader* getHeader(LoadedObjectIndex id)
    {
        return &objectEntries[id];
    }

    static ObjectRepositoryItem& getRepositoryItem(ObjectType type)
    {
        return object_repository[static_cast<uint8_t>(type)];
    }

    Object* getAny(const LoadedObjectHandle handle)
    {
        auto obj = _allObjects[getTypeOffset(handle.type) + handle.id];
        if (obj == (void*)-1)
        {
            obj = nullptr;
        }
        return obj;
    }

    template<>
    InterfaceSkinObject* get()
    {
        if (_interfaceObjects[0] == (void*)-1)
        {
            return nullptr;
        }

        return _interfaceObjects[0];
    }

    template<>
    SoundObject* get(size_t id)
    {
        if (_soundObjects[id] != reinterpret_cast<SoundObject*>(-1))
        {
            return _soundObjects[id];
        }
        return nullptr;
    }

    template<>
    SteamObject* get(size_t id)
    {
        if (_steamObjects[id] != reinterpret_cast<SteamObject*>(-1))
        {
            return _steamObjects[id];
        }
        return nullptr;
    }

    template<>
    RockObject* get(size_t id)
    {
        if (_rockObjects[id] != reinterpret_cast<RockObject*>(-1))
            return _rockObjects[id];
        else
            return nullptr;
    }

    template<>
    CargoObject* get(size_t id)
    {
        if (_cargoObjects[id] != (CargoObject*)-1)
            return _cargoObjects[id];
        else
            return nullptr;
    }

    template<>
    TrainSignalObject* get(size_t id)
    {
        if (_trainSignalObjects[id] != reinterpret_cast<TrainSignalObject*>(-1))
            return _trainSignalObjects[id];
        else
            return nullptr;
    }

    template<>
    RoadStationObject* get(size_t id)
    {
        if (_roadStationObjects[id] != reinterpret_cast<RoadStationObject*>(-1))
            return _roadStationObjects[id];
        else
            return nullptr;
    }

    template<>
    VehicleObject* get(size_t id)
    {
        if (_vehicleObjects[id] != reinterpret_cast<VehicleObject*>(-1))
            return _vehicleObjects[id];
        else
            return nullptr;
    }

    template<>
    TreeObject* get(size_t id)
    {
        if (_treeObjects[id] != reinterpret_cast<TreeObject*>(-1))
            return _treeObjects[id];
        else
            return nullptr;
    }

    template<>
    WallObject* get(size_t id)
    {
        if (_wallObjects[id] != reinterpret_cast<WallObject*>(-1))
            return _wallObjects[id];
        else
            return nullptr;
    }

    template<>
    BuildingObject* get(size_t id)
    {
        if (_buildingObjects[id] != reinterpret_cast<BuildingObject*>(-1))
            return _buildingObjects[id];
        else
            return nullptr;
    }

    template<>
    IndustryObject* get(size_t id)
    {
        if (_industryObjects[id] != reinterpret_cast<IndustryObject*>(-1))
            return _industryObjects[id];
        else
            return nullptr;
    }

    template<>
    CurrencyObject* get()
    {
        if (_currencyObjects[0] != reinterpret_cast<CurrencyObject*>(-1))
        {
            return _currencyObjects[0];
        }
        return nullptr;
    }

    template<>
    BridgeObject* get(size_t id)
    {
        if (_bridgeObjects[id] != reinterpret_cast<BridgeObject*>(-1))
            return _bridgeObjects[id];
        else
            return nullptr;
    }

    template<>
    TrainStationObject* get(size_t id)
    {
        if (_trainStationObjects[id] != reinterpret_cast<TrainStationObject*>(-1))
            return _trainStationObjects[id];
        else
            return nullptr;
    }

    template<>
    TrackExtraObject* get(size_t id)
    {
        if (_trackExtraObjects[id] != reinterpret_cast<TrackExtraObject*>(-1))
        {
            return _trackExtraObjects[id];
        }
        return nullptr;
    }

    template<>
    TrackObject* get(size_t id)
    {
        if (_trackObjects[id] != reinterpret_cast<TrackObject*>(-1))
            return _trackObjects[id];
        else
            return nullptr;
    }

    template<>
    RoadExtraObject* get(size_t id)
    {
        if (_roadExtraObjects[id] != reinterpret_cast<RoadExtraObject*>(-1))
        {
            return _roadExtraObjects[id];
        }
        return nullptr;
    }

    template<>
    RoadObject* get(size_t id)
    {
        if (_roadObjects[id] != reinterpret_cast<RoadObject*>(-1))
            return _roadObjects[id];
        else
            return nullptr;
    }

    template<>
    AirportObject* get(size_t id)
    {
        if (_airportObjects[id] != reinterpret_cast<AirportObject*>(-1))
            return _airportObjects[id];
        else
            return nullptr;
    }

    template<>
    DockObject* get(size_t id)
    {
        if (_dockObjects[id] != reinterpret_cast<DockObject*>(-1))
            return _dockObjects[id];
        else
            return nullptr;
    }

    template<>
    LandObject* get(size_t id)
    {
        if (_landObjects[id] != (LandObject*)-1)
            return _landObjects[id];
        else
            return nullptr;
    }

    template<>
    WaterObject* get()
    {
        if (_waterObjects[0] != reinterpret_cast<WaterObject*>(-1))
        {
            return _waterObjects[0];
        }
        return nullptr;
    }

    template<>
    CompetitorObject* get(size_t id)
    {
        if (_competitorObjects[id] != reinterpret_cast<CompetitorObject*>(-1))
        {
            return _competitorObjects[id];
        }
        return nullptr;
    }

    template<>
    ClimateObject* get()
    {
        if (_climateObjects[0] == (void*)-1)
        {
            return nullptr;
        }

        return _climateObjects[0];
    }

    template<>
    ScenarioTextObject* get()
    {
        if (_scenarioTextObjects[0] != (ScenarioTextObject*)-1)
            return _scenarioTextObjects[0];
        else
            return nullptr;
    }

    template<>
    RegionObject* get()
    {
        if (_regionObjects[0] != reinterpret_cast<RegionObject*>(-1))
            return _regionObjects[0];
        else
            return nullptr;
    }

    template<>
    TownNamesObject* get()
    {
        if (_townNamesObjects[0] != reinterpret_cast<TownNamesObject*>(-1))
            return _townNamesObjects[0];
        else
            return nullptr;
    }

    template<>
    LevelCrossingObject* get(size_t id)
    {
        if (_levelCrossingObjects[id] != reinterpret_cast<LevelCrossingObject*>(-1))
            return _levelCrossingObjects[id];
        else
            return nullptr;
    }

    template<>
    StreetLightObject* get()
    {
        if (_streetLightObjects[0] != reinterpret_cast<StreetLightObject*>(-1))
            return _streetLightObjects[0];
        else
            return nullptr;
    }

    template<>
    TunnelObject* get(size_t id)
    {
        if (_tunnelObjects[id] != reinterpret_cast<TunnelObject*>(-1))
            return _tunnelObjects[id];
        else
            return nullptr;
    }

    template<>
    SnowObject* get()
    {
        if (_snowObjects[0] != reinterpret_cast<SnowObject*>(-1))
            return _snowObjects[0];
        else
            return nullptr;
    }

    template<>
    HillShapesObject* get()
    {
        if (_hillShapeObjects[0] != reinterpret_cast<HillShapesObject*>(-1))
            return _hillShapeObjects[0];
        else
            return nullptr;
    }

    template<>
    ScaffoldingObject* get()
    {
        if (_scaffoldingObjects[0] != reinterpret_cast<ScaffoldingObject*>(-1))
            return _scaffoldingObjects[0];
        else
            return nullptr;
    }
    /*
    static void printHeader(header data)
    {
        printf("(%02X | %02X << 6) ", data.type & 0x3F, data.type >> 6);
        printf("%02X ", data.pad_01[0]);
        printf("%02X ", data.pad_01[1]);
        printf("%02X ", data.pad_01[2]);

        char name[8 + 1] = { 0 };
        memcpy(name, data.var_04, 8);
        printf("'%s', ", name);

        printf("%08X ", data.checksum);
    }
    */

    ObjectIndexEntry ObjectIndexEntry::read(std::byte** ptr)
    {
        ObjectIndexEntry entry{};

        entry._header = (ObjectHeader*)*ptr;
        *ptr += sizeof(ObjectHeader);

        entry._filename = (char*)*ptr;
        *ptr += strlen(entry._filename) + 1;

        // decoded_chunk_size
        // ObjectHeader2* h2 = (ObjectHeader2*)ptr;
        *ptr += sizeof(ObjectHeader2);

        entry._name = (char*)*ptr;
        *ptr += strlen(entry._name) + 1;

        // ObjectHeader3* h3 = (ObjectHeader3*)ptr;
        *ptr += sizeof(ObjectHeader3);

        uint8_t* countA = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countA; n++)
        {
            // header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        uint8_t* countB = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        for (int n = 0; n < *countB; n++)
        {
            // header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        return entry;
    }

    uint32_t getNumInstalledObjects()
    {
        return *_installedObjectCount;
    }

    std::vector<std::pair<uint32_t, ObjectIndexEntry>> getAvailableObjects(ObjectType type)
    {
        auto ptr = (std::byte*)_installedObjectList;
        std::vector<std::pair<uint32_t, ObjectIndexEntry>> list;

        for (uint32_t i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);
            if (entry._header->getType() == type)
                list.emplace_back(std::pair<uint32_t, ObjectIndexEntry>(i, entry));
        }

        return list;
    }

    // 0x00471B95
    void freeScenarioText()
    {
        call(0x00471B95);
    }

    // 0x0047176D
    void getScenarioText(ObjectHeader& object)
    {
        registers regs;
        regs.ebp = X86Pointer(&object);
        call(0x0047176D, regs);
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectHandle> findIndex(const ObjectHeader& header)
    {
        if ((header.flags & 0xFF) != 0xFF)
        {
            auto objectType = header.getType();
            const auto& typedObjectList = getRepositoryItem(objectType);
            auto maxObjectsForType = getMaxObjects(objectType);
            for (LoadedObjectId i = 0; i < maxObjectsForType; i++)
            {
                auto obj = typedObjectList.objects[i];
                if (obj != nullptr && obj != reinterpret_cast<Object*>(-1))
                {
                    const auto& objHeader = typedObjectList.object_entry_extendeds[i];
                    if (objHeader.isCustom())
                    {
                        if (header == objHeader)
                        {
                            return { LoadedObjectHandle{ objectType, i } };
                        }
                    }
                    else
                    {
                        if (header.getType() == objHeader.getType() && header.getName() == objHeader.getName())
                        {
                            return { LoadedObjectHandle{ objectType, i } };
                        }
                    }
                }
            }
        }
        return std::nullopt;
    }

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectHandle> findIndex(const ObjectIndexEntry& object)
    {
        return findIndex(*object._header);
    }

    // 0x0047237D
    void reloadAll()
    {
        call(0x0047237D);
    }

    enum class ObjectProcedure
    {
        load,
        unload,
        validate,
        drawPreview,
    };

    static bool callObjectFunction(const ObjectType type, Object& obj, const ObjectProcedure proc)
    {
        auto objectProcTable = (const uintptr_t*)0x004FE1C8;
        auto objectProc = objectProcTable[static_cast<size_t>(type)];

        registers regs;
        regs.al = static_cast<uint8_t>(proc);
        regs.esi = X86Pointer(&obj);
        return (call(objectProc, regs) & X86_FLAG_CARRY) == 0;
    }

    static bool callObjectFunction(const LoadedObjectHandle handle, ObjectProcedure proc)
    {
        auto* obj = getAny(handle);
        if (obj != nullptr)
        {
            return callObjectFunction(handle.type, *obj, proc);
        }

        throw std::runtime_error("Object not loaded at this index");
    }

    bool computeObjectChecksum(const ObjectHeader& object, stdx::span<const uint8_t> data);

    // 0x00471BC5
    static bool load(const ObjectHeader& header, LoadedObjectId id)
    {
        // somewhat duplicates isObjectInstalled
        const auto installedObjects = getAvailableObjects(header.getType());
        auto res = std::find_if(std::begin(installedObjects), std::end(installedObjects), [&header](auto& obj) { return *obj.second._header == header; });
        if (res == std::end(installedObjects))
        {
            // Object is not installed
            return false;
        }

        auto& [installOffset, installedObject] = *res;
        const auto filePath = Environment::getPath(Environment::path_id::objects) / fs::u8path(installedObject._filename);

        SawyerStreamReader stream(filePath);
        ObjectHeader loadingHeader;
        stream.read(&loadingHeader, sizeof(loadingHeader));
        if (loadingHeader != header)
        {
            // Something wrong has happened and installed object does not match index
            // Vanilla continued to search for subsequent matching installed headers.
            return false;
        }
        
        // Vanilla would branch and perform more efficient readChunk if size was kown from installedObject.ObjectHeader2
        const auto data = stream.readChunk();

        if (loadingHeader.checksum != computeObjectChecksum(loadingHeader, data))
        {
            // Something wrong has happened and installed object checksum is broken
            return false;
        }

        // 0x00471E79
        registers regs;
        regs.ebp = X86Pointer(&header);
        regs.ecx = static_cast<int32_t>(id);
        return (call(0x00471BC5, regs) & X86_FLAG_CARRY) == 0;
    }

    static LoadedObjectId getObjectId(LoadedObjectIndex index)
    {
        size_t objectType = 0;
        while (objectType < maxObjectTypes)
        {
            auto count = getMaxObjects(static_cast<ObjectType>(objectType));
            if (index < count)
            {
                return static_cast<LoadedObjectId>(index);
            }
            index -= count;
            objectType++;
        }
        return NullObjectId;
    }

    LoadObjectsResult loadAll(stdx::span<ObjectHeader> objects)
    {
        LoadObjectsResult result;
        result.success = true;

        unloadAll();

        LoadedObjectIndex index = 0;
        for (const auto& header : objects)
        {
            auto id = getObjectId(index);
            if (!load(header, id))
            {
                result.success = false;
                result.problemObject = header;
                unloadAll();
                break;
            }
            index++;
        }
        return result;
    }

    // 0x00472754
    static uint32_t computeChecksum(stdx::span<const uint8_t> data, uint32_t seed)
    {
        auto checksum = seed;
        for (auto d : data)
        {
            checksum = Utility::rol(checksum ^ d, 11);
        }
        return checksum;
    }

    // 0x0047270B
    bool computeObjectChecksum(const ObjectHeader& object, stdx::span<const uint8_t> data)
    {
        // Compute the checksum of header and data

        // Annoyingly the header you need to only compute the first byte of the flags
        stdx::span<const uint8_t> headerFlag(reinterpret_cast<const uint8_t*>(&object), 1);
        auto checksum = computeChecksum(headerFlag, objectChecksumMagic);

        // And then the name
        stdx::span<const uint8_t> headerName(reinterpret_cast<const uint8_t*>(&object.name), sizeof(ObjectHeader::name));
        checksum = computeChecksum(headerName, checksum);

        // Finally compute the datas checksum
        checksum = computeChecksum(data, checksum);

        return checksum == object.checksum;
    }

    static bool partialLoad(const ObjectHeader& header, stdx::span<uint8_t> objectData)
    {
        auto type = header.getType();
        size_t index = 0;
        for (; index < getMaxObjects(type); ++index)
        {
            if (getRepositoryItem(type).objects[index] == reinterpret_cast<Object*>(-1))
            {
                break;
            }
        }
        // No slot found. Not possible except if unload all had not been called
        if (index >= getMaxObjects(type))
        {
            return false;
        }

        auto* obj = reinterpret_cast<Object*>(objectData.data());
        getRepositoryItem(type).objects[index] = obj;
        getRepositoryItem(type).object_entry_extendeds[index] = ObjectEntry2(header, objectData.size());
        return true;
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
    void writePackedObjects(SawyerStreamWriter& fs, const std::vector<ObjectHeader>& packedObjects)
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
                auto obj = ObjectManager::getAny(*index);
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

    static void permutateObjectFilename(std::string& filename)
    {
        auto* firstChar = filename.c_str();
        auto* endChar = &filename[filename.size()];
        auto* c = endChar;
        do
        {
            c--;
            if (c == firstChar)
            {
                filename = "00000000";
                break;
            }
            if (*c < '0')
            {
                *c = '/';
            }
            if (*c == '9')
            {
                *c = '@';
            }
            if (*c == 'Z')
            {
                *c = '/';
            }
            *c = *c + 1;
        } while (*c == '0');
    }

    static void sanatiseObjectFilename(std::string& filename)
    {
        // Trim string at first space (note this copies vanilla but maybe shouldn't)
        auto space = filename.find_first_of(' ');
        if (space != std::string::npos)
        {
            filename = filename.substr(0, space);
        }
        // Make filename uppercase
        std::transform(std::begin(filename), std::end(filename), std::begin(filename), toupper);
    }

    // All object files are based on their internal object header name but
    // there is a chance of a name collision this function works out if the name
    // is possible and if not permutates the name until it is valid.
    static fs::path findObjectPath(std::string& filename)
    {
        auto objPath = Environment::getPath(Environment::PathId::objects);

        bool permutateName = false;
        do
        {
            if (permutateName)
            {
                permutateObjectFilename(filename);
            }
            objPath.replace_filename(filename);
            objPath.replace_extension(".DAT");
            permutateName = true;
        } while (fs::exists(objPath));
        return objPath;
    }

    // 0x0047285C
    static bool installObject(const ObjectHeader& objectHeader)
    {
        // Prepare progress bar
        char caption[512];
        auto* str = StringManager::formatString(caption, sizeof(caption), StringIds::installing_new_data);
        // Convert object name to string so it is properly terminated
        std::string objectname(objectHeader.getName());
        strcat(str, objectname.c_str());
        Ui::ProgressBar::begin(caption);
        Ui::ProgressBar::setProgress(50);
        Ui::processMessagesMini();

        // Get new file path
        std::string filename = objectname;
        sanatiseObjectFilename(filename);
        auto objPath = findObjectPath(filename);

        // Create new file and output object file
        Ui::ProgressBar::setProgress(180);
        SawyerStreamWriter stream(objPath);
        writePackedObjects(stream, { objectHeader });

        // Free file
        stream.close();
        Ui::ProgressBar::setProgress(240);
        Ui::ProgressBar::setProgress(255);
        Ui::ProgressBar::end();
        return true;
    }

    static bool isObjectInstalled(const ObjectHeader& objectHeader)
    {
        const auto objects = getAvailableObjects(objectHeader.getType());
        auto res = std::find_if(std::begin(objects), std::end(objects), [&objectHeader](auto& obj) { return *obj.second._header == objectHeader; });
        return res != std::end(objects);
    }

    // 0x00472687 based on
    bool tryInstallObject(const ObjectHeader& objectHeader, stdx::span<const uint8_t> data)
    {
        unloadAll();
        if (!computeObjectChecksum(objectHeader, data))
        {
            return false;
        }
        // Copy the object into Loco freeable memory (required for when partialLoad loads the object)
        uint8_t* objectData = static_cast<uint8_t*>(malloc(data.size()));
        if (objectData == nullptr)
        {
            return false;
        }
        std::copy(std::begin(data), std::end(data), objectData);

        auto* obj = reinterpret_cast<Object*>(objectData);
        if (!callObjectFunction(objectHeader.getType(), *obj, ObjectProcedure::validate))
        {
            return false;
        }

        if (_totalNumImages >= 266266)
        {
            return false;
        }

        // Warning this saves a copy of the objectData pointer and must be unloaded prior to exiting this function
        if (!partialLoad(objectHeader, stdx::span(objectData, data.size())))
        {
            return false;
        }

        // Object already installed so no need to install it
        if (isObjectInstalled(objectHeader))
        {
            unloadAll();
            return false;
        }

        bool result = installObject(objectHeader);

        unloadAll();
        return result;
    }

    // 0x00472031
    void unloadAll()
    {
        call(0x00472031);
    }

    void unload(const LoadedObjectHandle handle)
    {
        callObjectFunction(handle, ObjectProcedure::unload);
    }

    size_t getByteLength(const LoadedObjectHandle handle)
    {
        return objectEntries[getTypeOffset(handle.type) + handle.id].dataSize;
    }

    template<typename TObject>
    static void addAllInUseHeadersOfType(std::vector<ObjectHeader>& entries)
    {
        if constexpr (getMaxObjects(TObject::kObjectType) == 1)
        {
            auto* obj = ObjectManager::get<TObject>();
            if (obj != nullptr)
            {
                auto entry = getHeader(getTypeOffset(TObject::kObjectType));
                entries.push_back(*entry);
            }
            else
            {
                // Insert empty headers for any unused objects (required for save compatibility)
                // TODO: Move this into the S5 code.
                entries.emplace_back();
            }
        }
        else
        {
            for (size_t i = 0; i < getMaxObjects(TObject::kObjectType); ++i)
            {
                auto* obj = ObjectManager::get<TObject>(i);
                if (obj != nullptr)
                {
                    auto entry = getHeader(i + getTypeOffset(TObject::kObjectType));
                    entries.push_back(*entry);
                }
                else
                {
                    // Insert empty headers for any unused objects (required for save compatibility)
                    // TODO: Move this into the S5 code.
                    entries.emplace_back();
                }
            }
        }
    }

    template<typename... TObject>
    static void addAllInUseHeadersOfTypes(std::vector<ObjectHeader>& entries)
    {
        (addAllInUseHeadersOfType<TObject>(entries), ...);
    }

    std::vector<ObjectHeader> getHeaders()
    {
        std::vector<ObjectHeader> entries;
        entries.reserve(ObjectManager::maxObjects);

        addAllInUseHeadersOfTypes<InterfaceSkinObject, SoundObject, CurrencyObject, SteamObject, RockObject, WaterObject, LandObject, TownNamesObject, CargoObject, WallObject, TrainSignalObject, LevelCrossingObject, StreetLightObject, TunnelObject, BridgeObject, TrainStationObject, TrackExtraObject, TrackObject, RoadStationObject, RoadExtraObject, RoadObject, AirportObject, DockObject, VehicleObject, TreeObject, SnowObject, ClimateObject, HillShapesObject, BuildingObject, ScaffoldingObject, IndustryObject, RegionObject, CompetitorObject, ScenarioTextObject>(entries);

        return entries;
    }

    // 0x00472AFE
    ObjIndexPair getActiveObject(ObjectType objectType, uint8_t* edi)
    {
        const auto objects = getAvailableObjects(objectType);

        for (auto [index, object] : objects)
        {
            if (edi[index] & (1 << 0))
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        return { -1, ObjectIndexEntry{} };
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static const uint8_t descriptionRowHeight = 10;

    void drawGenericDescription(Gfx::Context& context, Ui::Point& rowPosition, const uint16_t designed, const uint16_t obsolete)
    {
        if (designed != 0)
        {
            FormatArguments args{};
            args.push(designed);
            Gfx::drawString_494B3F(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_designed, &args);
            rowPosition.y += descriptionRowHeight;
        }

        if (obsolete != 0xFFFF)
        {
            FormatArguments args{};
            args.push(obsolete);
            Gfx::drawString_494B3F(context, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_obsolete, &args);
            rowPosition.y += descriptionRowHeight;
        }
    }
}
