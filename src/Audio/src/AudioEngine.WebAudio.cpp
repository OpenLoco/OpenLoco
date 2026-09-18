#include <OpenLoco/Audio/AudioEngine.h>
#include <OpenLoco/Diagnostics/Logging.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <emscripten.h>
#include <span>
#include <string>
#include <vector>

namespace OpenLoco::Audio
{
    using namespace Diagnostics;

    namespace
    {
        struct AudioInstance
        {
            BufferId buffer = BufferId::null;
            ChannelId channel{};
            AudioAttributes attributes{};
            bool active = false;
        };

        std::vector<AudioInstance> _instances;
        std::vector<uint32_t> _buffers;
        std::array<int32_t, static_cast<size_t>(ChannelId::count)> _channelVolumes{};
        uint32_t _nextBufferId = 1;
        bool _initialised = false;
        bool _deviceOpen = false;
        bool _paused = false;

        float volumeToGain(const int32_t volume)
        {
            return std::pow(10.0F, static_cast<float>(volume) / 2000.0F);
        }

        float frequencyToPitch(const int32_t frequency)
        {
            return std::abs(frequency) < 100 ? 1.0F : std::max(0.01F, static_cast<float>(frequency) / 22000.0F);
        }

        float panToPosition(const int32_t pan)
        {
            return std::clamp(static_cast<float>(pan) / 4096.0F, -1.0F, 1.0F);
        }

        float effectiveGain(const AudioInstance& instance)
        {
            const auto master = _channelVolumes[static_cast<size_t>(ChannelId::master)];
            const auto channel = _channelVolumes[static_cast<size_t>(instance.channel)];
            return volumeToGain(instance.attributes.volume + master + channel);
        }

        AudioInstance* getInstance(const AudioHandle handle)
        {
            if (handle == AudioHandle::null)
            {
                return nullptr;
            }
            const auto index = static_cast<uint32_t>(handle);
            if (index >= _instances.size() || !_instances[index].active)
            {
                return nullptr;
            }
            return &_instances[index];
        }

        // clang-format off
        EM_JS(int, webAudioOpen, (), {
            const AudioContextClass = globalThis.AudioContext || globalThis.webkitAudioContext;
            if (!AudioContextClass)
            {
                return 0;
            }
            if (Module.openLocoWebAudio)
            {
                return 1;
            }

            const context = new AudioContextClass({ latencyHint : "interactive" });
            const dryBus = context.createGain();
            const wetBus = context.createGain();
            const convolver = context.createConvolver();
            dryBus.connect(context.destination);
            wetBus.connect(convolver);
            convolver.connect(context.destination);

            Module.openLocoWebAudio = {
                context,
                dryBus,
                wetBus,
                convolver,
                buffers : new Map(),
                handles : new Map(),
                impulseKey : "",

                disconnectSource(handle){
                    if (!handle.source) return;
            handle.source.onended = null;
            try
            {
                handle.source.stop();
            }
            catch (_)
            {
            }
            handle.source.disconnect();
            handle.source = null;
                },

                createSource(handle) {
            const buffer = this.buffers.get(handle.bufferId);
            if (!buffer)
            {
                return null;
            }
            const source = context.createBufferSource();
            source.buffer = buffer;
            source.loop = handle.loop;
            source.playbackRate.value = handle.pitch;
            source.connect(handle.gain);
            source.onended = () =>
            {
                if (!handle.loop)
                {
                    handle.state = "stopped";
                    handle.offset = 0;
                    handle.source = null;
                }
            };
            handle.source = source;
            return source;
                },

                play(handle) {
            if (handle.state === "playing")
            {
                return;
            }
            context.resume();
            const source = this.createSource(handle);
            if (!source)
            {
                return;
            }
            const duration = source.buffer.duration;
            source.start(0, duration > 0 ? handle.offset % duration : 0);
            handle.startedAt = context.currentTime;
            handle.state = "playing";
                },

                pause(handle) {
            if (handle.state !== "playing")
            {
                return;
            }
            handle.offset += (context.currentTime - handle.startedAt) * handle.pitch;
            this.disconnectSource(handle);
            handle.state = "paused";
                },

                stop(handle) {
            this.disconnectSource(handle);
            handle.offset = 0;
            handle.state = "stopped";
                },

                updateImpulse(density, diffusion, gainHF, decayTime, decayHFRatio,
                    reflectionsGain, reflectionsDelay, lateGain, lateDelay) {
            const values = [ density, diffusion, gainHF, decayTime, decayHFRatio, reflectionsGain, reflectionsDelay, lateGain, lateDelay ];
            const key = values.map(value => value.toFixed(4)).join(":");
            if (key === this.impulseKey)
            {
                return;
            }
            this.impulseKey = key;

            const duration = Math.max(0.1, Math.min(10, decayTime + lateDelay));
            const length = Math.max(1, Math.floor(context.sampleRate * duration));
            const impulse = context.createBuffer(2, length, context.sampleRate);
            let seed = 0x4f70656e;
            const random = () =>
            {
                seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
                return (seed / 0x100000000) * 2 - 1;
            };

            for (let channel = 0; channel < 2; channel++)
            {
                const data = impulse.getChannelData(channel);
                let damped = 0;
                const damping = Math.max(0.01, Math.min(0.99, gainHF * decayHFRatio));
                const lateSample = Math.floor(lateDelay * context.sampleRate);
                const reflectionSample = Math.floor(reflectionsDelay * context.sampleRate);
                const tapCount = Math.max(2, Math.floor(4 + density * 28));
                for (let tap = 0; tap < tapCount; tap++)
                {
                    const spread = Math.floor((tap / tapCount) * context.sampleRate * 0.08 * diffusion);
                    const index = Math.min(length - 1, reflectionSample + spread);
                    data[index] += random() * reflectionsGain * (1 - tap / (tapCount + 1));
                }
                for (let i = lateSample; i < length; i++)
                {
                    const time = (i - lateSample) / context.sampleRate;
                    const envelope = Math.pow(10, -3 * time / Math.max(0.1, decayTime));
                    const noise = random();
                    damped += (noise - damped) * damping;
                    const diffuse = noise * (1 - diffusion) + damped * diffusion;
                    data[i] += diffuse * envelope * lateGain * 0.35;
                }
            }
            this.convolver.buffer = impulse;
                }
    };
    return 1;
});

EM_JS(void, webAudioClose, (), {
    const backend = Module.openLocoWebAudio;
    if (!backend)
    {
        return;
    }
    for (const handle of backend.handles.values())
    {
        backend.stop(handle);
    }
    backend.handles.clear();
    backend.buffers.clear();
    backend.context.close();
    delete Module.openLocoWebAudio;
});

EM_JS(int, webAudioLoadBuffer, (uint32_t id, const uint8_t* data, uint32_t size, uint32_t sampleRate, uint32_t channels, uint32_t bits), {
    const backend = Module.openLocoWebAudio;
    if (!backend || !channels || (bits !== 8 && bits !== 16))
    {
        return 0;
    }
    const bytesPerSample = bits >> 3;
    const frames = Math.floor(size / (channels * bytesPerSample));
    const buffer = backend.context.createBuffer(channels, frames, sampleRate);
    for (let channel = 0; channel < channels; channel++)
    {
        const output = buffer.getChannelData(channel);
        for (let frame = 0; frame < frames; frame++)
        {
            const sample = frame * channels + channel;
            output[frame] = bits === 8
                ? (HEAPU8[data + sample] - 128) / 128
                : (() => {
                      const offset = data + sample * 2;
                      let value = HEAPU8[offset] | (HEAPU8[offset + 1] << 8);
                      if (value & 0x8000)
                      {
                          value -= 0x10000;
                      }
                      return value / 32768;
                  })();
        }
    }
    backend.buffers.set(id, buffer);
    return 1;
});

EM_JS(void, webAudioUnloadBuffer, (uint32_t id), {
    Module.openLocoWebAudio ?.buffers.delete(id);
});

EM_JS(int, webAudioCreate, (uint32_t id, uint32_t bufferId, float gain, float pan, float pitch, int loop), {
    const backend = Module.openLocoWebAudio;
    if (!backend || !backend.buffers.has(bufferId))
    {
        return 0;
    }
    const gainNode = backend.context.createGain();
    const panner = backend.context.createStereoPanner();
    const dry = backend.context.createGain();
    const wet = backend.context.createGain();
    gainNode.gain.value = gain;
    panner.pan.value = pan;
    dry.gain.value = 1;
    wet.gain.value = 0;
    gainNode.connect(panner);
    panner.connect(dry);
    panner.connect(wet);
    dry.connect(backend.dryBus);
    wet.connect(backend.wetBus);
    backend.handles.set(id, {
        bufferId,
        gain : gainNode,
        panner,
        dry,
        wet,
        pitch,
        loop : !!loop,
        source : null,
        state : "stopped",
        offset : 0,
        startedAt : 0,
        pausedByGlobal : false
    });
    return 1;
});

EM_JS(void, webAudioDestroy, (uint32_t id), {
    const backend = Module.openLocoWebAudio;
    const handle = backend ?.handles.get(id);
    if (!handle)
    {
        return;
    }
    backend.stop(handle);
    handle.gain.disconnect();
    handle.panner.disconnect();
    handle.dry.disconnect();
    handle.wet.disconnect();
    backend.handles.delete(id);
});

EM_JS(void, webAudioPlay, (uint32_t id), {
    const backend = Module.openLocoWebAudio;
    const handle = backend ?.handles.get(id);
    if (handle)
    {
        backend.play(handle);
    }
});

EM_JS(void, webAudioStop, (uint32_t id), {
    const backend = Module.openLocoWebAudio;
    const handle = backend ?.handles.get(id);
    if (handle)
    {
        backend.stop(handle);
    }
});

EM_JS(void, webAudioPause, (uint32_t id), {
    const backend = Module.openLocoWebAudio;
    const handle = backend ?.handles.get(id);
    if (handle)
    {
        backend.pause(handle);
    }
});

EM_JS(void, webAudioUnpause, (uint32_t id), {
    const backend = Module.openLocoWebAudio;
    const handle = backend ?.handles.get(id);
    if (handle&& handle.state === "paused")
    {
        backend.play(handle);
    }
});

EM_JS(int, webAudioState, (uint32_t id), {
    const handle = Module.openLocoWebAudio ?.handles.get(id);
    if (!handle)
    {
        return 0;
    }
    return handle.state === "playing" ? 1 : handle.state === "paused" ? 2 : 0;
});

EM_JS(void, webAudioSetAttributes, (uint32_t id, float gain, float pan, float pitch, int loop), {
    const handle = Module.openLocoWebAudio ?.handles.get(id);
    if (!handle)
    {
        return;
    }
    handle.gain.gain.value = gain;
    handle.panner.pan.value = pan;
    handle.pitch = pitch;
    handle.loop = !!loop;
    if (handle.source)
    {
        handle.source.playbackRate.value = pitch;
        handle.source.loop = !!loop;
    }
});

EM_JS(void, webAudioSetReverb, (uint32_t id, float density, float diffusion, float gain, float gainHF, float decayTime, float decayHFRatio, float reflectionsGain, float reflectionsDelay, float lateGain, float lateDelay), {
    const backend = Module.openLocoWebAudio;
    const handle = backend ?.handles.get(id);
    if (!handle)
    {
        return;
    }
    handle.wet.gain.value = Math.max(0, gain);
    if (gain <= 0)
    {
        return;
    }
    backend.updateImpulse(density, diffusion, gainHF, decayTime, decayHFRatio, reflectionsGain, reflectionsDelay, lateGain, lateDelay);
});

EM_JS(void, webAudioPauseAll, (), {
    const backend = Module.openLocoWebAudio;
    if (!backend)
    {
        return;
    }
    for (const handle of backend.handles.values())
    {
        handle.pausedByGlobal = handle.state === "playing";
        if (handle.pausedByGlobal)
        {
            backend.pause(handle);
        }
    }
});

EM_JS(void, webAudioUnpauseAll, (), {
    const backend = Module.openLocoWebAudio;
    if (!backend)
    {
        return;
    }
    backend.context.resume();
    for (const handle of backend.handles.values())
    {
        if (handle.pausedByGlobal)
        {
            handle.pausedByGlobal = false;
            backend.play(handle);
        }
    }
});

// clang-format on

void updateInstance(const uint32_t index, AudioInstance& instance)
{
    webAudioSetAttributes(index, effectiveGain(instance), panToPosition(instance.attributes.pan), frequencyToPitch(instance.attributes.frequency), instance.attributes.loop);
}
}

void initialize()
{
    _channelVolumes.fill(0);
    _instances.clear();
    _buffers.clear();
    _nextBufferId = 1;
    _paused = false;
    _initialised = true;
}

void shutdown()
{
    webAudioClose();
    _instances.clear();
    _buffers.clear();
    _deviceOpen = false;
    _initialised = false;
}

bool openDevice(const std::string&)
{
    _deviceOpen = webAudioOpen() != 0;
    if (!_deviceOpen)
    {
        Logging::error("Web Audio API is unavailable.");
    }
    return _deviceOpen;
}

void closeDevice()
{
    webAudioClose();
    _deviceOpen = false;
}

std::vector<std::string> getAvailableDevices()
{
    return { "Web Audio" };
}

BufferId loadBuffer(const std::span<const uint8_t> pcmData, const AudioFormat& format)
{
    if (!_deviceOpen || pcmData.empty())
    {
        return BufferId::null;
    }
    const auto id = _nextBufferId++;
    if (!webAudioLoadBuffer(id, pcmData.data(), static_cast<uint32_t>(pcmData.size()), format.sampleRate, format.channels, format.bitsPerSample))
    {
        return BufferId::null;
    }
    _buffers.push_back(id);
    return static_cast<BufferId>(id);
}

void unloadBuffer(const BufferId buffer)
{
    const auto id = static_cast<uint32_t>(buffer);
    webAudioUnloadBuffer(id);
    std::erase(_buffers, id);
}

AudioHandle create(const BufferId buffer, const ChannelId channel, const AudioAttributes& attributes)
{
    uint32_t index = 0;
    while (index < _instances.size() && _instances[index].active)
    {
        index++;
    }
    AudioInstance instance{ buffer, channel, attributes, true };
    if (!webAudioCreate(index, static_cast<uint32_t>(buffer), effectiveGain(instance), panToPosition(attributes.pan), frequencyToPitch(attributes.frequency), attributes.loop))
    {
        return AudioHandle::null;
    }
    if (index == _instances.size())
    {
        _instances.push_back(instance);
    }
    else
    {
        _instances[index] = instance;
    }
    return static_cast<AudioHandle>(index);
}

void destroy(const AudioHandle handle)
{
    auto* instance = getInstance(handle);
    if (instance == nullptr)
    {
        return;
    }
    webAudioDestroy(static_cast<uint32_t>(handle));
    instance->active = false;
}

void play(const AudioHandle handle)
{
    if (getInstance(handle) != nullptr)
    {
        webAudioPlay(static_cast<uint32_t>(handle));
    }
}

void stop(const AudioHandle handle)
{
    if (getInstance(handle) != nullptr)
    {
        webAudioStop(static_cast<uint32_t>(handle));
    }
}

void pause(const AudioHandle handle)
{
    if (getInstance(handle) != nullptr)
    {
        webAudioPause(static_cast<uint32_t>(handle));
    }
}

void unpause(const AudioHandle handle)
{
    if (getInstance(handle) != nullptr)
    {
        webAudioUnpause(static_cast<uint32_t>(handle));
    }
}

bool isPlaying(const AudioHandle handle)
{
    return getInstance(handle) != nullptr && webAudioState(static_cast<uint32_t>(handle)) == 1;
}

bool isPaused(const AudioHandle handle)
{
    return getInstance(handle) != nullptr && webAudioState(static_cast<uint32_t>(handle)) == 2;
}

void setVolume(const AudioHandle handle, const int32_t volume)
{
    auto* instance = getInstance(handle);
    if (instance == nullptr)
    {
        return;
    }
    instance->attributes.volume = volume;
    updateInstance(static_cast<uint32_t>(handle), *instance);
}

void setPan(const AudioHandle handle, const int32_t pan)
{
    auto* instance = getInstance(handle);
    if (instance == nullptr)
    {
        return;
    }
    instance->attributes.pan = pan;
    updateInstance(static_cast<uint32_t>(handle), *instance);
}

void setPitch(const AudioHandle handle, const int32_t frequency)
{
    auto* instance = getInstance(handle);
    if (instance == nullptr)
    {
        return;
    }
    instance->attributes.frequency = frequency;
    updateInstance(static_cast<uint32_t>(handle), *instance);
}

void setAttributes(const AudioHandle handle, const AudioAttributes& attributes)
{
    auto* instance = getInstance(handle);
    if (instance == nullptr)
    {
        return;
    }
    instance->attributes = attributes;
    updateInstance(static_cast<uint32_t>(handle), *instance);
}

void setChannelVolume(const ChannelId channel, const int32_t volume)
{
    _channelVolumes[static_cast<size_t>(channel)] = volume;
    for (uint32_t index = 0; index < _instances.size(); index++)
    {
        auto& instance = _instances[index];
        if (instance.active && (channel == ChannelId::master || instance.channel == channel))
        {
            updateInstance(index, instance);
        }
    }
}

int32_t getChannelVolume(const ChannelId channel)
{
    return _channelVolumes[static_cast<size_t>(channel)];
}

void setReverb(const AudioHandle handle, const ReverbParams& params)
{
    if (getInstance(handle) == nullptr)
    {
        return;
    }
    webAudioSetReverb(static_cast<uint32_t>(handle), params.density, params.diffusion, params.gain, params.gainHF, params.decayTime, params.decayHFRatio, params.reflectionsGain, params.reflectionsDelay, params.lateReverbGain, params.lateReverbDelay);
}

void reclaimFinishedInstances()
{
    for (uint32_t index = 0; index < _instances.size(); index++)
    {
        auto& instance = _instances[index];
        if (instance.active && webAudioState(index) == 0)
        {
            webAudioDestroy(index);
            instance.active = false;
        }
    }
}

void stopAll()
{
    for (uint32_t index = 0; index < _instances.size(); index++)
    {
        if (_instances[index].active)
        {
            webAudioDestroy(index);
            _instances[index].active = false;
        }
    }
}

void pauseAll()
{
    _paused = true;
    webAudioPauseAll();
}

void unpauseAll()
{
    _paused = false;
    webAudioUnpauseAll();
}

bool isEnabled()
{
    return _initialised && _deviceOpen && !_paused;
}
}
