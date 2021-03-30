#pragma once

#include "../Entities/Entity.h"
#include "Audio.h"
#include "Channel.h"

namespace OpenLoco::Audio
{
    struct channel_attributes
    {
        int32_t volume{};
        int32_t pan{};
        int32_t freq{};
    };

    class vehicle_channel
    {
    private:
        channel _channel;
        thing_id_t _vehicle_id = ThingId::null;
        sound_id _sound_id{};
        channel_attributes _attributes;

    public:
        vehicle_channel() = default;
        vehicle_channel(const vehicle_channel& c) = delete;
        explicit vehicle_channel(channel&& c);
        vehicle_channel(vehicle_channel&& c);
        vehicle_channel& operator=(vehicle_channel&& other);

        bool isFree() const { return _vehicle_id == ThingId::null; }

        void begin(thing_id_t vid);
        void update();
        void stop();
    };
}
