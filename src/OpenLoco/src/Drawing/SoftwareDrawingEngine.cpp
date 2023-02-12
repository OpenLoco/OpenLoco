#include "SoftwareDrawingEngine.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <algorithm>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;

namespace OpenLoco::Drawing
{
    using SetPaletteFunc = void (*)(const PaletteEntry* palette, int32_t index, int32_t count);

    // TODO: Move into the renderer.
    static loco_global<RenderTarget, 0x0050B884> _screenRT;
    static loco_global<Ui::ScreenInfo, 0x0050B894> _screenInfo;

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

        // Draw UI.
        Ui::WindowManager::render(rt, rect);
    }

    SoftwareDrawingContext& SoftwareDrawingEngine::getDrawingContext()
    {
        return _ctx;
    }

}
