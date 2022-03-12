#include "VehicleChannel.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Audio
{
    static std::pair<SoundId, Channel::Attributes> sub_48A590(const Vehicles::Vehicle2or6* v)
    {
        registers regs;
        regs.esi = X86Pointer(v);
        call(0x0048A590, regs);
        return { static_cast<SoundId>(regs.eax), { regs.ecx, regs.edx, regs.ebx } };
    }

    void VehicleChannel::begin(EntityId vid)
    {
        auto v = EntityManager::get<Vehicles::VehicleBase>(vid);
        if (v == nullptr)
        {
            return;
        }
        auto* veh26 = v->asVehicle2Or6();
        if (veh26 == nullptr)
        {
            return;
        }

        auto [sid, sa] = sub_48A590(veh26);
        auto loop = Audio::shouldSoundLoop(sid);
        auto sample = Audio::getSoundSample(sid);
        if (sample)
        {
            _vehicleId = vid;
            _soundId = sid;

            _channel.load(*sample);
            _channel.play(loop);
            _channel.setVolume(sa.volume);
            _channel.setPan(sa.pan);
            _channel.setFrequency(sa.freq);
        }
    }

    void VehicleChannel::update()
    {
        if (isFree())
        {
            return;
        }
        auto v = EntityManager::get<Vehicles::VehicleBase>(_vehicleId);
        if (v == nullptr)
        {
            stop();
            return;
        }

        auto* veh26 = v->asVehicle2Or6();
        if (veh26 == nullptr || !(veh26->var_4A & 1))
        {
            stop();
            return;
        }

        auto [sid, sa] = sub_48A590(veh26);
        if (_soundId != sid)
        {
            stop();
            return;
        }

        veh26->var_4A &= ~1;
        const auto& attributes = _channel.getAttributes();
        if (attributes.volume != sa.volume)
        {
            _channel.setVolume(sa.volume);
        }
        if (attributes.pan != sa.pan)
        {
            _channel.setPan(sa.pan);
        }
        if (attributes.freq != sa.freq)
        {
            _channel.setFrequency(sa.freq);
        }
    }

    void VehicleChannel::stop()
    {
        _channel.stop();
        _vehicleId = EntityId::null;
    }
}
