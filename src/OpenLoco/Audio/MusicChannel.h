#pragma once

#include "../Core/FileSystem.hpp"
#include "../Entities/Entity.h"
#include "Audio.h"
#include "Channel.h"

struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;

namespace OpenLoco::Audio
{
    class MusicChannel
    {
    private:
        Mix_Music* _musicTrack;
        int32_t _currentMusic = -1;

    public:
        MusicChannel() = default;
        MusicChannel(const MusicChannel&) = delete;
        ~MusicChannel();

        bool isPlaying() const;

        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void setVolume(int32_t volume);
        void disposeMusic();
    };
}
