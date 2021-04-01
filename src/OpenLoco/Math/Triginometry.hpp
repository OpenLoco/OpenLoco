#pragma once
#include "Vector.hpp"
#include <cstdint>

namespace OpenLoco
{
    enum class Pitch : uint8_t
    {
        flat = 0,
        up6deg = 1,  // Transition
        up12deg = 2, // Gentle
        up18deg = 3, // Transition
        up25deg = 4, // Steep
        down6deg = 5,
        down12deg = 6,
        down18deg = 7,
        down25deg = 8,
        up10deg = 9, // Gentle Curve Up
        down10deg = 10,
        up20deg = 11, // Steep Curve Up
        down20deg = 12,
    };
}

namespace OpenLoco::Math::Triginometry
{
    // 0x00503B6A
    // ROUND(COS((32/64+(L1/64))*(2*PI()))*256,0), ROUND(SIN(((L1/64))*(2*PI())) * 256,0)
    static constexpr Vector::TVector2<int32_t, 1> factorXY503B6A[64] = {
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
    // Note: pitch angles not quite correct
    constexpr int16_t factor503B50[] = {
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
        return (factor503B50[static_cast<uint8_t>(pitch)] * height) / 256;
    }

    constexpr auto computeXYVector(int16_t magnitude, uint8_t yaw)
    {
        return (factorXY503B6A[yaw] * magnitude) / 256;
    }
    constexpr auto computeXYVector(int16_t height, Pitch pitch, uint8_t yaw)
    {
        return computeXYVector(computeXYMagnitude(height, pitch), yaw);
    }
}
