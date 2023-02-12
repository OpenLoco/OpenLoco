#include "TrackObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x004A6CBA
    void TrackObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x, y, colourImage + 18);
        drawingCtx.drawImage(&rt, x, y, colourImage + 20);
        drawingCtx.drawImage(&rt, x, y, colourImage + 22);
    }

    // 0x004A6C6C
    bool TrackObject::validate() const
    {
        if (var_06 >= 3)
        {
            return false;
        }

        // vanilla missed this check
        if (costIndex > 32)
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
        if (hasPieceFlags(TrackObjectPieceFlags::diagonal | TrackObjectPieceFlags::largeCurve)
            && hasPieceFlags(TrackObjectPieceFlags::oneSided | TrackObjectPieceFlags::verySmallCurve))
        {
            return false;
        }
        if (numBridges > 7)
        {
            return false;
        }
        return numStations <= 7;
    }

    // 0x004A6A5F
    void TrackObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(TrackObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        std::fill(std::begin(mods), std::end(mods), 0xFF);

        // NOTE: These aren't dependent objects as this can load without the
        // related object.
        for (auto i = 0; i < numCompatible; ++i)
        {
            auto& mod = mods[i];
            mod = 0xFF;

            ObjectHeader modHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandle(modHeader);
            if (res.has_value())
            {
                mod = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        //for (size_t i = 0; i < 4; ++i)
        //{
        //    for (size_t j = 0; j < 4; ++j)
        //    {
        //        cargoOffsetBytes[i][j] = reinterpret_cast<const std::byte*>(remainingData.data());

        //        auto* bytes = reinterpret_cast<const int8_t*>(cargoOffsetBytes[i][j]);
        //        bytes++; // z
        //        auto length = 1;
        //        while (*bytes != -1)
        //        {
        //            length += 4; // x, y, x, y
        //            bytes += 4;
        //        }
        //        length += 4;
        //        remainingData = remainingData.subspan(length);
        //    }
        //}

        //for (size_t i = 0; i < sizeof(var_6E) / sizeof(var_6E[0]); ++i)
        //{
        //    var_6E[i] = reinterpret_cast<const std::byte*>(remainingData.data());

        //    auto* bytes = reinterpret_cast<const int8_t*>(var_6E[i]);
        //    bytes++; // z?
        //    auto length = 1;
        //    while (*bytes != -1)
        //    {
        //        length += 4; // x, y, x, y
        //        bytes += 4;
        //    }
        //    length += 4;

        //    remainingData = remainingData.subspan(length);
        //}

        //auto imgRes = ObjectManager::loadImageTable(remainingData);
        //image = imgRes.imageOffset;
        //assert(remainingData.size() == imgRes.tableLength);

        //auto imageOffset = image + TrainStation::ImageIds::totalPreviewImages;
        //for (size_t i = 0; i < sizeof(var_12) / sizeof(var_12[0]); ++i)
        //{
        //    var_12[i] = imageOffset;
        //    imageOffset += kDrawStyleTotalNumImages[drawStyle];
        //}
    }

    // 0x004A6C2D
    void TrackObject::unload()
    {
        name = 0;
        var_10 = 0;
        std::fill(std::begin(mods), std::end(mods), 0);
        var_0E = 0;
        var_1B = 0;
        image = 0;
        std::fill(std::begin(bridges), std::end(bridges), 0);
        std::fill(std::begin(stations), std::end(stations), 0);
    }
}
