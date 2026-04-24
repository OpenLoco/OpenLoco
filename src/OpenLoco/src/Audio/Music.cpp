#include "Audio.h"
#include "Config.h"
#include "Date.h"
#include "Jukebox.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Audio/AudioEngine.h>

using namespace OpenLoco::Environment;

namespace OpenLoco::Audio
{
    static AudioHandle _musicHandle = AudioHandle::null;

    void revalidateCurrentTrack()
    {
        const auto currentTrack = Jukebox::getCurrentTrack();
        if (currentTrack == Jukebox::kNoSong)
        {
            return;
        }

        using Config::MusicPlaylistType;
        const auto& cfg = Config::get();

        bool trackStillApplies = true;
        switch (cfg.audio.playlist)
        {
            case MusicPlaylistType::currentEra:
            {
                auto currentYear = getCurrentYear();
                const auto& info = Jukebox::getMusicInfo(currentTrack);
                if (currentYear < info.startYear || currentYear > info.endYear)
                {
                    trackStillApplies = false;
                }
                break;
            }

            case MusicPlaylistType::all:
                return;

            case MusicPlaylistType::custom:
                if (!cfg.audio.customJukebox[currentTrack])
                {
                    trackStillApplies = false;
                }
                break;
        }

        if (!trackStillApplies)
        {
            stopMusic();
            Jukebox::resetJukebox();
        }
    }

    void playBackgroundMusic()
    {
        auto& cfg = Config::get();
        if (cfg.audio.playJukeboxMusic == 0 || !isAudioEnabled() || SceneManager::isTitleMode() || SceneManager::isEditorMode() || SceneManager::isPaused())
        {
            return;
        }

        bool musicPlaying = _musicHandle != AudioHandle::null && isPlaying(_musicHandle);
        if (!musicPlaying)
        {
            const auto& mi = Jukebox::changeTrack();
            playMusic(mi.pathId, cfg.audio.mainVolume, false);
            Ui::WindowManager::invalidate(Ui::WindowType::options);
        }
    }

    void playMusic(PathId sample, int32_t volume, bool loop)
    {
        if (!isAudioEnabled())
        {
            return;
        }

        if (_musicHandle != AudioHandle::null)
        {
            destroy(_musicHandle);
            _musicHandle = AudioHandle::null;
        }

        auto musicSample = loadMusicSample(sample);
        if (musicSample.has_value())
        {
            AudioAttributes attribs{};
            attribs.volume = volume;
            attribs.loop = loop;
            _musicHandle = create(*musicSample, ChannelId::music, attribs);
            Audio::play(_musicHandle);
        }
    }

    void resetMusic()
    {
        stopMusic();
        Jukebox::resetJukebox();
    }

    void stopMusic()
    {
        if (_musicHandle != AudioHandle::null)
        {
            destroy(_musicHandle);
            _musicHandle = AudioHandle::null;
        }
    }

    void pauseMusic()
    {
        if (_musicHandle != AudioHandle::null && isPlaying(_musicHandle))
        {
            pause(_musicHandle);
        }
    }

    void unpauseMusic()
    {
        if (_musicHandle != AudioHandle::null && isPaused(_musicHandle))
        {
            unpause(_musicHandle);
        }
    }

    void setBgmVolume(int32_t volume)
    {
        auto& cfg = Config::get().audio;
        if (cfg.mainVolume == volume)
        {
            return;
        }

        cfg.mainVolume = volume;
        Config::write();

        if (_musicHandle != AudioHandle::null && Jukebox::getCurrentTrack() != Jukebox::kNoSong)
        {
            setVolume(_musicHandle, volume);
        }
    }
}
