#pragma once

#include "AudioChannel.h"
#include "AudioEffect.h"
#include "AudioFormat.h"
#include "AudioHandle.h"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace OpenLoco::Audio
{
    struct AudioAttributes
    {
        int32_t volume{};
        int32_t pan{};
        int32_t frequency{};
        bool loop = false;
    };

    void initialize();
    void shutdown();
    bool openDevice(const std::string& name);
    void closeDevice();
    std::vector<std::string> getAvailableDevices();

    // Buffer management
    BufferId loadBuffer(std::span<const uint8_t> pcmData, const AudioFormat& format);
    void unloadBuffer(BufferId buffer);

    // Handle management
    AudioHandle create(BufferId buffer, ChannelId channel, const AudioAttributes& attribs = {});
    void destroy(AudioHandle handle);

    // Playback control
    void play(AudioHandle handle);
    void stop(AudioHandle handle);
    void pause(AudioHandle handle);
    void unpause(AudioHandle handle);
    bool isPlaying(AudioHandle handle);
    bool isPaused(AudioHandle handle);

    // Attribute control
    void setVolume(AudioHandle handle, int32_t volume);
    void setPan(AudioHandle handle, int32_t pan);
    void setPitch(AudioHandle handle, int32_t frequency);
    void setAttributes(AudioHandle handle, const AudioAttributes& attribs);

    // Channel volume control
    void setChannelVolume(ChannelId channel, int32_t volume);
    int32_t getChannelVolume(ChannelId channel);

    // Effects
    void setReverb(AudioHandle handle, const ReverbParams& params);

    void reclaimFinishedInstances();

    // Global control
    void pauseAll();
    void unpauseAll();
    bool isEnabled();
}
