#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string_view>

constexpr uint16_t cursorDimSize = 32;
constexpr uint16_t rawCursorSize = cursorDimSize * cursorDimSize;
constexpr uint16_t encodedCursorSize = rawCursorSize / 8;

struct Cursor
{
    uint8_t data[encodedCursorSize];
    uint8_t mask[encodedCursorSize];
};

constexpr char rawTransparent = ' ';
constexpr char rawWhite = '.';
constexpr char rawBlack = 'X';

constexpr uint8_t pixWhite = 1;
constexpr uint8_t pixBlack = 0;

constexpr uint8_t maskTransparent = 0;
constexpr uint8_t maskOpaque = 1;

static constexpr Cursor convertCursor(std::string_view bitmap)
{
    assert(bitmap.length() == rawCursorSize);

    Cursor cursor{};
    uint8_t curBit{};
    uint16_t curPixel{};
    uint8_t dataByte{}, maskByte{};
    for (uint8_t rawPixel : bitmap)
    {
        // Convert raw pixel character into monochrome pixel data.
        uint8_t pix = pixBlack;
        if (rawPixel == rawWhite)
            pix = pixWhite;
        dataByte |= (pix << curBit);

        // Second, create a visibility mask.
        uint8_t mask = maskTransparent;
        if (rawPixel != rawTransparent)
            mask = maskOpaque;
        maskByte |= (mask << curBit);

        // Save data once 8 chars have been processed.
        curBit = (curBit + 1) % 8;
        if (curBit == 0)
        {
            cursor.data[curPixel] = dataByte;
            cursor.mask[curPixel] = maskByte;

            dataByte = 0;
            maskByte = 0;
            curPixel++;
        }
    }

    // Set the final bytes.
    cursor.data[curPixel] = dataByte;
    cursor.mask[curPixel] = maskByte;

    return cursor;
}

static void printCursorData(const Cursor cursor)
{
    uint16_t pixIndex{};
    for (uint8_t rawByte : cursor.data)
    {
        // clang-format off
        printf("%c%c%c%c%c%c%c%c",
            (rawByte & (1 << 0) ? rawBlack : rawWhite),
            (rawByte & (1 << 1) ? rawBlack : rawWhite),
            (rawByte & (1 << 2) ? rawBlack : rawWhite),
            (rawByte & (1 << 3) ? rawBlack : rawWhite),
            (rawByte & (1 << 4) ? rawBlack : rawWhite),
            (rawByte & (1 << 5) ? rawBlack : rawWhite),
            (rawByte & (1 << 6) ? rawBlack : rawWhite),
            (rawByte & (1 << 7) ? rawBlack : rawWhite)
        );
        // clang-format on

        pixIndex = (pixIndex + 1) % 4;
        if (pixIndex == 0)
            printf("\n");
    }
}

static void printCursorMask(const Cursor cursor)
{
    uint16_t pixIndex{};
    for (uint8_t rawByte : cursor.mask)
    {
        // clang-format off
        printf("%c%c%c%c%c%c%c%c",
            (rawByte & (1 << 0) ? rawBlack : rawTransparent),
            (rawByte & (1 << 1) ? rawBlack : rawTransparent),
            (rawByte & (1 << 2) ? rawBlack : rawTransparent),
            (rawByte & (1 << 3) ? rawBlack : rawTransparent),
            (rawByte & (1 << 4) ? rawBlack : rawTransparent),
            (rawByte & (1 << 5) ? rawBlack : rawTransparent),
            (rawByte & (1 << 6) ? rawBlack : rawTransparent),
            (rawByte & (1 << 7) ? rawBlack : rawTransparent)
        );
        // clang-format on

        pixIndex = (pixIndex + 1) % 4;
        if (pixIndex == 0)
            printf("\n");
    }
}

static int test()
{
    Cursor cursor1 = convertCursor(
        "                                "
        "                                "
        "               XXXXXXXXXXXXX    "
        "              X.............X   "
        "           X  X.XX..XX..XX..X   "
        "          XXXXX.X...X...X...X   "
        "         X....X.X...X...X...X   "
        "         XXX..X.X...X...X...X   "
        "      XXXXXX..X.X...X...X...X   "
        "     X........X.XX..XX..XX..X   "
        "     X........X.............X   "
        "     X..XX....XXXXXXXXXXXX.X    "
        "     X.XX.X..X...X  XX.XX X     "
        "     XXX.X.XXXXXXX  X.X.X X     "
        "       XX.X         XX.X        "
        "        XX           XX         "
        "                                "
        "    XXXXX                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "    X...X                       "
        "XXXXX...XXXXX                   "
        " X.........X                    "
        "  X.......X                     "
        "   X.....X                      "
        "    X...X                       "
        "     X.X                        "
        "      X                         ");

    printCursorData(cursor1);
    printCursorMask(cursor1);
}
