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

        static Cursor pointer;
        static Cursor blank;
        static Cursor upArrow;
        static Cursor upDownArrow;
        static Cursor handPointer;
        static Cursor busy;
        static Cursor diagonalArrows;
        static Cursor cursor124;
        static Cursor cursor131;
        static Cursor cursor127;
        static Cursor cursor128;
        static Cursor cursor129;
        static Cursor cursor130;
        static Cursor cursor132;
        static Cursor cursor133;
        static Cursor cursor138;
        static Cursor cursor137;
        static Cursor cursor139;
        static Cursor cursor141;
        static Cursor cursor142;
        static Cursor cursor143;
        static Cursor cursor144;
        static Cursor cursor145;
        static Cursor cursor158;
        static Cursor cursor159;
        static Cursor cursor161;
        static Cursor cursor160;
        static Cursor cursor163;
        static Cursor cursor162;
        static Cursor cursor173;
        static Cursor cursor172;
        static Cursor cursor178;
        static Cursor cursor177;
        static Cursor cursor175;
        static Cursor cursor176;
        static Cursor cursor180;
        static Cursor cursor179;
        static Cursor inwardArrows;
        static Cursor cursor169;
        static Cursor cursor168;
        static Cursor cursor170;
        static Cursor cursor184;
        static Cursor cursor185;
        static Cursor cursor186;
        static Cursor cursor189;

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
