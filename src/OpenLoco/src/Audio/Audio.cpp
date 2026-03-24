#include "Audio.h"
#include "Config.h"
#include "Date.h"
#include "Entities/EntityManager.h"
#include "Environment.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Jukebox.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/SoundObject.h"
#include "Objects/TreeObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/Vehicle2.h"
#include "Vehicles/VehicleHead.h"
#include "Vehicles/VehicleManager.h"
#include "Vehicles/VehicleTail.h"
#include <OpenLoco/Audio/AudioEngine.h>
#include <OpenLoco/Core/Exception.hpp>
#include <OpenLoco/Core/FileStream.h>
#include <array>
#include <cassert>
#include <numeric>
#include <unordered_map>

#ifdef _WIN32
#define __HAS_DEFAULT_DEVICE__
#endif

using namespace OpenLoco::Environment;
using namespace OpenLoco::Ui;
using namespace OpenLoco::Utility;
using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Audio
{
    static constexpr uint8_t kMaxVehicleSounds = 16;
    static constexpr int32_t kPlayAtCentre = 0x8000;
    static constexpr int32_t kPlayAtLocation = 0x8001;

    static bool _audioIsInitialised = false;
    static bool _audioIsPaused = false;
    static bool _audioIsEnabled = true;
    static std::optional<PathId> _chosenAmbientNoisePathId = std::nullopt;

    static uint8_t _numActiveVehicleSounds;
    static std::vector<std::string> _devices;

    static std::vector<BufferId> _samples;
    static std::unordered_map<uint16_t, BufferId> _objectSamples;
    static std::unordered_map<PathId, BufferId> _musicSamples;

    // Vehicle channel state
    struct VehicleChannelState
    {
        AudioHandle handle = AudioHandle::null;
        EntityId vehicleId = EntityId::null;
        SoundId soundId{};
        AudioAttributes attribs{};
    };
    static constexpr int32_t kNumVehicleChannels = 10;
    static std::array<VehicleChannelState, kNumVehicleChannels> _vehicleChannels{};

    // Ambient/music handles
    static AudioHandle _ambientHandle = AudioHandle::null;
    static AudioHandle _musicHandle = AudioHandle::null;
    static int32_t _ambientVolume = 0;

    static void playSound(SoundId id, const World::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency);

    static BufferId loadSoundFromWaveMemory(const WAVEFORMATEX& format, const void* pcm, size_t pcmLen)
    {
        AudioFormat fmt{};
        fmt.sampleRate = format.nSamplesPerSec;
        fmt.channels = format.nChannels;
        fmt.bitsPerSample = format.wBitsPerSample;
        return loadBuffer(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(pcm), pcmLen), fmt);
    }

    static std::vector<BufferId> loadSoundsFromCSS(const fs::path& path)
    {
        Logging::verbose("loadSoundsFromCSS({})", path.string());

        try
        {
            std::vector<BufferId> results;

            FileStream fs(path, StreamMode::read);
            auto numSounds = fs.readValue<uint32_t>();

            std::vector<uint32_t> soundOffsets(numSounds, 0);
            fs.read(soundOffsets.data(), numSounds * sizeof(uint32_t));

            std::vector<std::byte> pcm;
            for (uint32_t i = 0; i < numSounds; i++)
            {
                fs.setPosition(soundOffsets[i]);

                auto pcmLen = fs.readValue<uint32_t>();
                auto format = fs.readValue<WAVEFORMATEX>();

                pcm.resize(pcmLen);
                fs.read(pcm.data(), pcmLen);

                results.push_back(loadSoundFromWaveMemory(format, pcm.data(), pcmLen));
            }

            return results;
        }
        catch (const std::exception& ex)
        {
            Logging::error("loadSoundsFromCSS({}) failed: {}", path.string(), ex.what());
            return {};
        }
    }

    static std::optional<BufferId> loadMusicSample(PathId asset)
    {
        auto res = _musicSamples.find(asset);
        if (res != std::end(_musicSamples))
        {
            return res->second;
        }

        const auto path = Environment::getPath(asset);
        try
        {
            FileStream fs(path, StreamMode::read);

            const auto sig = fs.readValue<uint32_t>();
            if (sig != 0x46464952)
            {
                throw Exception::RuntimeError("Invalid signature.");
            }

            fs.readValue<uint32_t>();

            const auto riffType = fs.readValue<uint32_t>();
            if (riffType != 0x45564157)
            {
                throw Exception::RuntimeError("Invalid format.");
            }

            const auto fmtMarker = fs.readValue<uint32_t>();
            if (fmtMarker != 0x20746d66 && fmtMarker != 0x00746d66)
            {
                throw Exception::RuntimeError("Invalid format marker.");
            }

            fs.readValue<uint32_t>();

            const auto typeFormat = fs.readValue<uint16_t>();
            if (typeFormat != 1)
            {
                throw Exception::RuntimeError("Invalid format type, expected PCM.");
            }

            const auto channels = fs.readValue<uint16_t>();
            const auto sampleRate = fs.readValue<uint32_t>();

            fs.readValue<uint32_t>();
            fs.readValue<uint16_t>();

            const auto bits = fs.readValue<uint16_t>();

            const auto dataMarker = fs.readValue<uint32_t>();
            if (dataMarker != 0x61746164)
            {
                throw Exception::RuntimeError("Invalid data marker.");
            }

            const auto pcmLen = fs.readValue<uint32_t>();

            std::vector<std::uint8_t> pcm(pcmLen);
            fs.read(pcm.data(), pcmLen);

            AudioFormat fmt{};
            fmt.sampleRate = sampleRate;
            fmt.channels = channels;
            fmt.bitsPerSample = bits;
            auto id = loadBuffer(std::span<const uint8_t>(pcm.data(), pcmLen), fmt);
            _musicSamples[asset] = id;

            return id;
        }
        catch (const std::exception& ex)
        {
            Logging::error("Unable to load music sample '{}': {}", path, ex.what());
            return std::nullopt;
        }
    }

    static void disposeSamples()
    {
        for (auto& buf : _samples)
        {
            unloadBuffer(buf);
        }
        _samples.clear();
        for (auto& [_, buf] : _objectSamples)
        {
            unloadBuffer(buf);
        }
        _objectSamples.clear();
        for (auto& [_, buf] : _musicSamples)
        {
            unloadBuffer(buf);
        }
        _musicSamples.clear();
    }

    static void reinitialise()
    {
        disposeDSound();
        initialiseDSound();
    }

    static size_t getDeviceIndex(const std::string_view& deviceName)
    {
        if (deviceName.empty())
        {
            return 0;
        }

        if (_devices.empty())
        {
            getDevices();
        }

        auto fr = std::find(_devices.begin(), _devices.end(), deviceName);
        if (fr != _devices.end())
        {
            return fr - _devices.begin();
        }
        return std::numeric_limits<size_t>().max();
    }

    void initialiseDSound()
    {
        const auto& cfg = Config::get();
        openDevice(cfg.audio.device);
        initialize();

        auto css1path = Environment::getPath(Environment::PathId::css1);
        _samples = loadSoundsFromCSS(css1path);
        _audioIsInitialised = true;
    }

    void disposeDSound()
    {
        for (auto& vc : _vehicleChannels)
        {
            if (vc.handle != AudioHandle::null)
            {
                destroy(vc.handle);
                vc.handle = AudioHandle::null;
                vc.vehicleId = EntityId::null;
            }
        }

        if (_ambientHandle != AudioHandle::null)
        {
            destroy(_ambientHandle);
            _ambientHandle = AudioHandle::null;
        }

        if (_musicHandle != AudioHandle::null)
        {
            destroy(_musicHandle);
            _musicHandle = AudioHandle::null;
        }

        disposeSamples();
        shutdown();
        _audioIsInitialised = false;
    }

    void close()
    {
        if (_audioIsInitialised)
        {
            stopAmbientNoise();
            stopVehicleNoise();
            stopMusic();
        }
    }

#ifdef __HAS_DEFAULT_DEVICE__
    static const char* getDefaultDeviceName()
    {
        return StringManager::getString(StringIds::default_audio_device_name);
    }
#endif

    const std::vector<std::string>& getDevices()
    {
        _devices.clear();
#ifdef __HAS_DEFAULT_DEVICE__
        auto* ddChar = getDefaultDeviceName();
        std::string defaultDevice = ddChar == nullptr ? "" : std::string(ddChar);
        if (!defaultDevice.empty())
        {
            _devices.push_back(getDefaultDeviceName());
        }
#endif
        auto newDevices = getAvailableDevices();
        for (auto& device : newDevices)
        {
#ifdef __HAS_DEFAULT_DEVICE__
            if (defaultDevice.empty() || device != defaultDevice)
#endif
            {
                _devices.push_back(device);
            }
        }
        return _devices;
    }

    const char* getCurrentDeviceName()
    {
        auto index = getCurrentDevice();
        if (index == 0)
        {
#ifdef __HAS_DEFAULT_DEVICE__
            return getDefaultDeviceName();
#else
            if (!_devices.empty())
            {
                return _devices[0].c_str();
            }
#endif
        }

        const auto& cfg = Config::get();
        return cfg.audio.device.c_str();
    }

    size_t getCurrentDevice()
    {
        const auto& cfg = Config::get();
        return getDeviceIndex(cfg.audio.device);
    }

    void setDevice(size_t index)
    {
        if (index < _devices.size())
        {
            auto& cfg = Config::get();
#ifdef __HAS_DEFAULT_DEVICE__
            if (index == 0)
            {
                cfg.audio.device = {};
            }
            else
#endif
            {
                cfg.audio.device = _devices[index];
            }
            Config::write();
            reinitialise();
        }
    }

    void toggleSound()
    {
        _audioIsEnabled = !_audioIsEnabled;
        if (_audioIsEnabled)
        {
            return;
        }

        stopVehicleNoise();
        stopAmbientNoise();
        stopMusic();
        Config::write();
    }

    void pauseSound()
    {
        if (_audioIsPaused)
        {
            return;
        }

        _audioIsPaused = true;
        stopVehicleNoise();
        stopAmbientNoise();
        if (!SceneManager::isTitleMode())
        {
            pauseMusic();
        }
    }

    void unpauseSound()
    {
        _audioIsPaused = false;
        unpauseMusic();
    }

    static const SoundObject* getSoundObject(SoundId id)
    {
        auto idx = (int32_t)id & ~0x8000;
        return ObjectManager::get<SoundObject>(idx);
    }

    static Viewport* findBestViewportForSound(viewport_pos vpos)
    {
        auto w = WindowManager::find(WindowType::main, 0);
        if (w != nullptr)
        {
            auto viewport = w->viewports[0];
            if (viewport != nullptr && viewport->contains(vpos))
            {
                return viewport;
            }
        }

        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            w = WindowManager::get(i);
            if (w != nullptr && w->type != WindowType::main && w->type != WindowType::news)
            {
                auto viewport = w->viewports[0];
                if (viewport != nullptr && viewport->contains(vpos))
                {
                    return viewport;
                }
            }
        }

        return nullptr;
    }

    static constexpr std::array<int32_t, 32> kSoundVolumeTable = { {
        // clang-format off
           0,    0,    0,    0,    0, -400,    0,    0,
           0,    0,    0, -500,    0,    0,    0,    0,
           0,    0,    0,    0,    0,    0, -1900,    0,
           0,    0, -600, -600, -600, -600, -600, -600,
        // clang-format on
    } };

    static int32_t getVolumeForSoundId(SoundId id)
    {
        if (isObjectSoundId(id))
        {
            auto obj = getSoundObject(id);
            if (obj != nullptr)
            {
                return obj->volume;
            }
            return 0;
        }
        else
        {
            return kSoundVolumeTable[static_cast<int32_t>(id)];
        }
    }

    static int32_t calculateVolumeFromViewport([[maybe_unused]] SoundId id, const World::Pos3& mpos, const Viewport& viewport)
    {
        auto volume = 0;
        auto zVol = 0;
        auto tile = World::TileManager::get(mpos);
        if (!tile.isNull())
        {
            auto surface = tile.surface();
            if (surface != nullptr)
            {
                if ((surface->baseHeight()) - 5 > mpos.z)
                {
                    zVol = 8;
                }
            }
            volume = ((-1024 * viewport.zoom - 1) << zVol) + 1;
        }
        return volume;
    }

    void playSound(SoundId id, const World::Pos3& loc)
    {
        playSound(id, loc, kPlayAtLocation);
    }

    void playSound(SoundId id, int32_t pan)
    {
        playSound(id, {}, pan);
    }

    bool shouldSoundLoop(SoundId id)
    {
        if (!isObjectSoundId(id))
        {
            return false;
        }

        auto obj = getSoundObject(id);
        return obj->shouldLoop != 0;
    }

    void playSound(SoundId id, const World::Pos3& loc, int32_t volume, int32_t frequency)
    {
        playSound(id, loc, volume, kPlayAtLocation, frequency);
    }

    void playSound(SoundId id, const World::Pos3& loc, int32_t pan)
    {
        playSound(id, loc, 0, pan, 22050);
    }

    constexpr int32_t kVpSizeMin = 64;

    int32_t calculatePan(const coord_t coord, const int32_t screenSize)
    {
        const auto relativePosition = (coord << 16) / std::max(screenSize, kVpSizeMin);
        return (relativePosition - (1 << 15)) / 16;
    }

    void playSound(SoundId id, const World::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency)
    {
        if (_audioIsEnabled)
        {
            volume += getVolumeForSoundId(id);
            if (pan == kPlayAtLocation)
            {
                auto vpos = World::gameToScreen(loc, WindowManager::getCurrentRotation());
                auto viewport = findBestViewportForSound(vpos);
                if (viewport == nullptr)
                {
                    return;
                }

                volume += calculateVolumeFromViewport(id, loc, *viewport);
                pan = viewport->viewportToScreen(vpos).x;
                if (volume < -10000)
                {
                    return;
                }
            }
            else if (pan == kPlayAtCentre)
            {
                pan = 0;
            }

            if (pan != 0)
            {
                pan = calculatePan(pan, Ui::width());
            }

            auto buffer = getSoundBuffer(id);
            if (buffer)
            {
                AudioAttributes attribs{};
                attribs.volume = volume;
                attribs.pan = pan;
                attribs.frequency = frequency;
                attribs.loop = false;
                auto handle = create(*buffer, ChannelId::effects, attribs);
                Audio::play(handle);
            }
        }
    }

    std::optional<BufferId> getSoundBuffer(SoundId id)
    {
        if (isObjectSoundId(id))
        {
            auto sr = _objectSamples.find((uint16_t)id);
            if (sr == _objectSamples.end())
            {
                auto obj = getSoundObject(id);
                if (obj != nullptr)
                {
                    const auto* data = obj->getData();
                    assert(data != nullptr);
                    assert(data->offset == 8);
                    _objectSamples[static_cast<size_t>(id)] = loadSoundFromWaveMemory(data->pcmHeader, data->pcm(), data->length);
                    return _objectSamples[static_cast<size_t>(id)];
                }
            }
            else
            {
                return sr->second;
            }
        }
        else if (static_cast<size_t>(id) < _samples.size())
        {
            return _samples[static_cast<size_t>(id)];
        }
        return std::nullopt;
    }

    AudioHandle play(SoundId id, ChannelId channel, const AudioAttributes& attribs)
    {
        auto buffer = getSoundBuffer(id);
        if (!buffer)
        {
            return AudioHandle::null;
        }
        auto handle = create(*buffer, channel, attribs);
        Audio::play(handle);
        return handle;
    }

    // Vehicle audio

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

    static void updateVehicleNoise()
    {
        if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
        {
            if (!_audioIsPaused && _audioIsEnabled)
            {
                processVehicleSounds(0);
                processVehicleSounds(1);
                processVehicleSounds(2);
                for (auto& vc : _vehicleChannels)
                {
                    vehicleChannelUpdate(vc);
                }
                processVehicleSounds(3);
            }
        }
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

    // Ambient audio

    static constexpr auto kAmbientMinVolume = -3500;
    static constexpr auto kAmbientVolumeChangePerTick = 100;
    static constexpr auto kAmbientNumWaterTilesForOcean = 60;
    static constexpr auto kAmbientNumTreeTilesForForest = 30;
    static constexpr auto kAmbientNumMountainTilesForWilderness = 60;

    static constexpr int32_t getAmbientMaxVolume(uint8_t zoom)
    {
        constexpr int32_t _volumes[]{ -1200, -2000, -3000, -3000 };
        return _volumes[zoom];
    }

    static void updateAmbientNoise()
    {
        if (!_audioIsInitialised || _audioIsPaused || !_audioIsEnabled)
        {
            return;
        }

        auto* mainViewport = WindowManager::getMainViewport();
        std::optional<PathId> newAmbientSound = std::nullopt;
        int32_t maxVolume = kAmbientMinVolume;

        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && mainViewport != nullptr)
        {
            maxVolume = getAmbientMaxVolume(mainViewport->zoom);
            const auto centre = mainViewport->getCentreMapPosition();
            const auto topLeft = World::toTileSpace(centre) - World::TilePos2{ 5, 5 };
            const auto bottomRight = topLeft + World::TilePos2{ 11, 11 };
            const auto searchRange = World::getClampedRange(topLeft, bottomRight);

            size_t waterCount = 0;
            size_t wildernessCount = 0;
            size_t treeCount = 0;
            for (auto& tilePos : searchRange)
            {
                const auto tile = World::TileManager::get(tilePos);
                bool passedSurface = false;
                for (const auto& el : tile)
                {
                    auto* elSurface = el.as<World::SurfaceElement>();
                    if (elSurface != nullptr)
                    {
                        passedSurface = true;
                        if (elSurface->water() != 0)
                        {
                            waterCount++;
                            break;
                        }
                        else if (elSurface->snowCoverage() && elSurface->isLast())
                        {
                            wildernessCount++;
                            break;
                        }
                        else if (elSurface->baseZ() >= 64 && elSurface->isLast())
                        {
                            wildernessCount++;
                            break;
                        }
                        continue;
                    }
                    auto* elTree = el.as<World::TreeElement>();
                    if (passedSurface && elTree != nullptr)
                    {
                        const auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                        if (!treeObj->hasFlags(TreeObjectFlags::droughtResistant))
                        {
                            treeCount++;
                        }
                    }
                }
            }

            if (waterCount > kAmbientNumWaterTilesForOcean)
            {
                newAmbientSound = PathId::css3;
            }
            else if (wildernessCount > kAmbientNumMountainTilesForWilderness)
            {
                newAmbientSound = PathId::css2;
            }
            else if (treeCount > kAmbientNumTreeTilesForForest)
            {
                newAmbientSound = PathId::css4;
            }
        }

        bool ambientPlaying = _ambientHandle != AudioHandle::null && isPlaying(_ambientHandle);

        if (!newAmbientSound.has_value() || (ambientPlaying && _chosenAmbientNoisePathId != *newAmbientSound))
        {
            const auto newVolume = _ambientVolume - kAmbientVolumeChangePerTick;
            if (newVolume < kAmbientMinVolume)
            {
                _chosenAmbientNoisePathId = std::nullopt;
                if (_ambientHandle != AudioHandle::null)
                {
                    destroy(_ambientHandle);
                    _ambientHandle = AudioHandle::null;
                }
            }
            else
            {
                _ambientVolume = newVolume;
                if (_ambientHandle != AudioHandle::null)
                {
                    setVolume(_ambientHandle, newVolume);
                }
            }
            return;
        }

        if (_chosenAmbientNoisePathId != *newAmbientSound)
        {
            auto musicBuffer = loadMusicSample(*newAmbientSound);
            if (musicBuffer.has_value())
            {
                if (_ambientHandle != AudioHandle::null)
                {
                    destroy(_ambientHandle);
                }
                AudioAttributes attribs{};
                attribs.volume = kAmbientMinVolume;
                attribs.loop = true;
                _ambientHandle = create(*musicBuffer, ChannelId::effects, attribs);
                Audio::play(_ambientHandle);
                _ambientVolume = kAmbientMinVolume;
                _chosenAmbientNoisePathId = *newAmbientSound;
            }
        }
        else
        {
            auto newVolume = std::min(_ambientVolume + kAmbientVolumeChangePerTick, maxVolume);
            _ambientVolume = newVolume;
            if (_ambientHandle != AudioHandle::null)
            {
                setVolume(_ambientHandle, newVolume);
            }
        }
    }

    void stopAmbientNoise()
    {
        if (_audioIsInitialised && _ambientHandle != AudioHandle::null)
        {
            destroy(_ambientHandle);
            _ambientHandle = AudioHandle::null;
        }
    }

    void update()
    {
        updateVehicleNoise();
        updateAmbientNoise();
    }

    // Music

    void revalidateCurrentTrack()
    {
        const auto currentTrack = Jukebox::getCurrentTrack();
        if (currentTrack == Jukebox::kNoSong)
        {
            return;
        }

        using Config::MusicPlaylistType;
        const auto& cfg = Config::get();

        bool trackStillApplies = true;
        switch (cfg.audio.playlist)
        {
            case MusicPlaylistType::currentEra:
            {
                auto currentYear = getCurrentYear();
                const auto& info = Jukebox::getMusicInfo(currentTrack);
                if (currentYear < info.startYear || currentYear > info.endYear)
                {
                    trackStillApplies = false;
                }
                break;
            }

            case MusicPlaylistType::all:
                return;

            case MusicPlaylistType::custom:
                if (!cfg.audio.customJukebox[currentTrack])
                {
                    trackStillApplies = false;
                }
                break;
        }

        if (!trackStillApplies)
        {
            stopMusic();
            Jukebox::resetJukebox();
        }
    }

    void playBackgroundMusic()
    {
        auto& cfg = Config::get();
        if (cfg.audio.playJukeboxMusic == 0 || SceneManager::isTitleMode() || SceneManager::isEditorMode() || SceneManager::isPaused())
        {
            return;
        }

        bool musicPlaying = _musicHandle != AudioHandle::null && isPlaying(_musicHandle);
        if (!musicPlaying)
        {
            const auto& mi = Jukebox::changeTrack();
            playMusic(mi.pathId, cfg.audio.mainVolume, false);
            WindowManager::invalidate(WindowType::options);
        }
    }

    void playMusic(PathId sample, int32_t volume, bool loop)
    {
        if (!_audioIsInitialised || _audioIsPaused || !_audioIsEnabled)
        {
            return;
        }

        if (_musicHandle != AudioHandle::null)
        {
            destroy(_musicHandle);
            _musicHandle = AudioHandle::null;
        }

        auto musicSample = loadMusicSample(sample);
        if (musicSample.has_value())
        {
            AudioAttributes attribs{};
            attribs.volume = volume;
            attribs.loop = loop;
            _musicHandle = create(*musicSample, ChannelId::music, attribs);
            Audio::play(_musicHandle);
        }
    }

    void resetMusic()
    {
        stopMusic();
        Jukebox::resetJukebox();
    }

    void stopMusic()
    {
        if (_audioIsInitialised && _musicHandle != AudioHandle::null)
        {
            destroy(_musicHandle);
            _musicHandle = AudioHandle::null;
        }
    }

    void pauseMusic()
    {
        if (_audioIsInitialised && _musicHandle != AudioHandle::null && isPlaying(_musicHandle))
        {
            pause(_musicHandle);
        }
    }

    void unpauseMusic()
    {
        if (_audioIsInitialised && _musicHandle != AudioHandle::null && isPaused(_musicHandle))
        {
            unpause(_musicHandle);
        }
    }

    void resetSoundObjects()
    {
        for (auto& [_, buf] : _objectSamples)
        {
            unloadBuffer(buf);
        }
        _objectSamples.clear();
    }

    bool isAudioEnabled()
    {
        return _audioIsEnabled;
    }

    void setBgmVolume(int32_t volume)
    {
        auto& cfg = Config::get().audio;
        if (cfg.mainVolume == volume)
        {
            return;
        }

        cfg.mainVolume = volume;
        Config::write();

        if (_audioIsInitialised && _musicHandle != AudioHandle::null && Jukebox::getCurrentTrack() != Jukebox::kNoSong)
        {
            setVolume(_musicHandle, volume);
        }
    }
}
