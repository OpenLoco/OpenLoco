#pragma once

#include "../things/thing.h"
#include "audio.h"
#include "channel.h"

#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
#else
#include <experimental/filesystem>
#endif

struct _Mix_Music;
typedef struct _Mix_Music Mix_Music;

namespace openloco::audio
{
#ifdef _OPENLOCO_USE_BOOST_FS_
    namespace fs = boost::filesystem;
#else
    namespace fs = std::experimental::filesystem;
#endif

    class music_channel
    {
    private:
        Mix_Music* _music_track;
        int32_t _current_music = -1;

    public:
        music_channel() = default;
        music_channel(const music_channel&) = delete;
        ~music_channel();

        bool is_playing() const;

        bool load(const fs::path& path);
        bool play(bool loop);
        void stop();
        void set_volume(int32_t volume);
        void dispose_music();
    };
}
