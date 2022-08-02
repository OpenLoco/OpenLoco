#pragma once

#include "../Core/Span.hpp"
#include "Object.h"
#include <vector>

namespace OpenLoco::ObjectManager
{
    struct ObjectIndexEntry
    {
        ObjectHeader* _header;
        char* _filename;
        char* _name;

        static ObjectIndexEntry read(std::byte** ptr);
    };

    struct ObjIndexPair
    {
        int16_t index;
        ObjectIndexEntry object;
    };

    uint32_t getNumInstalledObjects();

    std::vector<std::pair<uint32_t, ObjectIndexEntry>> getAvailableObjects(ObjectType type);
    bool tryInstallObject(const ObjectHeader& object, stdx::span<const uint8_t> data);
    ObjIndexPair getActiveObject(ObjectType objectType, uint8_t* edi);
}
