#include "SoftwareDrawingEngine.h"
#include "Config.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Console/Console.h>
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <algorithm>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;

namespace OpenLoco::Drawing
{
    using SetPaletteFunc = void (*)(const PaletteEntry* palette, int32_t index, int32_t count);

    ScreenInfo _unscaledScreenInfo{};

    // TODO: Move into the renderer.
    static loco_global<RenderTarget, 0x0050B884> _screenRT;
    static loco_global<Ui::ScreenInfo, 0x0050B894> _screenInfo;

    // static RenderTarget _screenUiRT;
    static Ui::ScreenInfo _screenUiInfo;

    // TODO: Combine those into one struct.
    static loco_global<ScreenInvalidationData, 0x0050B8A0> _screenInvalidation;
    static loco_global<uint8_t[7500], 0x00E025C4> _screenInvalidationGrid;

    loco_global<SetPaletteFunc, 0x0052524C> _setPaletteCallback;

    SoftwareDrawingEngine::~SoftwareDrawingEngine()
    {
        if (_palette != nullptr)
        {
            SDL_FreePalette(_palette);
            _palette = nullptr;
        }
    }

    void SoftwareDrawingEngine::initialize(SDL_Window* window)
    {
        _window = window;
        createPalette();
    }

    // T[m][n]
    template<typename T>
    class Grid
    {
        T* ptr;
        size_t m;
        size_t n;

    public:
        Grid(T* data, const int32_t n, const int32_t m)
            : ptr(data)
            , m(m)
            , n(n)
        {
        }

        T* operator[](std::size_t idx)
        {
            return ptr + idx * n;
        }

        size_t getRows(size_t x, size_t dX, size_t y)
        {

            int dy = 0;
            for (size_t yy = y; yy < this->m; yy++)
            {

                for (size_t xx = x; xx < x + dX; xx++)
                {
                    if ((*this)[yy][xx] == 0)
                    {
                        return dy;
                    }
                }
                dy++;
            }
            return dy;
        }
    };

    void SoftwareDrawingEngine::resize(int32_t width, int32_t height)
    {
        // Scale the width and height by configured scale factor
        auto scaleFactor = Config::get().scaleFactor;
        auto scaledWidth = (int32_t)(width / scaleFactor);
        auto scaledHeight = (int32_t)(height / scaleFactor);

        width = scaledWidth * scaleFactor;
        height = scaledHeight * scaleFactor;

        _unscaledScreenInfo.width = width;
        _unscaledScreenInfo.height = height;

        int32_t widthShift = 6;
        int16_t blockWidth = 1 << widthShift;
        int32_t heightShift = 3;
        int16_t blockHeight = 1 << heightShift;

        if (_screenSurface != nullptr)
        {
            SDL_FreeSurface(_screenSurface);
        }
        if (_screenRGBASurface != nullptr)
        {
            SDL_FreeSurface(_screenRGBASurface);
        }
        if (_screenUiSurface != nullptr)
        {
            SDL_FreeSurface(_screenSurface);
        }
        if (_screenUiRGBASurface != nullptr)
        {
            SDL_FreeSurface(_screenUiRGBASurface);
        }

        // Screen
        _screenSurface = SDL_CreateRGBSurface(0, scaledWidth, scaledHeight, 8, 0, 0, 0, 0);
        SDL_SetSurfacePalette(_screenSurface, Gfx::getDrawingEngine().getPalette());

        _screenRGBASurface = SDL_CreateRGBSurface(0, scaledWidth, scaledHeight, 32, 0, 0, 0, 0);
        SDL_SetSurfaceBlendMode(_screenRGBASurface, SDL_BLENDMODE_NONE);

        // UI
        _screenUiSurface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
        SDL_SetSurfacePalette(_screenUiSurface, Gfx::getDrawingEngine().getPalette());

        _screenUiRGBASurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
        SDL_SetSurfaceBlendMode(_screenUiRGBASurface, SDL_BLENDMODE_NONE);

        // Mask for UI as transparency.
        if (SDL_SetColorKey(_screenUiRGBASurface, 1, SDL_MapRGB(_screenUiRGBASurface->format, 0xFF, 0xFF, 0xFF)) != 0)
        {
            printf("Unable to set colourkey: %s", SDL_GetError());
        }

        _finalSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
        SDL_SetSurfaceBlendMode(_finalSurface, SDL_BLENDMODE_NONE);

        auto& rt = Gfx::getScreenRT();
        if (rt.bits != nullptr)
        {
            delete[] rt.bits;
        }
        rt.bits = new uint8_t[_screenSurface->pitch * scaledHeight];
        rt.width = scaledWidth;
        rt.height = scaledHeight;
        rt.pitch = _screenSurface->pitch - scaledWidth;

        _screenInfo->width = scaledWidth;
        _screenInfo->height = scaledHeight;
        _screenInfo->width_2 = scaledWidth;
        _screenInfo->height_2 = scaledHeight;
        _screenInfo->width_3 = scaledWidth;
        _screenInfo->height_3 = scaledHeight;

        auto& uiRT = Gfx::getScreenUiRT();
        if (uiRT.bits != nullptr)
        {
            delete[] uiRT.bits;
        }
        uiRT.bits = new uint8_t[_screenUiSurface->pitch * height];
        uiRT.width = width;
        uiRT.height = height;
        uiRT.pitch = _screenUiSurface->pitch - width;

        _screenUiInfo.width = width;
        _screenUiInfo.height = height;
        _screenUiInfo.width_2 = width;
        _screenUiInfo.height_2 = height;
        _screenUiInfo.width_3 = width;
        _screenUiInfo.height_3 = height;
        
        _screenInvalidation->blockWidth = blockWidth;
        _screenInvalidation->blockHeight = blockHeight;
        _screenInvalidation->columnCount = (scaledWidth / blockWidth) + 1;
        _screenInvalidation->rowCount = (scaledHeight / blockHeight) + 1;
        _screenInvalidation->columnShift = widthShift;
        _screenInvalidation->rowShift = heightShift;
        _screenInvalidation->initialised = 1;
    }

    /**
     * 0x004C5C69
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void SoftwareDrawingEngine::invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        left = std::max(left, 0);
        top = std::max(top, 0);
        right = std::min(right, (int32_t)_screenInfo->width);
        bottom = std::min(bottom, (int32_t)_screenInfo->height);

        if (left >= right)
            return;
        if (top >= bottom)
            return;

        right--;
        bottom--;

        const int32_t dirtyBlockLeft = left >> _screenInvalidation->columnShift;
        const int32_t dirtyBlockRight = right >> _screenInvalidation->columnShift;
        const int32_t dirtyBlockTop = top >> _screenInvalidation->rowShift;
        const int32_t dirtyBlockBottom = bottom >> _screenInvalidation->rowShift;

        const size_t columns = _screenInvalidation->columnCount;
        const size_t rows = _screenInvalidation->rowCount;
        auto grid = Grid<uint8_t>(_screenInvalidationGrid, columns, rows);

        for (int16_t y = dirtyBlockTop; y <= dirtyBlockBottom; y++)
        {
            for (int16_t x = dirtyBlockLeft; x <= dirtyBlockRight; x++)
            {
                grid[y][x] = 0xFF;
            }
        }
    }

    // Helper function until all users of set_palette_callback are implemented
    static void updatePaletteStatic(const PaletteEntry* entries, int32_t index, int32_t count)
    {
        Gfx::getDrawingEngine().updatePalette(entries, index, count);
    }

    void SoftwareDrawingEngine::createPalette()
    {
        // Create a palette for the window
        _palette = SDL_AllocPalette(256);
        _setPaletteCallback = updatePaletteStatic;
    }

    // 0x004C5CFA
    void SoftwareDrawingEngine::render()
    {
        if (!_screenInvalidation->initialised)
            return;
            
        const size_t columns = _screenInvalidation->columnCount;
        const size_t rows = _screenInvalidation->rowCount;
        auto grid = Grid<uint8_t>(_screenInvalidationGrid, columns, rows);

        for (size_t x = 0; x < columns; x++)
        {
            for (size_t y = 0; y < rows; y++)
            {
                if (grid[y][x] == 0)
                    continue;

                // Don't determine columns will cause rendering z fighting issues
                const size_t dX = 1;

                // Check rows
                size_t dY = grid.getRows(x, dX, y);

                render(x, y, dX, dY);
            }
        }

        // Draw UI.
        auto& screenUiRT = Gfx::getScreenUiRT();

        auto rect = Rect(
            static_cast<int16_t>(0),
            static_cast<int16_t>(0),
            static_cast<uint16_t>(screenUiRT.width),
            static_cast<uint16_t>(screenUiRT.height));

        RenderTarget uiRT;
        uiRT.width = screenUiRT.width;
        uiRT.height = screenUiRT.height;
        uiRT.x = 0;
        uiRT.y = 0;
        uiRT.bits = screenUiRT.bits;
        uiRT.pitch = screenUiRT.pitch;
        uiRT.zoomLevel = 0;

        // auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        // drawingCtx.clear(Gfx::getScreenRT(), 0x0A0A0A0A);
        // drawingCtx.clear(screenUiRT, 0x0);

        Ui::WindowManager::render(screenUiRT, rect);
    }

    void SoftwareDrawingEngine::render(size_t x, size_t y, size_t dx, size_t dy)
    {
        const auto columns = _screenInvalidation->columnCount;
        const auto rows = _screenInvalidation->rowCount;
        auto grid = Grid<uint8_t>(_screenInvalidationGrid, columns, rows);

        // Unset dirty blocks
        for (size_t top = y; top < y + dy; top++)
        {
            for (uint32_t left = x; left < x + dx; left++)
            {
                grid[top][left] = 0;
            }
        }

        auto rect = Rect(
            static_cast<int16_t>(x * _screenInvalidation->blockWidth),
            static_cast<int16_t>(y * _screenInvalidation->blockHeight),
            static_cast<uint16_t>(dx * _screenInvalidation->blockWidth),
            static_cast<uint16_t>(dy * _screenInvalidation->blockHeight));

        this->render(rect);
    }

    void SoftwareDrawingEngine::updatePalette(const PaletteEntry* entries, int32_t index, int32_t count)
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

    void SoftwareDrawingEngine::render(const Rect& _rect)
    {
        auto max = Rect(0, 0, Ui::width(), Ui::height());
        auto rect = _rect.intersection(max);

        registers regs;
        regs.ax = rect.left();
        regs.bx = rect.top();
        regs.cx = rect.right() - 1;
        regs.dx = rect.bottom() - 1;
        call(0x00451D98, regs);

        RenderTarget rt;
        rt.width = rect.width();
        rt.height = rect.height();
        rt.x = rect.left();
        rt.y = rect.top();
        rt.bits = _screenRT->bits + rect.left() + ((_screenRT->width + _screenRT->pitch) * rect.top());
        rt.pitch = _screenRT->width + _screenRT->pitch - rect.width();
        rt.zoomLevel = 0;

        // TODO: Remove main window and draw that independent from UI.
        auto* viewport = Ui::WindowManager::getMainViewport();
        viewport->render(&rt);
    }

    static void copyRenderTargetToSurface(RenderTarget& rt, SDL_Surface* surface)
    {
        // Lock the surface before setting its pixels
        if (SDL_MUSTLOCK(surface))
        {
            if (SDL_LockSurface(surface) < 0)
            {
                return;
            }
        }

        // Copy pixels from the virtual screen buffer to the surface
        if (rt.bits != nullptr)
        {
            std::memcpy(surface->pixels, rt.bits, surface->pitch * surface->h);
        }

        // Unlock the surface
        if (SDL_MUSTLOCK(surface))
        {
            SDL_UnlockSurface(surface);
        }
    }

    void SoftwareDrawingEngine::present()
    {
        copyRenderTargetToSurface(Gfx::getScreenRT(), _screenSurface);
        copyRenderTargetToSurface(Gfx::getScreenUiRT(), _screenUiSurface);

        auto scaleFactor = Config::get().scaleFactor;
        auto* winSurface = SDL_GetWindowSurface(_window);

        // Convert palette based surface to RGBA for viewport.
        if (SDL_BlitSurface(_screenSurface, nullptr, _screenRGBASurface, nullptr))
        {
            Console::error("SDL_BlitSurface %s", SDL_GetError());
            exit(1);
        }

        if (scaleFactor > 1)
        {
            if (SDL_BlitScaled(_screenRGBASurface, nullptr, _finalSurface, nullptr))
            {
                Console::error("SDL_BlitSurface %s", SDL_GetError());
                exit(1);
            }
        }
        else
        {
            if (SDL_BlitSurface(_screenRGBASurface, nullptr, _finalSurface, nullptr))
            {
                Console::error("SDL_BlitSurface %s", SDL_GetError());
                exit(1);
            }
        }

        // Convert palette based surface to RGBA for UI.
        if (SDL_BlitSurface(_screenUiSurface, nullptr, _screenUiRGBASurface, nullptr))
        {
            Console::error("SDL_BlitSurface %s", SDL_GetError());
            exit(1);
        }

        if (scaleFactor > 1)
        {
            if (SDL_BlitScaled(_screenUiRGBASurface, nullptr, _finalSurface, nullptr))
            {
                Console::error("SDL_BlitSurface %s", SDL_GetError());
                exit(1);
            }
        }
        else
        {
            if (SDL_BlitSurface(_screenUiRGBASurface, nullptr, _finalSurface, nullptr))
            {
                Console::error("SDL_BlitSurface %s", SDL_GetError());
                exit(1);
            }
        }

        // Draw final surface to window.
        if (SDL_BlitSurface(_finalSurface, nullptr, winSurface, nullptr))
        {
            Console::error("SDL_BlitSurface %s", SDL_GetError());
            exit(1);
        }

        SDL_UpdateWindowSurface(_window);
    }

    SoftwareDrawingContext& SoftwareDrawingEngine::getDrawingContext()
    {
        return _ctx;
    }

}
