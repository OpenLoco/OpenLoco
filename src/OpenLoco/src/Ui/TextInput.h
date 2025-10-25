#pragma once

#include "Graphics/Gfx.h"

#include <cstdint>
#include <string>
#include <Localisation/Conversion.h>

namespace OpenLoco::Ui::TextInput
{
    constexpr int16_t textboxPadding = 4;

    struct InputSession
    {
        std::u32string buffer32; // in UTF-32
        size_t cursorPosition; // 0x01136FA2
        int16_t xOffset;       // 0x01136FA4
        uint8_t cursorFrame;   // 0x011370A9
        uint32_t inputLenLimit;

        InputSession() = default;

        // initialString is in Locomotion's 8-bit encoding
        InputSession(const std::string initialString, uint32_t inputSize)
        {
            buffer32 = Localisation::convertLocoToUnicode32(initialString);
            cursorPosition = buffer32.length();
            cursorFrame = 0;
            xOffset = 0;
            inputLenLimit = inputSize;
        };

        bool handleInput(uint32_t charCode, uint32_t keyCode);
        bool needsReoffsetting(int16_t containerWidth);
        void calculateTextOffset(int16_t containerWidth);
        void clearInput();
        void sanitizeInput();
        std::string getLocoString() const;
    };
}
