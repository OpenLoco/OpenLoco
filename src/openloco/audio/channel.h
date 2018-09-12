#pragma once

#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif

struct Mix_Chunk;

namespace openloco::audio
{
#ifdef _OPENLOCO_USE_BOOST_FS_
    namespace fs = boost::filesystem;
#else
    namespace fs = std::experimental::filesystem;
#endif

    class channel
    {
    public:
        int32_t const id{};

    private:
        Mix_Chunk* _chunk{};

    public:
        channel(int32_t id);
        channel(const channel&) = delete;
        channel(const channel&&);
        ~channel();
        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void set_volume(int32_t volume);
        bool is_playing() const;
    };
}
