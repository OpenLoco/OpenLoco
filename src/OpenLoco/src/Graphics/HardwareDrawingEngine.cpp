#include "HardwareDrawingEngine.h"
#include "Config.h"
#include "Graphics/FPSCounter.h"
#include "Logging.h"
#include "RenderTarget.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <algorithm>
#include <cstdlib>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Gfx
{
    using SetPaletteFunc = void (*)(const PaletteEntry* palette, int32_t index, int32_t count);

    HardwareDrawingEngine::HardwareDrawingEngine()
    {
        RenderTarget rtDummy{};

        initialiseDrawingContext<HardwareDrawingContext>();
        getDrawingContext().pushRenderTarget(rtDummy);
    }

    HardwareDrawingEngine::~HardwareDrawingEngine()
    {
        if (_glTexture != 0)
        {
            glDeleteTextures(1, &_glTexture);
            _glTexture = 0;
        }

        if (_palette != nullptr)
        {
            SDL_FreePalette(_palette);
            _palette = nullptr;
        }

        if (_screenSurface != nullptr)
        {
            SDL_FreeSurface(_screenSurface);
            _screenSurface = nullptr;
        }

        if (_screenRGBASurface != nullptr)
        {
            SDL_FreeSurface(_screenRGBASurface);
            _screenRGBASurface = nullptr;
        }

        if (_glContext != nullptr)
        {
            SDL_GL_DeleteContext(_glContext);
            _glContext = nullptr;
        }
    }

    void HardwareDrawingEngine::initialize(SDL_Window* window)
    {
        // Set OpenGL attributes before creating context
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        _glContext = SDL_GL_CreateContext(window);
        if (_glContext == nullptr)
        {
            Logging::error("OpenGL context could not be created! SDL_Error: {}", SDL_GetError());
            std::abort();
        }

        // Make the context current
        SDL_GL_MakeCurrent(window, _glContext);

        // Enable VSync
        SDL_GL_SetSwapInterval(1);

        _window = window;

        // Initialize OpenGL state
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);

        // Set up 2D orthographic projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        createPalette();
    }

    void HardwareDrawingEngine::resize(const int32_t width, const int32_t height)
    {
        // Scale the width and height by configured scale factor
        const auto scaleFactor = Config::get().scaleFactor;
        const auto scaledWidth = (int32_t)(width / scaleFactor);
        const auto scaledHeight = (int32_t)(height / scaleFactor);

        // Store viewport dimensions
        _viewportWidth = width;
        _viewportHeight = height;

        // Release old resources.
        if (_screenSurface != nullptr)
        {
            SDL_FreeSurface(_screenSurface);
        }
        if (_screenRGBASurface != nullptr)
        {
            SDL_FreeSurface(_screenRGBASurface);
        }

        if (_glTexture != 0)
        {
            glDeleteTextures(1, &_glTexture);
            _glTexture = 0;
        }

        // Create surfaces for palette conversion
        _screenSurface = SDL_CreateRGBSurface(0, scaledWidth, scaledHeight, 8, 0, 0, 0, 0);
        if (_screenSurface == nullptr)
        {
            Logging::error("SDL_CreateRGBSurface (_screenSurface) failed: {}", SDL_GetError());
            return;
        }

        _screenRGBASurface = SDL_CreateRGBSurface(0, scaledWidth, scaledHeight, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        if (_screenRGBASurface == nullptr)
        {
            Logging::error("SDL_CreateRGBSurface (_screenRGBASurface) failed: {}", SDL_GetError());
            return;
        }

        SDL_SetSurfaceBlendMode(_screenRGBASurface, SDL_BLENDMODE_NONE);
        SDL_SetSurfacePalette(_screenSurface, _palette);

        // Create OpenGL texture
        glGenTextures(1, &_glTexture);
        glBindTexture(GL_TEXTURE_2D, _glTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, scaleFactor > 1.0f ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Set up viewport and projection
        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        int32_t pitch = _screenSurface->pitch;

        RenderTarget& rt = _screenRT;
        if (rt.bits != nullptr)
        {
            delete[] rt.bits;
        }
        rt.bits = new uint8_t[pitch * scaledHeight];
        rt.width = scaledWidth;
        rt.height = scaledHeight;
        rt.pitch = pitch - scaledWidth;

        _screenInfo.width = scaledWidth;
        _screenInfo.height = scaledHeight;
        _screenInfo.width_2 = scaledWidth;
        _screenInfo.height_2 = scaledHeight;
        _screenInfo.width_3 = scaledWidth;
        _screenInfo.height_3 = scaledHeight;

        int32_t widthShift = 6;
        int16_t blockWidth = 1 << widthShift;
        int32_t heightShift = 3;
        int16_t blockHeight = 1 << heightShift;

        _invalidationGrid.reset(scaledWidth, scaledHeight, blockWidth, blockHeight);

        // Reset the drawing context, this holds the old screen render target.
        getDrawingContext().reset();

        // Push the screen render target so that by default we render to that.
        getDrawingContext().pushRenderTarget(rt);

        // Set the normal background colour.
        getDrawingContext().clearSingle(PaletteIndex::black0);
    }

    /**
     * 0x004C5C69
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void HardwareDrawingEngine::invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        // TODO: in 3d hardware this isn't required
        _invalidationGrid.invalidate(left, top, right, bottom);
    }

    // 0x004C5CFA
    void HardwareDrawingEngine::render()
    {
        // Need to first render the current dirty regions before updating the viewports.
        // This is needed to ensure it will copy the correct pixels when the viewport will be moved.
        renderDirtyRegions();

        // Updating the viewports will potentially move pixels and mark previously invisible regions as dirty.
        WindowManager::updateViewports();

        // Render the uncovered regions.
        renderDirtyRegions();

        // Draw FPS counter.
        if (Config::get().showFPS)
        {
            Gfx::drawFPS(getDrawingContext());
        }
    }

    void HardwareDrawingEngine::renderDirtyRegions()
    {
        _invalidationGrid.traverseDirtyCells([this](int32_t left, int32_t top, int32_t right, int32_t bottom) {
            this->render(Rect::fromLTRB(left, top, right, bottom));
        });
    }

    void HardwareDrawingEngine::render(const Rect& _rect)
    {
        auto max = Rect(0, 0, Ui::width(), Ui::height());
        auto rect = _rect.intersection(max);

        RenderTarget rt{};
        rt.width = rect.width();
        rt.height = rect.height();
        rt.x = rect.left();
        rt.y = rect.top();
        rt.bits = _screenRT.bits + rect.left() + ((_screenRT.width + _screenRT.pitch) * rect.top());
        rt.pitch = _screenRT.width + _screenRT.pitch - rect.width();
        rt.zoomLevel = 0;

        // Set the render target to the screen rt.
        getDrawingContext().pushRenderTarget(rt);

        // TODO: Remove main window and draw that independent from UI.

        // Draw UI.
        Ui::WindowManager::render(getDrawingContext(), rect);

        // Restore state.
        getDrawingContext().popRenderTarget();
    }

    void HardwareDrawingEngine::present()
    {
        // Lock the surface before setting its pixels
        if (SDL_MUSTLOCK(_screenSurface))
        {
            if (SDL_LockSurface(_screenSurface) < 0)
            {
                return;
            }
        }

        // Copy pixels from the virtual screen buffer to the surface
        auto& rt = getScreenRT();
        if (rt.bits != nullptr)
        {
            std::memcpy(_screenSurface->pixels, rt.bits, _screenSurface->pitch * _screenSurface->h);
        }

        // Unlock the surface
        if (SDL_MUSTLOCK(_screenSurface))
        {
            SDL_UnlockSurface(_screenSurface);
        }

        // Convert colours via palette mapping onto the RGBA surface.
        if (SDL_BlitSurface(_screenSurface, nullptr, _screenRGBASurface, nullptr))
        {
            Logging::error("SDL_BlitSurface {}", SDL_GetError());
            return;
        }

        // Upload texture to OpenGL
        glBindTexture(GL_TEXTURE_2D, _glTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _screenRGBASurface->w, _screenRGBASurface->h, GL_RGBA, GL_UNSIGNED_BYTE, _screenRGBASurface->pixels);

        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Render a textured quad using stored viewport dimensions
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _glTexture);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f((float)_viewportWidth, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f((float)_viewportWidth, (float)_viewportHeight);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, (float)_viewportHeight);
        glEnd();

        // demo triangle
        glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0); // red
        glVertex2f(-0.8, -0.8);
        glColor3f(0, 1, 0); // green
        glVertex2f(0.8, -0.8);
        glColor3f(0, 0, 1); // blue
        glVertex2f(0, 0.9);
        glEnd();

        // Swap buffers
        SDL_GL_SwapWindow(_window);
    }

    void HardwareDrawingEngine::movePixels(
        const RenderTarget& rt,
        int16_t dstX,
        int16_t dstY,
        int16_t width,
        int16_t height,
        int16_t srcX,
        int16_t srcY)
    {
        if (dstX == 0 && dstY == 0)
        {
            return;
        }

        // Adjust for move off canvas.
        // NOTE: when zooming, there can be x, y, dx, dy combinations that go off the
        // canvas; hence the checks. This code should ultimately not be called when
        // zooming because this function is specific to updating the screen on move
        int32_t lmargin = std::min(dstX - srcX, 0);
        int32_t rmargin = std::min((int32_t)rt.width - (dstX - srcX + width), 0);
        int32_t tmargin = std::min(dstY - srcY, 0);
        int32_t bmargin = std::min((int32_t)rt.height - (dstY - srcY + height), 0);

        dstX -= lmargin;
        dstY -= tmargin;
        width += lmargin + rmargin;
        height += tmargin + bmargin;

        int32_t stride = rt.width + rt.pitch;
        uint8_t* to = rt.bits + dstY * stride + dstX;
        uint8_t* from = rt.bits + (dstY - srcY) * stride + dstX - srcX;

        if (srcY > 0)
        {
            // If positive dy, reverse directions
            to += (height - 1) * stride;
            from += (height - 1) * stride;
            stride = -stride;
        }

        // Move bytes
        for (int32_t i = 0; i < height; i++)
        {
            std::memmove(to, from, width);
            to += stride;
            from += stride;
        }
    }
}
