#pragma once

#include "../Core/FileSystem.hpp"

struct Mix_Chunk;

namespace OpenLoco::Audio
{
    struct Sample;

    class Channel
    {
    public:
        static constexpr int32_t kUndefinedId = -1;

    private:
        int32_t _id = kUndefinedId;
        Mix_Chunk* _chunk{};
        bool _chunkOwner{};

    public:
        Channel() = default;
        Channel(int32_t id);
        Channel(const Channel&) = delete;
        Channel(Channel&&);
        Channel& operator=(Channel&& other);
        ~Channel();
        bool load(Sample& sample);
        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void setVolume(int32_t volume);
        void setPan(int32_t pan);
        void setFrequency(int32_t freq);
        bool isPlaying() const;
        bool isUndefined() const { return _id == kUndefinedId; }
        int32_t id() { return _id; }

    private:
        void disposeChunk();
    };
}
