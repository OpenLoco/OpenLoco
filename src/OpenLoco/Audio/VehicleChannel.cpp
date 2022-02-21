#include "VehicleChannel.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Audio
{
    uint8_t getZoomVolumeModifier(uint8_t zoom)
    {
        switch (zoom)
        {
            case 0:
                return 0;
            case 1:
                return 35;
            default:
                return 70;
        }
    }

    uint8_t getUndergroundVolumeModifier(const Map::Pos3& pos)
    {
        if (pos.x != Location::null && Map::validCoords(pos))
        {
            return 0;
        }
        auto* surface = Map::TileManager::get(pos).surface();
        if (surface->baseZ() * 4 > pos.z)
        {
            return 28;
        }
        return 0;
    }

    static std::pair<SoundId, Channel::Attributes> sub_48A590(const Vehicles::Vehicle2or6* v)
    {
        auto* w = Ui::WindowManager::find(v->soundWindowType, v->soundWindowNumber);
        auto* viewport = w->viewports[0];
        const auto uiPoint = viewport->viewportToScreen({ v->sprite_left, v->sprite_top });

        const auto zoomVolumeModifier = getZoomVolumeModifier(viewport->zoom);

        const auto relativePositionX = (uiPoint.x << 16) / std::max(Ui::width(), 64);
        const auto pan = (relativePositionX - (1 << 15)) / 16;

        const auto undergroundVolumeModifier = getUndergroundVolumeModifier(v->position);

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
