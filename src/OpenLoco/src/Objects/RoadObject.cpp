#include "RoadObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

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
        if (flags & Flags12::unk_03)
        {
            return numMods == 0;
        }
        return true;
    }

    // 0x00477BCF
    void RoadObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x00477BCF, regs);
    }

    // 0x00477D9E
    void RoadObject::unload()
    {
        name = 0;
        var_0B = 0;
        image = 0;
        std::fill(std::begin(bridges), std::end(bridges), 0);
    }
}
