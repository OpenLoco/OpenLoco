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
    private:
        void* _glContext = nullptr;  // SDL_GLContext
        unsigned int _glTexture = 0;  // GLuint - Main game texture
        unsigned int _gl3DTexture = 0; // GLuint - 3D rendering texture
        unsigned int _glFramebuffer = 0; // GLuint - Framebuffer for offscreen rendering
        unsigned int _glDepthBuffer = 0; // GLuint - Depth renderbuffer
        int32_t _viewportWidth = 0;
        int32_t _viewportHeight = 0;
        float _rotationAngle = 0.0f;

    public:
        HardwareDrawingEngine();
        ~HardwareDrawingEngine() override;

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

    private:
        void render3DScene(); // New helper method for 3D rendering
    };
}
