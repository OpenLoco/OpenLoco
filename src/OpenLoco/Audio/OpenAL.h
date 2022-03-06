#pragma once
#include "../Core/Span.hpp"
#include <AL/alc.h>
#include <string>
#include <vector>

// TODO: When ubuntu dependencies upreved remove AL/alc.h and forward declare ALCcontext and ALCdevice

namespace OpenAL
{
    class Source
    {
    private:
        uint32_t _id;

    public:
        Source(uint32_t id)
            : _id(id)
        {
        }

        void play();
        void stop();
        void setBuffer(uint32_t bufferId);
        // value to be of the range 0.0f -> 1.0f
        void setPitch(float value);
        // value to be of the range 0.0f -> 1.0f
        void setGain(float value);
        void setPosition(float x, float y, float z);
        // value to be of the range -0.5f -> 0.5f
        void setPan(float value);
        void setLooping(bool value);
        bool isPlaying() const;
        uint32_t getId() const { return _id; }
    };

    class Context
    {
    private:
        ALCcontext* _context = nullptr;
        bool _isOpen = false;

    public:
        ~Context();

        void close();
        void open(ALCdevice* device);
    };

    class Device
    {
    private:
        ALCdevice* _device = nullptr;
        Context _context;
        bool _isOpen = false;

    public:
        ~Device();

        void open(const std::string& name);
        void close();
        std::vector<std::string> getAvailableDeviceNames() const;
    };

    class BufferManager
    {
        std::vector<uint32_t> _buffers;

    public:
        ~BufferManager();
        uint32_t allocate(stdx::span<const uint8_t> data, uint32_t sampleRate, bool stereo, uint8_t bits);
        void deAllocate(uint32_t id);
        void dispose();
    };

    class SourceManager
    {
        std::vector<uint32_t> _sources;

    public:
        ~SourceManager();
        Source allocate();
        void deAllocate(const Source& source);
        void dispose();
    };

    float volumeFromLoco(int32_t volume);
    float freqFromLoco(int32_t freq);
    float panFromLoco(int32_t pan);
}
