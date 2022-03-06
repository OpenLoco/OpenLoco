#include "SoftwareDrawingEngine.h"
#include "../Interop/Interop.hpp"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include <SDL2/SDL.h>
#include <algorithm>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;

namespace OpenLoco::Drawing
{
    using SetPaletteFunc = void (*)(const PaletteEntry* palette, int32_t index, int32_t count);

    static loco_global<Ui::ScreenInfo, 0x0050B884> screen_info;
    static loco_global<uint8_t[1], 0x00E025C4> _E025C4;
    loco_global<SetPaletteFunc, 0x0052524C> set_palette_callback;

    static void windowDraw(Context* context, Ui::Window* w, Rect rect);
    static void windowDraw(Context* context, Ui::Window* w, int16_t left, int16_t top, int16_t right, int16_t bottom);
    static bool windowDrawSplit(Gfx::Context* context, Ui::Window* w, int16_t left, int16_t top, int16_t right, int16_t bottom);

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
    void SoftwareDrawingEngine::setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        left = std::max(left, 0);
        top = std::max(top, 0);
        right = std::min(right, (int32_t)screen_info->width);
        bottom = std::min(bottom, (int32_t)screen_info->height);

        if (left >= right)
            return;
        if (top >= bottom)
            return;

        right--;
        bottom--;

        const int32_t dirty_block_left = left >> screen_info->dirty_block_column_shift;
        const int32_t dirty_block_right = right >> screen_info->dirty_block_column_shift;
        const int32_t dirty_block_top = top >> screen_info->dirty_block_row_shift;
        const int32_t dirty_block_bottom = bottom >> screen_info->dirty_block_row_shift;

        const size_t columns = screen_info->dirty_block_columns;
        const size_t rows = screen_info->dirty_block_rows;
        auto grid = Grid<uint8_t>(_E025C4, columns, rows);

        for (int16_t y = dirty_block_top; y <= dirty_block_bottom; y++)
        {
            for (int16_t x = dirty_block_left; x <= dirty_block_right; x++)
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
        set_palette_callback = updatePaletteStatic;
    }

    // 0x004C5CFA
    void SoftwareDrawingEngine::drawDirtyBlocks()
    {
        const size_t columns = screen_info->dirty_block_columns;
        const size_t rows = screen_info->dirty_block_rows;
        auto grid = Grid<uint8_t>(_E025C4, columns, rows);

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

                drawDirtyBlocks(x, y, dX, dY);
            }
        }
    }

    void SoftwareDrawingEngine::drawDirtyBlocks(size_t x, size_t y, size_t dx, size_t dy)
    {
        const auto columns = screen_info->dirty_block_columns;
        const auto rows = screen_info->dirty_block_rows;
        auto grid = Grid<uint8_t>(_E025C4, columns, rows);

        // Unset dirty blocks
        for (size_t top = y; top < y + dy; top++)
        {
            for (uint32_t left = x; left < x + dx; left++)
            {
                grid[top][left] = 0;
            }
        }

        auto rect = Rect(
            static_cast<int16_t>(x * screen_info->dirty_block_width),
            static_cast<int16_t>(y * screen_info->dirty_block_height),
            static_cast<uint16_t>(dx * screen_info->dirty_block_width),
            static_cast<uint16_t>(dy * screen_info->dirty_block_height));

        this->drawRect(rect);
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

    void SoftwareDrawingEngine::drawRect(const Rect& _rect)
    {
        auto max = Rect(0, 0, Ui::width(), Ui::height());
        auto rect = _rect.intersection(max);

        registers regs;
        regs.ax = rect.left();
        regs.bx = rect.top();
        regs.cx = rect.right() - 1;
        regs.dx = rect.bottom() - 1;
        call(0x00451D98, regs);

        Context windowContext;
        windowContext.width = rect.width();
        windowContext.height = rect.height();
        windowContext.x = rect.left();
        windowContext.y = rect.top();
        windowContext.bits = screen_info->context.bits + rect.left() + ((screen_info->context.width + screen_info->context.pitch) * rect.top());
        windowContext.pitch = screen_info->context.width + screen_info->context.pitch - rect.width();
        windowContext.zoom_level = 0;

        for (size_t i = 0; i < Ui::WindowManager::count(); i++)
        {
            auto w = Ui::WindowManager::get(i);

            if (w->isTranslucent())
                continue;

            if (rect.right() <= w->x || rect.bottom() <= w->y)
                continue;

            if (rect.left() >= w->x + w->width || rect.top() >= w->y + w->height)
                continue;

            windowDraw(&windowContext, w, rect);
        }
    }

    static void windowDraw(Context* context, Ui::Window* w, Rect rect)
    {
        windowDraw(context, w, rect.left(), rect.top(), rect.right(), rect.bottom());
    }

    /**
     * 0x004C5EA9
     *
     * @param w
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    static void windowDraw(Context* context, Ui::Window* w, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        if (!w->isVisible())
            return;

        // Split window into only the regions that require drawing
        if (windowDrawSplit(context, w, left, top, right, bottom))
            return;

        // Clamp region
        left = std::max(left, w->x);
        top = std::max(top, w->y);
        right = std::min<int16_t>(right, w->x + w->width);
        bottom = std::min<int16_t>(bottom, w->y + w->height);
        if (left >= right)
            return;
        if (top >= bottom)
            return;

        // Draw the window in this region
        Ui::WindowManager::drawSingle(context, w, left, top, right, bottom);

        for (uint32_t index = Ui::WindowManager::indexOf(w) + 1; index < Ui::WindowManager::count(); index++)
        {
            auto v = Ui::WindowManager::get(index);

            // Don't draw overlapping opaque windows, they won't have changed
            if ((v->flags & Ui::WindowFlags::transparent) == 0)
                continue;

            Ui::WindowManager::drawSingle(context, v, left, top, right, bottom);
        }
    }

    /**
     * 0x004C5EA9
     *
     * @param context
     * @param w @<esi>
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     * @return
     */
    static bool windowDrawSplit(Gfx::Context* context, Ui::Window* w, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        // Divide the draws up for only the visible regions of the window recursively
        for (uint32_t index = Ui::WindowManager::indexOf(w) + 1; index < Ui::WindowManager::count(); index++)
        {
            auto topwindow = Ui::WindowManager::get(index);

            // Check if this window overlaps w
            if (topwindow->x >= right || topwindow->y >= bottom)
                continue;
            if (topwindow->x + topwindow->width <= left || topwindow->y + topwindow->height <= top)
                continue;
            if (topwindow->isTranslucent())
                continue;

            // A window overlaps w, split up the draw into two regions where the window starts to overlap
            if (topwindow->x > left)
            {
                // Split draw at topwindow.left
                windowDraw(context, w, left, top, topwindow->x, bottom);
                windowDraw(context, w, topwindow->x, top, right, bottom);
            }
            else if (topwindow->x + topwindow->width < right)
            {
                // Split draw at topwindow.right
                windowDraw(context, w, left, top, topwindow->x + topwindow->width, bottom);
                windowDraw(context, w, topwindow->x + topwindow->width, top, right, bottom);
            }
            else if (topwindow->y > top)
            {
                // Split draw at topwindow.top
                windowDraw(context, w, left, top, right, topwindow->y);
                windowDraw(context, w, left, topwindow->y, right, bottom);
            }
            else if (topwindow->y + topwindow->height < bottom)
            {
                // Split draw at topwindow.bottom
                windowDraw(context, w, left, top, right, topwindow->y + topwindow->height);
                windowDraw(context, w, left, topwindow->y + topwindow->height, right, bottom);
            }

            // Drawing for this region should be done now, exit
            return true;
        }

        // No windows overlap
        return false;
    }
}
