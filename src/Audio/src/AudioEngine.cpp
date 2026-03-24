#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#include <OpenLoco/Audio/AudioEngine.h>
#include <OpenLoco/Diagnostics/Logging.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace OpenLoco::Audio
{
    using namespace Diagnostics;

    struct AudioInstance
    {
        uint32_t sourceId{};
        ChannelId channel{};
        AudioAttributes attribs{};
        bool active = false;
    };

    static ALCdevice* _alcDevice = nullptr;
    static ALCcontext* _alcContext = nullptr;
    static std::vector<uint32_t> _alSources;
    static std::vector<uint32_t> _alBuffers;

    static bool _isInitialised = false;
    static bool _isPaused = false;

    static std::vector<AudioInstance> _instances;
    static std::array<int32_t, static_cast<size_t>(ChannelId::count)> _channelVolumes{};

    static ALuint _reverbEffect = 0;
    static ALuint _reverbSlot = 0;
    static bool _reverbAvailable = false;

    // Conversion helpers

    static float volumeToGain(int32_t volume)
    {
        // volume is in 100th dB: 10 ^ ((volume / 100) / 20)
        return std::pow(10.0f, static_cast<float>(volume) / 2000.0f);
    }

    static float freqToPitch(int32_t freq)
    {
        return freq / 22000.0f;
    }

    static float panToPosition(int32_t pan)
    {
        constexpr auto kRange = 4096.0f;
        return pan / kRange;
    }

    static AudioInstance* getInstance(AudioHandle handle)
    {
        auto idx = static_cast<uint32_t>(handle);
        if (idx >= _instances.size() || !_instances[idx].active)
        {
            return nullptr;
        }
        return &_instances[idx];
    }

    static float computeEffectiveGain(int32_t handleVolume, ChannelId channel)
    {
        auto masterVol = _channelVolumes[static_cast<size_t>(ChannelId::master)];
        auto channelVol = _channelVolumes[static_cast<size_t>(channel)];
        return volumeToGain(handleVolume + masterVol + channelVol);
    }

    static void applyVolume(AudioInstance& inst)
    {
        alSourcef(inst.sourceId, AL_GAIN, computeEffectiveGain(inst.attribs.volume, inst.channel));
    }

    static void applyPitch(AudioInstance& inst)
    {
        if (std::abs(inst.attribs.frequency) < 100)
        {
            alSourcef(inst.sourceId, AL_PITCH, 1.0f);
        }
        else
        {
            alSourcef(inst.sourceId, AL_PITCH, freqToPitch(inst.attribs.frequency));
        }
    }

    static void applyPan(uint32_t sourceId, int32_t pan)
    {
        auto p = panToPosition(pan);
        alSourcef(sourceId, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(sourceId, AL_POSITION, p, 0.0f, -std::sqrt(1.0f - p * p));
    }

    bool openDevice(const std::string& name)
    {
        closeDevice();

        _alcDevice = name.empty() ? alcOpenDevice(nullptr) : alcOpenDevice(name.c_str());
        if (_alcDevice == nullptr)
        {
            Logging::error("Failed to open OpenAL device '{}'", name.empty() ? "default" : name);
            return false;
        }

        constexpr ALCint attrs[] = {
            ALC_HRTF_SOFT, ALC_FALSE,
            0
        };

        _alcContext = alcCreateContext(_alcDevice, attrs);
        if (_alcContext == nullptr)
        {
            Logging::error("Failed to create OpenAL context");
            alcCloseDevice(_alcDevice);
            _alcDevice = nullptr;
            return false;
        }

        if (!alcMakeContextCurrent(_alcContext))
        {
            Logging::error("Failed to make OpenAL context current");
            alcDestroyContext(_alcContext);
            alcCloseDevice(_alcDevice);
            _alcContext = nullptr;
            _alcDevice = nullptr;
            return false;
        }

        Logging::info("OpenAL {}, Vendor: {}, Renderer: {}, initialized.",
            alGetString(AL_VERSION), alGetString(AL_VENDOR), alGetString(AL_RENDERER));

        _reverbAvailable = alcIsExtensionPresent(_alcDevice, "ALC_EXT_EFX") == ALC_TRUE;
        if (_reverbAvailable)
        {
            alGenEffects(1, &_reverbEffect);
            alEffecti(_reverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

            alGenAuxiliaryEffectSlots(1, &_reverbSlot);
            alAuxiliaryEffectSloti(_reverbSlot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(_reverbEffect));

            Logging::info("OpenAL EFX reverb initialized.");
        }

        return true;
    }

    void closeDevice()
    {
        if (_reverbAvailable)
        {
            alDeleteAuxiliaryEffectSlots(1, &_reverbSlot);
            alDeleteEffects(1, &_reverbEffect);
            _reverbSlot = 0;
            _reverbEffect = 0;
            _reverbAvailable = false;
        }
        if (_alcContext != nullptr)
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(_alcContext);
            _alcContext = nullptr;
        }
        if (_alcDevice != nullptr)
        {
            alcCloseDevice(_alcDevice);
            _alcDevice = nullptr;
        }
    }

    std::vector<std::string> getAvailableDevices()
    {
        std::vector<std::string> result;
        const ALCchar* devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        const char* ptr = devices;
        do
        {
            result.push_back(std::string(ptr));
            ptr += result.back().size() + 1;
        } while (*ptr != '\0');
        return result;
    }

    void initialize()
    {
        _channelVolumes.fill(0);
        _instances.clear();
        _isPaused = false;
        _isInitialised = true;
    }

    void shutdown()
    {
        for (auto& sourceId : _alSources)
        {
            alSourceStop(sourceId);
            alSourcei(sourceId, AL_BUFFER, 0);
        }
        _instances.clear();

        alDeleteSources(static_cast<ALsizei>(_alSources.size()), _alSources.data());
        _alSources.clear();
        alDeleteBuffers(static_cast<ALsizei>(_alBuffers.size()), _alBuffers.data());
        _alBuffers.clear();
        closeDevice();
        _isInitialised = false;
    }

    // Buffer management

    BufferId loadBuffer(std::span<const uint8_t> pcmData, const AudioFormat& format)
    {
        uint32_t id = 0;
        alGenBuffers(1, &id);
        _alBuffers.push_back(id);

        ALenum alFormat;
        if (format.channels > 1)
        {
            alFormat = (format.bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        }
        else
        {
            alFormat = (format.bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        }
        alBufferData(id, alFormat, pcmData.data(), static_cast<ALsizei>(pcmData.size()), format.sampleRate);
        return static_cast<BufferId>(id);
    }

    void unloadBuffer(BufferId buffer)
    {
        auto id = static_cast<uint32_t>(buffer);
        alDeleteBuffers(1, &id);
        _alBuffers.erase(std::remove(_alBuffers.begin(), _alBuffers.end(), id), _alBuffers.end());
    }

    // Handle management

    AudioHandle create(BufferId buffer, ChannelId channel, const AudioAttributes& attribs)
    {
        uint32_t idx = static_cast<uint32_t>(_instances.size());
        for (uint32_t i = 0; i < _instances.size(); ++i)
        {
            if (!_instances[i].active)
            {
                idx = i;
                break;
            }
        }

        uint32_t sourceId = 0;
        alGenSources(1, &sourceId);
        _alSources.push_back(sourceId);
        alSourcei(sourceId, AL_BUFFER, static_cast<ALint>(buffer));

        AudioInstance inst{};
        inst.sourceId = sourceId;
        inst.channel = channel;
        inst.attribs = attribs;
        inst.active = true;

        if (idx == _instances.size())
        {
            _instances.push_back(inst);
        }
        else
        {
            _instances[idx] = inst;
        }

        applyVolume(_instances[idx]);
        applyPitch(_instances[idx]);
        applyPan(sourceId, attribs.pan);
        alSourcei(sourceId, AL_LOOPING, attribs.loop ? AL_TRUE : AL_FALSE);

        return static_cast<AudioHandle>(idx);
    }

    void destroy(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return;
        }
        alSourceStop(inst->sourceId);
        alSourcei(inst->sourceId, AL_BUFFER, 0);
        alDeleteSources(1, &inst->sourceId);
        _alSources.erase(std::remove(_alSources.begin(), _alSources.end(), inst->sourceId), _alSources.end());
        inst->active = false;
    }

    // Playback control

    void play(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst != nullptr)
        {
            alSourcePlay(inst->sourceId);
        }
    }

    void stop(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst != nullptr)
        {
            alSourceStop(inst->sourceId);
        }
    }

    void pause(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst != nullptr)
        {
            alSourcePause(inst->sourceId);
        }
    }

    void unpause(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst != nullptr)
        {
            alSourcePlay(inst->sourceId);
        }
    }

    bool isPlaying(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return false;
        }
        int32_t state = 0;
        alGetSourcei(inst->sourceId, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    bool isPaused(AudioHandle handle)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return false;
        }
        int32_t state = 0;
        alGetSourcei(inst->sourceId, AL_SOURCE_STATE, &state);
        return state == AL_PAUSED;
    }

    // Attribute control

    void setVolume(AudioHandle handle, int32_t volume)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return;
        }
        inst->attribs.volume = volume;
        applyVolume(*inst);
    }

    void setPan(AudioHandle handle, int32_t pan)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return;
        }
        inst->attribs.pan = pan;
        applyPan(inst->sourceId, pan);
    }

    void setPitch(AudioHandle handle, int32_t frequency)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return;
        }
        inst->attribs.frequency = frequency;
        applyPitch(*inst);
    }

    void setAttributes(AudioHandle handle, const AudioAttributes& attribs)
    {
        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return;
        }
        inst->attribs = attribs;
        applyVolume(*inst);
        applyPitch(*inst);
        applyPan(inst->sourceId, attribs.pan);
        alSourcei(inst->sourceId, AL_LOOPING, attribs.loop ? AL_TRUE : AL_FALSE);
    }

    // Channel volume control

    void setChannelVolume(ChannelId channel, int32_t volume)
    {
        _channelVolumes[static_cast<size_t>(channel)] = volume;

        for (auto& inst : _instances)
        {
            if (inst.active && (channel == ChannelId::master || inst.channel == channel))
            {
                applyVolume(inst);
            }
        }
    }

    int32_t getChannelVolume(ChannelId channel)
    {
        return _channelVolumes[static_cast<size_t>(channel)];
    }

    // Effects

    void setReverb(AudioHandle handle, const ReverbParams& params)
    {
        if (!_reverbAvailable)
        {
            return;
        }

        auto* inst = getInstance(handle);
        if (inst == nullptr)
        {
            return;
        }

        if (params.gain > 0.0f)
        {
            alEffectf(_reverbEffect, AL_REVERB_DENSITY, params.density);
            alEffectf(_reverbEffect, AL_REVERB_DIFFUSION, params.diffusion);
            alEffectf(_reverbEffect, AL_REVERB_GAIN, params.gain);
            alEffectf(_reverbEffect, AL_REVERB_GAINHF, params.gainHF);
            alEffectf(_reverbEffect, AL_REVERB_DECAY_TIME, params.decayTime);
            alEffectf(_reverbEffect, AL_REVERB_DECAY_HFRATIO, params.decayHFRatio);
            alEffectf(_reverbEffect, AL_REVERB_REFLECTIONS_GAIN, params.reflectionsGain);
            alEffectf(_reverbEffect, AL_REVERB_REFLECTIONS_DELAY, params.reflectionsDelay);
            alEffectf(_reverbEffect, AL_REVERB_LATE_REVERB_GAIN, params.lateReverbGain);
            alEffectf(_reverbEffect, AL_REVERB_LATE_REVERB_DELAY, params.lateReverbDelay);
            alAuxiliaryEffectSloti(_reverbSlot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(_reverbEffect));
            alSource3i(inst->sourceId, AL_AUXILIARY_SEND_FILTER, static_cast<ALint>(_reverbSlot), 0, AL_FILTER_NULL);
        }
        else
        {
            alSource3i(inst->sourceId, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
        }
    }

    // Global control

    void pauseAll()
    {
        _isPaused = true;
        for (auto& inst : _instances)
        {
            if (!inst.active)
            {
                continue;
            }
            int32_t state = 0;
            alGetSourcei(inst.sourceId, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING)
            {
                alSourcePause(inst.sourceId);
            }
        }
    }

    void unpauseAll()
    {
        _isPaused = false;
        for (auto& inst : _instances)
        {
            if (!inst.active)
            {
                continue;
            }
            int32_t state = 0;
            alGetSourcei(inst.sourceId, AL_SOURCE_STATE, &state);
            if (state == AL_PAUSED)
            {
                alSourcePlay(inst.sourceId);
            }
        }
    }

    bool isEnabled()
    {
        return _isInitialised && !_isPaused;
    }
}
