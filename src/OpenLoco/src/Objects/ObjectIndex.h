#pragma once

#include "Object.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Span.hpp>
#include <optional>
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
    ObjIndexPair getActiveObject(ObjectType objectType, stdx::span<SelectedObjectsFlags> objectIndexFlags);
}
