#include "SoftwareDrawingEngine.h"
#include "Config.h"
#include "Logging.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cstdlib>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

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
        if (_screenTexture != nullptr)
        {
            SDL_DestroyTexture(_screenTexture);
            _screenTexture = nullptr;
        }
        if (_scaledScreenTexture != nullptr)
        {
            SDL_DestroyTexture(_scaledScreenTexture);
            _screenTexture = nullptr;
        }
        if (_screenTextureFormat != nullptr)
        {
            SDL_FreeFormat(_screenTextureFormat);
            _screenTextureFormat = nullptr;
        }

        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    static void applyImGuiStyle()
    {
        auto& style = ImGui::GetStyle();

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
        colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.29f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.51f, 0.59f, 0.63f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.46f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.40f, 0.40f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.49f, 0.55f, 0.55f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.46f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.62f, 0.62f, 0.62f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.39f, 0.39f, 0.39f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.51f, 0.59f, 0.59f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.36f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        style.FrameBorderSize = 1.0f;
        style.TabRounding = 0.0f;
        style.FramePadding = ImVec2(4.0f, 2.0f);
    }

    void SoftwareDrawingEngine::initialize(SDL_Window* window)
    {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

        _renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (_renderer == nullptr)
        {
            // Try to fallback to software renderer.
            Logging::warn("Hardware acceleration not available, falling back to software renderer.");
            _renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
            if (_renderer == nullptr)
            {
                Logging::error("Unable to create hardware or software renderer: {}", SDL_GetError());
                std::abort();
            }
        }
        _window = window;
        createPalette();

        // Initialize ImGUI.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

        io.Fonts->AddFontDefault();

        ImGui_ImplSDL2_InitForSDLRenderer(window, _renderer);
        ImGui_ImplSDLRenderer2_Init(_renderer);

        applyImGuiStyle();
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

    void SoftwareDrawingEngine::resize(const int32_t width, const int32_t height)
    {
        // Scale the width and height by configured scale factor
        const auto scaleFactor = Config::get().scaleFactor;
        const auto scaledWidth = (int32_t)(width / scaleFactor);
        const auto scaledHeight = (int32_t)(height / scaleFactor);

        int32_t widthShift = 6;
        int16_t blockWidth = 1 << widthShift;
        int32_t heightShift = 3;
        int16_t blockHeight = 1 << heightShift;

        // Release old resources.
        if (_screenSurface != nullptr)
        {
            SDL_FreeSurface(_screenSurface);
        }
        if (_screenRGBASurface != nullptr)
        {
            SDL_FreeSurface(_screenRGBASurface);
        }

        if (_screenTexture != nullptr)
        {
            SDL_DestroyTexture(_screenTexture);
            _screenTexture = nullptr;
        }

        if (_scaledScreenTexture != nullptr)
        {
            SDL_DestroyTexture(_scaledScreenTexture);
            _scaledScreenTexture = nullptr;
        }

        if (_screenTextureFormat != nullptr)
        {
            SDL_FreeFormat(_screenTextureFormat);
            _screenTextureFormat = nullptr;
        }

        // Surfaces.
        _screenSurface = SDL_CreateRGBSurface(0, scaledWidth, scaledHeight, 8, 0, 0, 0, 0);
        if (_screenSurface == nullptr)
        {
            Logging::error("SDL_CreateRGBSurface (_screenSurface) failed: {}", SDL_GetError());
            return;
        }

        _screenRGBASurface = SDL_CreateRGBSurface(0, scaledWidth, scaledHeight, 32, 0, 0, 0, 0);
        if (_screenRGBASurface == nullptr)
        {
            Logging::error("SDL_CreateRGBSurface (_screenRGBASurface) failed: {}", SDL_GetError());
            return;
        }

        SDL_SetSurfaceBlendMode(_screenRGBASurface, SDL_BLENDMODE_NONE);
        SDL_SetSurfacePalette(_screenSurface, _palette);

        SDL_RendererInfo rendererInfo{};
        int32_t result = SDL_GetRendererInfo(_renderer, &rendererInfo);
        if (result < 0)
        {
            Logging::error("HWDisplayDrawingEngine::Resize error: {}", SDL_GetError());
            return;
        }

        uint32_t pixelFormat = SDL_PIXELFORMAT_UNKNOWN;
        for (uint32_t i = 0; i < rendererInfo.num_texture_formats; i++)
        {
            uint32_t format = rendererInfo.texture_formats[i];
            if (!SDL_ISPIXELFORMAT_FOURCC(format) && !SDL_ISPIXELFORMAT_INDEXED(format)
                && (pixelFormat == SDL_PIXELFORMAT_UNKNOWN || SDL_BYTESPERPIXEL(format) < SDL_BYTESPERPIXEL(pixelFormat)))
            {
                pixelFormat = format;
            }
        }

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        _screenTexture = SDL_CreateTexture(_renderer, pixelFormat, SDL_TEXTUREACCESS_STREAMING, scaledWidth, scaledHeight);
        if (_screenTexture == nullptr)
        {
            Logging::error("SDL_CreateTexture (_screenTexture) failed: {}", SDL_GetError());
            return;
        }

        if (scaleFactor > 1.0f)
        {
            const auto scale = std::ceil(scaleFactor);
            // We only need this texture when we have a scale above 1x, this texture uses the actual canvas size.
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
            _scaledScreenTexture = SDL_CreateTexture(_renderer, pixelFormat, SDL_TEXTUREACCESS_TARGET, width * scale, height * scale);
            if (_scaledScreenTexture == nullptr)
            {
                Logging::error("SDL_CreateTexture (_scaledScreenTexture) failed: {}", SDL_GetError());
                return;
            }
        }

        uint32_t format;
        SDL_QueryTexture(_screenTexture, &format, nullptr, nullptr, nullptr);
        _screenTextureFormat = SDL_AllocFormat(format);

        int32_t pitch = _screenSurface->pitch;

        auto& rt = Gfx::getScreenRT();
        if (rt.bits != nullptr)
        {
            delete[] rt.bits;
        }
        rt.bits = new uint8_t[pitch * scaledHeight];
        rt.width = scaledWidth;
        rt.height = scaledHeight;
        rt.pitch = pitch - scaledWidth;

        _screenInfo->width = scaledWidth;
        _screenInfo->height = scaledHeight;
        _screenInfo->width_2 = scaledWidth;
        _screenInfo->height_2 = scaledHeight;
        _screenInfo->width_3 = scaledWidth;
        _screenInfo->height_3 = scaledHeight;

        _screenInvalidation->blockWidth = blockWidth;
        _screenInvalidation->blockHeight = blockHeight;
        _screenInvalidation->columnCount = (scaledWidth / blockWidth) + 1;
        _screenInvalidation->rowCount = (scaledHeight / blockHeight) + 1;
        _screenInvalidation->columnShift = widthShift;
        _screenInvalidation->rowShift = heightShift;
        _screenInvalidation->initialised = 1;
    }

    void SoftwareDrawingEngine::beginFrame()
    {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        SDL_RenderClear(_renderer);
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

    void SoftwareDrawingEngine::present()
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
        auto& rt = Gfx::getScreenRT();
        if (rt.bits != nullptr)
        {
            std::memcpy(_screenSurface->pixels, rt.bits, _screenSurface->pitch * _screenSurface->h);
        }

        // Unlock the surface
        if (SDL_MUSTLOCK(_screenSurface))
        {
            SDL_UnlockSurface(_screenSurface);
        }

        // Convert colors via palette mapping onto the RGBA surface.
        if (SDL_BlitSurface(_screenSurface, nullptr, _screenRGBASurface, nullptr))
        {
            Logging::error("SDL_BlitSurface {}", SDL_GetError());
            return;
        }

        // Stream the RGBA pixels into screen texture.
        void* pixels;
        int pitch;
        SDL_LockTexture(_screenTexture, NULL, &pixels, &pitch);
        SDL_ConvertPixels(_screenRGBASurface->w, _screenRGBASurface->h, _screenRGBASurface->format->format, _screenRGBASurface->pixels, _screenRGBASurface->pitch, _screenTextureFormat->format, pixels, pitch);
        SDL_UnlockTexture(_screenTexture);

        if (Config::get().scaleFactor > 1.0f)
        {
            // Copy screen texture to the scaled texture.
            SDL_SetRenderTarget(_renderer, _scaledScreenTexture);
            SDL_RenderCopy(_renderer, _screenTexture, nullptr, nullptr);

            // Copy scaled texture to primary render target.
            SDL_SetRenderTarget(_renderer, nullptr);
            SDL_RenderCopy(_renderer, _scaledScreenTexture, nullptr, nullptr);
        }
        else
        {
            SDL_RenderCopy(_renderer, _screenTexture, nullptr, nullptr);
        }

        ImGuiIO& io = ImGui::GetIO();

        ImGui::Render();
        SDL_RenderSetScale(_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

        // Display buffers.
        SDL_RenderPresent(_renderer);
    }

    SoftwareDrawingContext& SoftwareDrawingEngine::getDrawingContext()
    {
        return _ctx;
    }

}
