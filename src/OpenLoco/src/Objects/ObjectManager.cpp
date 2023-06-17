#include "ObjectManager.h"
#include "AirportObject.h"
#include "BridgeObject.h"
#include "BuildingObject.h"
#include "CargoObject.h"
#include "CliffEdgeObject.h"
#include "ClimateObject.h"
#include "CompetitorObject.h"
#include "CurrencyObject.h"
#include "DockObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Environment.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "HillShapesObject.h"
#include "IndustryObject.h"
#include "InterfaceSkinObject.h"
#include "LandObject.h"
#include "LevelCrossingObject.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "ObjectImageTable.h"
#include "ObjectIndex.h"
#include "ObjectStringTable.h"
#include "RegionObject.h"
#include "RoadExtraObject.h"
#include "RoadObject.h"
#include "RoadStationObject.h"
#include "S5/SawyerStream.h"
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
#include "Ui.h"
#include "Ui/ProgressBar.h"
#include "VehicleObject.h"
#include "WallObject.h"
#include "WaterObject.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Core/Numeric.hpp>
#include <OpenLoco/Core/Stream.hpp>
#include <OpenLoco/Core/Traits.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <vector>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Diagnostics;

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
        ObjectEntry2* objectEntryExtendeds;
    };
    assert_struct_size(ObjectRepositoryItem, 8);
#pragma pack(pop)

    static_assert(Traits::IsPOD<ObjectHeader>::value, "Object Header must be trivial for I/O purposes");

    loco_global<ObjectRepositoryItem[maxObjectTypes], 0x4FE0B8> _objectRepository;

    static loco_global<std::byte*, 0x0050D158> _dependentObjectsVector;
    static loco_global<std::byte[0x2002], 0x0112A17F> _dependentObjectVectorData;
    static loco_global<bool, 0x0050AEAD> _isFirstTime;
    static loco_global<bool, 0x0050D161> _isPartialLoaded;
    static loco_global<uint8_t, 0x0050D160> _isTemporaryObject; // 0xFF or 0
    static loco_global<Object*, 0x0050D15C> _temporaryObject;
    static loco_global<uint32_t, 0x009D9D52> _decodedSize;    // return of loadTemporaryObject (badly named)
    static loco_global<uint32_t, 0x0112A168> _numImages;      // return of loadTemporaryObject (badly named)
    static loco_global<uint8_t, 0x0112C211> _intelligence;    // return of loadTemporaryObject (badly named)
    static loco_global<uint8_t, 0x0112C212> _aggressiveness;  // return of loadTemporaryObject (badly named)
    static loco_global<uint8_t, 0x0112C213> _competitiveness; // return of loadTemporaryObject (badly named)

    static ObjectRepositoryItem& getRepositoryItem(ObjectType type)
    {
        return _objectRepository[enumValue(type)];
    }

    ObjectHeader& getHeader(const LoadedObjectHandle& handle)
    {
        return getRepositoryItem(handle.type).objectEntryExtendeds[handle.id];
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
        memcpy(name, data.name, 8);
        printf("'%s', ", name);

        printf("%08X ", data.checksum);
    }
    */

    // 0x004720EB
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectHandle> findObjectHandle(const ObjectHeader& header)
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
                    const auto& objHeader = typedObjectList.objectEntryExtendeds[i];

                    if (header == objHeader) // PERHAPS WRONG WAY ROUND
                    {
                        return { LoadedObjectHandle{ objectType, i } };
                    }
                }
            }
        }
        return std::nullopt;
    }

    // 0x0047206C
    // Returns std::nullopt if not loaded
    std::optional<LoadedObjectHandle> findObjectHandleFuzzy(const ObjectHeader& header)
    {
        auto res = findObjectHandle(header);
        if (res.has_value())
        {
            return res;
        }

        auto objectType = header.getType();
        const auto& typedObjectList = getRepositoryItem(objectType);
        auto maxObjectsForType = getMaxObjects(objectType);
        for (LoadedObjectId i = 0; i < maxObjectsForType; i++)
        {
            auto obj = typedObjectList.objects[i];
            if (obj == nullptr || obj != reinterpret_cast<Object*>(-1))
            {
                continue;
            }
            const auto& objHeader = typedObjectList.objectEntryExtendeds[i];

            if (!objHeader.isCustom())
            {
                continue;
            }
            if (header.getType() != objHeader.getType())
            {
                continue;
            }
            if (header.getName() != objHeader.getName())
            {
                continue;
            }
            if (header.getFuzzyFlags() != objHeader.getFuzzyFlags())
            {
                continue;
            }

            return { LoadedObjectHandle{ objectType, i } };
        }

        return std::nullopt;
    }

    enum class ObjectProcedure
    {
        load,
        unload,
        validate,
        drawPreview,
    };

    template<typename TVisitor>
    static auto visitObject(const ObjectType type, Object& obj, TVisitor&& visitor)
    {
        switch (type)
        {
            case ObjectType::interfaceSkin:
                return visitor(reinterpret_cast<InterfaceSkinObject*>(&obj));
            case ObjectType::sound:
                return visitor(reinterpret_cast<SoundObject*>(&obj));
            case ObjectType::currency:
                return visitor(reinterpret_cast<CurrencyObject*>(&obj));
            case ObjectType::steam:
                return visitor(reinterpret_cast<SteamObject*>(&obj));
            case ObjectType::cliffEdge:
                return visitor(reinterpret_cast<CliffEdgeObject*>(&obj));
            case ObjectType::water:
                return visitor(reinterpret_cast<WaterObject*>(&obj));
            case ObjectType::land:
                return visitor(reinterpret_cast<LandObject*>(&obj));
            case ObjectType::townNames:
                return visitor(reinterpret_cast<TownNamesObject*>(&obj));
            case ObjectType::cargo:
                return visitor(reinterpret_cast<CargoObject*>(&obj));
            case ObjectType::wall:
                return visitor(reinterpret_cast<WallObject*>(&obj));
            case ObjectType::trackSignal:
                return visitor(reinterpret_cast<TrainSignalObject*>(&obj));
            case ObjectType::levelCrossing:
                return visitor(reinterpret_cast<LevelCrossingObject*>(&obj));
            case ObjectType::streetLight:
                return visitor(reinterpret_cast<StreetLightObject*>(&obj));
            case ObjectType::tunnel:
                return visitor(reinterpret_cast<TunnelObject*>(&obj));
            case ObjectType::bridge:
                return visitor(reinterpret_cast<BridgeObject*>(&obj));
            case ObjectType::trackStation:
                return visitor(reinterpret_cast<TrainStationObject*>(&obj));
            case ObjectType::trackExtra:
                return visitor(reinterpret_cast<TrackExtraObject*>(&obj));
            case ObjectType::track:
                return visitor(reinterpret_cast<TrackObject*>(&obj));
            case ObjectType::roadStation:
                return visitor(reinterpret_cast<RoadStationObject*>(&obj));
            case ObjectType::roadExtra:
                return visitor(reinterpret_cast<RoadExtraObject*>(&obj));
            case ObjectType::road:
                return visitor(reinterpret_cast<RoadObject*>(&obj));
            case ObjectType::airport:
                return visitor(reinterpret_cast<AirportObject*>(&obj));
            case ObjectType::dock:
                return visitor(reinterpret_cast<DockObject*>(&obj));
            case ObjectType::vehicle:
                return visitor(reinterpret_cast<VehicleObject*>(&obj));
            case ObjectType::tree:
                return visitor(reinterpret_cast<TreeObject*>(&obj));
            case ObjectType::snow:
                return visitor(reinterpret_cast<SnowObject*>(&obj));
            case ObjectType::climate:
                return visitor(reinterpret_cast<ClimateObject*>(&obj));
            case ObjectType::hillShapes:
                return visitor(reinterpret_cast<HillShapesObject*>(&obj));
            case ObjectType::building:
                return visitor(reinterpret_cast<BuildingObject*>(&obj));
            case ObjectType::scaffolding:
                return visitor(reinterpret_cast<ScaffoldingObject*>(&obj));
            case ObjectType::industry:
                return visitor(reinterpret_cast<IndustryObject*>(&obj));
            case ObjectType::region:
                return visitor(reinterpret_cast<RegionObject*>(&obj));
            case ObjectType::competitor:
                return visitor(reinterpret_cast<CompetitorObject*>(&obj));
            case ObjectType::scenarioText:
                return visitor(reinterpret_cast<ScenarioTextObject*>(&obj));
        }
#if defined(_MSC_VER) // MSVC
        __assume(false);
#elif defined(__GNUC__) // GCC, Clang, ICC
        __builtin_unreachable();
#endif
    }

    static bool callObjectValidate(const ObjectType type, Object& obj)
    {
        return visitObject(type, obj, [](auto&& obj) {
            return obj->validate();
        });
    }

    static void callObjectUnload(const ObjectType type, Object& obj)
    {
        return visitObject(type, obj, [](auto&& obj) {
            return obj->unload();
        });
    }

    static void callObjectLoad(const LoadedObjectHandle& handle, Object& obj, stdx::span<const std::byte> data, DependentObjects* dependencies = nullptr)
    {
        return visitObject(handle.type, obj, [&](auto&& obj) {
            return obj->load(handle, data, dependencies);
        });
    }

    // 0x0047237D
    void reloadAll()
    {
        setTotalNumImages(0x201A); // TODO: Why this value?
        for (auto type = ObjectType::interfaceSkin; enumValue(type) <= enumValue(ObjectType::scenarioText); type = static_cast<ObjectType>(enumValue(type) + 1))
        {
            for (LoadedObjectId id = 0; id < getMaxObjects(type); id++)
            {
                LoadedObjectHandle handle{ type, id };
                auto* obj = getAny(handle);
                if (obj != nullptr)
                {
                    auto& extHdr = getRepositoryItem(type).objectEntryExtendeds[id];
                    callObjectLoad(handle, *obj, stdx::span<const std::byte>(reinterpret_cast<std::byte*>(obj), extHdr.dataSize));
                }
            }
        }
    }

    // 0x00472754
    static uint32_t computeChecksum(stdx::span<const std::byte> data, uint32_t seed)
    {
        auto checksum = seed;
        for (auto d : data)
        {
            checksum = Numerics::rol(checksum ^ static_cast<uint8_t>(d), 11);
        }
        return checksum;
    }

    // 0x0047270B
    static bool computeObjectChecksum(const ObjectHeader& object, stdx::span<const std::byte> data)
    {
        // Compute the checksum of header and data

        // Annoyingly the header you need to only compute the first byte of the flags
        const auto headerFlag = stdx::span(reinterpret_cast<const std::byte*>(&object), 1);
        auto checksum = computeChecksum(headerFlag, objectChecksumMagic);

        // And then the name
        const auto headerName = stdx::span(reinterpret_cast<const std::byte*>(&object.name), sizeof(ObjectHeader::name));
        checksum = computeChecksum(headerName, checksum);

        // Finally compute the datas checksum
        checksum = computeChecksum(data, checksum);

        return checksum == object.checksum;
    }

    // 0x00471B95
    void freeTemporaryObject()
    {
        if (_temporaryObject != nullptr && _temporaryObject != reinterpret_cast<Object*>(-1))
        {
            free(_temporaryObject);
            // For vanilla compatibility set as -1. Replace with nullptr when all users of temporaryObject implemented.
            _temporaryObject = reinterpret_cast<Object*>(-1);
        }
    }

    struct PreLoadedObject
    {
        stdx::span<std::byte> objectData;
        Object* object; // Owning pointer!
        ObjectHeader header;
    };

    static std::optional<PreLoadedObject> findAndPreLoadObject(const ObjectHeader& header)
    {
        auto installedObject = findObjectInIndex(header);
        if (!installedObject.has_value())
        {
            return std::nullopt;
        }

        const auto filePath = Environment::getPath(Environment::PathId::objects) / fs::u8path(installedObject->_filename);

        FileStream fs(filePath, StreamMode::read);
        SawyerStreamReader stream(fs);
        PreLoadedObject preLoadObj{};
        stream.read(&preLoadObj.header, sizeof(preLoadObj.header));
        if (preLoadObj.header != header)
        {
            // Something wrong has happened and installed object does not match index
            // Vanilla continued to search for subsequent matching installed headers.
            Logging::error("Missmatch between installed object header and object file header!");
            return std::nullopt;
        }

        // Vanilla would branch and perform more efficient readChunk if size was known from installedObject.ObjectHeader2
        auto data = stream.readChunk();

        if (!computeObjectChecksum(preLoadObj.header, data))
        {
            // Something wrong has happened and installed object checksum is broken
            Logging::error("Missmatch between installed object header checksum and object file checksum!");
            return std::nullopt;
        }

        // Copy the object into Loco freeable memory (required for when load loads the object)
        preLoadObj.object = reinterpret_cast<Object*>(malloc(data.size()));
        if (preLoadObj.object == nullptr)
        {
            return std::nullopt;
        }
        std::copy(std::begin(data), std::end(data), reinterpret_cast<std::byte*>(preLoadObj.object));

        preLoadObj.objectData = stdx::span<std::byte>(reinterpret_cast<std::byte*>(preLoadObj.object), data.size());

        if (!callObjectValidate(preLoadObj.header.getType(), *preLoadObj.object))
        {
            free(preLoadObj.object);
            // Object failed validation
            Logging::error("Object {} in index failed validation! (This should not be possible)", header.getName());
            return std::nullopt;
        }

        _decodedSize = preLoadObj.objectData.size();

        return preLoadObj;
    }

    // 0x0047176D
    // TODO: Return a std::unique_ptr and a ObjectHeader3 & ObjectHeader2 for the metadata
    bool loadTemporaryObject(ObjectHeader& header)
    {
        auto preLoadObj = findAndPreLoadObject(header);
        if (!preLoadObj.has_value())
        {
            return false;
        }

        const uint32_t oldNumImages = getTotalNumImages();
        setTotalNumImages(Gfx::G1ExpectedCount::kDisc);
        _temporaryObject = preLoadObj->object;
        _isPartialLoaded = true;
        _isTemporaryObject = 0xFF;
        auto* depObjs = Interop::addr<0x0050D158, uint8_t*>();
        DependentObjects dependencies;
        callObjectLoad({ preLoadObj->header.getType(), 0 }, *preLoadObj->object, preLoadObj->objectData, depObjs != reinterpret_cast<uint8_t*>(0xFFFFFFFF) ? &dependencies : nullptr);
        if (depObjs != reinterpret_cast<uint8_t*>(0xFFFFFFFF))
        {
            *depObjs++ = static_cast<uint8_t>(dependencies.required.size());
            if (!dependencies.required.empty())
            {
                std::copy(dependencies.required.begin(), dependencies.required.end(), reinterpret_cast<ObjectHeader*>(depObjs));
                depObjs += sizeof(ObjectHeader) * dependencies.required.size();
            }
            *depObjs++ = static_cast<uint8_t>(dependencies.willLoad.size());
            if (!dependencies.willLoad.empty())
            {
                std::copy(dependencies.willLoad.begin(), dependencies.willLoad.end(), reinterpret_cast<ObjectHeader*>(depObjs));
            }
        }
        _isTemporaryObject = 0;
        _isPartialLoaded = false;

        _numImages = getTotalNumImages() - Gfx::G1ExpectedCount::kDisc;
        setTotalNumImages(oldNumImages);
        return true;
    }

    Object* getTemporaryObject()
    {
        Object* obj = _temporaryObject;
        if (obj == reinterpret_cast<Object*>(-1))
        {
            return nullptr;
        }
        return obj;
    }

    // TODO: Pass this through other means to users
    bool isTemporaryObjectLoad()
    {
        return _isTemporaryObject == 0xFF;
    }

    // 0x00471BC5
    static bool load(const ObjectHeader& header, LoadedObjectId id)
    {
        auto preLoadObj = findAndPreLoadObject(header);
        if (!preLoadObj.has_value())
        {
            return false;
        }

        if (getTotalNumImages() >= Gfx::G1ExpectedCount::kObjects + Gfx::G1ExpectedCount::kDisc)
        {
            free(preLoadObj->object);
            // Too many objects loaded and no free image space
            return false;
        }

        _objectRepository[enumValue(preLoadObj->header.getType())].objects[id] = preLoadObj->object;
        auto& extendedHeader = _objectRepository[enumValue(preLoadObj->header.getType())].objectEntryExtendeds[id];
        extendedHeader = ObjectEntry2{
            preLoadObj->header, preLoadObj->objectData.size()
        };

        if (!*_isPartialLoaded)
        {
            callObjectLoad({ preLoadObj->header.getType(), id }, *preLoadObj->object, preLoadObj->objectData);
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
        auto handle = findObjectHandle(header);
        if (!handle)
        {
            return;
        }
        unload(*handle);
        free(_objectRepository[enumValue(handle->type)].objects[handle->id]);
        _objectRepository[enumValue(handle->type)].objects[handle->id] = reinterpret_cast<Object*>(-1);
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
                result.problemObjects.push_back(header);
                std::string str(header.getName());
                Logging::error("Failed to load: {}", str);
                // Could break early here but we want to list all of the failed objects
            }
            index++;
        }
        if (result.success == false)
        {
            unloadAll();
        }
        return result;
    }

    static bool partialLoad(const ObjectHeader& header, stdx::span<std::byte> objectData)
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
        getRepositoryItem(type).objectEntryExtendeds[index] = ObjectEntry2(header, objectData.size());
        return true;
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

    // All object files are based on their internal object header name but
    // there is a chance of a name collision this function works out if the name
    // is possible and if not permutates the name until it is valid.
    static fs::path findObjectPath(std::string& filename)
    {
        auto objPath = Environment::getPath(Environment::PathId::objects) / (filename + ".DAT");
        while (fs::exists(objPath))
        {
            permutateObjectFilename(filename);
            objPath.replace_filename(filename + ".DAT");
        }
        return objPath;
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
        const auto objPath = findObjectPath(filename);

        // Create new file and output object file
        Ui::ProgressBar::setProgress(180);
        FileStream fs(objPath, StreamMode::write);
        SawyerStreamWriter stream(fs);
        writePackedObjects(stream, { objectHeader });

        Ui::ProgressBar::setProgress(240);
        Ui::ProgressBar::setProgress(255);
        Ui::ProgressBar::end();
        return true;
    }

    // 0x00472687 based on
    bool tryInstallObject(const ObjectHeader& objectHeader, stdx::span<const std::byte> data)
    {
        unloadAll();
        if (!computeObjectChecksum(objectHeader, data))
        {
            return false;
        }
        // Copy the object into Loco freeable memory (required for when partialLoad loads the object)
        std::byte* objectData = static_cast<std::byte*>(malloc(data.size()));
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

        if (getTotalNumImages() >= Gfx::G1ExpectedCount::kObjects + Gfx::G1ExpectedCount::kDisc)
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
            auto handle = ObjectManager::findObjectHandle(header);
            if (handle)
            {
                // Unload the object so that the object data is restored to
                // its original file state
                ObjectManager::unload(*handle);

                auto encodingType = getBestEncodingForObjectType(header.getType());
                auto obj = ObjectManager::getAny(*handle);
                auto objSize = ObjectManager::getByteLength(*handle);

                fs.write(header);
                fs.writeChunk(encodingType, obj, objSize);
            }
            else
            {
                throw std::runtime_error("Unable to pack object: object not loaded");
            }
        }
    }

    // 0x00472031
    void unloadAll()
    {
        call(0x00472031);
    }

    void unload(const LoadedObjectHandle& handle)
    {
        auto* obj = getAny(handle);
        if (obj != nullptr)
        {
            callObjectUnload(handle.type, *obj);
        }
        else
        {
            throw std::runtime_error("Object not loaded at this index");
        }
    }

    size_t getByteLength(const LoadedObjectHandle& handle)
    {
        return getRepositoryItem(handle.type).objectEntryExtendeds[handle.id].dataSize;
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
                entries.push_back(kEmptyObjectHeader);
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
                    entries.push_back(kEmptyObjectHeader);
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

        addAllInUseHeadersOfTypes<InterfaceSkinObject, SoundObject, CurrencyObject, SteamObject, CliffEdgeObject, WaterObject, LandObject, TownNamesObject, CargoObject, WallObject, TrainSignalObject, LevelCrossingObject, StreetLightObject, TunnelObject, BridgeObject, TrainStationObject, TrackExtraObject, TrackObject, RoadStationObject, RoadExtraObject, RoadObject, AirportObject, DockObject, VehicleObject, TreeObject, SnowObject, ClimateObject, HillShapesObject, BuildingObject, ScaffoldingObject, IndustryObject, RegionObject, CompetitorObject, ScenarioTextObject>(entries);

        return entries;
    }

    // TODO: Should only be defined in ObjectSelectionWindow
    static constexpr uint8_t kDescriptionRowHeight = 10;

    void drawGenericDescription(Gfx::RenderTarget& rt, Ui::Point& rowPosition, const uint16_t designed, const uint16_t obsolete)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        if (designed != 0)
        {
            FormatArguments args{};
            args.push(designed);
            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_designed, &args);
            rowPosition.y += kDescriptionRowHeight;
        }

        if (obsolete != 0xFFFF)
        {
            FormatArguments args{};
            args.push(obsolete);
            drawingCtx.drawStringLeft(rt, rowPosition.x, rowPosition.y, Colour::black, StringIds::object_selection_obsolete, &args);
            rowPosition.y += kDescriptionRowHeight;
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

    // 0x004748FA
    void sub_4748FA()
    {
        call(0x004697A1);
        call(0x0047D9F2);
        call(0x004C57A6);
        call(0x00469F90);
    }

    void registerHooks()
    {
        registerHook(
            0x00472172,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                uint8_t index = regs.edx;
                LoadedObjectHandle handle = { static_cast<ObjectType>(regs.ecx), static_cast<LoadedObjectId>(regs.ebx) };

                // 0x2000 chosen as a large number
                stdx::span<const std::byte> data(static_cast<const std::byte*>(X86Pointer<const std::byte>(regs.ebp)), 0x2000);
                auto res = ObjectManager::loadStringTable(data, handle, index);

                regs = backup;
                regs.ebp += res.tableLength;
                regs.eax = res.str;
                return 0;
            });

        registerHook(
            0x0047221F,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                // 0x20000 chosen as a large number
                stdx::span<const std::byte> data(static_cast<const std::byte*>(X86Pointer<const std::byte>(regs.ebp)), 0x20000);
                auto res = ObjectManager::loadImageTable(data);

                regs = backup;
                regs.ebp += res.tableLength;
                regs.eax = res.imageOffset;
                return 0;
            });
    }
}
