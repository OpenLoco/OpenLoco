#include "channel.h"
#include "../console.h"
#include "audio.h"
#include <SDL2/SDL_mixer.h>

using namespace openloco::audio;

channel::channel(int32_t cid)
    : id(cid)
{
}

channel::channel(const channel&& c)
    : id(c.id)
    , _chunk(c._chunk)
{
}

channel::~channel()
{
    Mix_FreeChunk(_chunk);
    _chunk = nullptr;
}

bool channel::load(const fs::path& path)
{
    Mix_FreeChunk(_chunk);
#ifdef _OPENLOCO_USE_BOOST_FS_
    _chunk = Mix_LoadWAV(path.string().c_str());
#else
    _chunk = Mix_LoadWAV(path.u8string().c_str());
#endif
    return _chunk != nullptr;
}

bool channel::play(bool loop)
{
    int loops = loop ? -1 : 0;
    if (Mix_PlayChannel(id, _chunk, loops) == -1)
    {
        console::log("Error during Mix_PlayChannel: %s", Mix_GetError());
        return false;
    }
    return true;
}

void channel::stop()
{
    Mix_HaltChannel(id);
}

void channel::set_volume(int32_t volume)
{
    Mix_Volume(id, volume_loco_to_sdl(volume));
}

bool channel::is_playing() const
{
    return Mix_Playing(id) != 0;
}
