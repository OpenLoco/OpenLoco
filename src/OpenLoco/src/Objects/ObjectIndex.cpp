#include "ObjectIndex.h"
#include "Environment.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Gfx.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "ObjectManager.h"
#include "OpenLoco.h"
#include "RoadObject.h"
#include "TrackObject.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleManager.h"
#include "World/CompanyManager.h"
#include "World/IndustryManager.h"
#include "World/Station.h"
#include <OpenLoco/Core/FileStream.h>
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Core/Timer.hpp>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/String.hpp>
#include <cstdint>
#include <fstream>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::ObjectManager
{
    static loco_global<std::byte*, 0x0050D13C> _installedObjectList;
    static loco_global<uint32_t, 0x0112A110> _installedObjectCount;
    static loco_global<bool, 0x0112A17E> _customObjectsInIndex;
    static loco_global<std::byte*, 0x0050D158> _dependentObjectsVector;
    static loco_global<std::byte[0x2002], 0x0112A17F> _dependentObjectVectorData;
    static loco_global<bool, 0x0050AEAD> _isFirstTime;
    static loco_global<bool, 0x0050D161> _isPartialLoaded;
    static loco_global<int32_t, 0x0050D148> _50D144refCount;
    static loco_global<SelectedObjectsFlags*, 0x0050D144> _50D144;
    static loco_global<ObjectSelectionMeta, 0x0112C1C5> _objectSelectionMeta;
    static loco_global<std::array<uint16_t, kMaxObjectTypes>, 0x0112C181> _numObjectsPerType;

    static constexpr uint8_t kCurrentIndexVersion = 3;

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
            currentState.dateHash = std::rotr(currentState.dateHash, 5);
            currentState.totalFileSize += file.file_size();
        }

        // NB: vanilla used to set just flag 24 to 1; we use it as a version byte.
        currentState.numObjects |= kCurrentIndexVersion << 24;

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
        Core::Timer saveTimer;

        FileStream stream;
        const auto indexPath = Environment::getPathNoWarning(Environment::PathId::plugin1);
        stream.open(indexPath, StreamMode::write);
        if (!stream.isOpen())
        {
            Logging::error("Unable to save object index.");
            return;
        }
        stream.writeValue(header);
        stream.write(*_installedObjectList, header.fileSize);

        Logging::verbose("Saved object index in {} milliseconds.", saveTimer.elapsed());
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
        objHeader2.decodedFileSize = std::numeric_limits<uint32_t>::max();
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

    // TODO: Take dependent object vectors from loadTemporary
    static std::pair<ObjectIndexEntry, size_t> createNewEntry(std::byte* entryBuffer, const ObjectHeader& objHeader, const fs::path filename, const TempLoadMetaData& metaData)
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
        std::memcpy(&entryBuffer[newEntrySize], &metaData.fileSizeHeader, sizeof(metaData.fileSizeHeader));
        newEntrySize += sizeof(metaData.fileSizeHeader);

        // Name
        strcpy(reinterpret_cast<char*>(&entryBuffer[newEntrySize]), StringManager::getString(0x2000));
        entry._name = reinterpret_cast<char*>(&entryBuffer[newEntrySize]);
        newEntrySize += strlen(reinterpret_cast<char*>(&entryBuffer[newEntrySize])) + 1;

        // Header3
        std::memcpy(&entryBuffer[newEntrySize], &metaData.displayData, sizeof(metaData.displayData));
        newEntrySize += sizeof(metaData.displayData);

        auto* ptr = &_dependentObjectVectorData[0];

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
        try
        {
            FileStream stream;
            stream.open(filepath, StreamMode::read);
            if (!stream.isOpen())
            {
                Logging::error("Unable to open object index file.");
                return;
            }
            objHeader = stream.readValue<ObjectHeader>();
        }
        catch (const std::runtime_error& ex)
        {
            Logging::error("Unable to read object index header: {}", ex.what());
            return;
        }

        const auto curObjPos = usedBufferSize;
        const auto partialNewEntry = createPartialNewEntry(&_installedObjectList[usedBufferSize], objHeader, filepath.filename());
        usedBufferSize += partialNewEntry.second;
        _installedObjectCount++;

        _isPartialLoaded = true;
        _dependentObjectsVector = _dependentObjectVectorData;
        const auto loadResult = loadTemporaryObject(objHeader);
        _dependentObjectsVector = reinterpret_cast<std::byte*>(-1);
        _isPartialLoaded = false;
        _installedObjectCount--;
        // Rewind as it is only a partial object loaded
        usedBufferSize = curObjPos;

        if (!loadResult.has_value())
        {
            Logging::error("Unable to load the object '{}', can't add to index", objHeader.getName());
            return;
        }

        // Load full entry into temp buffer.
        // 0x009D1CC8
        std::byte newEntryBuffer[0x2000] = {};
        const auto [newEntry, newEntrySize] = createNewEntry(newEntryBuffer, objHeader, filepath.filename(), loadResult.value());

        freeTemporaryObject();

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
        Core::Timer loadTimer;

        const auto indexPath = Environment::getPathNoWarning(Environment::PathId::plugin1);
        if (!fs::exists(indexPath))
        {
            Logging::verbose("Object index does not exist.");
            return false;
        }
        FileStream stream;
        stream.open(indexPath, StreamMode::read);
        if (!stream.isOpen())
        {
            Logging::error("Unable to load the object index.");
            return false;
        }

        try
        {
            // 0x00112A14C -> 160
            auto header = stream.readValue<IndexHeader>();
            if (header.state != currentState)
            {
                Logging::info("Object index out of date.");
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
                stream.read(*_installedObjectList, header.fileSize);
                _installedObjectCount = header.numObjects;

                Logging::verbose("Loaded object index in {} milliseconds.", loadTimer.elapsed());
            }
        }
        catch (const std::runtime_error& ex)
        {
            Logging::error("Unable to load the object index: {}", ex.what());
            return false;
        }

        reloadAll();

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

    uint32_t getNumInstalledObjects()
    {
        return *_installedObjectCount;
    }

    std::vector<std::pair<ObjectIndexId, ObjectIndexEntry>> getAvailableObjects(ObjectType type)
    {
        auto ptr = (std::byte*)_installedObjectList;
        std::vector<std::pair<ObjectIndexId, ObjectIndexEntry>> list;

        for (ObjectIndexId i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);
            if (entry._header->getType() == type)
                list.emplace_back(std::pair<ObjectIndexId, ObjectIndexEntry>(i, entry));
        }

        return list;
    }

    static std::optional<std::pair<ObjectIndexId, ObjectIndexEntry>> internalFindObjectInIndex(const ObjectHeader& objectHeader)
    {
        const auto objects = getAvailableObjects(objectHeader.getType());
        auto res = std::find_if(std::begin(objects), std::end(objects), [&objectHeader](auto& obj) { return *obj.second._header == objectHeader; });
        if (res == std::end(objects))
        {
            return std::nullopt;
        }
        return *res;
    }

    std::optional<ObjectIndexEntry> findObjectInIndex(const ObjectHeader& objectHeader)
    {
        auto res = internalFindObjectInIndex(objectHeader);
        if (!res.has_value())
        {
            return std::nullopt;
        }
        return res->second;
    }

    bool isObjectInstalled(const ObjectHeader& objectHeader)
    {
        return findObjectInIndex(objectHeader).has_value();
    }

    // 0x00472AFE
    ObjIndexPair getActiveObject(ObjectType objectType, std::span<SelectedObjectsFlags> objectIndexFlags)
    {
        const auto objects = getAvailableObjects(objectType);

        for (auto [index, object] : objects)
        {
            if ((objectIndexFlags[index] & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        return { -1, ObjectIndexEntry{} };
    }

    // 0x0047400C
    static void setObjectIsRequiredByAnother(std::span<SelectedObjectsFlags> objectFlags, const ObjectHeader& objHeader)
    {
        const auto res = internalFindObjectInIndex(objHeader);
        if (!res.has_value())
        {
            return;
        }
        auto& [index, entry] = *res;
        objectFlags[index] |= SelectedObjectsFlags::requiredByAnother;

        for (auto& objHeader2 : entry._requiredObjects)
        {
            setObjectIsRequiredByAnother(objectFlags, objHeader2);
        }
    }

    // 0x004740CF
    static void refreshRequiredByAnother(std::span<SelectedObjectsFlags> objectFlags)
    {
        for (auto& val : objectFlags)
        {
            val &= ~SelectedObjectsFlags::requiredByAnother;
        }

        auto ptr = (std::byte*)_installedObjectList;
        for (ObjectIndexId i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);
            if ((objectFlags[i] & SelectedObjectsFlags::selected) == SelectedObjectsFlags::none)
            {
                continue;
            }

            for (auto& objHeader : entry._requiredObjects)
            {
                setObjectIsRequiredByAnother(objectFlags, objHeader);
            }
        }
    }

    // 0x00472C84
    static void resetSelectedObjectCountsAndSize(std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData)
    {
        std::fill(std::begin(selectionMetaData.numSelectedObjects), std::end(selectionMetaData.numSelectedObjects), 0U);

        uint32_t totalNumImages = 0;
        auto ptr = (std::byte*)_installedObjectList;
        for (ObjectIndexId i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);
            if ((objectFlags[i] & SelectedObjectsFlags::selected) == SelectedObjectsFlags::none)
            {
                continue;
            }

            selectionMetaData.numSelectedObjects[enumValue(entry._header->getType())]++;
            totalNumImages += entry._displayData->numImages;
        }

        selectionMetaData.numImages = totalNumImages;
    }

    static bool selectObjectFromIndexInternal(SelectObjectModes mode, bool isRecursed, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData);

    static bool selectObjectInternal(SelectObjectModes mode, bool isRecursed, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData, ObjectIndexId index, const ObjectIndexEntry& entry)
    {
        if (!isRecursed)
        {
            if ((mode & SelectObjectModes::markAsAlwaysRequired) != SelectObjectModes::none)
            {
                objectFlags[index] |= SelectedObjectsFlags::alwaysRequired;
            }
        }

        // Object already selected so skip
        if ((objectFlags[index] & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
        {
            return true;
        }

        // If this was selected too many objects would be selected
        if (selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())] >= getMaxObjects(objHeader.getType()))
        {
            GameCommands::setErrorText(StringIds::too_many_objects_of_this_type_selected);
            return false;
        }

        // All required objects must select if this selects
        for (const auto& header : entry._requiredObjects)
        {
            if (!selectObjectFromIndexInternal(mode, true, header, objectFlags, selectionMetaData))
            {
                return false;
            }
        }

        // With also load objects it doesn't matter if they can't load
        for (const auto& header : entry._alsoLoadObjects)
        {
            if ((mode & SelectObjectModes::selectAlsoLoads) != SelectObjectModes::none)
            {
                selectObjectFromIndexInternal(mode, true, header, objectFlags, selectionMetaData);
            }
        }

        // When in "Don't select dependent objects" mode
        // fail to select rather than select the dependent object.
        // Note: This mode is never used in vanilla!
        if (isRecursed && (mode & SelectObjectModes::selectDependents) == SelectObjectModes::none)
        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2040));
            buffer = StringManager::formatString(buffer, sizeof(buffer), StringIds::the_following_object_must_be_selected_first);
            objectCreateIdentifierName(buffer, objHeader);
            GameCommands::setErrorText(StringIds::buffer_2040);
            return false;
        }

        // If this object was selected too many images would be needed when loading
        if (entry._displayData->numImages + selectionMetaData.numImages > Gfx::G1ExpectedCount::kObjects)
        {
            GameCommands::setErrorText(StringIds::not_enough_space_for_graphics);
            return false;
        }

        // Its possible that we have loaded other objects so check again that we haven't exceeded object counts
        if (selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())] >= getMaxObjects(objHeader.getType()))
        {
            GameCommands::setErrorText(StringIds::too_many_objects_of_this_type_selected);
            return false;
        }

        selectionMetaData.numImages += entry._displayData->numImages;
        selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())]++;
        objectFlags[index] |= SelectedObjectsFlags::selected;
        return true;
    }

    static bool deselectObjectInternal(SelectObjectModes mode, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData, ObjectIndexId index, const ObjectIndexEntry& entry)
    {
        // Object already deselected
        if ((objectFlags[index] & SelectedObjectsFlags::selected) == SelectedObjectsFlags::none)
        {
            return true;
        }

        // Can't deselect in use objects (placed on map)
        if ((objectFlags[index] & SelectedObjectsFlags::inUse) != SelectedObjectsFlags::none)
        {
            GameCommands::setErrorText(StringIds::this_object_is_currently_in_use);
            return false;
        }

        // Can't deselect objects required by others
        if ((objectFlags[index] & SelectedObjectsFlags::requiredByAnother) != SelectedObjectsFlags::none)
        {
            GameCommands::setErrorText(StringIds::this_object_is_required_by_another_object);
            return false;
        }

        // Can't deselect always required objects
        // Note: Not used in Locomotion (RCT2 only)
        if ((objectFlags[index] & SelectedObjectsFlags::alwaysRequired) != SelectedObjectsFlags::none)
        {
            GameCommands::setErrorText(StringIds::this_object_is_always_required);
            return false;
        }

        for (const auto& header : entry._alsoLoadObjects)
        {
            if ((mode & SelectObjectModes::selectAlsoLoads) != SelectObjectModes::none)
            {
                selectObjectFromIndexInternal(mode, true, header, objectFlags, selectionMetaData);
            }
        }

        selectionMetaData.numImages -= entry._displayData->numImages;
        selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())]--;
        objectFlags[index] &= ~SelectedObjectsFlags::selected;
        return true;
    }

    // 0x00473D1D
    static bool selectObjectFromIndexInternal(SelectObjectModes mode, bool isRecursed, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData)
    {
        auto objIndexEntry = internalFindObjectInIndex(objHeader);
        if (!objIndexEntry.has_value())
        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2040));
            buffer = StringManager::formatString(buffer, sizeof(buffer), StringIds::data_for_following_object_not_found);
            objectCreateIdentifierName(buffer, objHeader);
            GameCommands::setErrorText(StringIds::buffer_2040);
            if (isRecursed)
            {
                resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
            }
            return false;
        }

        auto& [index, entry] = *objIndexEntry;

        bool success = (mode & SelectObjectModes::select) != SelectObjectModes::none
            ? selectObjectInternal(mode, isRecursed, objHeader, objectFlags, selectionMetaData, index, entry)
            : deselectObjectInternal(mode, objHeader, objectFlags, selectionMetaData, index, entry);

        if (success)
        {
            if (!isRecursed)
            {
                refreshRequiredByAnother(objectFlags);
            }
        }
        else
        {
            if (isRecursed)
            {
                resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
            }
        }
        return success;
    }

    bool selectObjectFromIndex(SelectObjectModes mode, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData)
    {
        return selectObjectFromIndexInternal(mode, false, objHeader, objectFlags, selectionMetaData);
    }

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

        entry._displayData = reinterpret_cast<ObjectHeader3*>(*ptr);
        *ptr += sizeof(ObjectHeader3);

        uint8_t* countA = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        entry._requiredObjects = std::span<ObjectHeader>(reinterpret_cast<ObjectHeader*>(*ptr), *countA);
        for (int n = 0; n < *countA; n++)
        {
            // header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        uint8_t* countB = (uint8_t*)*ptr;
        *ptr += sizeof(uint8_t);
        entry._alsoLoadObjects = std::span<ObjectHeader>(reinterpret_cast<ObjectHeader*>(*ptr), *countB);
        for (int n = 0; n < *countB; n++)
        {
            // header* subh = (header*)ptr;
            *ptr += sizeof(ObjectHeader);
        }

        return entry;
    }

    // 0x00472DA1
    static void markInUseObjectsByTile(std::array<std::span<uint8_t>, kMaxObjectTypes>& loadedObjectFlags)
    {
        // Iterate the whole map looking for things
        for (const auto pos : World::getWorldRange())
        {
            const auto tile = World::TileManager::get(pos);
            for (auto el : tile)
            {
                auto* elSurface = el.as<World::SurfaceElement>();
                auto* elTrack = el.as<World::TrackElement>();
                auto* elStation = el.as<World::StationElement>();
                auto* elSignal = el.as<World::SignalElement>();
                auto* elBuilding = el.as<World::BuildingElement>();
                auto* elTree = el.as<World::TreeElement>();
                auto* elWall = el.as<World::WallElement>();
                auto* elRoad = el.as<World::RoadElement>();
                auto* elIndustry = el.as<World::IndustryElement>();

                if (elSurface != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::land)][elSurface->terrain()] |= (1U << 0);
                    if (elSurface->var_4_E0())
                    {
                        loadedObjectFlags[enumValue(ObjectType::snow)][0] |= (1U << 0);
                    }
                }
                else if (elTrack != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::track)][elTrack->trackObjectId()] |= (1U << 0);
                    if (elTrack->hasBridge())
                    {
                        loadedObjectFlags[enumValue(ObjectType::bridge)][elTrack->bridge()] |= (1U << 0);
                    }
                    for (auto i = 0U; i < 4; ++i)
                    {
                        if (elTrack->hasMod(i))
                        {
                            auto* trackObj = get<TrackObject>(elTrack->trackObjectId());
                            loadedObjectFlags[enumValue(ObjectType::trackExtra)][trackObj->mods[i]] |= (1U << 0);
                        }
                    }
                }
                else if (elStation != nullptr)
                {
                    switch (elStation->stationType())
                    {
                        case StationType::trainStation:
                            loadedObjectFlags[enumValue(ObjectType::trackStation)][elStation->objectId()] |= (1U << 0);
                            break;
                        case StationType::roadStation:
                            loadedObjectFlags[enumValue(ObjectType::roadStation)][elStation->objectId()] |= (1U << 0);
                            break;
                        case StationType::airport:
                            loadedObjectFlags[enumValue(ObjectType::airport)][elStation->objectId()] |= (1U << 0);
                            break;
                        case StationType::docks:
                            loadedObjectFlags[enumValue(ObjectType::dock)][elStation->objectId()] |= (1U << 0);
                            break;
                    }
                }
                else if (elSignal != nullptr)
                {
                    if (elSignal->getLeft().hasSignal())
                    {
                        loadedObjectFlags[enumValue(ObjectType::trackSignal)][elSignal->getLeft().signalObjectId()] |= (1U << 0);
                    }
                    if (elSignal->getRight().hasSignal())
                    {
                        loadedObjectFlags[enumValue(ObjectType::trackSignal)][elSignal->getRight().signalObjectId()] |= (1U << 0);
                    }
                }
                else if (elBuilding != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::building)][elBuilding->objectId()] |= (1U << 0);
                    if (!elBuilding->isConstructed())
                    {
                        loadedObjectFlags[enumValue(ObjectType::scaffolding)][0] |= (1U << 0);
                    }
                }
                else if (elTree != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::tree)][elTree->treeObjectId()] |= (1U << 0);
                }
                else if (elWall != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::wall)][elWall->wallObjectId()] |= (1U << 0);
                }
                else if (elRoad != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::road)][elRoad->roadObjectId()] |= (1U << 0);
                    if (elRoad->hasBridge())
                    {
                        loadedObjectFlags[enumValue(ObjectType::bridge)][elRoad->bridge()] |= (1U << 0);
                    }
                    if (elRoad->hasLevelCrossing())
                    {
                        loadedObjectFlags[enumValue(ObjectType::levelCrossing)][elRoad->levelCrossingObjectId()] |= (1U << 0);
                    }
                    else
                    {
                        if (elRoad->streetLightStyle() != 0)
                        {
                            loadedObjectFlags[enumValue(ObjectType::streetLight)][0] |= (1U << 0);
                        }
                    }

                    auto* roadObj = get<RoadObject>(elRoad->roadObjectId());
                    if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
                    {
                        for (auto i = 0U; i < 2; ++i)
                        {
                            if (elRoad->hasMod(i))
                            {
                                loadedObjectFlags[enumValue(ObjectType::roadExtra)][roadObj->mods[i]] |= (1U << 0);
                            }
                        }
                    }
                }
                else if (elIndustry != nullptr)
                {
                    if (!elIndustry->isConstructed())
                    {
                        loadedObjectFlags[enumValue(ObjectType::scaffolding)][0] |= (1U << 0);
                    }
                }
            }
        }
    }

    // 0x00473050
    static void markInUseVehicleObjects(std::span<uint8_t> vehicleObjectFlags)
    {
        for (const auto& v : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            for (auto& car : train.cars)
            {
                for (auto& component : car)
                {
                    vehicleObjectFlags[component.body->objectId] |= (1U << 0);
                }
            }
        }
    }

    // 0x00473098
    static void markInUseIndustryObjects(std::span<uint8_t> industryObjectFlags)
    {
        for (auto& ind : IndustryManager::industries())
        {
            industryObjectFlags[ind.objectId] = (1U << 0);
        }
    }

    // 0x004730BC
    static void markInUseCompetitorObjects(std::span<uint8_t> competitorObjectFlags)
    {
        for (auto& company : CompanyManager::companies())
        {
            competitorObjectFlags[company.competitorId] = (1U << 0);
        }
    }

    // 0x00472D70
    static void markLoadedObjects(std::array<std::span<uint8_t>, kMaxObjectTypes>& loadedObjectFlags)
    {
        for (uint8_t i = 0; i < kMaxObjectTypes; ++i)
        {
            const auto type = static_cast<ObjectType>(i);
            for (LoadedObjectId j = 0U; j < getMaxObjects(type); ++j)
            {
                if (getAny(LoadedObjectHandle{ type, j }) != nullptr)
                {
                    loadedObjectFlags[i][j] |= (1U << 1);
                }
            }
        }
    }

    // 0x00472D3F
    static void markInUseObjects([[maybe_unused]] std::span<SelectedObjectsFlags> objectFlags)
    {
        std::array<uint8_t, kMaxObjects> allLoadedObjectFlags{};
        std::array<std::span<uint8_t>, kMaxObjectTypes> loadedObjectFlags;
        auto count = 0;
        for (uint8_t i = 0; i < kMaxObjectTypes; ++i)
        {
            const auto type = static_cast<ObjectType>(i);
            loadedObjectFlags[i] = std::span<uint8_t>(allLoadedObjectFlags.begin() + count, getMaxObjects(type));
            count += getMaxObjects(type);
        }

        markLoadedObjects(loadedObjectFlags);

        if ((addr<0x00525E28, uint32_t>() & 1) != 0)
        {
            loadedObjectFlags[enumValue(ObjectType::region)][0] |= (1U << 0);
            markInUseObjectsByTile(loadedObjectFlags);
        }

        markInUseVehicleObjects(loadedObjectFlags[enumValue(ObjectType::vehicle)]);
        markInUseIndustryObjects(loadedObjectFlags[enumValue(ObjectType::industry)]);
        markInUseCompetitorObjects(loadedObjectFlags[enumValue(ObjectType::competitor)]);

        // Copy results over to objectFlags
        auto ptr = (std::byte*)_installedObjectList;
        for (ObjectIndexId i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);

            auto objHandle = findObjectHandle(*entry._header);
            if (!objHandle.has_value())
            {
                continue;
            }
            const auto loadedFlags = loadedObjectFlags[enumValue(objHandle->type)][objHandle->id];
            if (loadedFlags & (1 << 0))
            {
                objectFlags[i] |= SelectedObjectsFlags::selected | SelectedObjectsFlags::inUse;
            }
            if (loadedFlags & (1 << 1))
            {
                objectFlags[i] |= SelectedObjectsFlags::selected;
            }
        }
    }

    // 0x00472CFD
    static void selectRequiredObjects(std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& meta)
    {
        for (const auto& header : std::array<ObjectHeader, 0>{})
        {
            selectObjectFromIndex(SelectObjectModes::defaultSelect | SelectObjectModes::markAsAlwaysRequired, header, objectFlags, meta);
        }
    }

    constexpr std::array<ObjectHeader, 1> kDefaultObjects = { ObjectHeader{ static_cast<uint32_t>(enumValue(ObjectType::region)), { 'R', 'E', 'G', 'U', 'S', ' ', ' ', ' ' }, 0U } };

    // 0x00472D19
    static void selectDefaultObjects(std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& meta)
    {
        for (const auto& header : kDefaultObjects)
        {
            selectObjectFromIndex(SelectObjectModes::defaultSelect, header, objectFlags, meta);
        }
    }

    // 0x00473A95
    void prepareSelectionList(bool markInUse)
    {
        _50D144refCount++;
        if (_50D144refCount != 1)
        {
            // All setup already
            return;
        }

        SelectedObjectsFlags* selectFlags = new SelectedObjectsFlags[_installedObjectCount]{};
        _50D144 = selectFlags;
        std::span<SelectedObjectsFlags> objectFlags{ selectFlags, _installedObjectCount };
        // throw on nullptr?

        _objectSelectionMeta = ObjectSelectionMeta{};
        _numObjectsPerType = std::array<uint16_t, kMaxObjectTypes>{};

        auto ptr = (std::byte*)_installedObjectList;
        for (ObjectIndexId i = 0; i < _installedObjectCount; i++)
        {
            auto entry = ObjectIndexEntry::read(&ptr);

            (*_numObjectsPerType)[enumValue(entry._header->getType())]++;
        }
        if (markInUse)
        {
            markInUseObjects(objectFlags);
        }
        resetSelectedObjectCountsAndSize(objectFlags, _objectSelectionMeta);
        selectRequiredObjects(objectFlags, _objectSelectionMeta); // nop
        selectDefaultObjects(objectFlags, _objectSelectionMeta);
        refreshRequiredByAnother(objectFlags);
        resetSelectedObjectCountsAndSize(objectFlags, _objectSelectionMeta);
    }
}
