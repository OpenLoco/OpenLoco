#pragma once
#include "OpenAL.h"

namespace OpenLoco::Audio
{
    class Channel
    {
    public:
        static constexpr int32_t kUndefinedId = -1;

        struct Attributes
        {
            int32_t volume{};
            int32_t pan{};
            int32_t freq{};
        };

    private:
        OpenAL::Source _source;
        bool _isLoaded = false;
        Attributes _attributes;

    public:
        Channel(OpenAL::Source source)
            : _source(source)
            , _attributes{}
        {
        }
        bool load(uint32_t buffer);
        bool play(bool loop);
        void stop();
        void setVolume(int32_t volume);
        void setPan(int32_t pan);
        void setFrequency(int32_t freq);
        bool isPlaying() const;
        const OpenAL::Source& getSource() { return _source; }
        const Attributes& getAttributes() { return _attributes; }
    };
}
