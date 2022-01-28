#include "VehicleChannel.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco;
using namespace OpenLoco::Audio;
using namespace OpenLoco::Interop;

static std::pair<SoundId, ChannelAttributes> sub_48A590(const Vehicles::Vehicle2or6* v)
{
    registers regs;
    regs.esi = X86Pointer(v);
    call(0x0048A590, regs);
    return { static_cast<SoundId>(regs.eax), { regs.ecx, regs.edx, regs.ebx } };
}

VehicleChannel::VehicleChannel(Channel&& c)
    : _channel(std::exchange(c, {}))
{
}

VehicleChannel::VehicleChannel(VehicleChannel&& vc)
    : _channel(std::exchange(vc._channel, {}))
    , _vehicleId(std::exchange(vc._vehicleId, EntityId::null))
    , _soundId(std::exchange(vc._soundId, {}))
    , _attributes(std::exchange(vc._attributes, {}))
{
}

VehicleChannel& VehicleChannel::operator=(VehicleChannel&& other)
{
    std::swap(_channel, other._channel);
    std::swap(_vehicleId, other._vehicleId);
    std::swap(_soundId, other._soundId);
    std::swap(_attributes, other._attributes);
    return *this;
}

void VehicleChannel::begin(EntityId vid)
{
    auto v = EntityManager::get<Vehicles::VehicleBase>(vid);
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
                _vehicleId = vid;
                _soundId = sid;
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

void VehicleChannel::update()
{
    if (!isFree())
    {
        auto v = EntityManager::get<Vehicles::VehicleBase>(_vehicleId);
        if (v != nullptr && v->isVehicle2Or6())
        {
            auto* veh26 = v->asVehicle2Or6();
            if (veh26 != nullptr && (veh26->var_4A & 1))
            {
                auto [sid, sa] = sub_48A590(veh26);
                if (_soundId == sid)
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

void VehicleChannel::stop()
{
    _channel.stop();
    _vehicleId = EntityId::null;
}
