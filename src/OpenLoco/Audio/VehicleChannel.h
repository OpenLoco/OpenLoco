#pragma once
#include "Audio.h"
#include "Channel.h"

namespace OpenLoco::Audio
{
    class VehicleChannel : public Channel
    {
    private:
        EntityId vehicleId = EntityId::null;
        SoundId soundId{};

    public:
        VehicleChannel(OpenAL::Source source)
            : Channel::Channel(source)
        {
        }

        bool isFree() const { return vehicleId == EntityId::null; }

        EntityId getId() const { return vehicleId; }
        void begin(EntityId vid);
        void update();
        void stop();
    };
}
