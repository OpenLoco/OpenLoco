#include "Screenshot.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "Graphics/Gfx.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/TileManager.h"
#include "ScenarioOptions.h"
#include "Ui.h"
#include "WindowManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <cstdint>
#include <fstream>
#include <png.h>
#include <string>

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non-portable

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::Ui
{
    static loco_global<int8_t, 0x00508F16> _screenshotCountdown;

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
                catch (const std::exception&)
                {
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
        static loco_global<uint8_t[256][4], 0x0113ED20> _113ED20;

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
                palette[i].blue = _113ED20[i][0];
                palette[i].green = _113ED20[i][1];
                palette[i].red = _113ED20[i][2];
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
        std::string scenarioName = Scenario::getOptions().scenarioName;

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

    static Ui::Viewport createGiantViewport(const uint16_t resolutionWidth, const uint16_t resolutionHeight, const uint8_t zoomLevel)
    {
        Ui::Viewport viewport{};
        viewport.width = resolutionWidth;
        viewport.height = resolutionHeight;
        viewport.x = 0;
        viewport.y = 0;
        viewport.viewWidth = viewport.width << zoomLevel;
        viewport.viewHeight = viewport.height << zoomLevel;
        viewport.zoom = zoomLevel;
        viewport.pad_11 = 0;
        viewport.flags = ViewportFlags::none;

        const coord_t centreX = (World::TileManager::getMapColumns() / 2) * 32 + 16;
        const coord_t centreY = (World::TileManager::getMapRows() / 2) * 32 + 16;
        const coord_t z = World::TileManager::getHeight({ centreX, centreY }).landHeight;

        auto pos = viewport.centre2dCoordinates({ centreX, centreY, z });
        viewport.viewX = pos.x;
        viewport.viewY = pos.y;

        return viewport;
    }

    static std::string saveGiantScreenshot()
    {
        const auto& main = WindowManager::getMainWindow();
        const auto zoomLevel = main->viewports[0]->zoom;

        const uint16_t resolutionWidth = ((World::TileManager::getMapColumns() * 32 * 2) >> zoomLevel) + 8;
        const uint16_t resolutionHeight = ((World::TileManager::getMapRows() * 32 * 1) >> zoomLevel) + 128;

        Ui::Viewport viewport = createGiantViewport(resolutionWidth, resolutionHeight, zoomLevel);

        // Ensure sprites appear regardless of rotation
        EntityManager::resetSpatialIndex();

        Gfx::RenderTarget rt{};
        rt.bits = static_cast<uint8_t*>(malloc(resolutionWidth * resolutionHeight));
        if (rt.bits == nullptr)
        {
            return {};
        }

        rt.x = 0;
        rt.y = 0;
        rt.width = resolutionWidth;
        rt.height = resolutionHeight;
        rt.pitch = 0;
        rt.zoomLevel = 0;

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
