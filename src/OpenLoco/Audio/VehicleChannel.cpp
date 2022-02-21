#include "VehicleChannel.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Audio
{
    static uint8_t getZoomVolumeModifier(uint8_t zoom)
    {
        return std::min<uint8_t>(zoom, 2) * 35;
    }

    static uint8_t getUndergroundVolumeModifier(const Map::Pos3& pos)
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

    static uint8_t getFalloffModifier(int32_t pan)
    {
        const auto absPan = std::min(std::abs(pan), 4095);

        uint8_t falloffModifier = 255;
        // This in theory is the max viewport width/height (might not be valid for modern screens)
        if (absPan > 2048)
        {
            if (absPan > 3072)
            {
                falloffModifier = 0;
            }
            else
            {
                falloffModifier = std::min((3072 - absPan) / 4, 255);
            }
        }
        return falloffModifier;
    }

    static int32_t calculatePan(const coord_t coord, const int32_t screenSize)
    {
        const auto relativePosition = (coord << 16) / std::max(screenSize, 64);
        return (relativePosition - (1 << 15)) / 16;
    }

    static std::pair<SoundId, Channel::Attributes> getChannelAttributesFromVehicle(const Vehicles::Vehicle2or6* v)
    {
        auto* w = Ui::WindowManager::find(v->soundWindowType, v->soundWindowNumber);
        auto* viewport = w->viewports[0];
        const auto uiPoint = viewport->viewportToScreen({ v->sprite_left, v->sprite_top });

        const auto zoomVolumeModifier = getZoomVolumeModifier(viewport->zoom);

        const auto panX = calculatePan(uiPoint.x, Ui::width());
        const auto panY = calculatePan(uiPoint.y, Ui::height());

        const auto undergroundVolumeModifier = getUndergroundVolumeModifier(v->position);

        const auto xFalloffModifier = getFalloffModifier(panX);
        const auto yFalloffModifier = getFalloffModifier(panY);

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

        auto [sid, sa] = getChannelAttributesFromVehicle(veh26);
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

        auto [sid, sa] = getChannelAttributesFromVehicle(veh26);
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
