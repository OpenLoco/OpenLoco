#pragma once

#include "../Graphics/Gfx.h"
#include "../Ui/Rect.h"
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
        void drawDirtyBlocks();
        void drawRect(const Ui::Rect& rect);
        void setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom);

        void createPalette();
        SDL_Palette* getPalette() { return _palette; }
        void updatePalette(const PaletteEntry* entries, int32_t index, int32_t count);

    private:
        void drawDirtyBlocks(size_t x, size_t y, size_t dx, size_t dy);

        SDL_Palette* _palette;
    };
}
