#include "RoadObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00477DFE
    void RoadObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        if (paintStyle == 1)
        {
            drawingCtx.drawImage(&rt, x, y, colourImage + 34);
            drawingCtx.drawImage(&rt, x, y, colourImage + 36);
            drawingCtx.drawImage(&rt, x, y, colourImage + 38);
        }
        else
        {
            drawingCtx.drawImage(&rt, x, y, colourImage + 34);
        }
    }

    // 0x00477DBC
    bool RoadObject::validate() const
    {
        // check missing in vanilla
        if (costIndex >= 32)
        {
            return false;
        }
        if (-sellCostFactor > buildCostFactor)
        {
            return false;
        }
        if (buildCostFactor <= 0)
        {
            return false;
        }
        if (tunnelCostFactor <= 0)
        {
            return false;
        }
        if (numBridges > 7)
        {
            return false;
        }
        if (numMods > 2)
        {
            return false;
        }
        if (hasFlags(RoadObjectFlags::unk_03))
        {
            return numMods == 0;
        }
        return true;
    }

    // 0x00477BCF
    void RoadObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(RoadObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // NOTE: These aren't dependent (objects unless otherwise stated) as this object can load without the
        // related object.
        // Load compatible roads/tracks
        compatibleTracks = 0;
        compatibleRoads = 0;
        for (auto i = 0; i < numCompatible; ++i)
        {
            ObjectHeader modHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandleFuzzy(modHeader);
            if (res.has_value())
            {
                if (res->type == ObjectType::track)
                {
                    compatibleTracks |= 1U << res->id;
                }
                else if (res->type == ObjectType::road)
                {
                    compatibleRoads |= 1U << res->id;
                }
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load Extra
        std::fill(std::begin(mods), std::end(mods), 0xFF);
        for (auto i = 0U, index = 0U; i < numMods; ++i)
        {
            ObjectHeader modHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandle(modHeader);
            if (res.has_value() && res->type == ObjectType::roadExtra)
            {
                mods[index++] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load Tunnel (DEPENDENT OBJECT)
        tunnel = 0xFF;
        {
            ObjectHeader unkHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(unkHeader);
            }
            auto res = ObjectManager::findObjectHandle(unkHeader);
            if (res.has_value())
            {
                tunnel = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load bridges (DEPENDENT OBJECT)
        std::fill(std::begin(bridges), std::end(bridges), 0xFF);
        for (auto i = 0U; i < numBridges; ++i)
        {
            ObjectHeader bridgeHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(bridgeHeader);
            }
            auto res = ObjectManager::findObjectHandle(bridgeHeader);
            if (res.has_value())
            {
                bridges[i] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load stations (DEPENDENT OBJECT)
        std::fill(std::begin(stations), std::end(stations), 0xFF);
        for (auto i = 0U; i < numStations; ++i)
        {
            ObjectHeader stationHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(stationHeader);
            }
            auto res = ObjectManager::findObjectHandle(stationHeader);
            if (res.has_value())
            {
                stations[i] = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x00477D9E
    void RoadObject::unload()
    {
        name = 0;
        tunnel = 0;
        image = 0;
        std::fill(std::begin(bridges), std::end(bridges), 0);
        std::fill(std::begin(mods), std::end(mods), 0);
        std::fill(std::begin(stations), std::end(stations), 0);
        compatibleTracks = 0;
        compatibleRoads = 0;
    }
}
