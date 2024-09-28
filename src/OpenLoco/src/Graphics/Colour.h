#pragma once

#include "Types.hpp"

namespace OpenLoco
{
    using PaletteIndex_t = uint8_t;

    enum class Colour : uint8_t
    {
        black = 0,
        grey = 1,
        white = 2,
        mutedDarkPurple = 3,
        mutedPurple = 4,
        purple = 5,
        darkBlue = 6,
        blue = 7,
        mutedDarkTeal = 8,
        mutedTeal = 9,
        darkGreen = 10,
        mutedSeaGreen = 11,
        mutedGrassGreen = 12,
        green = 13,
        mutedAvocadoGreen = 14,
        mutedOliveGreen = 15,
        yellow = 16,
        darkYellow = 17,
        orange = 18,
        amber = 19,
        darkOrange = 20,
        mutedDarkYellow = 21,
        mutedYellow = 22,
        brown = 23,
        mutedOrange = 24,
        mutedDarkRed = 25,
        darkRed = 26,
        red = 27,
        darkPink = 28,
        pink = 29,
        mutedRed = 30,
        max,
    };

    enum class ExtColour : uint8_t
    {
        black = 0,
        grey = 1,
        white = 2,
        mutedDarkPurple = 3,
        mutedPurple = 4,
        purple = 5,
        darkBlue = 6,
        blue = 7,
        mutedDarkTeal = 8,
        mutedTeal = 9,
        darkGreen = 10,
        mutedSeaGreen = 11,
        mutedGrassGreen = 12,
        green = 13,
        mutedAvocadoGreen = 14,
        mutedOliveGreen = 15,
        yellow = 16,
        darkYellow = 17,
        orange = 18,
        amber = 19,
        darkOrange = 20,
        mutedDarkYellow = 21,
        mutedYellow = 22,
        brown = 23,
        mutedOrange = 24,
        mutedDarkRed = 25,
        darkRed = 26,
        red = 27,
        darkPink = 28,
        pink = 29,
        mutedRed = 30,
        // First 30 are inherited from Colour
        clear = 31, // No colour
        water = 32, // Water palette map dynamically loaded from water object
        unk21,
        unk22,
        unk23,
        unk24,
        unk25,
        unk26,
        unk27,
        unk28,
        unk29,
        unk2A,
        unk2B,
        unk2C, // ghost
        unk2D,
        unk2E, // translucentGlass1
        unk2F, // translucentGlass2
        unk30, // translucentGlass0
        unk31, // translucentGhost
        unk32, // shadow
        unk33,
        unk34,
        translucentGrey1, // 0-1 black, grey
        translucentGrey2,
        translucentGrey0,
        translucentBlue1, // 6-7 darkBlue, blue
        translucentBlue2,
        translucentBlue0,
        translucentMutedDarkRed1, // 25 mutedDarkRed
        translucentMutedDarkRed2,
        translucentMutedDarkRed0,
        translucentMutedSeaGreen1, // 11 mutedSeaGreen
        translucentMutedSeaGreen2,
        translucentMutedSeaGreen0,
        translucentMutedPurple1, // 3-4 mutedDarkPurple, mutedPurple
        translucentMutedPurple2,
        translucentMutedPurple0,
        translucentMutedOliveGreen1, // 15 mutedOliveGreen
        translucentMutedOliveGreen2,
        translucentMutedOliveGreen0,
        translucentMutedYellow1, // 21-22 mutedDarkYellow, mutedYellow
        translucentMutedYellow2,
        translucentMutedYellow0,
        translucentYellow1, // 16-17 yellow, darkYellow
        translucentYellow2,
        translucentYellow0,
        translucentMutedGrassGreen1, // 12 mutedGrassGreen
        translucentMutedGrassGreen2,
        translucentMutedGrassGreen0,
        translucentMutedAvocadoGreen1, // 14 mutedAvocadoGreen
        translucentMutedAvocadoGreen2,
        translucentMutedAvocadoGreen0,
        translucentGreen1, // 10, 13 darkGreen, green
        translucentGreen2,
        translucentGreen0,
        translucentMutedOrange1, // 24 mutedOrange
        translucentMutedOrange2,
        translucentMutedOrange0,
        translucentPurple1, // 5 purple
        translucentPurple2,
        translucentPurple0,
        translucentRed1, // 26-27 darkRed, red
        translucentRed2,
        translucentRed0,
        translucentOrange1, // 18, 20 orange, darkOrange
        translucentOrange2,
        translucentOrange0,
        translucentMutedTeal1, // 8-9 mutedDarkTeal, mutedTeal
        translucentMutedTeal2,
        translucentMutedTeal0,
        translucentPink1, // 28-29 pink, darkPink
        translucentPink2,
        translucentPink0,
        translucentBrown1, // 23 brown
        translucentBrown2,
        translucentBrown0,
        translucentMutedRed1, // 30 mutedRed
        translucentMutedRed2,
        translucentMutedRed0,
        translucentWhite1, // 2 white
        translucentWhite2,
        translucentWhite0,
        translucentAmber1, // 19 amber
        translucentAmber2,
        translucentAmber0,
        unk74,
        unk75,
        unk76,
        unk77,
        unk78,
        unk79,
        unk7A,
        unk7B,
        unk7C,
        unk7D,
        unk7E,
        unk7F,
        unk80,
        unk81,
        unk82,
        unk83,
        unk84,
        unk85,
        unk86,
        unk87,
        unk88,
        unk89,
        unk8A,
        unk8B,
        unk8C,
        unk8D,
        unk8E,
        unk8F,
        unk90,
        unk91,
        unk92,
        max,
    };

    struct AdvancedColour
    {
        static constexpr uint8_t outlineFlag = 1 << 5;
        static constexpr uint8_t insetFlag = 1 << 6;
        static constexpr uint8_t translucentFlag = 1 << 7;
        static constexpr uint8_t fd = 0xFD;
        static constexpr uint8_t fe = 0xFE;
        static constexpr uint8_t ff = 0xFF;

    private:
        Colour _c = Colour::black;

    public:
        constexpr AdvancedColour() = default;
        constexpr AdvancedColour(const Colour c)
            : _c(c)
        {
        }

        constexpr explicit operator Colour() const { return static_cast<Colour>(enumValue(_c) & ~(outlineFlag | insetFlag | translucentFlag)); }

        // Returns the Colour without any additional flags set.
        [[nodiscard]] constexpr Colour c() const { return static_cast<Colour>(*this); }

        constexpr explicit operator uint8_t() const { return enumValue(_c); }

        [[nodiscard]] constexpr uint8_t u8() const { return static_cast<uint8_t>(*this); }

        [[nodiscard]] constexpr AdvancedColour outline() const
        {
            return { static_cast<Colour>(enumValue(_c) | outlineFlag) };
        }
        [[nodiscard]] constexpr bool isOutline() const { return enumValue(_c) & outlineFlag; }

        [[nodiscard]] constexpr AdvancedColour inset() const
        {
            return { static_cast<Colour>(enumValue(_c) | insetFlag) };
        }
        [[nodiscard]] constexpr bool isInset() const { return enumValue(_c) & insetFlag; }

        [[nodiscard]] constexpr AdvancedColour translucent() const
        {
            return { static_cast<Colour>(enumValue(_c) | translucentFlag) };
        }
        [[nodiscard]] constexpr bool isTranslucent() const { return enumValue(_c) & translucentFlag; }

        [[nodiscard]] constexpr AdvancedColour opaque() const
        {
            return { static_cast<Colour>(enumValue(_c) & ~translucentFlag) };
        }
        [[nodiscard]] constexpr bool isOpaque() const { return !isTranslucent(); }

        [[nodiscard]] constexpr AdvancedColour clearInset() const
        {
            return { static_cast<Colour>(enumValue(_c) & ~insetFlag) };
        }

        [[nodiscard]] constexpr AdvancedColour clearOutline() const
        {
            return { static_cast<Colour>(enumValue(_c) & ~outlineFlag) };
        }

        [[nodiscard]] static constexpr AdvancedColour FF()
        {
            return { static_cast<Colour>(ff) };
        }
        [[nodiscard]] constexpr bool isFF() const { return enumValue(_c) == ff; }

        [[nodiscard]] static constexpr AdvancedColour FE()
        {
            return { static_cast<Colour>(fe) };
        }
        [[nodiscard]] constexpr bool isFE() const { return enumValue(_c) == fe; }

        [[nodiscard]] static constexpr AdvancedColour FD()
        {
            return { static_cast<Colour>(fd) };
        }
        [[nodiscard]] constexpr bool isFD() const { return enumValue(_c) == fd; }
    };
    static_assert(sizeof(AdvancedColour) == 1);

    namespace Colours
    {
        void initColourMap();
        PaletteIndex_t getShade(Colour colour, uint8_t shade);
        ExtColour getTranslucent(Colour colour);
        ExtColour getTranslucent(Colour colour, uint8_t shade);
        ExtColour getShadow(Colour colour);
        ExtColour getGlass(Colour colour);
        constexpr ExtColour toExt(Colour c) { return static_cast<ExtColour>(c); }
    }

    namespace PaletteIndex
    {
        // All indexes are of the form {swatch colour name}{shade}
        // Shades are from dark 0x0 to light 0xB
        // Most swatches are 0xB in length apart from:
        //   transparent (1),
        //   grey (4),
        //   brightYellow (3),
        //   textRemap (6)
        //
        // There are 3 unused indexes:
        //   textRemap4,
        //   textRemap5,
        //   hotPinkUnused
        // 0xFF is technically unused as well

        constexpr PaletteIndex_t transparent = 0;
        constexpr PaletteIndex_t textRemap0 = 0x01;
        constexpr PaletteIndex_t textRemap1 = 0x02;
        constexpr PaletteIndex_t textRemap2 = 0x03;
        constexpr PaletteIndex_t textRemap3 = 0x04;
        constexpr PaletteIndex_t textRemap4 = 0x05; // Unused
        constexpr PaletteIndex_t textRemap5 = 0x06; // Unused
        constexpr PaletteIndex_t primaryRemap0 = 0x07;
        constexpr PaletteIndex_t primaryRemap1 = 0x08;
        constexpr PaletteIndex_t primaryRemap2 = 0x09;
        constexpr PaletteIndex_t black0 = 0x0A;
        constexpr PaletteIndex_t black1 = 0x0B;
        constexpr PaletteIndex_t black2 = 0x0C;
        constexpr PaletteIndex_t black3 = 0x0D;
        constexpr PaletteIndex_t black4 = 0x0E;
        constexpr PaletteIndex_t black5 = 0x0F;
        constexpr PaletteIndex_t black6 = 0x10;
        constexpr PaletteIndex_t black7 = 0x11;
        constexpr PaletteIndex_t black8 = 0x12;
        constexpr PaletteIndex_t black9 = 0x13;
        constexpr PaletteIndex_t blackA = 0x14;
        constexpr PaletteIndex_t blackB = 0x15;
        constexpr PaletteIndex_t mutedOliveGreen0 = 0x16;
        constexpr PaletteIndex_t mutedOliveGreen1 = 0x17;
        constexpr PaletteIndex_t mutedOliveGreen2 = 0x18;
        constexpr PaletteIndex_t mutedOliveGreen3 = 0x19;
        constexpr PaletteIndex_t mutedOliveGreen4 = 0x1A;
        constexpr PaletteIndex_t mutedOliveGreen5 = 0x1B;
        constexpr PaletteIndex_t mutedOliveGreen6 = 0x1C;
        constexpr PaletteIndex_t mutedOliveGreen7 = 0x1D;
        constexpr PaletteIndex_t mutedOliveGreen8 = 0x1E;
        constexpr PaletteIndex_t mutedOliveGreen9 = 0x1F;
        constexpr PaletteIndex_t mutedOliveGreenA = 0x20;
        constexpr PaletteIndex_t mutedOliveGreenB = 0x21;
        constexpr PaletteIndex_t mutedDarkYellow0 = 0x22;
        constexpr PaletteIndex_t mutedDarkYellow1 = 0x23;
        constexpr PaletteIndex_t mutedDarkYellow2 = 0x24;
        constexpr PaletteIndex_t mutedDarkYellow3 = 0x25;
        constexpr PaletteIndex_t mutedDarkYellow4 = 0x26;
        constexpr PaletteIndex_t mutedDarkYellow5 = 0x27;
        constexpr PaletteIndex_t mutedDarkYellow6 = 0x28;
        constexpr PaletteIndex_t mutedDarkYellow7 = 0x29;
        constexpr PaletteIndex_t mutedDarkYellow8 = 0x2A;
        constexpr PaletteIndex_t mutedDarkYellow9 = 0x2B;
        constexpr PaletteIndex_t mutedDarkYellowA = 0x2C;
        constexpr PaletteIndex_t mutedDarkYellowB = 0x2D;
        constexpr PaletteIndex_t yellow0 = 0x2E;
        constexpr PaletteIndex_t yellow1 = 0x2F;
        constexpr PaletteIndex_t yellow2 = 0x30;
        constexpr PaletteIndex_t yellow3 = 0x31;
        constexpr PaletteIndex_t yellow4 = 0x32;
        constexpr PaletteIndex_t yellow5 = 0x33;
        constexpr PaletteIndex_t yellow6 = 0x34;
        constexpr PaletteIndex_t yellow7 = 0x35;
        constexpr PaletteIndex_t yellow8 = 0x36;
        constexpr PaletteIndex_t yellow9 = 0x37;
        constexpr PaletteIndex_t yellowA = 0x38;
        constexpr PaletteIndex_t yellowB = 0x39;
        constexpr PaletteIndex_t mutedDarkRed0 = 0x3A;
        constexpr PaletteIndex_t mutedDarkRed1 = 0x3B;
        constexpr PaletteIndex_t mutedDarkRed2 = 0x3C;
        constexpr PaletteIndex_t mutedDarkRed3 = 0x3D;
        constexpr PaletteIndex_t mutedDarkRed4 = 0x3E;
        constexpr PaletteIndex_t mutedDarkRed5 = 0x3F;
        constexpr PaletteIndex_t mutedDarkRed6 = 0x40;
        constexpr PaletteIndex_t mutedDarkRed7 = 0x41;
        constexpr PaletteIndex_t mutedDarkRed8 = 0x42;
        constexpr PaletteIndex_t mutedDarkRed9 = 0x43;
        constexpr PaletteIndex_t mutedDarkRedA = 0x44;
        constexpr PaletteIndex_t mutedDarkRedB = 0x45;
        constexpr PaletteIndex_t mutedGrassGreen0 = 0x46;
        constexpr PaletteIndex_t mutedGrassGreen1 = 0x47;
        constexpr PaletteIndex_t mutedGrassGreen2 = 0x48;
        constexpr PaletteIndex_t mutedGrassGreen3 = 0x49;
        constexpr PaletteIndex_t mutedGrassGreen4 = 0x4A;
        constexpr PaletteIndex_t mutedGrassGreen5 = 0x4B;
        constexpr PaletteIndex_t mutedGrassGreen6 = 0x4C;
        constexpr PaletteIndex_t mutedGrassGreen7 = 0x4D;
        constexpr PaletteIndex_t mutedGrassGreen8 = 0x4E;
        constexpr PaletteIndex_t mutedGrassGreen9 = 0x4F;
        constexpr PaletteIndex_t mutedGrassGreenA = 0x50;
        constexpr PaletteIndex_t mutedGrassGreenB = 0x51;
        constexpr PaletteIndex_t mutedAvocadoGreen0 = 0x52;
        constexpr PaletteIndex_t mutedAvocadoGreen1 = 0x53;
        constexpr PaletteIndex_t mutedAvocadoGreen2 = 0x54;
        constexpr PaletteIndex_t mutedAvocadoGreen3 = 0x55;
        constexpr PaletteIndex_t mutedAvocadoGreen4 = 0x56;
        constexpr PaletteIndex_t mutedAvocadoGreen5 = 0x57;
        constexpr PaletteIndex_t mutedAvocadoGreen6 = 0x58;
        constexpr PaletteIndex_t mutedAvocadoGreen7 = 0x59;
        constexpr PaletteIndex_t mutedAvocadoGreen8 = 0x5A;
        constexpr PaletteIndex_t mutedAvocadoGreen9 = 0x5B;
        constexpr PaletteIndex_t mutedAvocadoGreenA = 0x5C;
        constexpr PaletteIndex_t mutedAvocadoGreenB = 0x5D;
        constexpr PaletteIndex_t green0 = 0x5E;
        constexpr PaletteIndex_t green1 = 0x5F;
        constexpr PaletteIndex_t green2 = 0x60;
        constexpr PaletteIndex_t green3 = 0x61;
        constexpr PaletteIndex_t green4 = 0x62;
        constexpr PaletteIndex_t green5 = 0x63;
        constexpr PaletteIndex_t green6 = 0x64;
        constexpr PaletteIndex_t green7 = 0x65;
        constexpr PaletteIndex_t green8 = 0x66;
        constexpr PaletteIndex_t green9 = 0x67;
        constexpr PaletteIndex_t greenA = 0x68;
        constexpr PaletteIndex_t greenB = 0x69;
        constexpr PaletteIndex_t mutedOrange0 = 0x6A;
        constexpr PaletteIndex_t mutedOrange1 = 0x6B;
        constexpr PaletteIndex_t mutedOrange2 = 0x6C;
        constexpr PaletteIndex_t mutedOrange3 = 0x6D;
        constexpr PaletteIndex_t mutedOrange4 = 0x6E;
        constexpr PaletteIndex_t mutedOrange5 = 0x6F;
        constexpr PaletteIndex_t mutedOrange6 = 0x70;
        constexpr PaletteIndex_t mutedOrange7 = 0x71;
        constexpr PaletteIndex_t mutedOrange8 = 0x72;
        constexpr PaletteIndex_t mutedOrange9 = 0x73;
        constexpr PaletteIndex_t mutedOrangeA = 0x74;
        constexpr PaletteIndex_t mutedOrangeB = 0x75;
        constexpr PaletteIndex_t mutedPurple0 = 0x76;
        constexpr PaletteIndex_t mutedPurple1 = 0x77;
        constexpr PaletteIndex_t mutedPurple2 = 0x78;
        constexpr PaletteIndex_t mutedPurple3 = 0x79;
        constexpr PaletteIndex_t mutedPurple4 = 0x7A;
        constexpr PaletteIndex_t mutedPurple5 = 0x7B;
        constexpr PaletteIndex_t mutedPurple6 = 0x7C;
        constexpr PaletteIndex_t mutedPurple7 = 0x7D;
        constexpr PaletteIndex_t mutedPurple8 = 0x7E;
        constexpr PaletteIndex_t mutedPurple9 = 0x7F;
        constexpr PaletteIndex_t mutedPurpleA = 0x80;
        constexpr PaletteIndex_t mutedPurpleB = 0x81;
        constexpr PaletteIndex_t blue0 = 0x82;
        constexpr PaletteIndex_t blue1 = 0x83;
        constexpr PaletteIndex_t blue2 = 0x84;
        constexpr PaletteIndex_t blue3 = 0x85;
        constexpr PaletteIndex_t blue4 = 0x86;
        constexpr PaletteIndex_t blue5 = 0x87;
        constexpr PaletteIndex_t blue6 = 0x88;
        constexpr PaletteIndex_t blue7 = 0x89;
        constexpr PaletteIndex_t blue8 = 0x8A;
        constexpr PaletteIndex_t blue9 = 0x8B;
        constexpr PaletteIndex_t blueA = 0x8C;
        constexpr PaletteIndex_t blueB = 0x8D;
        constexpr PaletteIndex_t mutedSeaGreen0 = 0x8E;
        constexpr PaletteIndex_t mutedSeaGreen1 = 0x8F;
        constexpr PaletteIndex_t mutedSeaGreen2 = 0x90;
        constexpr PaletteIndex_t mutedSeaGreen3 = 0x91;
        constexpr PaletteIndex_t mutedSeaGreen4 = 0x92;
        constexpr PaletteIndex_t mutedSeaGreen5 = 0x93;
        constexpr PaletteIndex_t mutedSeaGreen6 = 0x94;
        constexpr PaletteIndex_t mutedSeaGreen7 = 0x95;
        constexpr PaletteIndex_t mutedSeaGreen8 = 0x96;
        constexpr PaletteIndex_t mutedSeaGreen9 = 0x97;
        constexpr PaletteIndex_t mutedSeaGreenA = 0x98;
        constexpr PaletteIndex_t mutedSeaGreenB = 0x99;
        constexpr PaletteIndex_t purple0 = 0x9A;
        constexpr PaletteIndex_t purple1 = 0x9B;
        constexpr PaletteIndex_t purple2 = 0x9C;
        constexpr PaletteIndex_t purple3 = 0x9D;
        constexpr PaletteIndex_t purple4 = 0x9E;
        constexpr PaletteIndex_t purple5 = 0x9F;
        constexpr PaletteIndex_t purple6 = 0xA0;
        constexpr PaletteIndex_t purple7 = 0xA1;
        constexpr PaletteIndex_t purple8 = 0xA2;
        constexpr PaletteIndex_t purple9 = 0xA3;
        constexpr PaletteIndex_t purpleA = 0xA4;
        constexpr PaletteIndex_t purpleB = 0xA5;
        constexpr PaletteIndex_t red0 = 0xA6;
        constexpr PaletteIndex_t red1 = 0xA7;
        constexpr PaletteIndex_t red2 = 0xA8;
        constexpr PaletteIndex_t red3 = 0xA9;
        constexpr PaletteIndex_t red4 = 0xAA;
        constexpr PaletteIndex_t red5 = 0xAB;
        constexpr PaletteIndex_t red6 = 0xAC;
        constexpr PaletteIndex_t red7 = 0xAD;
        constexpr PaletteIndex_t red8 = 0xAE;
        constexpr PaletteIndex_t red9 = 0xAF;
        constexpr PaletteIndex_t redA = 0xB0;
        constexpr PaletteIndex_t redB = 0xB1;
        constexpr PaletteIndex_t orange0 = 0xB2;
        constexpr PaletteIndex_t orange1 = 0xB3;
        constexpr PaletteIndex_t orange2 = 0xB4;
        constexpr PaletteIndex_t orange3 = 0xB5;
        constexpr PaletteIndex_t orange4 = 0xB6;
        constexpr PaletteIndex_t orange5 = 0xB7;
        constexpr PaletteIndex_t orange6 = 0xB8;
        constexpr PaletteIndex_t orange7 = 0xB9;
        constexpr PaletteIndex_t orange8 = 0xBA;
        constexpr PaletteIndex_t orange9 = 0xBB;
        constexpr PaletteIndex_t orangeA = 0xBC;
        constexpr PaletteIndex_t orangeB = 0xBD;
        constexpr PaletteIndex_t mutedDarkTeal0 = 0xBE;
        constexpr PaletteIndex_t mutedDarkTeal1 = 0xBF;
        constexpr PaletteIndex_t mutedDarkTeal2 = 0xC0;
        constexpr PaletteIndex_t mutedDarkTeal3 = 0xC1;
        constexpr PaletteIndex_t mutedDarkTeal4 = 0xC2;
        constexpr PaletteIndex_t mutedDarkTeal5 = 0xC3;
        constexpr PaletteIndex_t mutedDarkTeal6 = 0xC4;
        constexpr PaletteIndex_t mutedDarkTeal7 = 0xC5;
        constexpr PaletteIndex_t mutedDarkTeal8 = 0xC6;
        constexpr PaletteIndex_t mutedDarkTeal9 = 0xC7;
        constexpr PaletteIndex_t mutedDarkTealA = 0xC8;
        constexpr PaletteIndex_t mutedDarkTealB = 0xC9;

        constexpr PaletteIndex_t pink0 = 0xCA; // Also used for secondary remap
        constexpr PaletteIndex_t pink1 = 0xCB;
        constexpr PaletteIndex_t pink2 = 0xCC;
        constexpr PaletteIndex_t pink3 = 0xCD;
        constexpr PaletteIndex_t pink4 = 0xCE;
        constexpr PaletteIndex_t pink5 = 0xCF;
        constexpr PaletteIndex_t pink6 = 0xD0;
        constexpr PaletteIndex_t pink7 = 0xD1;
        constexpr PaletteIndex_t pink8 = 0xD2;
        constexpr PaletteIndex_t pink9 = 0xD3;
        constexpr PaletteIndex_t pinkA = 0xD4;
        constexpr PaletteIndex_t pinkB = 0xD5;
        constexpr PaletteIndex_t secondaryRemap0 = 0xCA; // Also used for pink
        constexpr PaletteIndex_t secondaryRemap1 = 0xCB;
        constexpr PaletteIndex_t secondaryRemap2 = 0xCC;
        constexpr PaletteIndex_t secondaryRemap3 = 0xCD;
        constexpr PaletteIndex_t secondaryRemap4 = 0xCE;
        constexpr PaletteIndex_t secondaryRemap5 = 0xCF;
        constexpr PaletteIndex_t secondaryRemap6 = 0xD0;
        constexpr PaletteIndex_t secondaryRemap7 = 0xD1;
        constexpr PaletteIndex_t secondaryRemap8 = 0xD2;
        constexpr PaletteIndex_t secondaryRemap9 = 0xD3;
        constexpr PaletteIndex_t secondaryRemapA = 0xD4;
        constexpr PaletteIndex_t secondaryRemapB = 0xD5;

        constexpr PaletteIndex_t brown0 = 0xD6;
        constexpr PaletteIndex_t brown1 = 0xD7;
        constexpr PaletteIndex_t brown2 = 0xD8;
        constexpr PaletteIndex_t brown3 = 0xD9;
        constexpr PaletteIndex_t brown4 = 0xDA;
        constexpr PaletteIndex_t brown5 = 0xDB;
        constexpr PaletteIndex_t brown6 = 0xDC;
        constexpr PaletteIndex_t brown7 = 0xDD;
        constexpr PaletteIndex_t brown8 = 0xDE;
        constexpr PaletteIndex_t brown9 = 0xDF;
        constexpr PaletteIndex_t brownA = 0xE0;
        constexpr PaletteIndex_t brownB = 0xE1;
        constexpr PaletteIndex_t grey0 = 0xE2;
        constexpr PaletteIndex_t brightYellow0 = 0xE3;
        constexpr PaletteIndex_t brightYellow1 = 0xE4;
        constexpr PaletteIndex_t brightYellow2 = 0xE5;
        constexpr PaletteIndex_t amber0 = 0xE6;
        constexpr PaletteIndex_t amber1 = 0xE7;
        constexpr PaletteIndex_t amber2 = 0xE8;
        constexpr PaletteIndex_t amber3 = 0xE9;
        constexpr PaletteIndex_t amber4 = 0xEA;
        constexpr PaletteIndex_t amber5 = 0xEB;
        constexpr PaletteIndex_t amber6 = 0xEC;
        constexpr PaletteIndex_t amber7 = 0xED;
        constexpr PaletteIndex_t amber8 = 0xEE;
        constexpr PaletteIndex_t amber9 = 0xEF;
        constexpr PaletteIndex_t grey1 = 0xF0;
        constexpr PaletteIndex_t grey2 = 0xF1;
        constexpr PaletteIndex_t grey3 = 0xF2;
        constexpr PaletteIndex_t amberA = 0xF3;
        constexpr PaletteIndex_t amberB = 0xF4;
        constexpr PaletteIndex_t unusedHotPink = 0xF5; // Unused
        constexpr PaletteIndex_t primaryRemap3 = 0xF6;
        constexpr PaletteIndex_t primaryRemap4 = 0xF7;
        constexpr PaletteIndex_t primaryRemap5 = 0xF8;
        constexpr PaletteIndex_t primaryRemap6 = 0xF9;
        constexpr PaletteIndex_t primaryRemap7 = 0xFA;
        constexpr PaletteIndex_t primaryRemap8 = 0xFB;
        constexpr PaletteIndex_t primaryRemap9 = 0xFC;
        constexpr PaletteIndex_t primaryRemapA = 0xFD;
        constexpr PaletteIndex_t primaryRemapB = 0xFE;
        constexpr PaletteIndex_t index_FF = 0xFF;
    }
}
