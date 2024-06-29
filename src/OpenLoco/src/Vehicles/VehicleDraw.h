#include "Types.hpp"
#include <OpenLoco/Engine/Ui/Point.hpp>

namespace OpenLoco
{
    namespace Gfx
    {
        class DrawingContext;
    }
    struct VehicleObject;
    struct VehicleObjectBodySprite;
    enum class Pitch : uint8_t;

    // roll/animationFrame
    uint32_t getBodyImageIndex(const VehicleObjectBodySprite& sprite, const Pitch pitch, const uint8_t yaw, const uint8_t roll, const uint8_t cargoIndex);
    uint32_t getBrakingImageIndex(const VehicleObjectBodySprite& sprite, const Pitch pitch, const uint8_t yaw);

    void drawVehicleOverview(Gfx::DrawingContext& drawingCtx, Ui::Point offset, const VehicleObject& vehObject, const uint8_t yaw, const uint8_t roll, const ColourScheme colourScheme);
    void drawVehicleOverview(Gfx::DrawingContext& drawingCtx, Ui::Point offset, int16_t vehicleTypeIdx, uint8_t yaw, uint8_t roll, CompanyId companyId);
}
