#pragma once

#include "../Entities/Entity.h"
#include "Audio.h"
#include "Channel.h"

namespace OpenLoco::Audio
{
    struct ChannelAttributes
    {
        int32_t volume{};
        int32_t pan{};
        int32_t freq{};
    };

    class VehicleChannel
    {
    private:
        Channel _channel;
        EntityId _vehicleId = EntityId::null;
        SoundId _soundId{};
        ChannelAttributes _attributes;

    public:
        VehicleChannel() = default;
        VehicleChannel(const VehicleChannel& c) = delete;
        explicit VehicleChannel(Channel&& c);
        VehicleChannel(VehicleChannel&& c);
        VehicleChannel& operator=(VehicleChannel&& other);

        bool isFree() const { return _vehicleId == EntityId::null; }

        void begin(EntityId vid);
        void update();
        void stop();
    };
}
