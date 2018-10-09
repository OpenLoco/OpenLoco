#include "channel.h"
#include "../console.h"
#include "audio.h"
#include <SDL2/SDL_mixer.h>
#include <utility>

using namespace openloco::audio;

channel::channel(int32_t cid)
    : _id(cid)
{
}

channel::channel(channel&& c)
    : _id(std::exchange(c._id, undefined_id))
    , _chunk(std::exchange(c._chunk, nullptr))
    , _chunk_owner(std::exchange(c._chunk_owner, {}))
{
}

channel& channel::operator=(channel&& other)
{
    std::swap(_id, other._id);
    std::swap(_chunk, other._chunk);
    std::swap(_chunk_owner, other._chunk_owner);
    return *this;
}

channel::~channel()
{
    dispose_chunk();
}

bool channel::load(sample& sample)
{
    dispose_chunk();
    _chunk = sample.chunk;
    return true;
}

bool channel::load(const fs::path& path)
{
    dispose_chunk();
#ifdef _OPENLOCO_USE_BOOST_FS_
    _chunk = Mix_LoadWAV(path.string().c_str());
#else
    _chunk = Mix_LoadWAV(path.u8string().c_str());
#endif
    _chunk_owner = true;
    return _chunk != nullptr;
}

bool channel::play(bool loop)
{
    if (!is_undefined())
    {
        int loops = loop ? -1 : 0;
        if (Mix_PlayChannel(_id, _chunk, loops) == -1)
        {
            console::log("Error during Mix_PlayChannel: %s", Mix_GetError());
            return false;
        }
        return true;
    }
    return false;
}

void channel::stop()
{
    if (!is_undefined())
    {
        Mix_HaltChannel(_id);
    }
}

void channel::set_volume(int32_t volume)
{
    if (!is_undefined())
    {
        Mix_Volume(_id, volume_loco_to_sdl(volume));
    }
}

void channel::set_pan(int32_t pan)
{
    if (!is_undefined())
    {
        // clang-format off
        auto[left, right] = pan_loco_to_sdl(pan);
        // clang-format on
        Mix_SetPanning(_id, left, right);
    }
}

void channel::set_frequency(int32_t freq)
{
    // TODO
}

bool channel::is_playing() const
{
    return is_undefined() ? false : (Mix_Playing(_id) != 0);
}

void channel::dispose_chunk()
{
    if (_chunk_owner)
    {
        Mix_FreeChunk(_chunk);
    }
    _chunk = nullptr;
    _chunk_owner = false;
}
