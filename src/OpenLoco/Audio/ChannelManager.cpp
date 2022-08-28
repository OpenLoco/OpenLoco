#include "ChannelManager.h"
#include "VehicleChannel.h"
#include <algorithm>
#include <iterator>

namespace OpenLoco::Audio
{
    static std::unique_ptr<Channel> getNewChannel(OpenAL::SourceManager& sourceManager, ChannelId channelId)
    {
        if (channelId == ChannelId::vehicle)
            return std::make_unique<Channel>(new VehicleChannel(sourceManager.allocate()));
        else
            return std::make_unique<Channel>(sourceManager.allocate());
    }

    ChannelManager::ChannelManager(OpenAL::SourceManager& sourceManager)
    {
        _sourceManager = sourceManager;

        for (const auto& channelDef : kMixerChannelDefinitions)
        {
            auto result = virtualChannels.emplace(channelDef.first, VirtualChannel(1.f));

            if (result.second)
            {
                auto& kv = *result.first;

                for (size_t i = 0; i < channelDef.second.initialPhysicalChannels; ++i)
                {
                    /*auto channel = channelDef.first == ChannelId::vehicle
                        ? VehicleChannel(_sourceManager.allocate())
                        : Channel(_sourceManager.allocate());*/

                    kv.second.channels.push_back(getNewChannel(sourceManager, channelDef.first));
                }
            }
        }
    }

    Channel* ChannelManager::getFreeChannel(ChannelId channelId)
    {
        auto vChannels = virtualChannels.at(channelId).channels;
        auto freeChannel = std::find_if(std::begin(vChannels), std::end(vChannels), [](auto& channel) { return !channel.isPlaying(); });
        Channel* channel = nullptr;

        if (freeChannel == std::end(vChannels))
        {
            if (vChannels.size() < kMixerChannelDefinitions.at(channelId).maxPhysicalChannels)
            {
                vChannels.push_back(getNewChannel(_sourceManager, channelId));
                channel = vChannels.back();
            }
            // else - no free channels and we've hit the channel limit - sound just won't play
        }
        else
        {
            channel = &*freeChannel;
        }
        return channel;
    }

    bool ChannelManager::play(ChannelId channelId, PlaySoundParams&& soundParams)
    {
        auto channel = getFreeChannel(channelId);

        if (channel != nullptr && channel->load(soundParams.sample))
        {
            channel->setVolume(soundParams.volume);
            channel->setFrequency(soundParams.freq);
            channel->setPan(soundParams.pan);
            return channel->play(soundParams.loop);
        }

        return false;
    }

    bool ChannelManager::play(ChannelId channelId, PlaySoundParams&& soundParams, SoundId soundId)
    {
        auto channel = getFreeChannel(channelId);
        if (channel != nullptr && channel->load(soundParams.sample))
        {
            (dynamic_cast<VehicleChannel*>(channel)).
        }
        return false;
    }

    VirtualChannel& ChannelManager::getVirtualChannel(ChannelId channelId)
    {
        return virtualChannels.at(channelId);
    }

    Channel& ChannelManager::getFirstChannel(ChannelId channelId)
    {
        return virtualChannels.at(channelId).channels.front();
    }

    void ChannelManager::disposeChannels()
    {
        virtualChannels.clear();
    }

    void ChannelManager::stopChannel(ChannelId channelId)
    {
        for (auto& channel : virtualChannels.at(channelId).channels)
        {
            channel.stop();
        }
    }

    VirtualChannel::VirtualChannel(float gain)
        : gain(gain)
    {
    }
}
