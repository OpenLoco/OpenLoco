#include "IDrawingEngine.h"
#include "Graphics/Gfx.h"
#include "InvalidationGrid.h"
#include "RenderTarget.h"
#include "Ui.h"
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cstddef>
#include <memory>

struct SDL_Palette;
struct SDL_Surface;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_PixelFormat;

namespace OpenLoco::Gfx
{
    void IDrawingEngine::createPalette()
    {
        // Create a palette for the window
        _palette = SDL_AllocPalette(256);
    }

    void IDrawingEngine::updatePalette(const PaletteEntry* entries, int32_t index, int32_t count)
    {
        assert(index + count < 256);

        SDL_Color base[256]{};
        SDL_Color* basePtr = &base[index];
        auto* entryPtr = &entries[index];
        for (int i = 0; i < count; ++i, basePtr++, entryPtr++)
        {
            basePtr->r = entryPtr->r;
            basePtr->g = entryPtr->g;
            basePtr->b = entryPtr->b;
            basePtr->a = 0;
        }
        SDL_SetPaletteColors(_palette, &base[index], index, count);
    }
}
