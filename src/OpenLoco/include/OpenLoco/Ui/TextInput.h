#pragma once

#include "Graphics/Gfx.h"

#include <Localisation/Conversion.h>
#include <cstdint>
#include <string>

namespace OpenLoco::Ui::TextInput
{
    constexpr int16_t textboxPadding = 4;

    struct InputSession
    {
        std::string buffer;    // in UTF-8
        size_t cursorPosition; // 0x01136FA2
        int16_t xOffset;       // 0x01136FA4
        uint8_t cursorFrame;   // 0x011370A9
        uint32_t inputLenLimit;

        InputSession() = default;

        // initialString is in Locomotion's encoding
        InputSession(const std::string initialString, uint32_t inputSize)
        {
            buffer = Localisation::convertLocoToUnicode(initialString);
            cursorPosition = length();
            cursorFrame = 0;
            xOffset = 0;
            inputLenLimit = inputSize;
        };

        bool handleInput(uint32_t charCode, uint32_t keyCode);
        bool needsReoffsetting(int16_t containerWidth);
        void calculateTextOffset(int16_t containerWidth);
        void clearInput();
        std::string loco() const; // Returns the inputted text in Locomotion's encoding
        std::string utf8() const; // Returns the inputted text in UTF-8 encoding
        size_t length() const;
    };
}
