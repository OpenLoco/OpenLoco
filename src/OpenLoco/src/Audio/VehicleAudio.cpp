#include "Audio.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Location.hpp"
#include "Logging.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/SoundObject.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"
#include "Vehicles/VehicleTail.h"
#include <OpenLoco/Audio/AudioEngine.h>
#include <array>

using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Audio
{
    struct VehicleChannelState
    {
        AudioHandle handle = AudioHandle::null;
        EntityId vehicleId = EntityId::null;
        SoundId soundId{};
        AudioAttributes attribs{};
    };

    static constexpr int32_t kNumVehicleChannels = 10;
    static constexpr uint8_t kMaxVehicleSounds = 16;

    static std::array<VehicleChannelState, kNumVehicleChannels> _vehicleChannels{};
    static uint8_t _numActiveVehicleSounds;

    constexpr int8_t kVolumeModifierZoomIncrement = -35;
    constexpr int8_t kVolumeModifierUnderground = -28;
    constexpr uint8_t kVolumeModifierMax = 255;
    constexpr int32_t kVolumeMin = -100'00;
    constexpr int32_t kVehicleVolumeCalcMin = -81'91;
    constexpr int32_t kPanFalloffStart = 2048;
    constexpr int32_t kPanFalloffEnd = 3072;

    static int8_t getZoomVolumeModifier(uint8_t zoom)
    {
        return std::min<uint8_t>(zoom, 2) * kVolumeModifierZoomIncrement;
    }

    static int8_t getUndergroundVolumeModifier(const World::Pos3& pos)
    {
        if (pos.x == Location::null || !World::validCoords(pos))
        {
            return 0;
        }
        auto* surface = World::TileManager::get(pos).surface();
        if (surface->baseHeight() > pos.z)
        {
            return kVolumeModifierUnderground;
        }
        return 0;
    }

    static uint8_t getFalloffModifier(int32_t pan)
    {
        const auto absPan = std::abs(pan);

        uint8_t falloffModifier = kVolumeModifierMax;
        if (absPan > kPanFalloffStart)
        {
            if (absPan > kPanFalloffEnd)
            {
                falloffModifier = 0;
            }
            else
            {
                falloffModifier = static_cast<uint8_t>(std::min<uint16_t>((kPanFalloffEnd - absPan) / 4, kVolumeModifierMax));
            }
        }
        return falloffModifier;
    }

    static std::pair<SoundId, AudioAttributes> getVehicleAudioAttributes(const Vehicles::VehicleBase& base, const Vehicles::VehicleSound& soundParams)
    {
        auto* w = WindowManager::find(soundParams.soundWindowType, soundParams.soundWindowNumber);
        auto* viewport = w->viewports[0];
        const auto uiPoint = viewport->viewportToScreen({ base.spriteLeft, base.spriteTop });

        const auto zoomVolumeModifier = getZoomVolumeModifier(viewport->zoom);

        const auto panX = calculatePan(uiPoint.x, Ui::width());
        const auto panY = calculatePan(uiPoint.y, Ui::height());

        const auto undergroundVolumeModifier = getUndergroundVolumeModifier(base.position);

        const auto xFalloffModifier = getFalloffModifier(panX);
        const auto yFalloffModifier = getFalloffModifier(panY);

        const auto falloffVolumeModifier = std::min(xFalloffModifier, yFalloffModifier);

        const auto overallVolumeModifier = std::max(falloffVolumeModifier + undergroundVolumeModifier + zoomVolumeModifier, 0);

        const auto volume = std::max(((soundParams.drivingSoundVolume * overallVolumeModifier) / 8) + kVehicleVolumeCalcMin, kVolumeMin);

        AudioAttributes attribs{};
        attribs.volume = volume;
        attribs.pan = panX;
        attribs.frequency = soundParams.drivingSoundFrequency;
        return { makeObjectSoundId(soundParams.drivingSoundId), attribs };
    }

    static VehicleChannelState* getFreeVehicleChannel()
    {
        for (auto& vc : _vehicleChannels)
        {
            if (vc.vehicleId == EntityId::null)
            {
                return &vc;
            }
        }
        return nullptr;
    }

    static void vehicleChannelBegin(VehicleChannelState& vc, EntityId vid)
    {
        auto v = EntityManager::get<Vehicles::VehicleBase>(vid);
        if (v == nullptr || !v->hasSoundPlayer())
        {
            return;
        }
        auto* soundParams = v->getVehicleSound();
        if (soundParams == nullptr)
        {
            return;
        }

        auto [sid, attribs] = getVehicleAudioAttributes(*v, *soundParams);
        auto loop = shouldSoundLoop(sid);
        auto buffer = getSoundBuffer(sid);
        if (buffer)
        {
            vc.vehicleId = vid;
            vc.soundId = sid;
            vc.attribs = attribs;

            attribs.loop = loop;
            vc.handle = create(*buffer, ChannelId::vehicles, attribs);
            Audio::play(vc.handle);
        }
    }

    static void vehicleChannelUpdate(VehicleChannelState& vc)
    {
        if (vc.vehicleId == EntityId::null)
        {
            return;
        }
        auto v = EntityManager::get<Vehicles::VehicleBase>(vc.vehicleId);
        if (v == nullptr || !v->hasSoundPlayer())
        {
            destroy(vc.handle);
            vc.handle = AudioHandle::null;
            vc.vehicleId = EntityId::null;
            return;
        }

        auto* soundParams = v->getVehicleSound();
        if (soundParams == nullptr || ((soundParams->soundFlags & Vehicles::SoundFlags::flag0) == Vehicles::SoundFlags::none))
        {
            destroy(vc.handle);
            vc.handle = AudioHandle::null;
            vc.vehicleId = EntityId::null;
            return;
        }

        auto [sid, attribs] = getVehicleAudioAttributes(*v, *soundParams);
        if (vc.soundId != sid)
        {
            destroy(vc.handle);
            vc.handle = AudioHandle::null;
            vc.vehicleId = EntityId::null;
            return;
        }

        soundParams->soundFlags &= ~Vehicles::SoundFlags::flag0;
        if (vc.attribs.volume != attribs.volume)
        {
            setVolume(vc.handle, attribs.volume);
        }
        if (vc.attribs.pan != attribs.pan)
        {
            setPan(vc.handle, attribs.pan);
        }
        if (vc.attribs.frequency != attribs.frequency)
        {
            setPitch(vc.handle, attribs.frequency);
        }
        vc.attribs = attribs;
    }

    static void vehicleChannelStop(VehicleChannelState& vc)
    {
        if (vc.handle != AudioHandle::null)
        {
            destroy(vc.handle);
            vc.handle = AudioHandle::null;
        }
        vc.vehicleId = EntityId::null;
    }

    static void triggerVehicleSoundIfInView(Vehicles::VehicleBase& v, Vehicles::VehicleSound& soundParams)
    {
        if (soundParams.drivingSoundId == SoundObjectId::null)
        {
            return;
        }

        if (v.spriteLeft == Location::null)
        {
            return;
        }

        if (_numActiveVehicleSounds >= kMaxVehicleSounds)
        {
            return;
        }

        auto spritePosition = viewport_pos(v.spriteLeft, v.spriteTop);

        auto main = WindowManager::getMainWindow();
        if (main != nullptr && main->viewports[0] != nullptr)
        {
            auto viewport = main->viewports[0];
            ViewportRect extendedViewport = {};

            auto quarterWidth = viewport->viewWidth / 4;
            auto quarterHeight = viewport->viewHeight / 4;
            extendedViewport.left = viewport->viewX - quarterWidth;
            extendedViewport.top = viewport->viewY - quarterHeight;
            extendedViewport.right = viewport->viewX + viewport->viewWidth + quarterWidth;
            extendedViewport.bottom = viewport->viewY + viewport->viewHeight + quarterHeight;

            if (extendedViewport.contains(spritePosition))
            {
                _numActiveVehicleSounds += 1;
                soundParams.soundFlags |= Vehicles::SoundFlags::flag0;
                soundParams.soundWindowType = main->type;
                soundParams.soundWindowNumber = main->number;
                return;
            }
        }

        if (WindowManager::count() == 0)
        {
            return;
        }

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type == WindowType::main)
            {
                continue;
            }

            if (w->type == WindowType::news)
            {
                continue;
            }

            auto viewport = w->viewports[0];
            if (viewport == nullptr)
            {
                continue;
            }

            if (viewport->contains(spritePosition))
            {
                _numActiveVehicleSounds += 1;
                soundParams.soundFlags |= Vehicles::SoundFlags::flag0;
                soundParams.soundWindowType = w->type;
                soundParams.soundWindowNumber = w->number;
                return;
            }
        }
    }

    static void playVehicleSound(EntityId id, Vehicles::VehicleSound& soundParams)
    {
        if ((soundParams.soundFlags & Vehicles::SoundFlags::flag0) != Vehicles::SoundFlags::none)
        {
            Logging::verbose("playSound(vehicle #{})", enumValue(id));
            auto vc = getFreeVehicleChannel();
            if (vc != nullptr)
            {
                vehicleChannelBegin(*vc, id);
            }
        }
    }

    static void processVehicleForSound(Vehicles::VehicleBase& v, Vehicles::VehicleSound& soundParams, int32_t step)
    {
        switch (step)
        {
            case 0:
                soundParams.soundFlags &= ~Vehicles::SoundFlags::flag0;
                break;
            case 1:
                if ((soundParams.soundFlags & Vehicles::SoundFlags::flag1) == Vehicles::SoundFlags::none)
                {
                    triggerVehicleSoundIfInView(v, soundParams);
                }
                break;
            case 2:
                if ((soundParams.soundFlags & Vehicles::SoundFlags::flag1) != Vehicles::SoundFlags::none)
                {
                    triggerVehicleSoundIfInView(v, soundParams);
                }
                break;
            case 3:
                playVehicleSound(v.id, soundParams);
                break;
        }
    }

    static void processVehicleSounds(int32_t step)
    {
        if (step == 0)
        {
            _numActiveVehicleSounds = 0;
        }

        for (auto* v : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            processVehicleForSound(*train.veh2, train.veh2->sound, step);
            processVehicleForSound(*train.tail, train.tail->sound, step);
        }
    }

    void updateVehicleNoise()
    {
        if (!Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            return;
        }
        if (!isAudioEnabled())
        {
            return;
        }

        processVehicleSounds(0);
        processVehicleSounds(1);
        processVehicleSounds(2);
        for (auto& vc : _vehicleChannels)
        {
            vehicleChannelUpdate(vc);
        }
        processVehicleSounds(3);
    }

    void stopVehicleNoise()
    {
        for (auto& vc : _vehicleChannels)
        {
            vehicleChannelStop(vc);
        }
    }

    void stopVehicleNoise(EntityId head)
    {
        Vehicles::Vehicle train(head);
        for (auto& vc : _vehicleChannels)
        {
            if (vc.vehicleId == train.veh2->id || vc.vehicleId == train.tail->id)
            {
                vehicleChannelStop(vc);
            }
        }
    }
}
