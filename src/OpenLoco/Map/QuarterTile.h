#pragma once

#include <cstdint>

namespace OpenLoco::Map
{
    class QuarterTile
    {
    private:
        uint8_t _val{ 0 };

    public:
        explicit constexpr QuarterTile(uint8_t tileQuarter, uint8_t zQuarter)
            : _val(tileQuarter | (zQuarter << 4))
        {
        }

        explicit constexpr QuarterTile(uint8_t tileAndZQuarter)
            : _val(tileAndZQuarter)
        {
        }

        // Rotate both of the values amount. Returns new RValue QuarterTile
        constexpr const QuarterTile rotate(uint8_t amount) const
        {
            switch (amount)
            {
                default:
                    return QuarterTile{ *this };
                case 1:
                {
                    auto rotVal1 = _val << 1;
                    auto rotVal2 = rotVal1 >> 4;
                    // Clear the bit from the tileQuarter
                    rotVal1 &= 0b11101110;
                    // Clear the bit from the zQuarter
                    rotVal2 &= 0b00010001;
                    return QuarterTile{ static_cast<uint8_t>(rotVal1 | rotVal2) };
                }
                case 2:
                {
                    auto rotVal1 = _val << 2;
                    auto rotVal2 = rotVal1 >> 4;
                    // Clear the bit from the tileQuarter
                    rotVal1 &= 0b11001100;
                    // Clear the bit from the zQuarter
                    rotVal2 &= 0b00110011;
                    return QuarterTile{ static_cast<uint8_t>(rotVal1 | rotVal2) };
                }
                case 3:
                {
                    auto rotVal1 = _val << 3;
                    auto rotVal2 = rotVal1 >> 4;
                    // Clear the bit from the tileQuarter
                    rotVal1 &= 0b10001000;
                    // Clear the bit from the zQuarter
                    rotVal2 &= 0b01110111;
                    return QuarterTile{ static_cast<uint8_t>(rotVal1 | rotVal2) };
                }
            }
        }

        constexpr uint8_t getBaseQuarterOccupied() const
        {
            return _val & 0xF;
        }

        constexpr uint8_t getZQuarterOccupied() const
        {
            return (_val >> 4) & 0xF;
        }
    };
    static_assert(sizeof(QuarterTile) == 1);
}
