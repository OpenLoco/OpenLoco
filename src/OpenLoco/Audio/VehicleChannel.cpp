#include "VehicleChannel.h"
#include "../Interop/Interop.hpp"
#include "../Things/ThingManager.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco;
using namespace OpenLoco::Audio;
using namespace OpenLoco::Interop;

static std::tuple<sound_id, channel_attributes> sub_48A590(const Vehicles::vehicle* v)
{
    registers regs;
    regs.esi = (int32_t)v;
    call(0x0048A590, regs);
    return std::make_tuple<sound_id, channel_attributes>((sound_id)regs.eax, { regs.ecx, regs.edx, regs.ebx });
}

vehicle_channel::vehicle_channel(channel&& c)
    : _channel(std::exchange(c, {}))
{
}

vehicle_channel::vehicle_channel(vehicle_channel&& vc)
    : _channel(std::exchange(vc._channel, {}))
    , _vehicle_id(std::exchange(vc._vehicle_id, ThingId::null))
    , _sound_id(std::exchange(vc._sound_id, {}))
    , _attributes(std::exchange(vc._attributes, {}))
{
}

vehicle_channel& vehicle_channel::operator=(vehicle_channel&& other)
{
    std::swap(_channel, other._channel);
    std::swap(_vehicle_id, other._vehicle_id);
    std::swap(_sound_id, other._sound_id);
    std::swap(_attributes, other._attributes);
    return *this;
}

void vehicle_channel::begin(thing_id_t vid)
{
    auto v = ThingManager::get<Vehicles::vehicle>(vid);
    if (v != nullptr)
    {
        auto [sid, sa] = sub_48A590(v);
        auto loop = Audio::shouldSoundLoop(sid);
        auto sample = Audio::getSoundSample(sid);
        if (sample != nullptr)
        {
            _vehicle_id = vid;
            _sound_id = sid;
            _attributes = sa;

            _channel.load(*sample);
            _channel.play(loop);
            _channel.setVolume(sa.volume);
            _channel.setPan(sa.pan);
            _channel.setFrequency(sa.freq);
        }
    }
}

void vehicle_channel::update()
{
    if (!isFree())
    {
        auto v = ThingManager::get<Vehicles::vehicle>(_vehicle_id);
        if (v != nullptr && v->base_type == thing_base_type::vehicle && (v->getSubType() == Vehicles::VehicleThingType::vehicle_2 || v->getSubType() == Vehicles::VehicleThingType::tail) && (v->var_4A & 1))
        {
            auto [sid, sa] = sub_48A590(v);
            if (_sound_id == sid)
            {
                v->var_4A &= ~1;
                if (_attributes.volume != sa.volume)
                {
                    _channel.setVolume(sa.volume);
                }
                if (_attributes.pan != sa.pan)
                {
                    _channel.setPan(sa.pan);
                }
                if (_attributes.freq != sa.freq)
                {
                    _channel.setFrequency(sa.freq);
                }
                _attributes = sa;
                return;
            }
        }
        stop();
    }
}

void vehicle_channel::stop()
{
    _channel.stop();
    _vehicle_id = ThingId::null;
}
