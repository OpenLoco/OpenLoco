#pragma once

#include "../Types.hpp"

namespace OpenLoco
{
    using Colour_t = uint8_t;
    using PaletteIndex_t = uint8_t;

    enum class Colour2 : uint8_t
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

    struct AdvancedColour
    {
        static constexpr uint8_t outline_flag = 1 << 5;
        static constexpr uint8_t inset_flag = 1 << 6;
        static constexpr uint8_t translucent_flag = 1 << 7;
        static constexpr uint8_t fd = 0xFD;
        static constexpr uint8_t fe = 0xFE;
        static constexpr uint8_t ff = 0xFF;
    private:
        Colour2 _c = Colour2::black;

    public:
        constexpr AdvancedColour() = default;
        constexpr AdvancedColour(const Colour2 c)
            : _c(c)
        {
        }

        constexpr explicit operator Colour2() const { return static_cast<Colour2>(enumValue(_c) & ~(outline_flag | inset_flag | translucent_flag)); }
        constexpr Colour2 c() const { return static_cast<Colour2>(*this); }
        constexpr explicit operator uint8_t() const { return enumValue(_c); }
        constexpr uint8_t u8() const { return static_cast<uint8_t>(*this); }

        constexpr AdvancedColour outline()
        {
            _c = static_cast<Colour2>(enumValue(_c) | outline_flag);
            return *this;
        }
        constexpr bool isOutline() const { return enumValue(_c) & outline_flag; }

        constexpr AdvancedColour inset()
        {
            _c = static_cast<Colour2>(enumValue(_c) | inset_flag);
            return *this;
        }
        constexpr bool isInset() const { return enumValue(_c) & inset_flag; }

        constexpr AdvancedColour translucent()
        {
            _c = static_cast<Colour2>(enumValue(_c) | translucent_flag);
            return *this;
        }
        constexpr bool isTranslucent() const { return enumValue(_c) & translucent_flag; }

        constexpr AdvancedColour opaque()
        {
            _c = static_cast<Colour2>(enumValue(_c) & ~translucent_flag);
            return *this;
        }
        constexpr AdvancedColour clearInset()
        {
            _c = static_cast<Colour2>(enumValue(_c) & ~inset_flag);
            return *this;
        }
        constexpr AdvancedColour clearOutline()
        {
            _c = static_cast<Colour2>(enumValue(_c) & ~outline_flag);
            return *this;
        }
        constexpr bool isOpaque() const { return !isTranslucent(); }
        constexpr bool isFF() const { return enumValue(_c) == ff; }
        constexpr bool isFE() const { return enumValue(_c) == fe; }
        constexpr bool isFD() const { return enumValue(_c) == fd; }
    };
    static_assert(sizeof(AdvancedColour) == 1);

    namespace Colour
    {
        constexpr uint8_t outline_flag = 1 << 5;
        constexpr uint8_t inset_flag = 1 << 6;
        constexpr uint8_t translucent_flag = 1 << 7;

        constexpr Colour_t black = 0;
        constexpr Colour_t grey = 1;
        constexpr Colour_t white = 2;
        constexpr Colour_t mutedDarkPurple = 3;
        constexpr Colour_t mutedPurple = 4;
        constexpr Colour_t purple = 5;
        constexpr Colour_t darkBlue = 6;
        constexpr Colour_t blue = 7;
        constexpr Colour_t mutedDarkTeal = 8;
        constexpr Colour_t mutedTeal = 9;
        constexpr Colour_t darkGreen = 10;
        constexpr Colour_t mutedSeaGreen = 11;
        constexpr Colour_t mutedGrassGreen = 12;
        constexpr Colour_t green = 13;
        constexpr Colour_t mutedAvocadoGreen = 14;
        constexpr Colour_t mutedOliveGreen = 15;
        constexpr Colour_t yellow = 16;
        constexpr Colour_t darkYellow = 17;
        constexpr Colour_t orange = 18;
        constexpr Colour_t amber = 19;
        constexpr Colour_t darkOrange = 20;
        constexpr Colour_t mutedDarkYellow = 21;
        constexpr Colour_t mutedYellow = 22;
        constexpr Colour_t brown = 23;
        constexpr Colour_t mutedOrange = 24;
        constexpr Colour_t mutedDarkRed = 25;
        constexpr Colour_t darkRed = 26;
        constexpr Colour_t red = 27;
        constexpr Colour_t darkPink = 28;
        constexpr Colour_t pink = 29;
        constexpr Colour_t mutedRed = 30;

        constexpr Colour_t outline(Colour_t c)
        {
            return c | outline_flag;
        }

        constexpr Colour_t inset(Colour_t c)
        {
            return c | inset_flag;
        }

        constexpr Colour_t translucent(Colour_t c)
        {
            return c | translucent_flag;
        }

        constexpr Colour_t opaque(Colour_t c)
        {
            return c & ~translucent_flag;
        }

        void initColourMap();
        PaletteIndex_t getShade(Colour_t colour, uint8_t shade);
        PaletteIndex_t getTranslucent(Colour_t colour);
    }

    namespace Colours
    {
        inline void initColourMap() { Colour::initColourMap(); }
        inline PaletteIndex_t getShade(Colour2 colour, uint8_t shade) { return Colour::getShade(enumValue(colour), shade); }
        inline PaletteIndex_t getTranslucent(Colour2 colour) { return Colour::getTranslucent(enumValue(colour)); }
    }

    namespace PaletteIndex
    {
        constexpr PaletteIndex_t transparent = 0;
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
        constexpr PaletteIndex_t index_D0 = 0xD0;
        constexpr PaletteIndex_t index_D3 = 0xD3;
        constexpr PaletteIndex_t index_DB = 0xDB;
        constexpr PaletteIndex_t index_DE = 0xDE;
    }
}
