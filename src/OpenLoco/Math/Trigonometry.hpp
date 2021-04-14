#pragma once
#include "Vector.hpp"
#include <cstdint>

namespace OpenLoco
{
    enum class Pitch : uint8_t;
}

namespace OpenLoco::Math::Trigonometry
{
    // 0x00503B6A
    // ROUND(COS((32/64+(L1/64))*(2*PI()))*256,0), ROUND(SIN(((L1/64))*(2*PI())) * 256,0)
    // Where L1 represents an incrementing column 0 - 63
    // Note: Must be at least 32bit to ensure all users do not overflow
    static constexpr Vector::TVector2<int32_t, 1> yawToDirectionVector[64] = {
        { -256, 0 },
        { -255, 25 },
        { -251, 50 },
        { -245, 74 },
        { -237, 98 },
        { -226, 121 },
        { -213, 142 },
        { -198, 162 },
        { -181, 181 },
        { -162, 198 },
        { -142, 213 },
        { -121, 226 },
        { -98, 237 },
        { -74, 245 },
        { -50, 251 },
        { -25, 255 },
        { 0, 256 },
        { 25, 255 },
        { 50, 251 },
        { 74, 245 },
        { 98, 237 },
        { 121, 226 },
        { 142, 213 },
        { 162, 198 },
        { 181, 181 },
        { 198, 162 },
        { 213, 142 },
        { 226, 121 },
        { 237, 98 },
        { 245, 74 },
        { 251, 50 },
        { 255, 25 },
        { 256, 0 },
        { 255, -25 },
        { 251, -50 },
        { 245, -74 },
        { 237, -98 },
        { 226, -121 },
        { 213, -142 },
        { 198, -162 },
        { 181, -181 },
        { 162, -198 },
        { 142, -213 },
        { 121, -226 },
        { 98, -237 },
        { 74, -245 },
        { 50, -251 },
        { 25, -255 },
        { 0, -256 },
        { -25, -255 },
        { -50, -251 },
        { -74, -245 },
        { -98, -237 },
        { -121, -226 },
        { -142, -213 },
        { -162, -198 },
        { -181, -181 },
        { -198, -162 },
        { -213, -142 },
        { -226, -121 },
        { -237, -98 },
        { -245, -74 },
        { -251, -50 },
        { -255, -25 },
    };

    // 0x00503B50
    // -SIN((Y1/360)*2*PI())*256
    // Where Y1 represents the angle of pitch in degrees (0, 5.75, 11.75, 17, 22.5, reverse, 10, -10, 19.25, -19.25)
    // Note: pitch angles not quite correct in the Pitch enum class (decimal points can't go in identifier names)
    constexpr int16_t pitchHorizontalFactor[] = {
        0,
        -26,
        -52,
        -75,
        -98,
        26,
        52,
        75,
        98,
        -44,
        44,
        -84,
        84
    };

    constexpr auto computeXYMagnitude(int16_t height, Pitch pitch)
    {
        return (pitchHorizontalFactor[static_cast<uint8_t>(pitch)] * height) / 256;
    }

    constexpr auto computeXYVector(int16_t magnitude, uint8_t yaw)
    {
        return (yawToDirectionVector[yaw] * magnitude) / 256;
    }
    constexpr auto computeXYVector(int16_t height, Pitch pitch, uint8_t yaw)
    {
        return computeXYVector(computeXYMagnitude(height, pitch), yaw);
    }
}
