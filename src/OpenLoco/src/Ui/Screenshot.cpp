#include "Ui/Screenshot.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Platform/Platform.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <png.h>
#include <string>

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non-portable

using namespace OpenLoco::Ui;

namespace OpenLoco::Ui
{
    static int8_t _screenshotCountdown = 0; // 0x00508F16

    static ScreenshotType _screenshotType = ScreenshotType::regular;

    void triggerScreenshotCountdown(int8_t numTicks, ScreenshotType type)
    {
        _screenshotCountdown = numTicks;
        _screenshotType = type;
    }

    static std::string saveScreenshot();
    static std::string saveGiantScreenshot();

    void handleScreenshotCountdown()
    {
        if (_screenshotCountdown != 0)
        {
            _screenshotCountdown--;
            if (_screenshotCountdown == 0)
            {
                try
                {
                    std::string fileName;
                    if (_screenshotType == ScreenshotType::giant)
                    {
                        fileName = saveGiantScreenshot();
                    }
                    else
                    {
                        fileName = saveScreenshot();
                    }

                    FormatArguments::common(fileName.c_str());
                    Windows::Error::openQuiet(StringIds::screenshot_saved_as, StringIds::null);
                }
                catch (const std::exception& ex)
                {
                    OpenLoco::Diagnostics::Logging::error("Saving screenshot failed", ex.what());
                    Windows::Error::open(StringIds::screenshot_failed);
                }
            }
        }
    }

    static void pngWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        auto ostream = static_cast<std::ostream*>(png_get_io_ptr(png_ptr));
        ostream->write((const char*)data, length);
    }

    static void pngFlush(png_structp png_ptr)
    {
        auto ostream = static_cast<std::ostream*>(png_get_io_ptr(png_ptr));
        ostream->flush();
    }

    static void saveRenderTargetToPng(const Gfx::RenderTarget& rt, std::fstream& outputStream)
    {
        auto rgbaPalette = Gfx::getRgbaPalette();

        png_structp pngPtr = nullptr;
        png_colorp palette = nullptr;
        try
        {
            pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (pngPtr == nullptr)
            {
                throw Exception::RuntimeError("png_create_write_struct failed.");
            }

            png_set_write_fn(pngPtr, &outputStream, pngWriteData, pngFlush);

            // Set error handler
            if (setjmp(png_jmpbuf(pngPtr)))
            {
                throw Exception::RuntimeError("PNG ERROR");
            }

            auto infoPtr = png_create_info_struct(pngPtr);
            if (infoPtr == nullptr)
            {
                throw Exception::RuntimeError("png_create_info_struct failed.");
            }

            palette = (png_colorp)png_malloc(pngPtr, 246 * sizeof(png_color));
            if (palette == nullptr)
            {
                throw Exception::RuntimeError("png_malloc failed.");
            }

            for (size_t i = 0; i < 246; i++)
            {
                palette[i].blue = rgbaPalette[i].b;
                palette[i].green = rgbaPalette[i].g;
                palette[i].red = rgbaPalette[i].r;
            }
            png_set_PLTE(pngPtr, infoPtr, palette, 246);

            png_byte transparentIndex = 0;
            png_set_tRNS(pngPtr, infoPtr, &transparentIndex, 1, nullptr);
            png_set_IHDR(pngPtr, infoPtr, rt.width, rt.height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
            png_write_info(pngPtr, infoPtr);

            uint8_t* data = rt.bits;
            for (int y = 0; y < rt.height; y++)
            {
                png_write_row(pngPtr, data);
                data += rt.pitch + rt.width;
            }

            png_write_end(pngPtr, nullptr);
            png_destroy_info_struct(pngPtr, &infoPtr);
            png_free(pngPtr, palette);
            png_destroy_write_struct(&pngPtr, nullptr);
        }
        catch (const std::exception&)
        {
            png_free(pngPtr, palette);
            png_destroy_write_struct(&pngPtr, nullptr);
            throw;
        }
    }

    // 0x00452667
    static std::string prepareSaveScreenshot(const Gfx::RenderTarget& rt)
    {
        auto screenshotsFolderPath = Environment::getPathNoWarning(Environment::PathId::screenshots);
        Environment::autoCreateDirectory(screenshotsFolderPath);

        std::string scenarioName = getGameState().scenarioName;
        if (scenarioName.length() == 0)
        {
            scenarioName = StringManager::getString(StringIds::screenshot_filename_template);
        }

        std::string fileName = std::string(scenarioName) + ".png";
        fs::path path;
        int16_t suffix;
        for (suffix = 1; suffix < std::numeric_limits<int16_t>().max(); suffix++)
        {
            if (!fs::exists(screenshotsFolderPath / fileName))
            {
                path = screenshotsFolderPath / fileName;
                break;
            }

            fileName = std::string(scenarioName) + " (" + std::to_string(suffix) + ").png";
        }

        if (path.empty())
        {
            throw Exception::RuntimeError("Failed finding filename");
        }

        std::fstream outputStream(path.c_str(), std::ios::out | std::ios::binary);
        saveRenderTargetToPng(rt, outputStream);

        return fileName;
    }

    static std::string saveScreenshot()
    {
        auto& drawingEngine = Gfx::getDrawingEngine();
        auto& rt = drawingEngine.getScreenRT();
        return prepareSaveScreenshot(rt);
    }

    static Ui::Viewport createGiantViewport(const int32_t resolutionWidth, const int32_t resolutionHeight, const ZoomLevel zoomLevel)
    {
        Ui::Viewport viewport{};
        viewport.width = resolutionWidth;
        viewport.height = resolutionHeight;
        viewport.x = 0;
        viewport.y = 0;
        viewport.viewWidth = zoomLevel.applyTo<int32_t>(viewport.width);
        viewport.viewHeight = zoomLevel.applyTo<int32_t>(viewport.height);
        viewport.zoom = zoomLevel;
        viewport.pad_11 = 0;
        viewport.flags = ViewportFlags::none;

        const coord_t centreX = static_cast<coord_t>((World::TileManager::getMapWidth() / 2) + (World::kTileSize / 2));
        const coord_t centreY = static_cast<coord_t>((World::TileManager::getMapHeight() / 2) + (World::kTileSize / 2));
        const coord_t z = World::TileManager::getHeight({ centreX, centreY }).landHeight;

        auto pos = viewport.centre2dCoordinates({ centreX, centreY, z });
        viewport.viewX = pos.x;
        viewport.viewY = pos.y;

        return viewport;
    }

    static std::string saveGiantScreenshot()
    {
        const auto& main = WindowManager::getMainWindow();

        const auto zoomLevel = ZoomLevel{ std::max<int8_t>(static_cast<int8_t>(main->viewports[0]->zoom), ZoomLevel::full) };

        const int32_t resolutionWidth = zoomLevel.applyInversedTo<int32_t>(World::TileManager::getMapWidth() * 2) + 8;
        const int32_t resolutionHeight = zoomLevel.applyInversedTo<int32_t>(World::TileManager::getMapHeight() * 1) + 8;

        Ui::Viewport viewport = createGiantViewport(resolutionWidth, resolutionHeight, zoomLevel);

        // Ensure sprites appear regardless of rotation
        EntityManager::resetSpatialIndex();

        // A giant screenshot captures the whole map at the current zoom level, so it can be far
        // larger than the screen (a 4096 tile map at full zoom is over 30 GB). Refuse to attempt
        // such an allocation rather than slowing the machine down to a crawl.
        constexpr uint64_t kMaxGiantScreenshotBytes = 1ULL << 30;
        const auto numPixels = static_cast<uint64_t>(resolutionWidth) * static_cast<uint64_t>(resolutionHeight);
        if (numPixels > kMaxGiantScreenshotBytes || numPixels > std::numeric_limits<size_t>::max())
        {
            OpenLoco::Diagnostics::Logging::error("A giant screenshot of the map at this zoom level needs {} MB ({}x{} pixels); zoom out to take a smaller screenshot", numPixels / (1024 * 1024), resolutionWidth, resolutionHeight);
            throw Exception::RuntimeError("Giant screenshot is too large");
        }

        Gfx::RenderTarget rt{};
        rt.bits = static_cast<uint8_t*>(malloc(static_cast<size_t>(numPixels)));
        if (rt.bits == nullptr)
        {
            OpenLoco::Diagnostics::Logging::error("Unable to allocate {} MB for a {}x{} giant screenshot", numPixels / (1024 * 1024), resolutionWidth, resolutionHeight);
            throw Exception::RuntimeError("Unable to allocate memory for a giant screenshot");
        }

        rt.x = 0;
        rt.y = 0;
        rt.width = resolutionWidth;
        rt.height = resolutionHeight;
        rt.pitch = 0;

        auto& drawingEngine = Gfx::getDrawingEngine();
        auto& drawingCtx = drawingEngine.getDrawingContext();

        drawingCtx.pushRenderTarget(rt);

        viewport.render(drawingCtx);

        drawingCtx.popRenderTarget();

        auto fileName = prepareSaveScreenshot(rt);
        free(rt.bits);

        return fileName;
    }
}
