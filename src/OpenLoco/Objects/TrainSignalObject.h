#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"
#include <vector>

namespace OpenLoco
{
    namespace Gfx
    {
        struct Context;
    }

    namespace TrainSignalObjectFlags
    {
        constexpr uint16_t isLeft = 1 << 0;
        constexpr uint16_t hasLights = 1 << 1;
    }

    namespace TrainSignal::ImageIds
    {
        constexpr uint32_t redLights = 80;
        constexpr uint32_t redLights2 = 88;
        constexpr uint32_t greenLights = 96;
        constexpr uint32_t greenLights2 = 104;
    }
#pragma pack(push, 1)
    struct TrainSignalObject
    {
        static constexpr auto kObjectType = ObjectType::trackSignal;

        string_id name;
        uint16_t flags;           // 0x02
        uint8_t animationSpeed;   // 0x04
        uint8_t num_frames;       // 0x05
        int16_t cost_factor;      // 0x06
        int16_t sell_cost_factor; // 0x08
        uint8_t cost_index;       // 0x0A
        uint8_t var_0B;
        uint16_t var_0C;
        uint32_t image;         // 0x0E
        uint8_t num_compatible; // 0x12
        uint8_t mods[7];        // 0x13
        uint16_t designed_year; // 0x1A
        uint16_t obsolete_year; // 0x1C

        bool validate() const;
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> data);
        void unload();
        void drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const;
    };
#pragma pack(pop)
    static_assert(sizeof(TrainSignalObject) == 0x1E);

    const std::vector<uint8_t> signalFrames2State = { 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0 };
    const std::vector<uint8_t> signalFrames3State = { 1, 2, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0 };
    const std::vector<uint8_t> signalFrames4State = { 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    static const std::vector<std::vector<uint8_t>> signalFrames = {
        signalFrames2State,
        signalFrames3State,
        signalFrames4State,
    };
}
