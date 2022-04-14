#include "VehicleChannel.h"
#include "../Entities/EntityManager.h"
#include "../Interop/Interop.hpp"
#include "../Map/TileManager.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Audio
{
    // Vehicle Volume is multiplied by the modifier then divided by 8 to get in terms of hundredth decibels
    constexpr float kVolumeModifierZoomIncrement = 0.137f; // 13.7% decrease in volume for each zoom increment (up to 2)
    constexpr float kVolumeModifierUnderground = 0.11f;    // 11.0% decrease in volume when underground
    constexpr float kVolumeModifierMax = 1.f;

    constexpr float kVolumeMin = 0.f; // hundredth decibels (-100dB)
    // Calculated min is 255*255/8=8128 but original has done 256*256/8=8192
    // We have kept original value but could be changed to correctly represent the full range of volume
    constexpr int32_t kVehicleVolumeCalcMin = -81'91; // hundredth decibels (-81.91dB)

    constexpr int32_t kPanFalloffStart = 2048;
    constexpr int32_t kPanFalloffEnd = 3072;

    static float getZoomVolumeModifier(uint8_t zoom)
    {
        return std::min<uint8_t>(zoom, 2) * kVolumeModifierZoomIncrement;
    }

    static float getUndergroundVolumeModifier(const Map::Pos3& pos)
    {
        if (pos.x == Location::null || !Map::validCoords(pos))
        {
            return 0;
        }
        auto* surface = Map::TileManager::get(pos).surface();
        if (surface->baseZ() * 4 > pos.z)
        {
            return kVolumeModifierUnderground;
        }
        return 0;
    }

    static float getFalloffModifier(int32_t pan)
    {
        const auto absPan = std::abs(pan);

        float falloffModifier = kVolumeModifierMax;
        // This in theory is the max viewport width/height (might not be valid for modern screens)
        if (absPan > kPanFalloffStart)
        {
            if (absPan > kPanFalloffEnd)
            {
                falloffModifier = 0;
            }
            else
            {
                falloffModifier = std::min(static_cast<float>(kPanFalloffEnd - absPan) / 1024.f, kVolumeModifierMax);
            }
        }
        return falloffModifier;
    }

    // 0x0048A590
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

        const auto overallVolumeModifier = std::max(falloffVolumeModifier + undergroundVolumeModifier + zoomVolumeModifier, 0.f);

        auto drivingSoundVolume = static_cast<float>(v->drivingSoundVolume) / static_cast<float>(std::numeric_limits<uint8_t>().max());
        const auto volume = std::max(drivingSoundVolume * overallVolumeModifier / 8.f, kVolumeMin);

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
