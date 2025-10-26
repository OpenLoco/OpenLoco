#pragma once

#include "Graphics/Gfx.h"
#include "InvalidationGrid.h"
#include "RenderTarget.h"
#include "SoftwareDrawingContext.h"
#include "Ui.h"
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
    class IDrawingEngine
    {
    public:
        virtual ~IDrawingEngine() = default;

        virtual void initialize(SDL_Window* window) = 0;
        virtual void resize(int32_t width, int32_t height) = 0;
        virtual void render() = 0;
        virtual void present() = 0;

        virtual DrawingContext& getDrawingContext() = 0;

    protected:
        // TODO: Move into the renderer.
        // 0x0050B884
        RenderTarget _screenRT{};
        // 0x0050B894
        Ui::ScreenInfo _screenInfo{};

        SDL_Renderer* _renderer{};
        SDL_Window* _window{};
        SDL_Palette* _palette{};
        SDL_Surface* _screenSurface{};
        SDL_Surface* _screenRGBASurface{};
        SDL_Texture* _screenTexture{};
        SDL_Texture* _scaledScreenTexture{};
        SDL_PixelFormat* _screenTextureFormat{};
        SDL_Texture* _screenRGBATexture{};
    };
}
