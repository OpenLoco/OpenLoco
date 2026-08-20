#include "Graphics/UnicodeFont.h"
#include "Environment.h"
#include "Graphics/Colour.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageId.h"
#include "Logging.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
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
        bool preferHangul = false;
        bool preferCjk = false;
        int nativePx = 0;
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

    static bool isHangul(uint32_t cp)
    {
        return (cp >= 0x1100 && cp <= 0x11FF) || (cp >= 0x3130 && cp <= 0x318F) || (cp >= 0xA960 && cp <= 0xA97F)
            || (cp >= 0xAC00 && cp <= 0xD7FF);
    }

    static bool isCjk(uint32_t cp)
    {
        return (cp >= 0x3040 && cp <= 0x30FF) || (cp >= 0x31F0 && cp <= 0x31FF) || (cp >= 0x3400 && cp <= 0x4DBF)
            || (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFF66 && cp <= 0xFF9D);
    }

    static fs::path bundledFontsDir()
    {
        return Environment::getPathNoWarning(Environment::PathId::languageFiles).parent_path() / "fonts";
    }

    static void tryLoadFontFile(const fs::path& path, bool preferHangul, bool preferCjk, int nativePx = 0)
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
            font.preferHangul = preferHangul;
            font.preferCjk = preferCjk;
            font.nativePx = nativePx;
            if (stbtt_InitFont(&font.info, font.data->data(), offset) != 0)
            {
                Logging::info("Loaded Unicode font: {}", path.filename().string());
                _fonts.push_back(std::move(font));
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

        const auto bundled = bundledFontsDir();
        tryLoadFontFile(bundled / "Galmuri7.ttf", true, true, 8);
        tryLoadFontFile(bundled / "Galmuri9.ttf", true, true, 10);
        tryLoadFontFile(bundled / "Galmuri14.ttf", true, true, 15);
        tryLoadFontFile(bundled / "A2Z-Bold.ttf", true, false);
#ifdef _WIN32
        const bool hasHangulFont = std::any_of(_fonts.begin(), _fonts.end(), [](const LoadedFont& font) { return font.preferHangul; });
        if (!hasHangulFont)
        {
            const auto localAppData = Platform::getEnvironmentVariable("LOCALAPPDATA");
            if (!localAppData.empty())
            {
                tryLoadFontFile(fs::path(localAppData) / "Microsoft/Windows/Fonts/에이투지체-7Bold.ttf", true, false);
            }
        }
#endif
        tryLoadFontFile(bundled / "NotoSansCJKjp-Regular.otf", false, true);
        tryLoadFontFile(bundled / "NotoSansCJKsc-Regular.otf", false, true);

#ifdef _WIN32
        fs::path fontsDir = "C:/Windows/Fonts";
        const auto windir = Platform::getEnvironmentVariable("WINDIR");
        if (!windir.empty())
        {
            fontsDir = fs::path(windir) / "Fonts";
        }
        tryLoadFontFile(fontsDir / "malgunbd.ttf", true, false);
        tryLoadFontFile(fontsDir / "YuGothM.ttc", false, true);
        tryLoadFontFile(fontsDir / "msyh.ttc", false, true);
        tryLoadFontFile(fontsDir / "segoeui.ttf", false, false);
#elif defined(__APPLE__) && defined(__MACH__)
        tryLoadFontFile("/System/Library/Fonts/AppleSDGothicNeo.ttc", true, false);
        tryLoadFontFile("/System/Library/Fonts/Hiragino Sans GB.ttc", false, true);
#else
        tryLoadFontFile("/usr/share/fonts/truetype/nanum/NanumGothicBold.ttf", true, false);
        tryLoadFontFile("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", false, true);
#endif

        if (_fonts.empty() && !_loggedMissingFont)
        {
            _loggedMissingFont = true;
            Logging::warn("No Unicode font found; CJK text will still show as '?'");
        }
    }

    static const LoadedFont* findFontWithGlyph(uint32_t codepoint, int pixelHeight)
    {
        initialise();
        const bool wantHangul = isHangul(codepoint);
        const bool wantCjk = isCjk(codepoint);

        const auto hasGlyph = [codepoint](const LoadedFont& font) {
            return stbtt_FindGlyphIndex(&font.info, static_cast<int>(codepoint)) != 0;
        };

        const auto pickPreferred = [&](bool LoadedFont::*flag) -> const LoadedFont* {
            const LoadedFont* best = nullptr;
            int bestDist = 1000;
            for (const auto& font : _fonts)
            {
                if (!(font.*flag) || !hasGlyph(font))
                {
                    continue;
                }
                const int dist = font.nativePx > 0 ? std::abs(font.nativePx - pixelHeight) : 50;
                if (dist < bestDist)
                {
                    best = &font;
                    bestDist = dist;
                }
            }
            return best;
        };

        if (wantHangul)
        {
            if (const auto* best = pickPreferred(&LoadedFont::preferHangul))
            {
                return best;
            }
        }
        if (wantCjk)
        {
            if (const auto* best = pickPreferred(&LoadedFont::preferCjk))
            {
                return best;
            }
        }
        for (const auto& font : _fonts)
        {
            if (hasGlyph(font))
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
        const int pixelHeight = pixelHeightFor(font);
        const auto* loaded = findFontWithGlyph(codepoint, pixelHeight);
        if (loaded == nullptr)
        {
            glyph.advanceWidth = getCharacterWidth(font, '?');
            return _glyphCache.emplace(key, std::move(glyph)).first->second;
        }

        const int rasterHeight = loaded->nativePx > 0 ? loaded->nativePx : pixelHeight;
        // Pixel fonts and CJK outlines are drawn to the em square.
        // ScaleForPixelHeight also fits unused descent, which squashes glyphs.
        const float scale = stbtt_ScaleForMappingEmToPixels(&loaded->info, static_cast<float>(rasterHeight));

        int advanceWidth = 0;
        int leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&loaded->info, static_cast<int>(codepoint), &advanceWidth, &leftSideBearing);
        (void)leftSideBearing;
        glyph.advanceWidth = static_cast<int16_t>(std::max(1, static_cast<int>(std::lround(advanceWidth * scale))));

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
        glyph.yOffset = static_cast<int16_t>(std::lround(ascent * scale + yoff));
        glyph.pixels.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
        for (int i = 0; i < w * h; ++i)
        {
            glyph.pixels[static_cast<std::size_t>(i)] = bitmap[i] >= 48 ? PaletteIndex::textRemap0 : PaletteIndex::transparent;
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
