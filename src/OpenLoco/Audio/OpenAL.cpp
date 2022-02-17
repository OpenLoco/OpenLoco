#include "OpenAL.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <algorithm>
#include <cmath>

namespace OpenAL
{
    void Device::open(const std::string& name)
    {
        if (_isOpen)
        {
            _context.close();
            alcCloseDevice(_device);
        }
        if (name.empty())
        {
            // Open default device
            _device = alcOpenDevice(nullptr);
        }
        else
        {
            // requires c string so cant make name string_view
            _device = alcOpenDevice(name.c_str());
        }
        _context.open(_device);
        _isOpen = true;
    }

    void Device::close()
    {
        if (_isOpen)
        {
            _context.close();
            alcCloseDevice(_device);
            _device = nullptr;
            _isOpen = false;
        }
    }

    Device::~Device()
    {
        close();
    }

    std::vector<std::string> Device::getAvailableDeviceNames() const
    {
        const ALCchar* devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);

        std::vector<std::string> devicesVec;
        const char* ptr = devices;
        do
        {
            devicesVec.push_back(std::string(ptr));
            ptr += devicesVec.back().size() + 1;
        } while (*ptr != '\0');

        return devicesVec;
    }

    void Context::open(ALCdevice* device)
    {
        _context = alcCreateContext(device, nullptr);
        // OpenLoco only ever needs the one context so make this one current for lifetime
        alcMakeContextCurrent(_context);
        _isOpen = true;
    }

    void Context::close()
    {
        if (_isOpen)
        {
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(_context);
            _context = nullptr;
            _isOpen = false;
        }
    }

    Context::~Context()
    {
        close();
    }

    void Source::play()
    {
        alSourcePlay(_id);
    }

    void Source::stop()
    {
        alSourceStop(_id);
    }

    void Source::setBuffer(uint32_t bufferId)
    {
        alSourcei(_id, AL_BUFFER, bufferId);
    }

    void Source::setPitch(float value)
    {
        alSourcef(_id, AL_PITCH, value);
    }

    void Source::setGain(float value)
    {
        alSourcef(_id, AL_GAIN, value);
    }

    void Source::setPosition(float x, float y, float z)
    {
        alSource3f(_id, AL_POSITION, x, y, z);
    }

    void Source::setPan(float value)
    {
        alSourcef(_id, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcei(_id, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(_id, AL_POSITION, value, 0.0f, -std::sqrt(1.0f - value * value));
    }

    void Source::setLooping(bool value)
    {
        alSourcei(_id, AL_LOOPING, value ? AL_TRUE : AL_FALSE);
    }

    bool Source::isPlaying() const
    {
        int32_t value = AL_PLAYING;
        alGetSourcei(_id, AL_SOURCE_STATE, &value);
        return value == AL_PLAYING;
    }

    float volumeFromLoco(int32_t volume)
    {
        // NOTE: Needs further adjustment
        // volume is in 100th dB so to convert 10 ^ ((volume / 100) / 20)
        const auto alVol = std::pow(10.0f, static_cast<float>(volume) / 2000.0f);
        return alVol;
    }

    float freqFromLoco(int32_t freq)
    {
        return freq / 22000.0f;
    }

    float panFromLoco(int32_t pan)
    {
        constexpr auto kRange = 4096.0f;
        return pan / kRange;
    }

    BufferManager::~BufferManager()
    {
        dispose();
    }

    uint32_t BufferManager::allocate(stdx::span<const uint8_t> data, uint32_t sampleRate, bool stereo, uint8_t bits)
    {
        uint32_t id = 0;
        alGenBuffers(1, &id);
        _buffers.push_back(id);
        uint32_t format = 0;
        if (stereo)
        {
            if (bits == 8)
            {
                format = AL_FORMAT_STEREO8;
            }
            else
            {
                format = AL_FORMAT_STEREO16;
            }
        }
        else
        {
            if (bits == 8)
            {
                format = AL_FORMAT_MONO8;
            }
            else
            {
                format = AL_FORMAT_MONO16;
            }
        }
        alBufferData(id, format, data.begin(), data.size(), sampleRate);
        return id;
    }

    void BufferManager::deAllocate(uint32_t id)
    {
        alDeleteBuffers(1, &id);
        _buffers.erase(std::remove(std::begin(_buffers), std::end(_buffers), id));
    }

    void BufferManager::dispose()
    {
        alDeleteBuffers(_buffers.size(), _buffers.data());
        _buffers.clear();
    }

    SourceManager::~SourceManager()
    {
        dispose();
    }

    Source SourceManager::allocate()
    {
        uint32_t id = 0;
        alGenSources(1, &id);
        _sources.push_back(id);
        return Source(id);
    }

    void SourceManager::deAllocate(const Source& source)
    {
        const auto id = source.getId();
        alDeleteSources(1, &id);
        _sources.erase(std::remove(std::begin(_sources), std::end(_sources), id));
    }

    void SourceManager::dispose()
    {
        alDeleteSources(_sources.size(), _sources.data());
        _sources.clear();
    }
}
