#pragma once

#include "Object.h"
#include "ObjectManager.h" // TODO: Split off entry def to different header
#include <OpenLoco/Core/EnumFlags.hpp>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace OpenLoco::ObjectManager
{
    struct ObjectHeader3;

    enum class SelectedObjectsFlags : uint8_t
    {
        none = 0,

        selected = 1U << 0,
        inUse = 1U << 2,
        requiredByAnother = 1U << 3,
        alwaysRequired = 1U << 4, // Unused (from RCT2)
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SelectedObjectsFlags);

    struct ObjectIndexEntry
    {
        ObjectHeader _header;
        ObjectHeader2 _header2;
        ObjectHeader3 _displayData;
        std::string _filepath; // u8string
        std::string _name;
        std::vector<ObjectHeader> _requiredObjects;
        std::vector<ObjectHeader> _alsoLoadObjects;
    };

    // Index into the overall ObjectIndex. Note: Not a type specific index!
    using ObjectIndexId = int16_t;
    constexpr ObjectIndexId kNullObjectIndex = -1;

    struct ObjIndexPair
    {
        ObjectIndexId index;
        ObjectIndexEntry object;
    };

    uint32_t getNumInstalledObjects();

    void loadIndex();

    std::vector<ObjIndexPair> getAvailableObjects(ObjectType type);
    bool isObjectInstalled(const ObjectHeader& objectHeader);
    std::optional<ObjectIndexEntry> findObjectInIndex(const ObjectHeader& objectHeader);
    const ObjectIndexEntry& getObjectInIndex(ObjectIndexId index);
    ObjIndexPair getActiveObject(ObjectType objectType, std::span<SelectedObjectsFlags> objectIndexFlags);

#pragma pack(push, 1)
    struct ObjectSelectionMeta
    {
        std::array<uint16_t, kMaxObjectTypes> numSelectedObjects;
        uint32_t numImages;
    };
    static_assert(sizeof(ObjectSelectionMeta) == 0x48);
#pragma pack(pop)

    enum class SelectObjectModes : uint8_t
    {
        none = 0,
        select = 1U << 0,               // When not set we are in deselection mode
        selectDependents = 1U << 1,     // Always set
        selectAlsoLoads = 1U << 2,      // Always set
        markAsAlwaysRequired = 1U << 3, // Unused (from RCT2)

        defaultDeselect = selectDependents | selectAlsoLoads,
        defaultSelect = defaultDeselect | select,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(SelectObjectModes);
    bool selectObjectFromIndex(SelectObjectModes mode, const ObjectHeader& objHeader, std::span<SelectedObjectsFlags> objectFlags, ObjectSelectionMeta& selectionMetaData);
    void prepareSelectionList(bool markInUse);
    void freeSelectionList();
    void markOnlyLoadedObjects(std::span<SelectedObjectsFlags> objectFlags);
    void loadSelectionListObjects(std::span<SelectedObjectsFlags> objectFlags);
    void unloadUnselectedSelectionListObjects(std::span<SelectedObjectsFlags> objectFlags);
    std::optional<ObjectType> validateObjectSelection(std::span<SelectedObjectsFlags> objectFlags);
}
