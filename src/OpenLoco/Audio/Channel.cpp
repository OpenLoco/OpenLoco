#include "Channel.h"
#include "../Console.h"
#include <utility>

namespace OpenLoco::Audio
{
    bool Channel::load(uint32_t buffer)
    {
        _source.setBuffer(buffer);
        _isLoaded = true;
        return true;
    }

    bool Channel::play(bool loop)
    {
        if (_isLoaded == false)
        {
            return false;
        }
        _source.setLooping(loop);
        _source.play();
        return true;
    }

    void Channel::stop()
    {
        _source.stop();
        _source.setBuffer(0); // Unload buffer allowing destruct of buffers
        _isLoaded = false;
    }

    void Channel::setVolume(int32_t volume)
    {
        _attributes.volume = volume;
        _source.setGain(OpenAL::volumeFromLoco(volume));
    }

    void Channel::setPan(int32_t pan)
    {
        _attributes.pan = pan;
        _source.setPan(OpenAL::panFromLoco(pan));
    }

    void Channel::setFrequency(int32_t freq)
    {
        _attributes.freq = freq;
        _source.setPitch(OpenAL::freqFromLoco(freq));
    }

    bool Channel::isPlaying() const
    {
        return _source.isPlaying();
    }
}
