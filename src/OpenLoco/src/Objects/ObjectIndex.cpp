#include "ObjectIndex.h"
#include "Environment.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Gfx.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "ObjectManager.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/ProgressBar.h"
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

    // 0x00473D1D
    static bool selectObjectFromIndexInternal(uint8_t bl, bool isRecursed, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData)
    {
        if (!isRecursed)
        {
            // Vanilla had a few pointless no side effect loops here
        }

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

        auto& val = objectFlags[index];

        if (bl & (1u << 0))
        {
            if (!isRecursed)
            {
                if (bl & (1u << 3))
                {
                    val |= SelectedObjectsFlags::alwaysRequired;
                }
            }

            if ((val & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
            {
                if (!isRecursed)
                {
                    refreshRequiredByAnother(objectFlags);
                }
                return true;
            }

            if (selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())] >= getMaxObjects(objHeader.getType()))
            {
                GameCommands::setErrorText(StringIds::too_many_objects_of_this_type_selected);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            for (const auto& header : entry._requiredObjects)
            {
                if (!selectObjectFromIndexInternal(bl, true, header, objectFlags, selectionMetaData))
                {
                    if (isRecursed)
                    {
                        resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                    }
                    return false;
                }
            }

            for (const auto& header : entry._alsoLoadObjects)
            {
                if (bl & (1U << 2))
                {
                    selectObjectFromIndexInternal(bl, true, header, objectFlags, selectionMetaData);
                }
            }

            if (isRecursed && !(bl & (1U << 1)))
            {
                auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2040));
                buffer = StringManager::formatString(buffer, sizeof(buffer), StringIds::the_following_object_must_be_selected_first);
                objectCreateIdentifierName(buffer, objHeader);
                GameCommands::setErrorText(StringIds::buffer_2040);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            if (entry._displayData->numImages + selectionMetaData.numImages > Gfx::G1ExpectedCount::kObjects)
            {
                GameCommands::setErrorText(StringIds::not_enough_space_for_graphics);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            // Its possible that we have loaded other objects so check again
            if (selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())] >= getMaxObjects(objHeader.getType()))
            {
                GameCommands::setErrorText(StringIds::too_many_objects_of_this_type_selected);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            selectionMetaData.numImages += entry._displayData->numImages;
            selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())]++;
            val |= SelectedObjectsFlags::selected;
            if (!isRecursed)
            {
                refreshRequiredByAnother(objectFlags);
            }
            return true;
        }
        else
        {
            if ((val & SelectedObjectsFlags::selected) == SelectedObjectsFlags::none)
            {
                if (!isRecursed)
                {
                    refreshRequiredByAnother(objectFlags);
                }
                return true;
            }

            if ((val & SelectedObjectsFlags::inUse) != SelectedObjectsFlags::none)
            {
                GameCommands::setErrorText(StringIds::this_object_is_currently_in_use);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            if ((val & SelectedObjectsFlags::requiredByAnother) != SelectedObjectsFlags::none)
            {
                GameCommands::setErrorText(StringIds::this_object_is_required_by_another_object);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            // Note: Not used in Locomotion (RCT2 only)
            if ((val & SelectedObjectsFlags::alwaysRequired) != SelectedObjectsFlags::none)
            {
                GameCommands::setErrorText(StringIds::this_object_is_always_required);
                if (isRecursed)
                {
                    resetSelectedObjectCountsAndSize(objectFlags, selectionMetaData);
                }
                return false;
            }

            for (const auto& header : entry._alsoLoadObjects)
            {
                if (bl & (1U << 2))
                {
                    selectObjectFromIndexInternal(bl, true, header, objectFlags, selectionMetaData);
                }
            }

            selectionMetaData.numImages -= entry._displayData->numImages;
            selectionMetaData.numSelectedObjects[enumValue(objHeader.getType())]--;
            val &= ~SelectedObjectsFlags::selected;
            if (!isRecursed)
            {
                refreshRequiredByAnother(objectFlags);
            }
            return true;
        }
    }

    bool selectObjectFromIndex(uint8_t bl, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData)
    {
        return selectObjectFromIndexInternal(bl, false, objHeader, objectFlags, selectionMetaData);
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
}
