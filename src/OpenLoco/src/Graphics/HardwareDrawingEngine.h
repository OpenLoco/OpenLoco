#pragma once

#include "Graphics/Gfx.h"
#include "HardwareDrawingContext.h"
#include "IDrawingEngine.h"
#include "InvalidationGrid.h"
#include <OpenLoco/Engine/Ui/Rect.hpp>
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
    class HardwareDrawingEngine : public IDrawingEngine
    {
    public:
        HardwareDrawingEngine();
        ~HardwareDrawingEngine() override;

        void initialize(SDL_Window* window) override;
        void resize(int32_t width, int32_t height) override;
        void render() override;
        void present() override;
        DrawingContext& getDrawingContext() override;

        // Renders a specific region.
        void render(const Ui::Rect& rect);

        // Invalidates a region, this forces it to be rendered next frame.
        void invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom);

        void createPalette();
        SDL_Palette* getPalette() { return _palette; }
        void updatePalette(const PaletteEntry* entries, int32_t index, int32_t count);

        const RenderTarget& getScreenRT();

        // Moves the pixels in the specified render target.
        void movePixels(
            const RenderTarget& rt,
            int16_t dstX,
            int16_t dstY,
            int16_t width,
            int16_t height,
            int16_t srcX,
            int16_t srcY);

        const Ui::ScreenInfo& getScreenInfo() const;

    private:
        void renderDirtyRegions();

    private:
        HardwareDrawingContext _ctx;
        InvalidationGrid _invalidationGrid;
    };
}
