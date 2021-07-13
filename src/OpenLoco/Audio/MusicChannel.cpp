#include "MusicChannel.h"
#include <SDL2/SDL_mixer.h>

using namespace OpenLoco::Audio;

MusicChannel::~MusicChannel()
{
    disposeMusic();
}

bool MusicChannel::isPlaying() const
{
    return Mix_PlayingMusic() != 0;
}

bool MusicChannel::load(const fs::path& path)
{
    auto paths = path.u8string();
    auto music = Mix_LoadMUS(paths.c_str());
    if (music != nullptr)
    {
        disposeMusic();
        _music_track = music;
        return true;
    }
    return false;
}

bool MusicChannel::play(bool loop)
{
    if (_music_track != nullptr)
    {
        auto loops = loop ? -1 : 1;
        if (Mix_PlayMusic(_music_track, loops) == 0)
        {
            return true;
        }
    }
    return false;
}

void MusicChannel::stop()
{
    Mix_HaltMusic();
}

void MusicChannel::setVolume(int32_t volume)
{
    Mix_VolumeMusic(volumeLocoToSDL(volume));
}

void MusicChannel::disposeMusic()
{
    Mix_FreeMusic(_music_track);
    _music_track = nullptr;
    _current_music = -1;
}
