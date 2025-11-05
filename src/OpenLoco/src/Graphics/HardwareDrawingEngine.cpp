#include "HardwareDrawingEngine.h"
#include "Config.h"
#include "Graphics/FPSCounter.h"
#include "GpuRenderTarget.h"
#include "Logging.h"
#include "RenderTarget.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

// OpenGL FBO extension constants
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif

// OpenGL extension function pointers for framebuffer objects
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint* framebuffers);
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint* framebuffers);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint* renderbuffers);
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint* renderbuffers);
typedef void (APIENTRYP PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;

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
        if (_glFramebuffer != 0)
        {
            glDeleteFramebuffers(1, &_glFramebuffer);
            _glFramebuffer = 0;
        }

        if (_glDepthBuffer != 0)
        {
            glDeleteRenderbuffers(1, &_glDepthBuffer);
            _glDepthBuffer = 0;
        }

        if (_gl3DTexture != 0)
        {
            glDeleteTextures(1, &_gl3DTexture);
            _gl3DTexture = 0;
        }

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

        // Load OpenGL extension functions for framebuffer objects
        glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
        glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
        glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
        glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
        glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)SDL_GL_GetProcAddress("glGenRenderbuffers");
        glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
        glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)SDL_GL_GetProcAddress("glBindRenderbuffer");
        glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)SDL_GL_GetProcAddress("glRenderbufferStorage");
        glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
        glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus");

        if (!glGenFramebuffers || !glBindFramebuffer || !glFramebufferTexture2D)
        {
            Logging::error("Failed to load OpenGL framebuffer extensions!");
            std::abort();
        }

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

        // Initialize rotation angle
        _rotationAngle = 0.0f;

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

        if (_gl3DTexture != 0)
        {
            glDeleteTextures(1, &_gl3DTexture);
            _gl3DTexture = 0;
        }

        if (_glDepthBuffer != 0)
        {
            glDeleteRenderbuffers(1, &_glDepthBuffer);
            _glDepthBuffer = 0;
        }

        if (_glFramebuffer != 0)
        {
            glDeleteFramebuffers(1, &_glFramebuffer);
            _glFramebuffer = 0;
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

        // Create OpenGL texture for game rendering
        glGenTextures(1, &_glTexture);
        glBindTexture(GL_TEXTURE_2D, _glTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, scaleFactor > 1.0f ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Create OpenGL texture for 3D rendering
        glGenTextures(1, &_gl3DTexture);
        glBindTexture(GL_TEXTURE_2D, _gl3DTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Create depth renderbuffer
        glGenRenderbuffers(1, &_glDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _glDepthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

        // Create framebuffer for offscreen rendering
        glGenFramebuffers(1, &_glFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, _glFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gl3DTexture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _glDepthBuffer);

        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            Logging::error("Framebuffer is not complete! Status: 0x{:X}", status);
        }

        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

        // Render 3D scene to texture
        render3DScene();
    }

    void HardwareDrawingEngine::render3DScene()
    {
        // Bind framebuffer for offscreen rendering
        glBindFramebuffer(GL_FRAMEBUFFER, _glFramebuffer);

        // Clear the framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Transparent background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Enable depth testing for 3D
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);

        // Set up 3D perspective projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const float aspect = (float)_viewportWidth / (float)_viewportHeight;
        const float fov = 60.0f;
        const float zNear = 0.1f;
        const float zFar = 100.0f;
        const float fH = std::tan(fov / 360.0f * 3.14159265f) * zNear;
        const float fW = fH * aspect;
        glFrustum(-fW, fW, -fH, fH, zNear, zFar);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Position camera
        glTranslatef(0.0f, 0.0f, -5.0f);

        // Apply rotation
        glRotatef(_rotationAngle, 0.0f, 1.0f, 0.0f);           // Rotate around Y axis
        glRotatef(_rotationAngle * 0.5f, 1.0f, 0.0f, 0.0f);   // Rotate around X axis

        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw 3D pyramid
        glBegin(GL_TRIANGLES);
        // Front face
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // red with alpha
        glVertex3f(0.0f, 1.0f, 0.0f);
        glColor4f(0.0f, 1.0f, 0.0f, 0.8f); // green with alpha
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glColor4f(0.0f, 0.0f, 1.0f, 0.8f); // blue with alpha
        glVertex3f(1.0f, -1.0f, 1.0f);

        // Right face
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // red
        glVertex3f(0.0f, 1.0f, 0.0f);
        glColor4f(0.0f, 0.0f, 1.0f, 0.8f); // blue
        glVertex3f(1.0f, -1.0f, 1.0f);
        glColor4f(1.0f, 1.0f, 0.0f, 0.8f); // yellow
        glVertex3f(1.0f, -1.0f, -1.0f);

        // Back face
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // red
        glVertex3f(0.0f, 1.0f, 0.0f);
        glColor4f(1.0f, 1.0f, 0.0f, 0.8f); // yellow
        glVertex3f(1.0f, -1.0f, -1.0f);
        glColor4f(1.0f, 0.0f, 1.0f, 0.8f); // magenta
        glVertex3f(-1.0f, -1.0f, -1.0f);

        // Left face
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // red
        glVertex3f(0.0f, 1.0f, 0.0f);
        glColor4f(1.0f, 0.0f, 1.0f, 0.8f); // magenta
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glColor4f(0.0f, 1.0f, 0.0f, 0.8f); // green
        glVertex3f(-1.0f, -1.0f, 1.0f);

        // Bottom face (square base)
        glColor4f(0.0f, 1.0f, 1.0f, 0.8f); // cyan
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);

        glColor4f(0.0f, 1.0f, 1.0f, 0.8f); // cyan
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glEnd();

        glDisable(GL_BLEND);

        // Update rotation angle for next frame
        _rotationAngle += 1.0f;
        if (_rotationAngle >= 360.0f)
        {
            _rotationAngle -= 360.0f;
        }

        // Unbind framebuffer (render to screen again)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        // Upload game texture to OpenGL
        glBindTexture(GL_TEXTURE_2D, _glTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _screenRGBASurface->w, _screenRGBASurface->h, GL_RGBA, GL_UNSIGNED_BYTE, _screenRGBASurface->pixels);

        // Clear the screen framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Disable depth test for 2D rendering
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);

        // Set up 2D orthographic projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, _viewportWidth, _viewportHeight, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Render game texture (background layer)
        glBindTexture(GL_TEXTURE_2D, _glTexture);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // White color for proper texture rendering

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

        // Enable blending for 3D overlay
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Render 3D texture overlay
        glBindTexture(GL_TEXTURE_2D, _gl3DTexture);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

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

        glDisable(GL_BLEND);

        // Swap buffers to display
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

    GpuRenderTarget HardwareDrawingEngine::createGpuRenderTarget(int32_t width, int32_t height, bool hasDepth)
    {
        GpuRenderTarget rt;
        rt.width = static_cast<int16_t>(width);
        rt.height = static_cast<int16_t>(height);
        rt.hasDepth = hasDepth;
        rt.x = 0;
        rt.y = 0;
        rt.zoomLevel = 0;

        // Create texture
        glGenTextures(1, &rt.textureId);
        glBindTexture(GL_TEXTURE_2D, rt.textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Create framebuffer
        glGenFramebuffers(1, &rt.framebufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, rt.framebufferId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt.textureId, 0);

        // Create depth buffer if requested
        if (hasDepth)
        {
            glGenRenderbuffers(1, &rt.depthBufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, rt.depthBufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt.depthBufferId);
        }

        // Check framebuffer status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            Logging::error("GPU RenderTarget framebuffer is not complete! Status: 0x{:X}", status);
            destroyGpuRenderTarget(rt);
            return GpuRenderTarget{}; // Return invalid RT
        }

        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Logging::info("Created GPU RenderTarget: {}x{} (texture={}, fbo={}, depth={})",
                      width, height, rt.textureId, rt.framebufferId, rt.depthBufferId);

        return rt;
    }

    void HardwareDrawingEngine::destroyGpuRenderTarget(GpuRenderTarget& rt)
    {
        if (rt.depthBufferId != 0)
        {
            glDeleteRenderbuffers(1, &rt.depthBufferId);
            rt.depthBufferId = 0;
        }

        if (rt.framebufferId != 0)
        {
            glDeleteFramebuffers(1, &rt.framebufferId);
            rt.framebufferId = 0;
        }

        if (rt.textureId != 0)
        {
            glDeleteTextures(1, &rt.textureId);
            rt.textureId = 0;
        }

        rt.width = 0;
        rt.height = 0;
    }
}
