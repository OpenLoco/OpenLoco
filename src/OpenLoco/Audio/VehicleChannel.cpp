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
        const auto panX = (relativePositionX - (1 << 15)) / 16;
        const auto relativePositionY = (uiPoint.y << 16) / std::max(Ui::height(), 64);
        const auto panY = (relativePositionY - (1 << 15)) / 16;

        const auto undergroundVolumeModifier = getUndergroundVolumeModifier(v->position);

        const auto xFalloff = std::min(std::abs(panX), 4095);
        const auto yFalloff = std::min(std::abs(panY), 4095);

        auto xFalloffModifier = 255;
        auto yFalloffModifier = 255;
        // This in theory is the max viewport width (might not be valid for modern screens)
        if (xFalloff > 2048)
        {
            if (xFalloff > 3072)
            {
                xFalloffModifier = 0;
            }
            else
            {
                xFalloffModifier = std::min((3072 - xFalloff) / 4, 255);
            }
        }
        // This in theory is the max viewport height (might not be valid for modern screens)
        if (yFalloff > 2048)
        {
            if (yFalloff > 3072)
            {
                yFalloffModifier = 0;
            }
            else
            {
                yFalloffModifier = std::min((3072 - yFalloff) / 4, 255);
            }
        }

        const auto falloffVolumeModifier = std::min(xFalloffModifier, yFalloffModifier);

        const auto overalVolumeModifier = std::max(falloffVolumeModifier - undergroundVolumeModifier - zoomVolumeModifier, 0);

        // volume is in hundredth decibels max decrease in volume is -100dB.
        const auto volume = std::min(((v->drivingSoundVolume * overalVolumeModifier) / 8) - 8191, -10000);

        return { makeObjectSoundId(v->drivingSoundId), { volume, panX, v->drivingSoundFrequency } };
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
