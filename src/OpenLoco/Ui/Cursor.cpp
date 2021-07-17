#include "Cursor.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string_view>

using namespace OpenLoco::Ui;

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

int _main()
{
    Cursor cursor1 = Cursor(
        1, 2, "                                "
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
    return 0;
}
