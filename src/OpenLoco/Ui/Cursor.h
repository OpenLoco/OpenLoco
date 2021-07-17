#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace OpenLoco::Ui
{
    constexpr char rawTransparent = ' ';
    constexpr char rawWhite = '.';
    constexpr char rawBlack = 'X';

    constexpr uint16_t cursorDimSize = 32;
    constexpr uint16_t rawCursorSize = cursorDimSize * cursorDimSize;
    constexpr uint16_t encodedCursorSize = rawCursorSize / 8;

    class Cursor
    {
    public:
        int x;
        int y;
        uint8_t data[encodedCursorSize]{};
        uint8_t mask[encodedCursorSize]{};

    public:
        constexpr Cursor(int x, int y, std::string_view bitmap);
    };

    constexpr Cursor::Cursor(int x, int y, std::string_view bitmap)
        : x(x)
        , y(y)
    {
        assert(bitmap.length() == rawCursorSize);

        this->x = x;
        this->y = y;
        uint8_t curBit{};
        uint16_t curPixel{};
        uint8_t dataByte{}, maskByte{};
        for (char rawPixel : bitmap)
        {
            // Default is transparent
            uint8_t dataBit = 0;
            uint8_t maskBit = 0;

            switch (rawPixel)
            {
                case rawBlack:
                    dataBit = 1;
                    maskBit = 1;
                    break;
                case rawWhite:
                    dataBit = 0;
                    maskBit = 1;
                    break;
            }

            dataByte |= (dataBit << (7 - curBit));
            maskByte |= (maskBit << (7 - curBit));

            // Save dataBit once 8 chars have been processed.
            curBit = (curBit + 1) % 8;
            if (curBit == 0)
            {
                this->data[curPixel] = dataByte;
                this->mask[curPixel] = maskByte;

                dataByte = 0;
                maskByte = 0;
                curPixel++;
            }
        }

        // Set the final bytes.
        this->data[curPixel] = dataByte;
        this->mask[curPixel] = maskByte;
    }
}
