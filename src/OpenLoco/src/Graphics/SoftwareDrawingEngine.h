#pragma once

#include "Graphics/Gfx.h"
#include "IDrawingEngine.h"
#include "InvalidationGrid.h"
#include "SoftwareDrawingContext.h"
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

namespace OpenLoco::Ui
{
    struct ScreenInfo;
}

namespace OpenLoco::Gfx
{
    struct RenderTarget;

    class SoftwareDrawingEngine : public IDrawingEngine
    {
    public:
        SoftwareDrawingEngine();
        ~SoftwareDrawingEngine() override;

        void initialize(SDL_Window* window) override;
        void resize(int32_t width, int32_t height) override;
        void render() override;
        void render(const Ui::Rect& rect) override;
        void present() override;

        void invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom) override;
        void renderDirtyRegions() override;

        void movePixels(
            const RenderTarget& rt,
            int16_t dstX,
            int16_t dstY,
            int16_t width,
            int16_t height,
            int16_t srcX,
            int16_t srcY) override;
    };
}
