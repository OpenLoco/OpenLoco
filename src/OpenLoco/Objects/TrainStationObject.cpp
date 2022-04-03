#include "TrainStationObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "ObjectManager.h"

namespace OpenLoco
{
    // 0x00490A26
    void TrainStationObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&context, x - 34, y - 34, colourImage);

        auto colour = ExtColour::translucentMutedDarkRed1;
        if (!(flags & TrainStationFlags::recolourable))
        {
            colour = ExtColour::unk2E;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        Gfx::drawImage(&context, x - 34, y - 34, translucentImage);
    }

    // 0x00490A68
    void TrainStationObject::drawDescription(Gfx::Context& context, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(context, rowPosition, designed_year, obsolete_year);
    }

    std::vector<TrainStationObject::CargoOffset> TrainStationObject::getCargoOffsets(const uint8_t rotation, const uint8_t nibble) const
    {
        const auto* bytes = cargoOffsetBytes[rotation][nibble];
        uint8_t z = *reinterpret_cast<const uint8_t*>(bytes);
        bytes++;
        std::vector<CargoOffset> result;
        while (*bytes != static_cast<std::byte>(0xFF))
        {
            result.push_back({
                Map::Pos3{
                    *reinterpret_cast<const int8_t*>(bytes),
                    *reinterpret_cast<const int8_t*>(bytes + 1),
                    z,
                },
                Map::Pos3{
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
