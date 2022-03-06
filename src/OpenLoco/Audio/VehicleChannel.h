#pragma once
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
        VehicleChannel(const Channel& channel)
            : _channel(channel)
        {
        }

        bool isFree() const { return _vehicleId == EntityId::null; }

        void begin(EntityId vid);
        void update();
        void stop();
    };
}
