#include "Channel.h"
#include "../Console.h"
#include "Audio.h"
#include <SDL2/SDL_mixer.h>
#include <utility>

using namespace OpenLoco::Audio;

Channel::Channel(int32_t cid)
    : _id(cid)
{
}

Channel::Channel(Channel&& c)
    : _id(std::exchange(c._id, kUndefinedId))
    , _chunk(std::exchange(c._chunk, nullptr))
    , _chunk_owner(std::exchange(c._chunk_owner, {}))
{
}

Channel& Channel::operator=(Channel&& other)
{
    std::swap(_id, other._id);
    std::swap(_chunk, other._chunk);
    std::swap(_chunk_owner, other._chunk_owner);
    return *this;
}

Channel::~Channel()
{
    disposeChunk();
}

bool Channel::load(Sample& sample)
{
    disposeChunk();
    _chunk = sample.chunk;
    return true;
}

bool Channel::load(const fs::path& path)
{
    disposeChunk();
    _chunk = Mix_LoadWAV(path.u8string().c_str());
    _chunk_owner = true;
    return _chunk != nullptr;
}

bool Channel::play(bool loop)
{
    if (!isUndefined())
    {
        int loops = loop ? -1 : 0;
        if (Mix_PlayChannel(_id, _chunk, loops) == -1)
        {
            Console::log("Error during Mix_PlayChannel: %s", Mix_GetError());
            return false;
        }
        return true;
    }
    return false;
}

void Channel::stop()
{
    if (!isUndefined())
    {
        Mix_HaltChannel(_id);
    }
}

void Channel::setVolume(int32_t volume)
{
    if (!isUndefined())
    {
        Mix_Volume(_id, volumeLocoToSDL(volume));
    }
}

void Channel::setPan(int32_t pan)
{
    if (!isUndefined())
    {
        auto [left, right] = panLocoToSDL(pan);
        Mix_SetPanning(_id, left, right);
    }
}

void Channel::setFrequency(int32_t freq)
{
    // TODO
}

bool Channel::isPlaying() const
{
    return isUndefined() ? false : (Mix_Playing(_id) != 0);
}

void Channel::disposeChunk()
{
    if (_chunk_owner)
    {
        Mix_FreeChunk(_chunk);
    }
    _chunk = nullptr;
    _chunk_owner = false;
}
