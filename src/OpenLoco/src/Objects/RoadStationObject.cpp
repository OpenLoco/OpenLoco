#include "RoadStationObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00490C17
    void RoadStationObject::drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        drawingCtx.drawImage(x - 34, y - 34, colourImage);

        auto colour = ExtColour::translucentMutedDarkRed1;
        if (!hasFlags(RoadStationFlags::recolourable))
        {
            colour = ExtColour::unk2E;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        drawingCtx.drawImage(x - 34, y - 34, translucentImage);
    }

    // 0x00490C59
    void RoadStationObject::drawDescription(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(drawingCtx, rowPosition, designedYear, obsoleteYear);
    }

    // 0x00490BD8
    bool RoadStationObject::validate() const
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
        if (paintStyle >= 1)
        {
            return false;
        }
        if (numCompatible > 7)
        {
            return false;
        }
        if (hasFlags(RoadStationFlags::passenger) && hasFlags(RoadStationFlags::freight))
        {
            return false;
        }
        return true;
    }

    constexpr std::array<uint32_t, 1> kDrawStyleTotalNumImages = {
        RoadStation::ImageIds::Style0::totalNumImages,
    };

    // 0x00490ABE
    void RoadStationObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(RoadStationObject));

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

        if (hasFlags(RoadStationFlags::passenger | RoadStationFlags::freight))
        {
            ObjectHeader cargoHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            auto res = ObjectManager::findObjectHandle(cargoHeader);
            if (res.has_value())
            {
                cargoType = res->id;
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        for (size_t i = 0; i < 4; ++i)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                cargoOffsetBytes[i][j] = static_cast<uint32_t>(remainingData.data() - data.data());

                auto* bytes = reinterpret_cast<const int8_t*>(remainingData.data());
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

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);

        auto imageOffset = image + RoadStation::ImageIds::totalPreviewImages;
        for (size_t i = 0; i < std::size(imageOffsets); ++i)
        {
            imageOffsets[i] = imageOffset;
            imageOffset += kDrawStyleTotalNumImages[paintStyle];
        }
    }

    // 0x00490B8E
    void RoadStationObject::unload()
    {
        name = 0;
        image = 0;

        std::fill(std::begin(imageOffsets), std::end(imageOffsets), 0);
        std::fill(std::begin(mods), std::end(mods), 0);
        cargoType = 0;
        std::fill(&cargoOffsetBytes[0][0], &cargoOffsetBytes[0][0] + sizeof(cargoOffsetBytes) / sizeof(uint32_t), 0);
    }

    sfl::static_vector<RoadStationObject::CargoOffset, Limits::kMaxStationCargoDensity> RoadStationObject::getCargoOffsets(const uint8_t rotation, const uint8_t nibble) const
    {
        assert(rotation < 4 && nibble < 4);

        const auto* bytes = reinterpret_cast<const std::byte*>(this) + cargoOffsetBytes[rotation][nibble];
        uint8_t z = *reinterpret_cast<const uint8_t*>(bytes);
        bytes++;
        sfl::static_vector<CargoOffset, Limits::kMaxStationCargoDensity> result;
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

            // The game can't handle anything larger than 16 so we've made the static vector 16 max
            if (result.size() == result.max_size())
            {
                break;
            }
        }
        return result;
    }
}
