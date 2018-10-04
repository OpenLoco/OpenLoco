#include "music_channel.h"
#include <SDL2/SDL_mixer.h>

using namespace openloco::audio;

music_channel::~music_channel()
{
    dispose_music();
}

bool music_channel::is_playing() const
{
    return Mix_PlayingMusic() != 0;
}

bool music_channel::load(const fs::path& path)
{
#ifdef _OPENLOCO_USE_BOOST_FS_
    auto paths = path.string();
#else
    auto paths = path.u8string();
#endif

    auto music = Mix_LoadMUS(paths.c_str());
    if (music != nullptr)
    {
        dispose_music();
        _music_track = music;
        return true;
    }
    return false;
}

bool music_channel::play(bool loop)
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

void music_channel::stop()
{
    Mix_HaltMusic();
}

void music_channel::set_volume(int32_t volume)
{
    Mix_VolumeMusic(volume_loco_to_sdl(volume));
}

void music_channel::dispose_music()
{
    Mix_FreeMusic(_music_track);
    _music_track = nullptr;
    _current_music = -1;
}
