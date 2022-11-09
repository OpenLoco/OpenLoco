#pragma once

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

        static Cursor blank;
        static Cursor upArrow;
        static Cursor upDownArrow;
        static Cursor busy;
        static Cursor diagonalArrows;
        static Cursor picker;        // 124
        static Cursor plantTree;     // 131
        static Cursor placeFountain; // 127
        static Cursor placeStatue;   // 128
        static Cursor placeBench;    // 129
        static Cursor crosshair;     // 130
        static Cursor placeTrashBin; // 132
        static Cursor placeLantern;  // 133
        static Cursor placeFence;    // 138
        static Cursor placeFlowers;  // 137
        static Cursor placePath;     // 139
        static Cursor landTool;      // 141
        static Cursor waterTool;     // 142
        static Cursor placeHome;     // 143
        static Cursor placeVolcano;  // 144
        static Cursor footsteps;     // 145
        static Cursor brush;         // 158
        static Cursor placeBanner;   // 159
        static Cursor openHand;      // 161
        static Cursor dragHand;      // 160
        static Cursor placeTrain;    // 163
        static Cursor placeTrainAlt; // 162
        static Cursor placeBus;      // 173
        static Cursor placeBusAlt;   // 172
        static Cursor placeTruck;    // 178
        static Cursor placeTruckAlt; // 177
        static Cursor placeTram;     // 175
        static Cursor placeTramAlt;  // 176
        static Cursor placePlane;    // 180
        static Cursor placeShip;     // 179
        static Cursor inwardArrows;
        static Cursor placeTown;     // 169
        static Cursor placeBuilding; // 168
        static Cursor placeFactory;  // 170
        static Cursor bulldozerTool; // 184
        static Cursor placeSignal;   // 185
        static Cursor placeHQ;       // 186
        static Cursor placeStation;  // 189

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
    }
}
