#include "ObjectManager.h"
#include "../Audio/Audio.h"
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

    loco_global<ObjectRepositoryItem[maxObjectTypes], 0x4FE0B8> object_repository;

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

        _installedObjectCount = 0;
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

    static ObjectRepositoryItem& getRepositoryItem(ObjectType type)
    {
        return object_repository[enumValue(type)];
    }

    ObjectHeader& getHeader(const LoadedObjectHandle& handle)
    {
        return getRepositoryItem(handle.type).object_entry_extendeds[handle.id];
    }

    Object* getAny(const LoadedObjectHandle& handle)
    {
        auto obj = getRepositoryItem(handle.type).objects[handle.id];
        if (obj == (void*)-1)
        {
            obj = nullptr;
        }
        return obj;
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

                    if (header == objHeader)
                    {
                        return { LoadedObjectHandle{ objectType, i } };
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

    static bool callObjectValidate(const ObjectType type, Object& obj)
    {
        switch (type)
        {
            case ObjectType::interfaceSkin:
                return reinterpret_cast<InterfaceSkinObject*>(&obj)->validate();
            case ObjectType::sound:
                return reinterpret_cast<SoundObject*>(&obj)->validate();
            case ObjectType::currency:
                return reinterpret_cast<CurrencyObject*>(&obj)->validate();
            case ObjectType::steam:
                return reinterpret_cast<SteamObject*>(&obj)->validate();
            case ObjectType::rock:
                return reinterpret_cast<RockObject*>(&obj)->validate();
            case ObjectType::water:
                return reinterpret_cast<WaterObject*>(&obj)->validate();
            case ObjectType::land:
                return reinterpret_cast<LandObject*>(&obj)->validate();
            case ObjectType::townNames:
                return reinterpret_cast<TownNamesObject*>(&obj)->validate();
            case ObjectType::cargo:
                return reinterpret_cast<CargoObject*>(&obj)->validate();
            case ObjectType::wall:
                return reinterpret_cast<WallObject*>(&obj)->validate();
            case ObjectType::trackSignal:
                return reinterpret_cast<TrainSignalObject*>(&obj)->validate();
            case ObjectType::levelCrossing:
                return reinterpret_cast<LevelCrossingObject*>(&obj)->validate();
            default:
                auto objectProcTable = (const uintptr_t*)0x004FE1C8;
                auto objectProc = objectProcTable[static_cast<size_t>(type)];

                registers regs;
                regs.al = enumValue(ObjectProcedure::validate);
                regs.esi = X86Pointer(&obj);
                return (call(objectProc, regs) & X86_FLAG_CARRY) == 0;
        }
    }

    static void callObjectUnload(const ObjectType type, Object& obj)
    {
        switch (type)
        {
            case ObjectType::interfaceSkin:
                reinterpret_cast<InterfaceSkinObject*>(&obj)->unload();
                break;
            case ObjectType::sound:
                reinterpret_cast<SoundObject*>(&obj)->unload();
                break;
            case ObjectType::currency:
                reinterpret_cast<CurrencyObject*>(&obj)->unload();
                break;
            case ObjectType::steam:
                reinterpret_cast<SteamObject*>(&obj)->unload();
                break;
            case ObjectType::rock:
                reinterpret_cast<RockObject*>(&obj)->unload();
                break;
            case ObjectType::water:
                reinterpret_cast<WaterObject*>(&obj)->unload();
                break;
            case ObjectType::land:
                reinterpret_cast<LandObject*>(&obj)->unload();
                break;
            case ObjectType::townNames:
                reinterpret_cast<TownNamesObject*>(&obj)->unload();
                break;
            case ObjectType::cargo:
                reinterpret_cast<CargoObject*>(&obj)->unload();
                break;
            case ObjectType::wall:
                reinterpret_cast<WallObject*>(&obj)->unload();
                break;
            case ObjectType::trackSignal:
                reinterpret_cast<TrainSignalObject*>(&obj)->unload();
                break;
            case ObjectType::levelCrossing:
                reinterpret_cast<LevelCrossingObject*>(&obj)->unload();
                break;
            default:
                auto objectProcTable = (const uintptr_t*)0x004FE1C8;
                auto objectProc = objectProcTable[static_cast<size_t>(type)];

                registers regs;
                regs.al = enumValue(ObjectProcedure::unload);
                regs.esi = X86Pointer(&obj);
                call(objectProc, regs);
                break;
        }
    }

    static bool callObjectFunction(const ObjectType type, Object& obj, const ObjectProcedure proc)
    {
        switch (proc)
        {
            case ObjectProcedure::validate:
                return callObjectValidate(type, obj);
            case ObjectProcedure::unload:
                callObjectUnload(type, obj);
                return true;
            default:
                throw std::runtime_error("Don't call this function with load/drawPreview.");
        }
    }

    static void callObjectLoad(const LoadedObjectHandle& handle, Object& obj, stdx::span<std::byte> data)
    {
        switch (handle.type)
        {
            case ObjectType::interfaceSkin:
                reinterpret_cast<InterfaceSkinObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::sound:
                reinterpret_cast<SoundObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::currency:
                reinterpret_cast<CurrencyObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::steam:
                reinterpret_cast<SteamObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::rock:
                reinterpret_cast<RockObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::water:
                reinterpret_cast<WaterObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::land:
                reinterpret_cast<LandObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::townNames:
                reinterpret_cast<TownNamesObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::cargo:
                reinterpret_cast<CargoObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::wall:
                reinterpret_cast<WallObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::trackSignal:
                reinterpret_cast<TrainSignalObject*>(&obj)->load(handle, data);
                break;
            case ObjectType::levelCrossing:
                reinterpret_cast<LevelCrossingObject*>(&obj)->load(handle, data);
                break;
            default:
                auto objectProcTable = (const uintptr_t*)0x004FE1C8;
                auto objectProc = objectProcTable[static_cast<size_t>(handle.type)];

                registers regs;
                regs.al = static_cast<uint8_t>(ObjectProcedure::load);
                regs.esi = X86Pointer(&obj);
                regs.ebx = handle.id;
                regs.ecx = enumValue(handle.type);
                call(objectProc, regs);
                break;
        }
    }

    static bool callObjectFunction(const LoadedObjectHandle& handle, ObjectProcedure proc)
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

        const auto& installedObject = res->second;
        const auto filePath = Environment::getPath(Environment::PathId::objects) / fs::u8path(installedObject._filename);

        SawyerStreamReader stream(filePath);
        ObjectHeader loadingHeader;
        stream.read(&loadingHeader, sizeof(loadingHeader));
        if (loadingHeader != header)
        {
            // Something wrong has happened and installed object does not match index
            // Vanilla continued to search for subsequent matching installed headers.
            return false;
        }

        // Vanilla would branch and perform more efficient readChunk if size was known from installedObject.ObjectHeader2
        auto data = stream.readChunk();

        if (!computeObjectChecksum(loadingHeader, data))
        {
            // Something wrong has happened and installed object checksum is broken
            return false;
        }

        // Copy the object into Loco freeable memory (required for when load loads the object)
        auto* object = reinterpret_cast<Object*>(malloc(data.size()));
        std::copy(std::begin(data), std::end(data), reinterpret_cast<uint8_t*>(object));

        if (!callObjectFunction(loadingHeader.getType(), *object, ObjectProcedure::validate))
        {
            free(object);
            object = nullptr;
            // Object failed validation
            return false;
        }

        if (_totalNumImages >= Gfx::G1ExpectedCount::kObjects + Gfx::G1ExpectedCount::kDisc)
        {
            free(object);
            object = nullptr;
            // Too many objects loaded and no free image space
            return false;
        }

        object_repository[enumValue(loadingHeader.getType())].objects[id] = object;
        auto& extendedHeader = object_repository[enumValue(loadingHeader.getType())].object_entry_extendeds[id];
        extendedHeader = ObjectEntry2{
            loadingHeader, data.size()
        };

        if (!*_isPartialLoaded)
        {
            callObjectLoad({ loadingHeader.getType(), id }, *object, stdx::span<std::byte>(reinterpret_cast<std::byte*>(object), data.size()));
        }

        return true;
    }

    static std::optional<LoadedObjectId> findFreeObjectId(const ObjectType type)
    {
        for (LoadedObjectId id = 0; id < getMaxObjects(type); ++id)
        {
            if (getAny({ type, id }) == nullptr)
            {
                return id;
            }
        }
        return std::nullopt;
    }

    // 0x00471FF8
    void unload(const ObjectHeader& header)
    {
        auto handle = findIndex(header);
        if (!handle)
        {
            return;
        }
        unload(*handle);
        free(object_repository[enumValue(handle->type)].objects[handle->id]);
        object_repository[enumValue(handle->type)].objects[handle->id] = reinterpret_cast<Object*>(-1);
    }

    // 0x00471BCE
    bool load(const ObjectHeader& header)
    {
        auto id = findFreeObjectId(header.getType());
        if (!id)
        {
            return false;
        }
        return load(header, *id);
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
            if (!header.isEmpty() && !load(header, id))
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
        if (!callObjectValidate(objectHeader.getType(), *obj))
        {
            return false;
        }

        if (_totalNumImages >= Gfx::G1ExpectedCount::kObjects + Gfx::G1ExpectedCount::kDisc)
        {
            // Free objectData?
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

    void unload(const LoadedObjectHandle& handle)
    {
        callObjectFunction(handle, ObjectProcedure::unload);
    }

    size_t getByteLength(const LoadedObjectHandle& handle)
    {
        return getRepositoryItem(handle.type).object_entry_extendeds[handle.id].dataSize;
    }

    template<typename TObject>
    static void addAllInUseHeadersOfType(std::vector<ObjectHeader>& entries)
    {
        if constexpr (getMaxObjects(TObject::kObjectType) == 1)
        {
            auto* obj = ObjectManager::get<TObject>();
            if (obj != nullptr)
            {
                auto entry = getHeader({ TObject::kObjectType, 0 });
                entries.push_back(entry);
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
            for (LoadedObjectId i = 0; i < getMaxObjects(TObject::kObjectType); ++i)
            {
                auto* obj = ObjectManager::get<TObject>(i);
                if (obj != nullptr)
                {
                    auto entry = getHeader({ TObject::kObjectType, i });
                    entries.push_back(entry);
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

    // 0x004796A9
    void updateYearly1()
    {
        // set default levelCrossing
        call(0x004796A9);
    }

    // 0x004C3A9E
    void updateYearly2()
    {
        // update available vehicles/roads/airports/etc.
        call(0x004C3A9E);
    }
}
