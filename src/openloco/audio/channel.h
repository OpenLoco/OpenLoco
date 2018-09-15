#pragma once

#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif

struct Mix_Chunk;

namespace openloco::audio
{
    struct sample;

#ifdef _OPENLOCO_USE_BOOST_FS_
    namespace fs = boost::filesystem;
#else
    namespace fs = std::experimental::filesystem;
#endif

    class channel
    {
    public:
        static constexpr int32_t undefined_id = -1;

    private:
        int32_t _id = undefined_id;
        Mix_Chunk* _chunk{};
        bool _chunk_owner{};

    public:
        channel() = default;
        channel(int32_t id);
        channel(const channel&) = delete;
        channel(channel&&);
        channel& operator=(channel&& other);
        ~channel();
        bool load(sample& sample);
        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void set_volume(int32_t volume);
        void set_pan(int32_t pan);
        void set_frequency(int32_t freq);
        bool is_playing() const;
        bool is_undefined() const { return _id == undefined_id; }
        int32_t id() { return _id; }

    private:
        void dispose_chunk();
    };
}
