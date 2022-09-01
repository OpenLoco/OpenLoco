#pragma once

#include "./Audio.h"
#include "Channel.h"
#include "OpenAL.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace OpenLoco::Audio
{
    enum class ChannelId
    {
        bgm,
        unk_1,
        ambient,
        title,
        vehicle, // * 10
        soundFX  // * 64
    };

    struct VirtualChannelAttributes
    {
        size_t initialPhysicalChannels;
        size_t maxPhysicalChannels;
    };

    static const std::unordered_map<ChannelId, VirtualChannelAttributes> kMixerChannelDefinitions = {
        { ChannelId::bgm, VirtualChannelAttributes{ 1, 1 } },
        { ChannelId::unk_1, VirtualChannelAttributes{ 0, 0 } },
        { ChannelId::ambient, VirtualChannelAttributes{ 1, 1 } },
        { ChannelId::title, VirtualChannelAttributes{ 1, 1 } },
        { ChannelId::vehicle, VirtualChannelAttributes{ 10, 64 } },
        { ChannelId::soundFX, VirtualChannelAttributes{ 16, 64 } },
    };

    struct PlaySoundParams
    {
        uint32_t sample{};
        int32_t volume{};
        int32_t pan{};
        int32_t freq{};
        bool loop{};
    };

    class VirtualChannel
    {
        float gain;

    public:
        std::vector<Channel*> channels;

        VirtualChannel(float initialGain);
    };

    class ChannelManager
    {
        std::unordered_map<ChannelId, VirtualChannel> virtualChannels;
        OpenAL::SourceManager _sourceManager;

        Channel* getFreeChannel(ChannelId channelId);

    public:
        ChannelManager(){};
        ChannelManager(OpenAL::SourceManager& sourceManager);
        bool play(ChannelId channelId, PlaySoundParams&& soundParams);
        // bool play(ChannelId channelId, PlaySoundParams&& soundParams, SoundId soundId);
        bool play(ChannelId channelId, PlaySoundParams&& soundParams, EntityId entityId);
        VirtualChannel& getVirtualChannel(ChannelId channelId);
        Channel& getFirstChannel(ChannelId channelId);
        void disposeChannels();
        void stopChannels(ChannelId channelId);
        void stopNonPlayingChannels(ChannelId channelId);

        // vehicle specific
        void updateVehicleChannels();
    };
}
