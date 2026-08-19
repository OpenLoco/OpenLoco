#include "Graphics/UnicodeFont.h"
#include "Graphics/Colour.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageId.h"
#include "Logging.h"
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Core/FileSystem.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4244 4245 4456 4505 4701)
#endif
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <stb_truetype.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Gfx::UnicodeFont
{
    struct Glyph
    {
        int16_t width{};
        int16_t height{};
        int16_t xOffset{};
        int16_t yOffset{};
        int16_t advanceWidth{};
        std::vector<uint8_t> pixels;
    };

    struct LoadedFont
    {
        std::shared_ptr<std::vector<uint8_t>> data;
        stbtt_fontinfo info{};
    };

    static std::vector<LoadedFont> _fonts;
    static std::unordered_map<uint64_t, Glyph> _glyphCache;
    static bool _initialised = false;
    static bool _loggedMissingFont = false;

    static uint64_t glyphKey(Font font, uint32_t codepoint)
    {
        return (static_cast<uint64_t>(static_cast<uint16_t>(font)) << 32) | codepoint;
    }

    static int pixelHeightFor(Font font)
    {
        switch (font)
        {
            case Font::small:
                return 8;
            case Font::large:
                return 16;
            case Font::medium_normal:
            case Font::medium_bold:
            case Font::m1:
            case Font::m2:
            default:
                return 10;
        }
    }

    static std::vector<fs::path> candidateFontPaths()
    {
        std::vector<fs::path> paths;
#ifdef _WIN32
        fs::path fontsDir = "C:/Windows/Fonts";
        const auto windir = Platform::getEnvironmentVariable("WINDIR");
        if (!windir.empty())
        {
            fontsDir = fs::path(windir) / "Fonts";
        }
        paths.insert(paths.end(), {
                                      fontsDir / "malgun.ttf",
                                      fontsDir / "malgunbd.ttf",
                                      fontsDir / "YuGothM.ttc",
                                      fontsDir / "msyh.ttc",
                                      fontsDir / "msyh.ttf",
                                      fontsDir / "msgothic.ttc",
                                      fontsDir / "gulim.ttc",
                                      fontsDir / "simsun.ttc",
                                      fontsDir / "segoeui.ttf",
                                      fontsDir / "arial.ttf",
                                  });
#elif defined(__APPLE__) && defined(__MACH__)
        paths.insert(paths.end(), {
                                      "/System/Library/Fonts/AppleSDGothicNeo.ttc",
                                      "/System/Library/Fonts/Supplemental/AppleGothic.ttf",
                                      "/System/Library/Fonts/Hiragino Sans GB.ttc",
                                      "/Library/Fonts/Arial Unicode.ttf",
                                  });
#else
        paths.insert(paths.end(), {
                                      "/usr/share/fonts/truetype/nanum/NanumGothic.ttf",
                                      "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                                      "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
                                      "/usr/share/fonts/opentype/noto/NotoSansCJKkr-Regular.otf",
                                      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                  });
#endif
        return paths;
    }

    static void tryLoadFontFile(const fs::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            return;
        }

        in.seekg(0, std::ios::end);
        const auto size = static_cast<std::size_t>(in.tellg());
        in.seekg(0, std::ios::beg);
        if (size == 0)
        {
            return;
        }

        auto data = std::make_shared<std::vector<uint8_t>>(size);
        in.read(reinterpret_cast<char*>(data->data()), static_cast<std::streamsize>(size));
        if (!in)
        {
            return;
        }

        const int count = std::max(1, stbtt_GetNumberOfFonts(data->data()));
        const int maxFaces = std::min(count, 4);
        for (int i = 0; i < maxFaces; ++i)
        {
            const int offset = stbtt_GetFontOffsetForIndex(data->data(), i);
            if (offset < 0)
            {
                continue;
            }

            LoadedFont font;
            font.data = data;
            if (stbtt_InitFont(&font.info, font.data->data(), offset) != 0)
            {
                _fonts.push_back(std::move(font));
                if (_fonts.size() >= 6)
                {
                    return;
                }
            }
        }
    }

    static void initialise()
    {
        if (_initialised)
        {
            return;
        }
        _initialised = true;

        for (const auto& path : candidateFontPaths())
        {
            tryLoadFontFile(path);
            if (_fonts.size() >= 6)
            {
                break;
            }
        }

        if (_fonts.empty() && !_loggedMissingFont)
        {
            _loggedMissingFont = true;
            Logging::warn("No system Unicode font found; CJK text will still show as '?'");
        }
        else if (!_fonts.empty())
        {
            Logging::info("Loaded {} Unicode font face(s) for non-Latin text", _fonts.size());
        }
    }

    static const LoadedFont* findFontWithGlyph(uint32_t codepoint)
    {
        initialise();
        for (const auto& font : _fonts)
        {
            if (stbtt_FindGlyphIndex(&font.info, static_cast<int>(codepoint)) != 0)
            {
                return &font;
            }
        }
        return nullptr;
    }

    static const Glyph& getGlyph(Font font, uint32_t codepoint)
    {
        const auto key = glyphKey(font, codepoint);
        if (auto it = _glyphCache.find(key); it != _glyphCache.end())
        {
            return it->second;
        }

        Glyph glyph;
        const auto* loaded = findFontWithGlyph(codepoint);
        if (loaded == nullptr)
        {
            glyph.advanceWidth = getCharacterWidth(font, '?');
            return _glyphCache.emplace(key, std::move(glyph)).first->second;
        }

        const int pixelHeight = pixelHeightFor(font);
        const float scale = stbtt_ScaleForPixelHeight(&loaded->info, static_cast<float>(pixelHeight));

        int advanceWidth = 0;
        int leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&loaded->info, static_cast<int>(codepoint), &advanceWidth, &leftSideBearing);
        (void)leftSideBearing;
        glyph.advanceWidth = static_cast<int16_t>(std::max(1, static_cast<int>(advanceWidth * scale + 0.5f)));

        int ascent = 0;
        stbtt_GetFontVMetrics(&loaded->info, &ascent, nullptr, nullptr);

        int w = 0;
        int h = 0;
        int xoff = 0;
        int yoff = 0;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&loaded->info, scale, scale, static_cast<int>(codepoint), &w, &h, &xoff, &yoff);
        if (bitmap == nullptr || w <= 0 || h <= 0)
        {
            return _glyphCache.emplace(key, std::move(glyph)).first->second;
        }

        glyph.width = static_cast<int16_t>(w);
        glyph.height = static_cast<int16_t>(h);
        glyph.xOffset = static_cast<int16_t>(xoff);
        glyph.yOffset = static_cast<int16_t>(ascent * scale + yoff + 0.5f);
        glyph.pixels.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
        for (int i = 0; i < w * h; ++i)
        {
            glyph.pixels[static_cast<std::size_t>(i)] = bitmap[i] >= 96 ? PaletteIndex::textRemap0 : PaletteIndex::transparent;
        }
        stbtt_FreeBitmap(bitmap, nullptr);

        return _glyphCache.emplace(key, std::move(glyph)).first->second;
    }

    int16_t getAdvanceWidth(Font font, uint32_t codepoint)
    {
        return getGlyph(font, codepoint).advanceWidth;
    }

    void draw(DrawingContext& ctx, Ui::Point& pos, Font font, uint32_t codepoint, PaletteMap::View palette, int8_t yOffset)
    {
        const auto& glyph = getGlyph(font, codepoint);
        if (!glyph.pixels.empty())
        {
            G1Element element;
            element.offset = const_cast<uint8_t*>(glyph.pixels.data());
            element.width = glyph.width;
            element.height = glyph.height;
            element.xOffset = glyph.xOffset;
            element.yOffset = static_cast<int16_t>(glyph.yOffset + yOffset);
            element.flags = G1ElementFlags::hasTransparency;
            ctx.drawPaletteBitmap(pos, element, palette);
            pos.x += glyph.advanceWidth;
            return;
        }

        const auto chrImage = getImageForCharacter(font, '?');
        ctx.drawImagePaletteSet(pos + Ui::Point(0, yOffset), chrImage.withPrimary(Colour::black), palette, {});
        pos.x += getCharacterWidth(font, '?');
    }
}
