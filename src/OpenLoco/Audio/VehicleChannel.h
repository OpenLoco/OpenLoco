#pragma once
#include "Audio.h"
#include "Channel.h"

namespace OpenLoco::Audio
{
    class VehicleChannel
    {
    private:
        Channel _channel;
        EntityId _vehicleId = EntityId::null;
        SoundId _soundId{};

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
