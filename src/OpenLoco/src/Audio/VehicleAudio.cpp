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

    static constexpr ReverbParams kTunnelReverb = {
        .density = 1.0f,
        .diffusion = 0.7f,
        .gain = 0.9f,
        .gainHF = 0.6f,
        .decayTime = 4.0f,
        .decayHFRatio = 0.5f,
        .reflectionsGain = 0.4f,
        .reflectionsDelay = 0.01f,
        .lateReverbGain = 2.0f,
        .lateReverbDelay = 0.025f,
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

    static Viewport* findBestViewportForEntity(const World::Pos3& position)
    {
        auto vpPos = World::gameToScreen(position, WindowManager::getCurrentRotation());

        auto main = WindowManager::getMainWindow();
        if (main != nullptr && main->viewports[0] != nullptr)
        {
            if (main->viewports[0]->contains(vpPos))
            {
                return main->viewports[0];
            }
        }

        for (size_t i = 0; i < WindowManager::count(); i++)
        {
            auto w = WindowManager::get(i);
            if (w->type == WindowType::main || w->type == WindowType::news)
            {
                continue;
            }
            auto viewport = w->viewports[0];
            if (viewport != nullptr && viewport->contains(vpPos))
            {
                return viewport;
            }
        }

        return nullptr;
    }

    static AudioAttributes getVehicleAudioAttributes(const Vehicles::VehicleBase& base, const Vehicles::VehicleSound& soundParams, const Viewport& viewport)
    {
        auto vpPos = World::gameToScreen(base.position, WindowManager::getCurrentRotation());
        const auto uiPoint = viewport.viewportToScreen(vpPos);

        const auto zoomVolumeModifier = getZoomVolumeModifier(viewport.zoom);

        const auto panX = calculatePan(uiPoint.x, Ui::width());
        const auto panY = calculatePan(uiPoint.y, Ui::height());

        const auto undergroundModifier = isUnderground(base.position) ? kVolumeModifierUnderground : 0;

        const auto xFalloffModifier = getFalloffModifier(panX);
        const auto yFalloffModifier = getFalloffModifier(panY);

        const auto falloffVolumeModifier = std::min(xFalloffModifier, yFalloffModifier);

        const auto overallVolumeModifier = std::max(falloffVolumeModifier + undergroundModifier + zoomVolumeModifier, 0);

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

    static void updateSingleVehicleSound(Vehicles::VehicleBase& base, Vehicles::VehicleSound& sound)
    {
        if (sound.drivingSoundId == SoundObjectId::null)
        {
            if (sound.audioHandle != AudioHandle::null)
            {
                stopVehicleSound(sound);
            }
            return;
        }

        auto* viewport = findBestViewportForEntity(base.position);
        if (viewport == nullptr)
        {
            if (sound.audioHandle != AudioHandle::null)
            {
                stopVehicleSound(sound);
            }
            return;
        }

        auto sid = makeObjectSoundId(sound.drivingSoundId);
        auto attribs = getVehicleAudioAttributes(base, sound, *viewport);

        if (sound.audioHandle == AudioHandle::null)
        {
            attribs.loop = shouldSoundLoop(sid);
            auto buffer = getSoundBuffer(sid);
            if (buffer)
            {
                sound.activeSoundId = sound.drivingSoundId;
                sound.audioHandle = create(*buffer, ChannelId::vehicles, attribs);
                setReverb(sound.audioHandle, isUnderground(base.position) ? kTunnelReverb : kNoReverb);
                Audio::play(sound.audioHandle);
            }
            return;
        }

        if (sound.drivingSoundId != sound.activeSoundId)
        {
            stopVehicleSound(sound);
            return;
        }

        setVolume(sound.audioHandle, attribs.volume);
        setPan(sound.audioHandle, attribs.pan);
        setPitch(sound.audioHandle, attribs.frequency);
        setReverb(sound.audioHandle, isUnderground(base.position) ? kTunnelReverb : kNoReverb);
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

        for (auto* v : VehicleManager::VehicleList())
        {
            Vehicles::Vehicle train(*v);
            updateSingleVehicleSound(*train.veh2, train.veh2->sound);
            updateSingleVehicleSound(*train.tail, train.tail->sound);
        }
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
