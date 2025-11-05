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
    class IDrawingEngine;

    template<typename T>
    concept DrawingContextType = std::is_base_of_v<DrawingContext, T> && !std::is_same_v<DrawingContext, T>;

    class IDrawingEngine
    {
    public:
        virtual ~IDrawingEngine() = default;

        virtual void initialize(SDL_Window* window) = 0;
        virtual void resize(int32_t width, int32_t height) = 0;

        // Renders all invalidated regions.
        virtual void render() = 0;

        // Renders a specific region.
        virtual void render(const Ui::Rect& rect) = 0;

        // Presents the final image to the screen.
        virtual void present() = 0;

        // Invalidates a region, this forces it to be rendered next frame.
        virtual void invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom) = 0;

        // Moves the pixels in the specified render target.
        virtual void movePixels(
            const RenderTarget& rt,
            int16_t dstX,
            int16_t dstY,
            int16_t width,
            int16_t height,
            int16_t srcX,
            int16_t srcY)
            = 0;

        virtual void renderDirtyRegions() = 0;

        const RenderTarget& getScreenRT() const { return _screenRT; }
        const Ui::ScreenInfo& getScreenInfo() const { return _screenInfo; }
        DrawingContext& getDrawingContext() { return *_ctx; }

        void createPalette();
        SDL_Palette* getPalette() { return _palette; }
        void updatePalette(const PaletteEntry* entries, int32_t index, int32_t count);

    protected:
        // TODO: Move into the renderer.
        // 0x0050B884
        RenderTarget _screenRT{};
        // 0x0050B894
        Ui::ScreenInfo _screenInfo{};

        template<DrawingContextType T>
        void initialiseDrawingContext() { _ctx = std::make_unique<T>(); }

        InvalidationGrid _invalidationGrid;

        SDL_Renderer* _renderer{};
        SDL_Window* _window{};
        SDL_Palette* _palette{};
        SDL_Surface* _screenSurface{};
        SDL_Surface* _screenRGBASurface{};
        SDL_Texture* _screenTexture{};
        SDL_Texture* _scaledScreenTexture{};
        SDL_PixelFormat* _screenTextureFormat{};
        SDL_Texture* _screenRGBATexture{};

    private:
        std::unique_ptr<DrawingContext> _ctx;
    };
}
