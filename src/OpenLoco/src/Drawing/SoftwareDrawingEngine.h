#pragma once

#include "Graphics/Gfx.h"
#include "SoftwareDrawingContext.h"
#include "Ui/Rect.h"
#include <algorithm>
#include <cstddef>

struct SDL_Palette;

namespace OpenLoco::Drawing
{
#pragma pack(push, 1)
    struct PaletteEntry
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    };
#pragma pack(pop)

    class SoftwareDrawingEngine
    {
    public:
        ~SoftwareDrawingEngine();

        // Renders all invalidated regions.
        void render();

        // Renders a specific region.
        void render(const Ui::Rect& rect);

        // Invalidates a region, this forces it to be rendered next frame.
        void invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom);

        void createPalette();
        SDL_Palette* getPalette() { return _palette; }
        void updatePalette(const PaletteEntry* entries, int32_t index, int32_t count);

        SoftwareDrawingContext getDrawingContext();

    private:
        void render(size_t x, size_t y, size_t dx, size_t dy);

        SDL_Palette* _palette;
    };
}
