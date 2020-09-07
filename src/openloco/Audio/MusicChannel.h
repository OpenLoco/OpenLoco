#pragma once

#include "../Core/FileSystem.hpp"
#include "../things/thing.h"
#include "Audio.h"
#include "Channel.h"

struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;

namespace openloco::audio
{
    class music_channel
    {
    private:
        Mix_Music* _music_track;
        int32_t _current_music = -1;

    public:
        music_channel() = default;
        music_channel(const music_channel&) = delete;
        ~music_channel();

        bool isPlaying() const;

        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void setVolume(int32_t volume);
        void disposeMusic();
    };
}
