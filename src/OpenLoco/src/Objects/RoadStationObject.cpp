#include "RoadStationObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectManager.h"
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x00490C17
    void RoadStationObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawImage(&rt, x - 34, y - 34, colourImage);

        auto colour = ExtColour::translucentMutedDarkRed1;
        if (!hasFlags(RoadStationFlags::recolourable))
        {
            colour = ExtColour::unk2E;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        drawingCtx.drawImage(&rt, x - 34, y - 34, translucentImage);
    }

    // 0x00490C59
    void RoadStationObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
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

    // 0x00490ABE
    void RoadStationObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00490ABE, regs);
    }

    // 0x00490B8E
    void RoadStationObject::unload()
    {
        name = 0;
        image = 0;
        var_10 = 0;
        var_14 = 0;
        var_18 = 0;
        var_1C = 0;
        std::fill(std::begin(mods), std::end(mods), 0);
        var_2C = 0;
        std::fill(std::begin(var_2E), std::end(var_2E), 0);
    }
}
