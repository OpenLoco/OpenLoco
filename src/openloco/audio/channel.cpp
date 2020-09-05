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
    disposeChunk();
}

bool channel::load(sample& sample)
{
    disposeChunk();
    _chunk = sample.chunk;
    return true;
}

bool channel::load(const fs::path& path)
{
    disposeChunk();
    _chunk = Mix_LoadWAV(path.u8string().c_str());
    _chunk_owner = true;
    return _chunk != nullptr;
}

bool channel::play(bool loop)
{
    if (!isUndefined())
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
    if (!isUndefined())
    {
        Mix_HaltChannel(_id);
    }
}

void channel::setVolume(int32_t volume)
{
    if (!isUndefined())
    {
        Mix_Volume(_id, volumeLocoToSDL(volume));
    }
}

void channel::setPan(int32_t pan)
{
    if (!isUndefined())
    {
        auto [left, right] = panLocoToSDL(pan);
        Mix_SetPanning(_id, left, right);
    }
}

void channel::setFrequency(int32_t freq)
{
    // TODO
}

bool channel::isPlaying() const
{
    return isUndefined() ? false : (Mix_Playing(_id) != 0);
}

void channel::disposeChunk()
{
    if (_chunk_owner)
    {
        Mix_FreeChunk(_chunk);
    }
    _chunk = nullptr;
    _chunk_owner = false;
}
