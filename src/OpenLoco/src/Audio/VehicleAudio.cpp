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

using namespace OpenLoco::Ui;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Audio
{
    static constexpr uint8_t kMaxVehicleSounds = 16;

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

    static bool isUnderground(const World::Pos3& pos)
    {
        if (pos.x == Location::null || !World::validCoords(pos))
        {
            return false;
        }
        auto* surface = World::TileManager::get(pos).surface();
        return surface->baseHeight() > pos.z;
    }

    static int8_t getUndergroundVolumeModifier(const World::Pos3& pos)
    {
        return isUnderground(pos) ? kVolumeModifierUnderground : 0;
    }

    static constexpr ReverbParams kTunnelReverb = {
        .density = 0.8f,
        .diffusion = 0.5f,
        .gain = 0.6f,
        .gainHF = 0.7f,
        .decayTime = 2.5f,
        .decayHFRatio = 0.65f,
        .reflectionsGain = 0.2f,
        .reflectionsDelay = 0.015f,
        .lateReverbGain = 1.5f,
        .lateReverbDelay = 0.02f,
    };

    static constexpr ReverbParams kNoReverb = {
        .gain = 0.0f,
    };

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

    static AudioAttributes getVehicleAudioAttributes(const Vehicles::VehicleBase& base, const Vehicles::VehicleSound& soundParams)
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
        return attribs;
    }

    static void stopVehicleSound(Vehicles::VehicleSound& sound)
    {
        if (sound.audioHandle != AudioHandle::null)
        {
            destroy(sound.audioHandle);
            sound.audioHandle = AudioHandle::null;
        }
        sound.activeSoundId = SoundObjectId::null;
    }

    static void beginVehicleSound(Vehicles::VehicleBase& base, Vehicles::VehicleSound& sound)
    {
        auto sid = makeObjectSoundId(sound.drivingSoundId);
        auto attribs = getVehicleAudioAttributes(base, sound);
        attribs.loop = shouldSoundLoop(sid);

        auto buffer = getSoundBuffer(sid);
        if (buffer)
        {
            sound.activeSoundId = sound.drivingSoundId;
            sound.audioHandle = create(*buffer, ChannelId::vehicles, attribs);
            setReverb(sound.audioHandle, isUnderground(base.position) ? kTunnelReverb : kNoReverb);
            Audio::play(sound.audioHandle);
        }
    }

    static void updateVehicleSound(Vehicles::VehicleBase& base, Vehicles::VehicleSound& sound)
    {
        if (sound.audioHandle == AudioHandle::null)
        {
            return;
        }

        if (!base.hasSoundPlayer())
        {
            stopVehicleSound(sound);
            return;
        }

        if ((sound.soundFlags & Vehicles::SoundFlags::flag0) == Vehicles::SoundFlags::none)
        {
            stopVehicleSound(sound);
            return;
        }

        if (sound.drivingSoundId != sound.activeSoundId)
        {
            stopVehicleSound(sound);
            return;
        }

        auto attribs = getVehicleAudioAttributes(base, sound);
        sound.soundFlags &= ~Vehicles::SoundFlags::flag0;

        setVolume(sound.audioHandle, attribs.volume);
        setPan(sound.audioHandle, attribs.pan);
        setPitch(sound.audioHandle, attribs.frequency);
        setReverb(sound.audioHandle, isUnderground(base.position) ? kTunnelReverb : kNoReverb);
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

    static void tryBeginVehicleSound(Vehicles::VehicleBase& v, Vehicles::VehicleSound& sound)
    {
        if ((sound.soundFlags & Vehicles::SoundFlags::flag0) != Vehicles::SoundFlags::none
            && sound.audioHandle == AudioHandle::null)
        {
            beginVehicleSound(v, sound);
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
                tryBeginVehicleSound(v, soundParams);
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

        for (auto* v : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            updateVehicleSound(*train.veh2, train.veh2->sound);
            updateVehicleSound(*train.tail, train.tail->sound);
        }

        processVehicleSounds(3);
    }

    void stopVehicleNoise()
    {
        for (auto* v : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            stopVehicleSound(train.veh2->sound);
            stopVehicleSound(train.tail->sound);
        }
    }

    void stopVehicleNoise(EntityId head)
    {
        Vehicles::Vehicle train(head);
        stopVehicleSound(train.veh2->sound);
        stopVehicleSound(train.tail->sound);
    }
}
