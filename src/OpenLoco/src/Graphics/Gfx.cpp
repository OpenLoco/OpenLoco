#include "Gfx.h"
#include "Colour.h"
#include "Config.h"
#include "Environment.h"
#include "Graphics/DrawSprite.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ImageIds.h"
#include "Input.h"
#include "Localisation/Formatting.h"
#include "Localisation/LanguageFiles.h"
#include "Localisation/StringManager.h"
#include "Logging.h"
#include "PaletteMap.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Core/Stream.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Utility;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Gfx
{
    constexpr uint32_t kG1CountTemporary = 0x1000;

    static loco_global<G1Element[G1ExpectedCount::kDisc + kG1CountTemporary + G1ExpectedCount::kObjects], 0x9E2424> _g1Elements;

    static std::unique_ptr<std::byte[]> _g1Buffer;

    static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;

    // 0x004FFAE8
    ImageId applyGhostToImage(uint32_t imageIndex)
    {
        if (Config::get().old.constructionMarker)
        {
            return ImageId(imageIndex).withTranslucency(ExtColour::unk31);
        }
        else
        {
            return ImageId(imageIndex, ExtColour::unk2C);
        }
    }

    static std::vector<G1Element> convertElements(const std::vector<G1Element32>& elements32)
    {
        auto elements = std::vector<G1Element>();
        elements.reserve(elements32.size());
        std::transform(
            elements32.begin(),
            elements32.end(),
            std::back_inserter(elements),
            [](G1Element32 src) { return G1Element(src); });
        return elements;
    }

    // 0x0044733C
    void loadG1()
    {
        auto g1Path = Environment::getPath(Environment::PathId::g1);
        std::ifstream stream(g1Path, std::ios::in | std::ios::binary);
        if (!stream)
        {
            throw Exception::RuntimeError("Opening g1 file failed.");
        }

        G1Header header;
        if (!readData(stream, header))
        {
            throw Exception::RuntimeError("Reading g1 file header failed.");
        }

        if (header.numEntries != G1ExpectedCount::kDisc)
        {
            if (header.numEntries == G1ExpectedCount::kSteam)
            {
                Logging::verbose("Got Steam G1.DAT variant, will fix elements automatically.");
            }
            else
            {
                Logging::warn("G1 element count doesn't match expected value:\nExpected {}; Got {}", G1ExpectedCount::kDisc, header.numEntries);
            }
        }

        // Read element headers
        auto elements32 = std::vector<G1Element32>(header.numEntries);
        if (!readData(stream, elements32.data(), header.numEntries))
        {
            throw Exception::RuntimeError("Reading g1 element headers failed.");
        }
        auto elements = convertElements(elements32);

        // Read element data
        auto elementData = std::make_unique<std::byte[]>(header.totalSize);
        if (!readData(stream, elementData.get(), header.totalSize))
        {
            throw Exception::RuntimeError("Reading g1 elements failed.");
        }
        stream.close();

        // The steam G1.DAT is missing two localised tutorial icons, and a smaller font variant
        // This code copies the closest variants into their place, and moves other elements accordingly
        if (header.numEntries == G1ExpectedCount::kSteam)
        {
            // Temporarily convert offsets to absolute indexes
            for (size_t i = 0; i < elements.size(); i++)
            {
                if (elements[i].hasFlags(G1ElementFlags::hasZoomSprites))
                {
                    elements[i].zoomOffset = static_cast<int16_t>(i - elements[i].zoomOffset);
                }
            }

            elements.resize(G1ExpectedCount::kDisc);

            // Extra two tutorial images
            std::copy_n(&elements[3549], header.numEntries - 3549, &elements[3551]);
            std::copy_n(&elements[3551], 1, &elements[3549]);
            std::copy_n(&elements[3551], 1, &elements[3550]);

            // Extra font variant
            std::copy_n(&elements[1788], 223, &elements[3898]);

            // Restore relative offsets
            for (size_t i = 0; i < elements.size(); i++)
            {
                if (elements[i].hasFlags(G1ElementFlags::hasZoomSprites))
                {
                    elements[i].zoomOffset = static_cast<int16_t>(i - elements[i].zoomOffset);
                }
            }
        }

        // Adjust memory offsets
        for (auto& element : elements)
        {
            element.offset += (uintptr_t)elementData.get();
        }

        _g1Buffer = std::move(elementData);
        std::copy(elements.begin(), elements.end(), _g1Elements.get());
    }

    // 0x004949BC
    void initialiseCharacterWidths()
    {
        struct FontEntry
        {
            int16_t offset;
            int8_t widthFudge;
        };
        constexpr std::array<FontEntry, 4> fonts = {
            FontEntry{ Font::medium_normal, -1 },
            FontEntry{ Font::medium_bold, -1 },
            FontEntry{ Font::small, -1 },
            FontEntry{ Font::large, 1 },
        };

        for (const auto& font : fonts)
        {
            // Supported character range is from 32 -> 255
            for (auto i = 0; i < 224; ++i)
            {
                auto c = i + 32;
                // TODO: Use an indirection to convert from character to imageId
                auto* element = getG1Element(ImageIds::characters_medium_normal_space + i + font.offset);
                if (element == nullptr)
                {
                    continue;
                }
                auto width = element->width + font.widthFudge;
                // Characters from 123 to 150 are unused
                // Unsure why this zeros it out though since a negative width isn't an issue
                if (c >= '{' && c <= '\x96')
                {
                    width = 0;
                }
                _characterWidths[font.offset + i] = width;
            }
        }
        // Vanilla setup scrolling text related globals here (unused)
    }

    // 0x00452336
    void initialiseNoiseMaskMap()
    {
        call(0x00452336);
    }

    // 0x004CD406
    void invalidateScreen()
    {
        invalidateRegion(0, 0, Ui::width(), Ui::height());
    }

    static std::unique_ptr<Gfx::SoftwareDrawingEngine> engine;

    Gfx::SoftwareDrawingEngine& getDrawingEngine()
    {
        if (!engine)
        {
            engine = std::make_unique<Gfx::SoftwareDrawingEngine>();
        }
        return *engine;
    }

    /**
     * 0x004C5C69
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void invalidateRegion(int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        getDrawingEngine().invalidateRegion(left, top, right, bottom);
    }

    // 0x004C5CFA
    void render()
    {
        getDrawingEngine().render();
    }

    loco_global<char[512], 0x0112CC04> _byte_112CC04;
    loco_global<char[512], 0x0112CE04> _byte_112CE04;

    // 0x004CF63B
    // TODO: Split this into two functions, one for rendering and one for processing messages.
    void renderAndUpdate()
    {
        char backup1[512] = { 0 };
        char backup2[512] = { 0 };

        std::memcpy(backup1, _byte_112CC04, 512);
        std::memcpy(backup2, _byte_112CE04, 512);

        if (Ui::dirtyBlocksInitialised())
        {
            auto& drawingEngine = Gfx::getDrawingEngine();
            drawingEngine.render();
            drawingEngine.present();
        }

        if (Input::hasFlag(Input::Flags::rightMousePressed))
        {
            Ui::processMessagesMini();
        }
        else
        {
            Ui::processMessages();
        }

        if (addr<0x005252AC, uint32_t>() != 0)
        {
            //            sub_4058F5();
        }

        std::memcpy(_byte_112CC04, backup1, 512);
        std::memcpy(_byte_112CE04, backup2, 512);
    }

    void render(Rect rect)
    {
        getDrawingEngine().render(rect);
    }

    /**
     * 0x004C5DD5
     * rct2: window_draw_all
     *
     * @param left @<ax>
     * @param top @<bx>
     * @param right @<dx>
     * @param bottom @<bp>
     */
    void render(int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        render(Rect::fromLTRB(left, top, right, bottom));
    }

    uint32_t recolour(uint32_t image)
    {
        return ImageIdFlags::remap | image;
    }

    uint32_t recolour(uint32_t image, ExtColour colour)
    {
        return ImageIdFlags::remap | (enumValue(colour) << 19) | image;
    }
    uint32_t recolour(uint32_t image, Colour colour)
    {
        return recolour(image, Colours::toExt(colour));
    }

    uint32_t recolour2(uint32_t image, Colour colour1, Colour colour2)
    {
        return ImageIdFlags::remap | ImageIdFlags::remap2 | (enumValue(colour1) << 19) | (enumValue(colour2) << 24) | image;
    }

    uint32_t recolour2(uint32_t image, ColourScheme colourScheme)
    {
        return recolour2(image, colourScheme.primary, colourScheme.secondary);
    }

    uint32_t recolourTranslucent(uint32_t image, ExtColour colour)
    {
        return ImageIdFlags::translucent | (enumValue(colour) << 19) | image;
    }

    G1Element* getG1Element(uint32_t imageId)
    {
        const auto id = getImageIndex(imageId);
        if (id < _g1Elements.size())
        {
            return &_g1Elements[id];
        }
        return nullptr;
    }

    static loco_global<Gfx::PaletteEntry[256], 0x0113ED20> _113ED20;

    // 0x0046E07B
    void loadCurrency()
    {
        call(0x0046E07B);
    }

    // 0x00452457
    void loadPalette(uint32_t imageIndex, uint8_t modifier)
    {
        auto* g1Palette = getG1Element(imageIndex);
        if (g1Palette == nullptr)
        {
            return;
        }
        uint8_t* colourData = g1Palette->offset;
        for (auto i = g1Palette->xOffset; i < g1Palette->width + g1Palette->xOffset; ++i)
        {
            _113ED20[i].b = (*colourData++ * modifier) / 256;
            _113ED20[i].g = (*colourData++ * modifier) / 256;
            _113ED20[i].r = (*colourData++ * modifier) / 256;
        }
        getDrawingEngine().updatePalette(_113ED20, 10, 236);
    }

    // 0x004523F4
    void loadDefaultPalette()
    {
        auto* g1Palette = getG1Element(ImageIds::default_palette);
        uint8_t* colourData = g1Palette->offset;
        for (auto i = g1Palette->xOffset; i < g1Palette->width + g1Palette->xOffset; ++i)
        {
            _113ED20[i].b = *colourData++;
            _113ED20[i].g = *colourData++;
            _113ED20[i].r = *colourData++;
        }
        getDrawingEngine().updatePalette(_113ED20, 10, 236);
    }

    // 0x004530F8
    ImageExtents getImagesMaxExtent(const ImageId baseImageId, const size_t numImages)
    {
        uint8_t bitmap[200][200] = {};

        RenderTarget rt = {
            /*.bits = */ reinterpret_cast<uint8_t*>(bitmap),
            /*.x = */ -100,
            /*.y = */ -100,
            /*.width = */ 200,
            /*.height = */ 200,
            /*.pitch = */ 0,
            /*.zoom_level = */ 0,
        };

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // Draw all the images on top of the one bitmap
        for (size_t i = 0; i < numImages; ++i)
        {
            drawingCtx.drawImage(rt, { 0, 0 }, baseImageId.withIndexOffset(i));
        }

        // Explore the bitmap to find the extents of the images drawn
        int32_t spriteWidth = -1;
        for (int32_t i = 99; i != 0; --i)
        {
            for (int32_t j = 0; j < 200; j++)
            {
                if (bitmap[j][100 - i] != 0)
                {
                    spriteWidth = i;
                    break;
                }
            }

            if (spriteWidth != -1)
                break;

            for (int32_t j = 0; j < 200; j++)
            {
                if (bitmap[j][100 + i] != 0)
                {
                    spriteWidth = i;
                    break;
                }
            }

            if (spriteWidth != -1)
                break;
        }

        spriteWidth++;
        int32_t spriteHeightNegative = -1;

        for (int32_t i = 99; i != 0; --i)
        {
            for (int32_t j = 0; j < 200; j++)
            {
                if (bitmap[100 - i][j] != 0)
                {
                    spriteHeightNegative = i;
                    break;
                }
            }

            if (spriteHeightNegative != -1)
                break;
        }
        spriteHeightNegative++;

        int32_t spriteHeightPositive = -1;

        for (int32_t i = 99; i != 0; --i)
        {
            for (int32_t j = 0; j < 200; j++)
            {
                if (bitmap[100 + i][j] != 0)
                {
                    spriteHeightPositive = i;
                    break;
                }
            }

            if (spriteHeightPositive != -1)
                break;
        }
        spriteHeightPositive++;

        return ImageExtents{ static_cast<uint8_t>(spriteWidth), static_cast<uint8_t>(spriteHeightNegative), static_cast<uint8_t>(spriteHeightPositive) };
    }
}
