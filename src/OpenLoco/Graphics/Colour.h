#pragma once

#include "../Types.hpp"

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
        null = 32,  // Does not represent any palette
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
    };

    struct AdvancedColour
    {
        static constexpr uint8_t outline_flag = 1 << 5;
        static constexpr uint8_t inset_flag = 1 << 6;
        static constexpr uint8_t translucent_flag = 1 << 7;
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

        constexpr explicit operator Colour() const { return static_cast<Colour>(enumValue(_c) & ~(outline_flag | inset_flag | translucent_flag)); }
        constexpr Colour c() const { return static_cast<Colour>(*this); }
        constexpr explicit operator uint8_t() const { return enumValue(_c); }
        constexpr uint8_t u8() const { return static_cast<uint8_t>(*this); }

        constexpr AdvancedColour outline()
        {
            _c = static_cast<Colour>(enumValue(_c) | outline_flag);
            return *this;
        }
        constexpr bool isOutline() const { return enumValue(_c) & outline_flag; }

        constexpr AdvancedColour inset()
        {
            _c = static_cast<Colour>(enumValue(_c) | inset_flag);
            return *this;
        }
        constexpr bool isInset() const { return enumValue(_c) & inset_flag; }

        constexpr AdvancedColour translucent()
        {
            _c = static_cast<Colour>(enumValue(_c) | translucent_flag);
            return *this;
        }
        constexpr bool isTranslucent() const { return enumValue(_c) & translucent_flag; }

        constexpr AdvancedColour opaque()
        {
            _c = static_cast<Colour>(enumValue(_c) & ~translucent_flag);
            return *this;
        }
        constexpr AdvancedColour clearInset()
        {
            _c = static_cast<Colour>(enumValue(_c) & ~inset_flag);
            return *this;
        }
        constexpr AdvancedColour clearOutline()
        {
            _c = static_cast<Colour>(enumValue(_c) & ~outline_flag);
            return *this;
        }
        constexpr bool isOpaque() const { return !isTranslucent(); }
        constexpr AdvancedColour FF()
        {
            _c = static_cast<Colour>(ff);
            return *this;
        }
        constexpr bool isFF() const { return enumValue(_c) == ff; }
        constexpr AdvancedColour FE()
        {
            _c = static_cast<Colour>(fe);
            return *this;
        }
        constexpr bool isFE() const { return enumValue(_c) == fe; }
        constexpr AdvancedColour FD()
        {
            _c = static_cast<Colour>(fd);
            return *this;
        }
        constexpr bool isFD() const { return enumValue(_c) == fd; }
    };
    static_assert(sizeof(AdvancedColour) == 1);

    namespace Colours
    {
        void initColourMap();
        PaletteIndex_t getShade(Colour colour, uint8_t shade);
        ExtColour getTranslucent(Colour colour);
        constexpr ExtColour toExt(Colour c) { return static_cast<ExtColour>(c); }
    }

    namespace PaletteIndex
    {
        constexpr PaletteIndex_t transparent = 0;
        constexpr PaletteIndex_t primaryRemap0 = 0x07;
        constexpr PaletteIndex_t primaryRemap1 = 0x08;
        constexpr PaletteIndex_t primaryRemap2 = 0x09;
        constexpr PaletteIndex_t index_0A = 0x0A;
        constexpr PaletteIndex_t index_0C = 0x0C;
        constexpr PaletteIndex_t index_0E = 0x0E;
        constexpr PaletteIndex_t index_11 = 0x11;
        constexpr PaletteIndex_t index_12 = 0x12;
        constexpr PaletteIndex_t index_15 = 0x15;
        constexpr PaletteIndex_t index_1F = 0x1F;
        constexpr PaletteIndex_t index_24 = 0x24;
        constexpr PaletteIndex_t index_29 = 0x29;
        constexpr PaletteIndex_t index_2C = 0x2C;
        constexpr PaletteIndex_t index_2E = 0x2E;
        constexpr PaletteIndex_t index_30 = 0x30;
        constexpr PaletteIndex_t index_31 = 0x31;
        constexpr PaletteIndex_t index_32 = 0x32;
        constexpr PaletteIndex_t index_33 = 0x33;
        constexpr PaletteIndex_t index_35 = 0x35;
        constexpr PaletteIndex_t index_38 = 0x38;
        constexpr PaletteIndex_t index_3B = 0x3B;
        constexpr PaletteIndex_t index_3E = 0x3E;
        constexpr PaletteIndex_t index_3F = 0x3F;
        constexpr PaletteIndex_t index_41 = 0x41;
        constexpr PaletteIndex_t index_43 = 0x43;
        constexpr PaletteIndex_t index_44 = 0x44;
        constexpr PaletteIndex_t index_47 = 0x47;
        constexpr PaletteIndex_t index_4A = 0x4A;
        constexpr PaletteIndex_t index_4B = 0x4B;
        constexpr PaletteIndex_t index_4D = 0x4D;
        constexpr PaletteIndex_t index_50 = 0x50;
        constexpr PaletteIndex_t index_53 = 0x53;
        constexpr PaletteIndex_t index_56 = 0x56;
        constexpr PaletteIndex_t index_58 = 0x58;
        constexpr PaletteIndex_t index_59 = 0x59;
        constexpr PaletteIndex_t index_5C = 0x5C;
        constexpr PaletteIndex_t index_5F = 0x5F;
        constexpr PaletteIndex_t index_62 = 0x62;
        constexpr PaletteIndex_t index_64 = 0x64;
        constexpr PaletteIndex_t index_65 = 0x65;
        constexpr PaletteIndex_t index_66 = 0x66;
        constexpr PaletteIndex_t index_67 = 0x67;
        constexpr PaletteIndex_t index_68 = 0x68;
        constexpr PaletteIndex_t index_6B = 0x6B;
        constexpr PaletteIndex_t index_6E = 0x6E;
        constexpr PaletteIndex_t index_71 = 0x71;
        constexpr PaletteIndex_t index_74 = 0x74;
        constexpr PaletteIndex_t index_7D = 0x7D;
        constexpr PaletteIndex_t index_85 = 0x85;
        constexpr PaletteIndex_t index_89 = 0x89;
        constexpr PaletteIndex_t index_9D = 0x9D;
        constexpr PaletteIndex_t index_A1 = 0xA1;
        constexpr PaletteIndex_t index_A2 = 0xA2;
        constexpr PaletteIndex_t index_A3 = 0xA3;
        constexpr PaletteIndex_t index_AC = 0xAC;
        constexpr PaletteIndex_t index_AD = 0xAD;
        constexpr PaletteIndex_t index_B8 = 0xB8;
        constexpr PaletteIndex_t index_BA = 0xBA;
        constexpr PaletteIndex_t index_BB = 0xBB;
        constexpr PaletteIndex_t index_BC = 0xBC;
        constexpr PaletteIndex_t index_C3 = 0xC3;
        constexpr PaletteIndex_t index_C6 = 0xC6;
        constexpr PaletteIndex_t secondaryRemap0 = 0xCA;
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
        constexpr PaletteIndex_t index_D0 = 0xD0;
        constexpr PaletteIndex_t index_D3 = 0xD3;
        constexpr PaletteIndex_t index_DB = 0xDB;
        constexpr PaletteIndex_t index_DE = 0xDE;
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
