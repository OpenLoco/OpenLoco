#pragma once

#include "Object.h"
#include <OpenLoco/Core/Span.hpp>
#include <optional>
#include <vector>

namespace OpenLoco::ObjectManager
{
    struct ObjectHeader3;

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
    ObjIndexPair getActiveObject(ObjectType objectType, uint8_t* edi);
}
