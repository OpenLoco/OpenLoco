#include "Gfx.h"
#include "Colour.h"
#include "Config.h"
#include "Environment.h"
#include "Font.h"
#include "Graphics/DrawSprite.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "ImageIds.h"
#include "Input.h"
#include "Localisation/Formatting.h"
#include "Localisation/LanguageFiles.h"
#include "Localisation/StringManager.h"
#include "Logging.h"
#include "Objects/CurrencyObject.h"
#include "Objects/ObjectManager.h"
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

    static int32_t getFontBaseIndex(Font font)
    {
        // The Font type currently encodes the index, this may change in the future.
        return enumValue(font);
    }

    static uint32_t getImageIdForCharacter(Font font, uint8_t character)
    {
        return ImageIds::characters_medium_normal_space + (character - 32) + getFontBaseIndex(font);
    }

    // 0x004949BC
    static void initialiseCharacterWidths()
    {
        struct FontEntry
        {
            Font offset;
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
            for (uint8_t i = 0; i < 224; ++i)
            {
                uint8_t chr = i + 32;
                auto* element = getG1Element(getImageIdForCharacter(font.offset, chr));
                if (element == nullptr)
                {
                    continue;
                }
                auto width = element->width + font.widthFudge;
                // Characters from 123 to 150 are unused
                // Unsure why this zeros it out though since a negative width isn't an issue
                if (chr >= 123 && chr <= 150)
                {
                    width = 0;
                }
                _characterWidths[enumValue(font.offset) + i] = width;
            }
        }
        // Vanilla setup scrolling text related globals here (unused)
    }

    void initialise()
    {
        initialiseCharacterWidths();

        loadDefaultPalette();

        auto& drawingEngine = getDrawingEngine();
        auto& drawingCtx = drawingEngine.getDrawingContext();
        drawingCtx.clearSingle(PaletteIndex::black0);
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

    // 0x004CF63B
    // TODO: Split this into two functions, one for rendering and one for processing messages.
    void renderAndUpdate()
    {
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
        const auto* currencyObject = ObjectManager::get<CurrencyObject>();
        if (currencyObject == nullptr)
        {
            return;
        }

        const auto loadCurrencySymbol = [](const auto font, const auto imageId, const auto dstImageId, int offsetWidth) {
            const auto* image = getG1Element(imageId);
            if (image == nullptr)
            {
                return;
            }

            auto* currencyElement = getG1Element(dstImageId);
            if (currencyElement == nullptr)
            {
                return;
            }

            Gfx::setCharacterWidth(font, U'£', image->width + offsetWidth);
            *currencyElement = *image;
        };

        loadCurrencySymbol(Gfx::Font::small, currencyObject->objectIcon, ImageIds::characters_small_currency_sign, -1);
        loadCurrencySymbol(Gfx::Font::medium_normal, currencyObject->objectIcon + 1, ImageIds::characters_medium_normal_currency_sign, -1);
        loadCurrencySymbol(Gfx::Font::medium_bold, currencyObject->objectIcon + 2, ImageIds::characters_medium_bold_currency_sign, -1);
        loadCurrencySymbol(Gfx::Font::large, currencyObject->objectIcon + 3, ImageIds::characters_large_currency_sign, 1);

        invalidateScreen();
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
        drawingCtx.pushRenderTarget(rt);

        // Draw all the images on top of the one bitmap
        for (size_t i = 0; i < numImages; ++i)
        {
            drawingCtx.drawImage({ 0, 0 }, baseImageId.withIndexOffset(i));
        }

        drawingCtx.popRenderTarget();

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
            {
                break;
            }

            for (int32_t j = 0; j < 200; j++)
            {
                if (bitmap[j][100 + i] != 0)
                {
                    spriteWidth = i;
                    break;
                }
            }

            if (spriteWidth != -1)
            {
                break;
            }
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
            {
                break;
            }
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
            {
                break;
            }
        }
        spriteHeightPositive++;

        return ImageExtents{ static_cast<uint8_t>(spriteWidth), static_cast<uint8_t>(spriteHeightNegative), static_cast<uint8_t>(spriteHeightPositive) };
    }

    int16_t getCharacterWidth(Font font, char32_t character)
    {
        return _characterWidths[getFontBaseIndex(font) + character - 32];
    }

    void setCharacterWidth(Font font, char32_t character, int16_t width)
    {
        _characterWidths[getFontBaseIndex(font) + character - 32] = width;
    }

    ImageId getImageForCharacter(Font font, char32_t character)
    {
        const auto imageId = getImageIdForCharacter(font, character);
        return ImageId(imageId);
    }

    // 0x00451DCB
    void movePixelsOnScreen(int16_t dstX, int16_t dstY, int16_t width, int16_t height, int16_t srcX, int16_t srcY)
    {
        auto& drawingEngine = getDrawingEngine();
        auto& screenRT = drawingEngine.getScreenRT();

        drawingEngine.movePixels(screenRT, dstX, dstY, width, height, srcX, srcY);
    }
}
