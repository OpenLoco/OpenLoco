#pragma once
#include "Audio.h"
#include "Channel.h"

namespace OpenLoco::Audio
{
    class VehicleChannel : public Channel
    {
    private:
        EntityId _vehicleId = EntityId::null;
        SoundId _soundId{};

    public:
        VehicleChannel(OpenAL::Source source)
            : Channel::Channel(source)
        {
        }

        bool isFree() const { return _vehicleId == EntityId::null; }

        EntityId getId() const { return _vehicleId; }
        void begin(EntityId vid);
        void update();
        void stop();
    };
}
