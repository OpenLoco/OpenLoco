#include "Screenshot.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Platform/Platform.h"
#include "../S5/S5.h"
#include "../Ui.h"
#include <cstdint>
#include <fstream>
#include <png.h>
#include <string>

#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non-portable

using namespace OpenLoco::Interop;
using namespace OpenLoco::Ui;

namespace OpenLoco::Input
{
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

    // 0x00452667
    std::string saveScreenshot()
    {
        auto basePath = Platform::getUserDirectory();
        std::string scenarioName = S5::getOptions().scenarioName;

        if (scenarioName.length() == 0)
            scenarioName = StringManager::getString(StringIds::screenshot_filename_template);

        std::string fileName = std::string(scenarioName) + ".png";
        fs::path path;
        int16_t suffix;
        for (suffix = 1; suffix < std::numeric_limits<int16_t>().max(); suffix++)
        {
            if (!fs::exists(basePath / fileName))
            {
                path = basePath / fileName;
                break;
            }

            fileName = std::string(scenarioName) + " (" + std::to_string(suffix) + ").png";
        }

        if (path.empty())
        {
            throw std::runtime_error("Failed finding filename");
        }

        std::fstream outputStream(path.c_str(), std::ios::out | std::ios::binary);

        static loco_global<uint8_t[256][4], 0x0113ED20> _113ED20;

        png_structp pngPtr = nullptr;
        png_colorp palette = nullptr;
        try
        {
            pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (pngPtr == nullptr)
                throw std::runtime_error("png_create_write_struct failed.");

            png_set_write_fn(pngPtr, &outputStream, pngWriteData, pngFlush);

            // Set error handler
            if (setjmp(png_jmpbuf(pngPtr)))
            {
                throw std::runtime_error("PNG ERROR");
            }

            auto infoPtr = png_create_info_struct(pngPtr);
            if (infoPtr == nullptr)
                throw std::runtime_error("png_create_info_struct failed.");

            palette = (png_colorp)png_malloc(pngPtr, 246 * sizeof(png_color));
            if (palette == nullptr)
                throw std::runtime_error("png_malloc failed.");

            for (size_t i = 0; i < 246; i++)
            {
                palette[i].blue = _113ED20[i][0];
                palette[i].green = _113ED20[i][1];
                palette[i].red = _113ED20[i][2];
            }
            png_set_PLTE(pngPtr, infoPtr, palette, 246);
            auto& rt = Gfx::getScreenRT();

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

        return fileName;
    }
}
