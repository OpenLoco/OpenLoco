#pragma once

#include "../Core/FileSystem.hpp"

struct Mix_Chunk;

namespace OpenLoco::audio
{
    struct sample;

    class channel
    {
    public:
        static constexpr int32_t undefined_id = -1;

    private:
        int32_t _id = undefined_id;
        Mix_Chunk* _chunk{};
        bool _chunk_owner{};

    public:
        channel() = default;
        channel(int32_t id);
        channel(const channel&) = delete;
        channel(channel&&);
        channel& operator=(channel&& other);
        ~channel();
        bool load(sample& sample);
        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void setVolume(int32_t volume);
        void setPan(int32_t pan);
        void setFrequency(int32_t freq);
        bool isPlaying() const;
        bool isUndefined() const { return _id == undefined_id; }
        int32_t id() { return _id; }

    private:
        void disposeChunk();
    };
}
