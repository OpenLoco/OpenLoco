#include "Audio.h"
#include "Environment.h"
#include "Game.h"
#include "GameStateFlags.h"
#include "Map/SurfaceElement.h"
#include "Map/TileLoop.hpp"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/ObjectManager.h"
#include "Objects/TreeObject.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Audio/AudioEngine.h>
#include <optional>

using namespace OpenLoco::Environment;

namespace OpenLoco::Audio
{
    static AudioHandle _ambientHandle = AudioHandle::null;
    static int32_t _ambientVolume = 0;
    static std::optional<PathId> _chosenAmbientNoisePathId = std::nullopt;

    static constexpr auto kAmbientMinVolume = -3500;
    static constexpr auto kAmbientVolumeChangePerTick = 100;
    static constexpr auto kAmbientNumWaterTilesForOcean = 60;
    static constexpr auto kAmbientNumTreeTilesForForest = 30;
    static constexpr auto kAmbientNumMountainTilesForWilderness = 60;

    static constexpr int32_t getAmbientMaxVolume(uint8_t zoom)
    {
        constexpr int32_t _volumes[]{ -1200, -2000, -3000, -3000 };
        return _volumes[zoom];
    }

    void updateAmbientNoise()
    {
        if (!isAudioEnabled())
        {
            return;
        }

        auto* mainViewport = Ui::WindowManager::getMainViewport();
        std::optional<PathId> newAmbientSound = std::nullopt;
        int32_t maxVolume = kAmbientMinVolume;

        if (Game::hasFlags(GameStateFlags::tileManagerLoaded) && mainViewport != nullptr)
        {
            maxVolume = getAmbientMaxVolume(mainViewport->zoom);
            const auto centre = mainViewport->getCentreMapPosition();
            const auto topLeft = World::toTileSpace(centre) - World::TilePos2{ 5, 5 };
            const auto bottomRight = topLeft + World::TilePos2{ 11, 11 };
            const auto searchRange = World::getClampedRange(topLeft, bottomRight);

            size_t waterCount = 0;
            size_t wildernessCount = 0;
            size_t treeCount = 0;
            for (auto& tilePos : searchRange)
            {
                const auto tile = World::TileManager::get(tilePos);
                bool passedSurface = false;
                for (const auto& el : tile)
                {
                    auto* elSurface = el.as<World::SurfaceElement>();
                    if (elSurface != nullptr)
                    {
                        passedSurface = true;
                        if (elSurface->water() != 0)
                        {
                            waterCount++;
                            break;
                        }
                        else if (elSurface->snowCoverage() && elSurface->isLast())
                        {
                            wildernessCount++;
                            break;
                        }
                        else if (elSurface->baseZ() >= 64 && elSurface->isLast())
                        {
                            wildernessCount++;
                            break;
                        }
                        continue;
                    }
                    auto* elTree = el.as<World::TreeElement>();
                    if (passedSurface && elTree != nullptr)
                    {
                        const auto* treeObj = ObjectManager::get<TreeObject>(elTree->treeObjectId());
                        if (!treeObj->hasFlags(TreeObjectFlags::droughtResistant))
                        {
                            treeCount++;
                        }
                    }
                }
            }

            if (waterCount > kAmbientNumWaterTilesForOcean)
            {
                newAmbientSound = PathId::css3;
            }
            else if (wildernessCount > kAmbientNumMountainTilesForWilderness)
            {
                newAmbientSound = PathId::css2;
            }
            else if (treeCount > kAmbientNumTreeTilesForForest)
            {
                newAmbientSound = PathId::css4;
            }
        }

        bool ambientPlaying = _ambientHandle != AudioHandle::null && isPlaying(_ambientHandle);

        if (!newAmbientSound.has_value() || (ambientPlaying && _chosenAmbientNoisePathId != *newAmbientSound))
        {
            const auto newVolume = _ambientVolume - kAmbientVolumeChangePerTick;
            if (newVolume < kAmbientMinVolume)
            {
                _chosenAmbientNoisePathId = std::nullopt;
                if (_ambientHandle != AudioHandle::null)
                {
                    destroy(_ambientHandle);
                    _ambientHandle = AudioHandle::null;
                }
            }
            else
            {
                _ambientVolume = newVolume;
                if (_ambientHandle != AudioHandle::null)
                {
                    setVolume(_ambientHandle, newVolume);
                }
            }
            return;
        }

        if (_chosenAmbientNoisePathId != *newAmbientSound)
        {
            auto musicBuffer = loadMusicSample(*newAmbientSound);
            if (musicBuffer.has_value())
            {
                if (_ambientHandle != AudioHandle::null)
                {
                    destroy(_ambientHandle);
                }
                AudioAttributes attribs{};
                attribs.volume = kAmbientMinVolume;
                attribs.loop = true;
                _ambientHandle = create(*musicBuffer, ChannelId::effects, attribs);
                Audio::play(_ambientHandle);
                _ambientVolume = kAmbientMinVolume;
                _chosenAmbientNoisePathId = *newAmbientSound;
            }
        }
        else
        {
            auto newVolume = std::min(_ambientVolume + kAmbientVolumeChangePerTick, maxVolume);
            _ambientVolume = newVolume;
            if (_ambientHandle != AudioHandle::null)
            {
                setVolume(_ambientHandle, newVolume);
            }
        }
    }

    void stopAmbientNoise()
    {
        if (_ambientHandle != AudioHandle::null)
        {
            destroy(_ambientHandle);
            _ambientHandle = AudioHandle::null;
        }
    }
}
