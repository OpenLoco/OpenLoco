#include "VehicleChannel.h"
#include "../Interop/Interop.hpp"
#include "../Ptr.h"
#include "../Things/ThingManager.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco;
using namespace OpenLoco::Audio;
using namespace OpenLoco::Interop;

static std::pair<sound_id, channel_attributes> sub_48A590(const Vehicles::Vehicle2or6* v)
{
    registers regs;
    regs.esi = ToInt(v);
    call(0x0048A590, regs);
    return { static_cast<sound_id>(regs.eax), { (int32_t)regs.ecx, (int32_t)regs.edx, (int32_t)regs.ebx } };
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
    auto v = ThingManager::get<Vehicles::VehicleBase>(vid);
    if (v != nullptr && v->isVehicle2Or6())
    {
        auto* veh26 = v->asVehicle2Or6();
        if (veh26 != nullptr)
        {
            auto [sid, sa] = sub_48A590(veh26);
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
}

void vehicle_channel::update()
{
    if (!isFree())
    {
        auto v = ThingManager::get<Vehicles::VehicleBase>(_vehicle_id);
        if (v != nullptr && v->isVehicle2Or6())
        {
            auto* veh26 = v->asVehicle2Or6();
            if (veh26 != nullptr && (veh26->var_4A & 1))
            {
                auto [sid, sa] = sub_48A590(veh26);
                if (_sound_id == sid)
                {
                    veh26->var_4A &= ~1;
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
        }
        stop();
    }
}

void vehicle_channel::stop()
{
    _channel.stop();
    _vehicle_id = ThingId::null;
}
