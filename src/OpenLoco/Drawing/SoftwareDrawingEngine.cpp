#include "SoftwareDrawingEngine.h"
#include "../Interop/Interop.hpp"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include <algorithm>

using namespace OpenLoco::interop;
using namespace OpenLoco::gfx;
using namespace OpenLoco::ui;

namespace OpenLoco::Drawing
{
    static loco_global<ui::screen_info_t, 0x0050B884> screen_info;
    static loco_global<uint8_t[1], 0x00E025C4> _E025C4;

    static void windowDraw(drawpixelinfo_t* dpi, ui::window* w, Rect rect);
    static void windowDraw(drawpixelinfo_t* dpi, ui::window* w, int16_t left, int16_t top, int16_t right, int16_t bottom);
    static bool windowDrawSplit(gfx::drawpixelinfo_t* dpi, ui::window* w, int16_t left, int16_t top, int16_t right, int16_t bottom);

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

                // Determine columns
                size_t xx;
                for (xx = x; xx < columns; xx++)
                {
                    if (grid[y][xx] == 0)
                        break;
                }
                size_t dX = xx - x;

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

    void SoftwareDrawingEngine::drawRect(const Rect& _rect)
    {
        auto max = Rect(0, 0, ui::width(), ui::height());
        auto rect = _rect.intersection(max);

        registers regs;
        regs.ax = rect.left();
        regs.bx = rect.top();
        regs.cx = rect.right() - 1;
        regs.dx = rect.bottom() - 1;
        call(0x00451D98, regs);

        drawpixelinfo_t windowDPI;
        windowDPI.width = rect.width();
        windowDPI.height = rect.height();
        windowDPI.x = rect.left();
        windowDPI.y = rect.top();
        windowDPI.bits = screen_info->dpi.bits + rect.left() + ((screen_info->dpi.width + screen_info->dpi.pitch) * rect.top());
        windowDPI.pitch = screen_info->dpi.width + screen_info->dpi.pitch - rect.width();
        windowDPI.zoom_level = 0;

        for (size_t i = 0; i < ui::WindowManager::count(); i++)
        {
            auto w = ui::WindowManager::get(i);

            if (w->isTranslucent())
                continue;

            if (rect.right() <= w->x || rect.bottom() <= w->y)
                continue;

            if (rect.left() >= w->x + w->width || rect.top() >= w->y + w->height)
                continue;

            windowDraw(&windowDPI, w, rect);
        }
    }

    static void windowDraw(drawpixelinfo_t* dpi, ui::window* w, Rect rect)
    {
        windowDraw(dpi, w, rect.left(), rect.top(), rect.right(), rect.bottom());
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
    static void windowDraw(drawpixelinfo_t* dpi, ui::window* w, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        if (!w->isVisible())
            return;

        // Split window into only the regions that require drawing
        if (windowDrawSplit(dpi, w, left, top, right, bottom))
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
        ui::WindowManager::drawSingle(dpi, w, left, top, right, bottom);

        for (uint32_t index = ui::WindowManager::indexOf(w) + 1; index < ui::WindowManager::count(); index++)
        {
            auto v = ui::WindowManager::get(index);

            // Don't draw overlapping opaque windows, they won't have changed
            if ((v->flags & ui::window_flags::transparent) == 0)
                continue;

            ui::WindowManager::drawSingle(dpi, v, left, top, right, bottom);
        }
    }

    /**
     * 0x004C5EA9
     *
     * @param dpi
     * @param w @<esi>
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     * @return
     */
    static bool windowDrawSplit(gfx::drawpixelinfo_t* dpi, ui::window* w, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        // Divide the draws up for only the visible regions of the window recursively
        for (uint32_t index = ui::WindowManager::indexOf(w) + 1; index < ui::WindowManager::count(); index++)
        {
            auto topwindow = ui::WindowManager::get(index);

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
                windowDraw(dpi, w, left, top, topwindow->x, bottom);
                windowDraw(dpi, w, topwindow->x, top, right, bottom);
            }
            else if (topwindow->x + topwindow->width < right)
            {
                // Split draw at topwindow.right
                windowDraw(dpi, w, left, top, topwindow->x + topwindow->width, bottom);
                windowDraw(dpi, w, topwindow->x + topwindow->width, top, right, bottom);
            }
            else if (topwindow->y > top)
            {
                // Split draw at topwindow.top
                windowDraw(dpi, w, left, top, right, topwindow->y);
                windowDraw(dpi, w, left, topwindow->y, right, bottom);
            }
            else if (topwindow->y + topwindow->height < bottom)
            {
                // Split draw at topwindow.bottom
                windowDraw(dpi, w, left, top, right, topwindow->y + topwindow->height);
                windowDraw(dpi, w, left, topwindow->y + topwindow->height, right, bottom);
            }

            // Drawing for this region should be done now, exit
            return true;
        }

        // No windows overlap
        return false;
    }
}
