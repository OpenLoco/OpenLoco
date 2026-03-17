#include "Blueprints.h"
#include "Construction.h"
#include "GameCommands/Track/CreateSignal.h"
#include "GameCommands/Track/CreateTrack.h"
#include "GameCommands/Track/CreateTrainStation.h"
#include "GameCommands/Track/RemoveSignal.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "GameCommands/Track/RemoveTrainStation.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco
{
    currency32_t removeBlueprint(const CopiedTrack& copiedTrack, const World::Pos3& ghostBPPos, const uint8_t flags)
    {
        currency32_t totalRes = 0;
        for (auto& stationArg : copiedTrack.stationArgs)
        {
            GameCommands::TrainStationRemovalArgs args;
            args.pos = World::Pos3(stationArg.pos.x + ghostBPPos.x, stationArg.pos.y + ghostBPPos.y, ghostBPPos.z);
            args.rotation = stationArg.rotation & 3;
            args.index = 0;
            args.trackId = stationArg.trackId;
            args.type = stationArg.type;
            const auto res = GameCommands::doCommand(args, flags);
            if (res != GameCommands::FAILURE)
            {
                totalRes += res;
            }
        }

        for (auto& signalArg : copiedTrack.signalArgs)
        {
            GameCommands::SignalRemovalArgs args;
            args.pos = World::Pos3(signalArg.pos.x + ghostBPPos.x, signalArg.pos.y + ghostBPPos.y, ghostBPPos.z);
            args.rotation = signalArg.rotation & 3;
            args.index = 0;
            args.trackId = signalArg.trackId;
            args.trackObjType = signalArg.trackObjType;
            args.flags = signalArg.sides;
            const auto res = GameCommands::doCommand(args, flags);
            if (res != GameCommands::FAILURE)
            {
                totalRes += res;
            }
        }

        for (auto& trackArg : copiedTrack.trackArgs)
        {
            GameCommands::TrackRemovalArgs args;
            args.pos = World::Pos3(trackArg.pos.x + ghostBPPos.x, trackArg.pos.y + ghostBPPos.y, ghostBPPos.z);
            args.rotation = trackArg.rotation & 3;
            args.index = 0;
            args.trackId = trackArg.trackId;
            args.trackObjectId = trackArg.trackObjectId;
            const auto res = GameCommands::doCommand(args, flags);
            if (res != GameCommands::FAILURE)
            {
                totalRes += res;
            }
        }
        return totalRes;
    }

    currency32_t placeBlueprint(const CopiedTrack& _copiedTrack, const World::Pos3& ghostBPPos, const uint8_t flags)
    {
        // Duplicate as we need to adjust the position
        auto copiedTrack = CopiedTrack(_copiedTrack);
        currency32_t result = 0;
        // First we do a trial placement of track (can't do signals and stations as they would need the track already down)
        const auto noApplyFlags = flags & ~GameCommands::Flags::apply;
        for (auto& trackArgs : copiedTrack.trackArgs)
        {
            trackArgs.pos += ghostBPPos;
            result = GameCommands::doCommand(trackArgs, noApplyFlags);
            if (result == GameCommands::FAILURE)
            {
                return GameCommands::FAILURE;
            }
        }
        // Now we do the actual placement

        GameCommands::setErrorSound(false);
        bool builtAnything = false;

        result = 0;
        for (auto& trackArgs : copiedTrack.trackArgs)
        {
            const auto res = GameCommands::doCommand(trackArgs, flags);
            if (res != GameCommands::FAILURE)
            {
                builtAnything = true;
                result += res;
            }
        }
        for (auto& signalArgs : copiedTrack.signalArgs)
        {
            signalArgs.pos += ghostBPPos;
            const auto res = GameCommands::doCommand(signalArgs, flags);
            if (res != GameCommands::FAILURE)
            {
                builtAnything = true;
                result += res;
            }
        }
        for (auto& stationArgs : copiedTrack.stationArgs)
        {
            stationArgs.pos += ghostBPPos;
            const auto res = GameCommands::doCommand(stationArgs, flags);
            if (res != GameCommands::FAILURE)
            {
                builtAnything = true;
                result += res;
            }
        }

        GameCommands::setErrorSound(true);

        if (builtAnything)
        {
            Ui::WindowManager::close(Ui::WindowType::error);
        }
        return result;
    }

    CopiedTrack copyTrackToBlueprint(const World::TilePos2 posA, const World::TilePos2 posB)
    {
        auto dirX = posB.x - posA.x > 0 ? 1 : -1;
        auto dirY = posB.y - posA.y > 0 ? 1 : -1;

        CopiedTrack copiedTrack{};
        for (auto yPos = posA.y; yPos != posB.y + dirY; yPos += dirY)
        {
            for (auto xPos = posA.x; xPos != posB.x + dirX; xPos += dirX)
            {
                auto pos = World::toWorldSpace({ xPos, yPos });

                auto tile = World::TileManager::get(pos);
                World::TrackElement* elProcessedTrack = nullptr;
                for (auto& el : tile)
                {
                    auto* elTrack = el.as<World::TrackElement>();
                    auto* elSignal = el.as<World::SignalElement>();
                    auto* elStation = el.as<World::StationElement>();
                    if (elTrack != nullptr)
                    {
                        if (elTrack->isGhost() || elTrack->isAiAllocated())
                        {
                            elProcessedTrack = nullptr;
                            continue;
                        }
                        if (elTrack->owner() != CompanyManager::getControllingId())
                        {
                            elProcessedTrack = nullptr;
                            continue;
                        }
                        if (elTrack->sequenceIndex() != 0)
                        {
                            elProcessedTrack = nullptr;
                            continue;
                        }
                        GameCommands::TrackPlacementArgs args{};
                        auto& trackPiece0 = World::TrackData::getTrackPiece(elTrack->trackId())[0];
                        args.pos = World::Pos3(pos.x, pos.y, elTrack->baseHeight()) - World::Pos3(trackPiece0.x, trackPiece0.y, trackPiece0.z);
                        args.trackId = elTrack->trackId();
                        args.rotation = elTrack->rotation();
                        args.trackObjectId = elTrack->trackObjectId();
                        args.bridge = elTrack->hasBridge() ? elTrack->bridge() : 0xFFU;
                        args.unk = false;
                        args.mods = elTrack->mods();
                        copiedTrack.trackArgs.push_back(args);
                        elProcessedTrack = elTrack;
                    }
                    else if (elSignal != nullptr && elProcessedTrack != nullptr)
                    {
                        if (elSignal->isGhost() || elSignal->isAiAllocated())
                        {
                            continue;
                        }
                        GameCommands::SignalPlacementArgs args{};
                        auto& trackPiece0 = World::TrackData::getTrackPiece(elProcessedTrack->trackId())[0];
                        args.pos = World::Pos3(pos.x, pos.y, elSignal->baseHeight()) - World::Pos3(trackPiece0.x, trackPiece0.y, trackPiece0.z);
                        args.index = 0;
                        args.rotation = elSignal->rotation();
                        args.trackId = elProcessedTrack->trackId();
                        args.trackObjType = elProcessedTrack->trackObjectId();
                        args.type = elSignal->getLeft().signalObjectId();
                        uint16_t sideFlags = 0U;
                        if (elSignal->getLeft().hasSignal() && !elSignal->isLeftGhost())
                        {
                            sideFlags |= 0x8000U;
                        }
                        if (elSignal->getRight().hasSignal() && !elSignal->isRightGhost())
                        {
                            sideFlags |= 0x4000U;
                        }
                        args.sides = sideFlags;
                        copiedTrack.signalArgs.push_back(args);
                    }
                    else if (elStation != nullptr && elProcessedTrack != nullptr)
                    {
                        if (elStation->isGhost() || elStation->isAiAllocated())
                        {
                            continue;
                        }
                        GameCommands::TrainStationPlacementArgs args{};
                        auto& trackPiece0 = World::TrackData::getTrackPiece(elProcessedTrack->trackId())[0];
                        args.pos = World::Pos3(pos.x, pos.y, elStation->baseHeight()) - World::Pos3(trackPiece0.x, trackPiece0.y, trackPiece0.z);
                        args.rotation = elStation->rotation();
                        args.trackId = elProcessedTrack->trackId();
                        args.index = 0;
                        args.trackObjectId = elProcessedTrack->trackObjectId();
                        args.type = elStation->objectId();
                        copiedTrack.stationArgs.push_back(args);
                    }
                }
            }
        }
        auto minPos2 = World::toWorldSpace(posB);
        for (const auto& args : copiedTrack.trackArgs)
        {
            minPos2.x = std::min(minPos2.x, args.pos.x);
            minPos2.y = std::min(minPos2.y, args.pos.y);
        }
        auto minPos = World::Pos3(minPos2, World::TileManager::get(minPos2).surface()->baseHeight());

        for (auto& args : copiedTrack.trackArgs)
        {
            args.pos -= minPos;
        }
        for (auto& args : copiedTrack.signalArgs)
        {
            args.pos -= minPos;
        }
        for (auto& args : copiedTrack.stationArgs)
        {
            args.pos -= minPos;
        }

        return copiedTrack;
    }
}
