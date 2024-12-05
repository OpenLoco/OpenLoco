#include "ObjectIndex.h"
#include "Environment.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameStateFlags.h"
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
    // Was previously 0x0050D13C count was in 0x0112A110
    static std::vector<ObjectIndexEntry> _installedObjectList;

    static loco_global<bool, 0x0112A17E> _customObjectsInIndex;
    static loco_global<std::byte*, 0x0050D158> _dependentObjectsVector;
    static loco_global<std::byte[0x2002], 0x0112A17F> _dependentObjectVectorData;
    static loco_global<bool, 0x0050AEAD> _isFirstTime;
    static loco_global<bool, 0x0050D161> _isPartialLoaded;
    static loco_global<int32_t, 0x0050D148> _50D144refCount;
    static loco_global<SelectedObjectsFlags*, 0x0050D144> _50D144;
    static loco_global<ObjectSelectionMeta, 0x0112C1C5> _objectSelectionMeta;
    static loco_global<std::array<uint16_t, kMaxObjectTypes>, 0x0112C181> _numObjectsPerType;

    static constexpr uint8_t kCurrentIndexVersion = 5;
    static constexpr uint32_t kMaxStringLength = 1024;

    struct ObjectFolderState
    {
        uint32_t numObjects = 0;
        uint32_t totalFileSize = 0;
        uint32_t dateHash = 0;
        std::string basePath = "";

        constexpr bool operator==(const ObjectFolderState& rhs) const
        {
            return (numObjects == rhs.numObjects) && (totalFileSize == rhs.totalFileSize) && (dateHash == rhs.dateHash) && (basePath == rhs.basePath);
        }
    };

    struct ObjectFoldersState
    {
        ObjectFolderState vanillaInstall;
        ObjectFolderState install;
        ObjectFolderState customObjects;

        constexpr bool operator==(const ObjectFoldersState& rhs) const
        {
            return (vanillaInstall == rhs.vanillaInstall) && (install == rhs.install) && (customObjects == rhs.customObjects);
        }
    };

    struct IndexHeader
    {
        uint32_t version;
        ObjectFoldersState state;
        uint32_t fileSize;
        uint32_t numObjects; // duplicates ObjectFolderState.numObjects but without high 1 and includes corrupted .dat's
    };

    // Iterates an objects folder
    // optionally recurses
    // func takes a const fs::directory_entry parameter and returns false to stop iteration
    template<typename Func>
    static void iterateObjectFolder(fs::path path, bool shouldRecurse, Func&& func)
    {
        if (!fs::exists(path))
        {
            return;
        }
        auto f = [&func](const fs::directory_entry& file) {
            if (!file.is_regular_file())
            {
                return true;
            }
            const auto extension = file.path().extension().u8string();
            if (!Utility::iequals(extension, ".DAT"))
            {
                return true;
            }
            return func(file);
        };

        if (shouldRecurse)
        {
            for (const auto& file : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied))
            {
                if (!f(file))
                {
                    break;
                }
            }
        }
        else
        {
            for (const auto& file : fs::directory_iterator(path, fs::directory_options::skip_permission_denied))
            {
                if (!f(file))
                {
                    break;
                }
            }
        }
    }

    // 0x00470F3C
    static ObjectFolderState getCurrentObjectFolderState(fs::path path, bool shouldRecurse)
    {
        ObjectFolderState currentState{};

        currentState.basePath = path.u8string();

        iterateObjectFolder(path, shouldRecurse, [&currentState](const fs::directory_entry& file) {
            currentState.numObjects++;
            const auto lastWrite = file.last_write_time().time_since_epoch().count();
            currentState.dateHash ^= ((lastWrite >> 32) ^ (lastWrite & 0xFFFFFFFF));
            currentState.dateHash = std::rotr(currentState.dateHash, 5);
            currentState.totalFileSize += file.file_size();
            return true;
        });

        return currentState;
    }

    // 0x00471712
    static bool hasCustomObjectsInIndex()
    {
        for (auto& obj : _installedObjectList)
        {
            if (obj._header.isCustom())
            {
                return true;
            }
        }
        return false;
    }

    static void serialiseEntry(Stream& stream, const ObjectIndexEntry& entry)
    {
        // Header
        stream.writeValue(entry._header);

        // Filepath
        stream.writeValue<uint32_t>(entry._filepath.size());
        stream.write(entry._filepath.data(), entry._filepath.size());

        // Header2
        stream.writeValue(entry._header2.decodedFileSize);

        // Name
        stream.writeValue<uint32_t>(entry._name.size());
        stream.write(entry._name.data(), entry._name.size());

        // Header3
        stream.write(&entry._displayData, sizeof(entry._displayData));

        // ObjectList1
        stream.writeValue<uint32_t>(entry._alsoLoadObjects.size());
        for (auto& alo : entry._alsoLoadObjects)
        {
            stream.writeValue(alo);
        }

        // ObjectList2
        stream.writeValue<uint32_t>(entry._requiredObjects.size());
        for (auto& ro : entry._requiredObjects)
        {
            stream.writeValue(ro);
        }
    }

    static void serialiseFolderState(Stream& stream, const ObjectFolderState& ofs)
    {
        stream.writeValue(ofs.numObjects);
        stream.writeValue(ofs.totalFileSize);
        stream.writeValue(ofs.dateHash);

        stream.writeValue<uint32_t>(ofs.basePath.size());
        stream.write(ofs.basePath.data(), ofs.basePath.size());
    }

    static void serialiseHeader(Stream& stream, const IndexHeader& header)
    {
        stream.writeValue(header.version);

        serialiseFolderState(stream, header.state.vanillaInstall);
        serialiseFolderState(stream, header.state.install);
        serialiseFolderState(stream, header.state.customObjects);

        stream.writeValue(header.fileSize);
        stream.writeValue(header.numObjects);
    }

    static void serialiseIndex(Stream& stream, const IndexHeader& header, const std::vector<ObjectIndexEntry>& entries)
    {
        serialiseHeader(stream, header);
        stream.writeValue<uint32_t>(entries.size());
        for (auto& entry : entries)
        {
            serialiseEntry(stream, entry);
        }
    }

    static std::string deserialiseString(Stream& stream)
    {
        std::string result;
        const auto size = stream.readValue<uint32_t>();
        if (size > kMaxStringLength) // Arbitrary max length to prevent issues of massive allocation on bad data
        {
            return result;
        }
        result.resize(size);
        stream.read(result.data(), result.size());
        return result;
    }

    static ObjectIndexEntry deserialiseEntry(Stream& stream)
    {
        ObjectIndexEntry entry{};
        // Header
        entry._header = stream.readValue<ObjectHeader>();

        // Filepath
        entry._filepath = deserialiseString(stream);

        // Header2
        entry._header2.decodedFileSize = stream.readValue<uint32_t>();

        // Name
        entry._name = deserialiseString(stream);

        // Header3
        entry._displayData = stream.readValue<ObjectHeader3>();

        // ObjectList1
        entry._alsoLoadObjects.resize(stream.readValue<uint32_t>());
        for (auto& alo : entry._alsoLoadObjects)
        {
            alo = stream.readValue<ObjectHeader>();
        }

        // ObjectList2
        entry._requiredObjects.resize(stream.readValue<uint32_t>());
        for (auto& ro : entry._requiredObjects)
        {
            ro = stream.readValue<ObjectHeader>();
        }
        return entry;
    }

    static ObjectFolderState deserialiseFolderState(Stream& stream)
    {
        ObjectFolderState ofs{};
        ofs.numObjects = stream.readValue<uint32_t>();
        ofs.totalFileSize = stream.readValue<uint32_t>();
        ofs.dateHash = stream.readValue<uint32_t>();

        ofs.basePath = deserialiseString(stream);
        return ofs;
    }

    static IndexHeader deserialiseHeader(Stream& stream)
    {
        IndexHeader header{};
        header.version = stream.readValue<uint32_t>();

        header.state.vanillaInstall = deserialiseFolderState(stream);
        header.state.install = deserialiseFolderState(stream);
        header.state.customObjects = deserialiseFolderState(stream);

        header.fileSize = stream.readValue<uint32_t>();
        header.numObjects = stream.readValue<uint32_t>();
        return header;
    }

    static std::vector<ObjectIndexEntry> deserialiseIndex(Stream& stream)
    {
        std::vector<ObjectIndexEntry> entries;
        entries.resize(stream.readValue<uint32_t>());
        for (auto& entry : entries)
        {
            entry = deserialiseEntry(stream);
        }
        return entries;
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
        serialiseIndex(stream, header, _installedObjectList);

        Logging::verbose("Saved object index in {} milliseconds.", saveTimer.elapsed());
    }

    static ObjectIndexEntry createPartialNewEntry(const ObjectHeader& objHeader, const fs::path filepath)
    {
        ObjectIndexEntry entry{};

        // Header
        entry._header = objHeader;

        // Filepath
        entry._filepath = filepath.u8string();

        // Header2 NULL
        entry._header2.decodedFileSize = std::numeric_limits<uint32_t>::max();

        // Name NULL
        entry._name = "";

        // Header3 NULL
        entry._displayData = ObjectHeader3{};

        // ObjectList1
        entry._alsoLoadObjects.clear();

        // ObjectList2
        entry._requiredObjects.clear();

        return entry;
    }

    static ObjectIndexEntry createNewEntry(const ObjectHeader& objHeader, const fs::path filepath, const TempLoadMetaData& metaData)
    {
        ObjectIndexEntry entry{};

        // Header
        entry._header = objHeader;

        // Filepath
        entry._filepath = filepath.u8string();

        // Header2
        entry._header2 = metaData.fileSizeHeader;

        // Name
        entry._name = StringManager::getString(0x2000);

        // Header3
        entry._displayData = metaData.displayData;

        // ObjectList1
        entry._alsoLoadObjects = metaData.dependentObjects.willLoad;

        // ObjectList2
        entry._requiredObjects = metaData.dependentObjects.required;

        return entry;
    }

    // Adds a new object to the index by: 1. creating a partial index, 2. validating, 3. creating a full index entry
    static void addObjectToIndex(const fs::path filepath)
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

        const auto partialNewEntry = createPartialNewEntry(objHeader, filepath);
        _installedObjectList.push_back(partialNewEntry);

        _isPartialLoaded = true;
        const auto loadResult = loadTemporaryObject(objHeader);
        _isPartialLoaded = false;
        _installedObjectList.pop_back();

        if (!loadResult.has_value())
        {
            Logging::error("Unable to load the object '{}', can't add to index", objHeader.getName());
            return;
        }

        // Load full entry into temp buffer.
        // 0x009D1CC8
        const auto newEntry = createNewEntry(objHeader, filepath, loadResult.value());

        freeTemporaryObject();

        auto duplicate = std::ranges::find(_installedObjectList, objHeader, &ObjectIndexEntry::_header);
        if (duplicate != _installedObjectList.end())
        {
            Logging::error("Duplicate object found {}, {} won't be added to index", duplicate->_filepath, newEntry._filepath);
            return;
        }
        _installedObjectList.push_back(newEntry); // Previously ordered by name...
    }

    static void addObjectsInFolder(fs::path path, bool shouldRecurse, uint32_t numObjects)
    {
        uint8_t progress = 0; // Progress is used for the ProgressBar Ui element
        uint32_t i = 0;
        iterateObjectFolder(path, shouldRecurse, [&i, &numObjects, &progress](const fs::directory_entry& file) {
            Ui::processMessagesMini();
            i++;

            // Cheap calculation of (curObjectCount / totalObjectCount) * 256
            const auto newProgress = (i << 8) / ((numObjects & 0xFFFFFF) + 1);
            if (progress != newProgress)
            {
                progress = newProgress;
                Ui::ProgressBar::setProgress(newProgress);
            }

            // For now there are a few places that assume there are int16_t max items
            if (_installedObjectList.size() >= static_cast<size_t>(std::numeric_limits<ObjectIndexId>::max()))
            {
                return false;
            }
            addObjectToIndex(file.path());
            return true;
        });
    }

    // 0x0047118B
    static void createIndex(const ObjectFoldersState& currentState)
    {
        Ui::processMessagesMini();
        const auto progressString = _isFirstTime ? StringIds::starting_for_the_first_time : StringIds::checking_object_files;
        Ui::ProgressBar::begin(progressString);

        // Reset
        reloadAll();
        _installedObjectList.clear();

        // Create new index by iterating all DAT files and processing
        IndexHeader header{};
        header.version = kCurrentIndexVersion;
        const auto vanillaObjectPath = Environment::getPathNoWarning(Environment::PathId::vanillaObjects);
        addObjectsInFolder(vanillaObjectPath, false, currentState.vanillaInstall.numObjects);
        const auto objectPath = Environment::getPathNoWarning(Environment::PathId::objects);
        addObjectsInFolder(objectPath, true, currentState.install.numObjects);
        const auto customObjectPath = Environment::getPathNoWarning(Environment::PathId::customObjects);
        addObjectsInFolder(customObjectPath, true, currentState.customObjects.numObjects);
        std::ranges::sort(_installedObjectList, {}, [](const auto& entry) { return entry._name; });

        // New index creation completed. Reset and save result.
        reloadAll();
        header.fileSize = 0;
        header.numObjects = _installedObjectList.size();
        header.state = currentState;
        saveIndex(header);

        Ui::ProgressBar::end();
    }

    static bool tryLoadIndex(const ObjectFoldersState& currentState)
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
            auto header = deserialiseHeader(stream);
            if (header.version != kCurrentIndexVersion || header.state != currentState)
            {
                return false;
            }
            else
            {
                _installedObjectList = deserialiseIndex(stream);
                if (_installedObjectList.empty())
                {
                    return false;
                }
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
        const auto vanillaObjectPath = Environment::getPathNoWarning(Environment::PathId::vanillaObjects);
        const auto vanillaState = getCurrentObjectFolderState(vanillaObjectPath, false);
        const auto objectPath = Environment::getPathNoWarning(Environment::PathId::objects);
        const auto objectState = getCurrentObjectFolderState(objectPath, true);
        const auto customObjectPath = Environment::getPathNoWarning(Environment::PathId::customObjects);
        const auto customState = getCurrentObjectFolderState(customObjectPath, true);

        const auto currentState = ObjectFoldersState{ vanillaState, objectState, customState };

        if (!tryLoadIndex(currentState))
        {
            createIndex(currentState);
        }

        _customObjectsInIndex = hasCustomObjectsInIndex();
    }

    uint32_t getNumInstalledObjects()
    {
        return _installedObjectList.size();
    }

    std::vector<ObjIndexPair> getAvailableObjects(ObjectType type)
    {
        std::vector<ObjIndexPair> list;

        for (ObjectIndexId i = 0; i < static_cast<int16_t>(_installedObjectList.size()); i++)
        {
            if (_installedObjectList[i]._header.getType() == type)
            {
                list.push_back(ObjIndexPair{ i, _installedObjectList[i] });
            }
        }

        return list;
    }

    static std::optional<ObjIndexPair> internalFindObjectInIndex(const ObjectHeader& objectHeader)
    {
        auto res = std::ranges::find(_installedObjectList, objectHeader, &ObjectIndexEntry::_header);
        if (res == std::end(_installedObjectList))
        {
            return std::nullopt;
        }
        return ObjIndexPair{ static_cast<ObjectIndexId>(std::distance(std::begin(_installedObjectList), res)), *res };
    }

    std::optional<ObjectIndexEntry> findObjectInIndex(const ObjectHeader& objectHeader)
    {
        auto res = internalFindObjectInIndex(objectHeader);
        if (!res.has_value())
        {
            return std::nullopt;
        }
        return res->object;
    }

    const ObjectIndexEntry& getObjectInIndex(ObjectIndexId index)
    {
        return _installedObjectList[index];
    }

    bool isObjectInstalled(const ObjectHeader& objectHeader)
    {
        return findObjectInIndex(objectHeader).has_value();
    }

    // 0x00472AFE
    ObjIndexPair getActiveObject(ObjectType objectType, std::span<SelectedObjectsFlags> objectIndexFlags)
    {
        const auto objects = getAvailableObjects(objectType);

        for (auto& [index, object] : objects)
        {
            if ((objectIndexFlags[index] & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        return { ObjectManager::kNullObjectIndex, ObjectIndexEntry{} };
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

        for (ObjectIndexId i = 0; i < static_cast<int16_t>(_installedObjectList.size()); i++)
        {
            auto& entry = _installedObjectList[i];
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
        for (ObjectIndexId i = 0; i < static_cast<int16_t>(_installedObjectList.size()); i++)
        {
            auto& entry = _installedObjectList[i];
            if ((objectFlags[i] & SelectedObjectsFlags::selected) == SelectedObjectsFlags::none)
            {
                continue;
            }

            selectionMetaData.numSelectedObjects[enumValue(entry._header.getType())]++;
            totalNumImages += entry._displayData.numImages;
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
            buffer = StringManager::formatString(buffer, 512, StringIds::the_following_object_must_be_selected_first);
            objectCreateIdentifierName(buffer, objHeader);
            GameCommands::setErrorText(StringIds::buffer_2040);
            return false;
        }

        // If this object was selected too many images would be needed when loading
        if (entry._displayData.numImages + selectionMetaData.numImages > Gfx::G1ExpectedCount::kObjects)
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

        selectionMetaData.numImages += entry._displayData.numImages;
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

        selectionMetaData.numImages -= entry._displayData.numImages;
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
            buffer = StringManager::formatString(buffer, 512, StringIds::data_for_following_object_not_found);
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

    // 0x00472DA1
    static void markInUseObjectsByTile(std::array<std::span<uint8_t>, kMaxObjectTypes>& loadedObjectFlags)
    {
        // Iterate the whole map looking for things
        for (const auto pos : World::getWorldRange())
        {
            const auto tile = World::TileManager::get(pos);
            for (const auto& el : tile)
            {
                const auto* elSurface = el.as<World::SurfaceElement>();
                const auto* elTrack = el.as<World::TrackElement>();
                const auto* elStation = el.as<World::StationElement>();
                const auto* elSignal = el.as<World::SignalElement>();
                const auto* elBuilding = el.as<World::BuildingElement>();
                const auto* elTree = el.as<World::TreeElement>();
                const auto* elWall = el.as<World::WallElement>();
                const auto* elRoad = el.as<World::RoadElement>();
                const auto* elIndustry = el.as<World::IndustryElement>();

                if (elSurface != nullptr)
                {
                    loadedObjectFlags[enumValue(ObjectType::land)][elSurface->terrain()] |= (1U << 0);
                    if (elSurface->snowCoverage())
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
                            loadedObjectFlags[enumValue(ObjectType::trainStation)][elStation->objectId()] |= (1U << 0);
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
        forEachLoadedObject([&loadedObjectFlags](const LoadedObjectHandle& handle) {
            loadedObjectFlags[enumValue(handle.type)][handle.id] |= (1U << 1);
        });
    }

    static void applyLoadedObjectMarkToIndex(std::span<SelectedObjectsFlags> objectFlags, const std::array<std::span<uint8_t>, kMaxObjectTypes>& loadedObjectFlags)
    {
        for (ObjectIndexId i = 0; i < static_cast<int16_t>(_installedObjectList.size()); i++)
        {
            objectFlags[i] &= ~(SelectedObjectsFlags::inUse | SelectedObjectsFlags::selected);

            auto entry = _installedObjectList[i];

            auto objHandle = findObjectHandle(entry._header);
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

    // 0x00472D3F
    static void markInUseObjects(std::span<SelectedObjectsFlags> objectFlags)
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

        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            loadedObjectFlags[enumValue(ObjectType::region)][0] |= (1U << 0);
            markInUseObjectsByTile(loadedObjectFlags);
        }

        markInUseVehicleObjects(loadedObjectFlags[enumValue(ObjectType::vehicle)]);
        markInUseIndustryObjects(loadedObjectFlags[enumValue(ObjectType::industry)]);
        markInUseCompetitorObjects(loadedObjectFlags[enumValue(ObjectType::competitor)]);

        // Copy results over to objectFlags
        applyLoadedObjectMarkToIndex(objectFlags, loadedObjectFlags);
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

        SelectedObjectsFlags* selectFlags = new SelectedObjectsFlags[_installedObjectList.size()]{};
        _50D144 = selectFlags;
        std::span<SelectedObjectsFlags> objectFlags{ selectFlags, _installedObjectList.size() };
        // throw on nullptr?

        _objectSelectionMeta = ObjectSelectionMeta{};
        _numObjectsPerType = std::array<uint16_t, kMaxObjectTypes>{};

        for (auto& entry : _installedObjectList)
        {
            (*_numObjectsPerType)[enumValue(entry._header.getType())]++;
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

    // 0x00473B91
    void freeSelectionList()
    {
        _50D144refCount--;
        if (_50D144refCount == 0)
        {
            delete[] _50D144;
            _50D144 = nullptr;
        }
    }

    // 0x004BF935
    void markOnlyLoadedObjects(std::span<SelectedObjectsFlags> objectFlags)
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

        // Copy results over to objectFlags
        applyLoadedObjectMarkToIndex(objectFlags, loadedObjectFlags);
    }

    // 0x00474874
    void loadSelectionListObjects(std::span<SelectedObjectsFlags> objectFlags)
    {
        for (auto i = 0U; i < objectFlags.size(); i++)
        {
            if ((objectFlags[i] & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
            {
                auto& header = _installedObjectList[i]._header;
                if (!findObjectHandle(header))
                {
                    load(header);
                }
            }
        }
    }

    // 0x00474821
    void unloadUnselectedSelectionListObjects(std::span<SelectedObjectsFlags> objectFlags)
    {
        for (ObjectType type = ObjectType::interfaceSkin; enumValue(type) <= enumValue(ObjectType::scenarioText); type = static_cast<ObjectType>(enumValue(type) + 1))
        {
            auto objects = getAvailableObjects(type);
            for (auto& [i, object] : objects)
            {
                if ((objectFlags[i] & SelectedObjectsFlags::selected) == SelectedObjectsFlags::none)
                {
                    unload(object._header);
                }
            }
        }
    }

    static bool validateObjectTypeSelection(std::span<SelectedObjectsFlags> objectFlags, const ObjectType type, auto&& predicate)
    {
        for (ObjectIndexId i = 0; i < static_cast<int16_t>(_installedObjectList.size()); i++)
        {
            auto& entry = _installedObjectList[i];
            if (((objectFlags[i] & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
                && type == entry._header.getType()
                && predicate(entry))
            {
                return true;
            }
        }
        return false;
    }

    constexpr std::array<std::pair<ObjectType, StringId>, 15> kValidateTypeAndErrorMessage = {
        std::make_pair(ObjectType::region, StringIds::region_type_must_be_selected),
        std::make_pair(ObjectType::scaffolding, StringIds::scaffolding_type_must_be_selected),
        std::make_pair(ObjectType::industry, StringIds::industry_type_must_be_selected),
        std::make_pair(ObjectType::building, StringIds::town_building_type_must_be_selected),
        std::make_pair(ObjectType::interfaceSkin, StringIds::interface_type_must_be_selected),
        std::make_pair(ObjectType::vehicle, StringIds::vehicle_type_must_be_selected),
        std::make_pair(ObjectType::land, StringIds::land_type_must_be_selected),
        std::make_pair(ObjectType::currency, StringIds::currency_type_must_be_selected),
        std::make_pair(ObjectType::water, StringIds::water_type_must_be_selected),
        std::make_pair(ObjectType::townNames, StringIds::town_name_type_must_be_selected),
        std::make_pair(ObjectType::levelCrossing, StringIds::level_crossing_type_must_be_selected),
        std::make_pair(ObjectType::streetLight, StringIds::street_light_type_must_be_selected),
        std::make_pair(ObjectType::snow, StringIds::snow_type_must_be_selected),
        std::make_pair(ObjectType::climate, StringIds::climate_type_must_be_selected),
        std::make_pair(ObjectType::hillShapes, StringIds::map_generation_type_must_be_selected),
    };

    // 0x00474167
    std::optional<ObjectType> validateObjectSelection(std::span<SelectedObjectsFlags> objectFlags)
    {
        const auto atLeastOneSelected = [](const ObjectIndexEntry&) { return true; };
        // Validate all the simple object types that
        // require at least one item selected of the type
        for (auto& [objectType, errorMessage] : kValidateTypeAndErrorMessage)
        {
            if (!validateObjectTypeSelection(objectFlags, objectType, atLeastOneSelected))
            {
                GameCommands::setErrorText(errorMessage);
                return objectType;
            }
        }

        // Validate the more complex road object type that
        // has more complex logic
        if (!validateObjectTypeSelection(
                objectFlags, ObjectType::road, [](const ObjectIndexEntry& entry) {
                    const auto tempObj = loadTemporaryObject(entry._header);
                    if (!tempObj.has_value())
                    {
                        return false;
                    }
                    auto* roadObj = reinterpret_cast<RoadObject*>(getTemporaryObject());
                    if (!roadObj->hasFlags(RoadObjectFlags::unk_03))
                    {
                        freeTemporaryObject();
                        return false;
                    }
                    if (roadObj->hasFlags(RoadObjectFlags::unk_00))
                    {
                        freeTemporaryObject();
                        return false;
                    }
                    freeTemporaryObject();
                    return true;
                }))
        {
            GameCommands::setErrorText(StringIds::at_least_one_generic_dual_direction_road_type_must_be_selected);
            return ObjectType::road;
        }

        return std::nullopt;
    }
}
