#include "Audio.h"
#include "Config.h"
#include "Environment.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/ObjectManager.h"
#include "Objects/SoundObject.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
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
    static constexpr int32_t kPlayAtCentre = 0x8000;
    static constexpr int32_t kPlayAtLocation = 0x8001;

    static bool _audioIsEnabled = true;
    static std::vector<std::string> _devices;

    static std::vector<BufferId> _samples;
    static std::unordered_map<uint16_t, BufferId> _objectSamples;
    static std::unordered_map<PathId, BufferId> _musicSamples;

    void updateVehicleNoise();
    void updateAmbientNoise();

    static void playSound(SoundId id, ChannelId channel, const World::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency);

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

    std::optional<BufferId> loadMusicSample(PathId asset)
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

        setChannelVolume(ChannelId::master, cfg.audio.masterVolume);
        setChannelVolume(ChannelId::music, cfg.audio.musicVolume);
        setChannelVolume(ChannelId::effects, cfg.audio.effectsVolume);
        setChannelVolume(ChannelId::vehicles, cfg.audio.vehiclesVolume);
        setChannelVolume(ChannelId::ui, cfg.audio.uiVolume);
        setChannelVolume(ChannelId::ambient, cfg.audio.ambientVolume);

        auto css1path = Environment::getPath(Environment::PathId::css1);
        _samples = loadSoundsFromCSS(css1path);
    }

    void disposeDSound()
    {
        stopVehicleNoise();
        stopAmbientNoise();
        stopMusic();
        disposeSamples();
        shutdown();
    }

    void close()
    {
        stopAll();
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
        stopVehicleNoise();
        stopAmbientNoise();
        if (!SceneManager::isTitleMode())
        {
            pauseMusic();
        }
    }

    void unpauseSound()
    {
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

    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc)
    {
        playSound(id, channel, loc, kPlayAtLocation);
    }

    void playSound(SoundId id, ChannelId channel, int32_t pan)
    {
        playSound(id, channel, {}, pan);
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

    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc, int32_t volume, int32_t frequency)
    {
        playSound(id, channel, loc, volume, kPlayAtLocation, frequency);
    }

    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc, int32_t pan)
    {
        playSound(id, channel, loc, 0, pan, 22050);
    }

    constexpr int32_t kVpSizeMin = 64;

    int32_t calculatePan(const coord_t coord, const int32_t screenSize)
    {
        const auto relativePosition = (coord << 16) / std::max(screenSize, kVpSizeMin);
        return (relativePosition - (1 << 15)) / 16;
    }

    void playSound(SoundId id, ChannelId channel, const World::Pos3& loc, int32_t volume, int32_t pan, int32_t frequency)
    {
        if (!_audioIsEnabled)
        {
            return;
        }

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
            auto handle = create(*buffer, channel, attribs);
            Audio::play(handle);
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

    void update()
    {
        reclaimFinishedInstances();
        updateVehicleNoise();
        updateAmbientNoise();
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
}
