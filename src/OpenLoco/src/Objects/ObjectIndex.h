#pragma once

#include "Object.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <array>
#include <optional>
#include <span>
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
        ObjectHeader* _header;
        ObjectHeader3* _displayData;
        char* _filename;
        char* _name;
        std::span<ObjectHeader> _requiredObjects;
        std::span<ObjectHeader> _alsoLoadObjects;

        static ObjectIndexEntry read(std::byte** ptr);
    };

    struct ObjIndexPair
    {
        int16_t index;
        ObjectIndexEntry object;
    };

    // Index into the overall ObjectIndex. Note: Not a type specific index!
    using ObjectIndexId = uint32_t;

    uint32_t getNumInstalledObjects();

    void loadIndex();

    std::vector<std::pair<ObjectIndexId, ObjectIndexEntry>> getAvailableObjects(ObjectType type);
    bool isObjectInstalled(const ObjectHeader& objectHeader);
    std::optional<ObjectIndexEntry> findObjectInIndex(const ObjectHeader& objectHeader);
    ObjIndexPair getActiveObject(ObjectType objectType, std::span<SelectedObjectsFlags> objectIndexFlags);
    struct ObjectSelectionMeta
    {
        uint32_t numImages;
        std::array<uint16_t, kMaxObjectTypes> numSelectedObjects;
    };
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
}
