#include "TrainStationObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

namespace OpenLoco
{
    // 0x00490A26
    void TrainStationObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 34, y - 34, colourImage);

        auto colour = ExtColour::translucentMutedDarkRed1;
        if (!hasFlags(TrainStationFlags::recolourable))
        {
            colour = ExtColour::unk2E;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        drawingCtx.drawImage(&rt, x - 34, y - 34, translucentImage);
    }

    // 0x00490A68
    void TrainStationObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x004909F3
    bool TrainStationObject::validate() const
    {
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
        if (drawStyle >= 1)
        {
            return false;
        }
        return numCompatible <= 7;
    }

    constexpr std::array<uint32_t, 2> kDrawStyleTotalNumImages = {
        TrainStation::ImageIds::Style0::totalNumImages,
        TrainStation::ImageIds::Style1::totalNumImages,
    };

    // 0x004908F1
    void TrainStationObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(TrainStationObject));

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

        for (size_t i = 0; i < 4; ++i)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                cargoOffsetBytes[i][j] = reinterpret_cast<const std::byte*>(remainingData.data());

                auto* bytes = reinterpret_cast<const int8_t*>(cargoOffsetBytes[i][j]);
                bytes++; // z
                auto length = 1;
                while (*bytes != -1)
                {
                    length += 4; // x, y, x, y
                    bytes += 4;
                }
                length += 4;
                remainingData = remainingData.subspan(length);
            }
        }

        for (size_t i = 0; i < sizeof(manualPower) / sizeof(manualPower[0]); ++i)
        {
            manualPower[i] = reinterpret_cast<const std::byte*>(remainingData.data());

            auto* bytes = reinterpret_cast<const int8_t*>(manualPower[i]);
            bytes++; // z?
            auto length = 1;
            while (*bytes != -1)
            {
                length += 4; // x, y, x, y
                bytes += 4;
            }
            length += 4;

            remainingData = remainingData.subspan(length);
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);

        auto imageOffset = image + TrainStation::ImageIds::totalPreviewImages;
        for (size_t i = 0; i < sizeof(var_12) / sizeof(var_12[0]); ++i)
        {
            var_12[i] = imageOffset;
            imageOffset += kDrawStyleTotalNumImages[drawStyle];
        }
    }

    // 0x004909A5
    void TrainStationObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(var_12), std::end(var_12), 0);
        std::fill(std::begin(mods), std::end(mods), 0);
        std::fill(&cargoOffsetBytes[0][0], &cargoOffsetBytes[0][0] + sizeof(cargoOffsetBytes) / sizeof(std::byte*), nullptr);
        std::fill(std::begin(manualPower), std::end(manualPower), nullptr);
    }

    std::vector<TrainStationObject::CargoOffset> TrainStationObject::getCargoOffsets(const uint8_t rotation, const uint8_t nibble) const
    {
        assert(rotation < 4 && nibble < 4);

        const auto* bytes = cargoOffsetBytes[rotation][nibble];
        uint8_t z = *reinterpret_cast<const uint8_t*>(bytes);
        bytes++;
        std::vector<CargoOffset> result;
        while (*bytes != static_cast<std::byte>(0xFF))
        {
            result.push_back({
                World::Pos3{
                    *reinterpret_cast<const int8_t*>(bytes),
                    *reinterpret_cast<const int8_t*>(bytes + 1),
                    z,
                },
                World::Pos3{
                    *reinterpret_cast<const int8_t*>(bytes + 2),
                    *reinterpret_cast<const int8_t*>(bytes + 3),
                    z,
                },
            });
            bytes += 4;
        }
        return result;
    }
}
