#include "NetworkClient.h"
#include "../Config.h"
#include "../Console.h"
#include "../GameCommands/GameCommands.h"
#include "../Platform/Platform.h"
#include "../S5/S5.h"
#include "../SceneManager.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Stream.hpp"
#include "NetworkConnection.h"

using namespace OpenLoco;
using namespace OpenLoco::Network;

NetworkClient::~NetworkClient()
{
    close();
}

NetworkClientStatus NetworkClient::getStatus() const
{
    return _status;
}

uint32_t NetworkClient::getLocalTick() const
{
    return _localTick;
}

void NetworkClient::connect(std::string_view host, port_t port)
{
    auto szHost = std::string(host);
    _serverEndpoint = Socket::resolve(Protocol::any, szHost, port);

    _sockets.push_back(Socket::createUdp());
    auto& socket = _sockets.back();
    _serverConnection = std::make_unique<NetworkConnection>(socket.get(), _serverEndpoint->clone());

    auto szHostIpAddress = _serverEndpoint->getIpAddress();
    Console::log("Resolved endpoint for %s:%d", szHostIpAddress.c_str(), port);

    beginReceivePacketLoop();

    _status = NetworkClientStatus::connecting;
    _timeout = Platform::getTime() + 5000;

    sendConnectPacket();

    initStatus("Connecting to " + szHost + "...");
}

void NetworkClient::onClose()
{
    _serverConnection = nullptr;
    if (_status != NetworkClientStatus::none && _status != NetworkClientStatus::connecting)
    {
        _status = NetworkClientStatus::closed;
        clearScreenFlag(ScreenFlags::networked);
        Console::log("Disconnected from server");
    }
    else if (_status == NetworkClientStatus::connecting)
    {
        endStatus("Failed to connect to server");
        _status = NetworkClientStatus::closed;
    }
}

void NetworkClient::onUpdate()
{
    processReceivedPackets();
    if (_status == NetworkClientStatus::connecting)
    {
        if (Platform::getTime() >= _timeout)
        {
            close();
            Console::log("Failed to connect to server");
            endStatus("Failed to connect to server");
        }
    }
    else
    {
        if (hasTimedOut())
        {
            Console::log("Connection with server timed out");
            close();
        }
        else
        {
            switch (_status)
            {
                case NetworkClientStatus::connectedSuccessfully:
                    sendRequestStatePacket();
                    _status = NetworkClientStatus::waitingForState;
                    break;
                case NetworkClientStatus::waitingForState:
                    break;
                default:
                    break;
            }
        }
    }
}

void NetworkClient::processReceivedPackets()
{
    if (_serverConnection != nullptr)
    {
        while (auto packet = _serverConnection->takeNextPacket())
        {
            onReceivePacketFromServer(*packet);

            // A packet handler may close the connection
            if (_serverConnection == nullptr)
            {
                return;
            }
        }
        _serverConnection->update();
    }
}

bool NetworkClient::hasTimedOut() const
{
    if (_serverConnection != nullptr)
    {
        return _serverConnection->hasTimedOut();
    }
    return false;
}

void NetworkClient::onReceivePacket(IUdpSocket& socket, std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
    // TODO do we really need the check, it is possible but unlikely
    //      for something else to hijack the UDP client port
    if (_serverEndpoint != nullptr && endpoint->equals(*_serverEndpoint))
    {
        _serverConnection->receivePacket(packet);
    }
}

void NetworkClient::onCancel()
{
    switch (_status)
    {
        case NetworkClientStatus::connecting:
            Console::log("Connecting to server cancelled");
            close();
            break;
        default:
            break;
    }
}

void NetworkClient::onReceivePacketFromServer(const Packet& packet)
{
    switch (packet.header.kind)
    {
        case PacketKind::connectResponse:
            receiveConnectionResponsePacket(*reinterpret_cast<const ConnectResponsePacket*>(packet.data));
            break;
        case PacketKind::requestStateResponse:
            receiveRequestStateResponsePacket(*reinterpret_cast<const RequestStateResponse*>(packet.data));
            break;
        case PacketKind::requestStateResponseChunk:
            receiveRequestStateResponseChunkPacket(*reinterpret_cast<const RequestStateResponseChunk*>(packet.data));
            break;
        case PacketKind::receiveChatMessage:
            receiveChatMessagePacket(*reinterpret_cast<const ReceiveChatMessage*>(packet.data));
            break;
        case PacketKind::ping:
            receivePingPacket(*reinterpret_cast<const PingPacket*>(packet.data));
            break;
        case PacketKind::gameCommand:
            receiveGameCommandPacket(*reinterpret_cast<const GameCommandPacket*>(packet.data));
            break;
        default:
            break;
    }
}

void NetworkClient::sendConnectPacket()
{
    const auto& config = Config::get();
    ConnectPacket packet;
    std::strncpy(packet.name, config.preferredName, sizeof(packet.name));
    packet.version = kNetworkVersion;
    _serverConnection->sendPacket(packet);
}

void NetworkClient::sendRequestStatePacket()
{
    _requestStateCookie = (std::rand() << 16) | std::rand();
    _requestStateChunksReceived.clear();

    RequestStatePacket packet;
    packet.cookie = _requestStateCookie;
    _serverConnection->sendPacket(packet);
}

void NetworkClient::receiveConnectionResponsePacket(const ConnectResponsePacket& response)
{
    if (response.result == ConnectionResult::success)
    {
        _status = NetworkClientStatus::connectedSuccessfully;
        setStatus("Connected to server successfully");
        setScreenFlag(ScreenFlags::networked);
    }
    else
    {
        close();
    }
}

void NetworkClient::receiveRequestStateResponsePacket(const RequestStateResponse& response)
{
    if (response.cookie == _requestStateCookie)
    {
        _requestStateNumChunks = response.numChunks;
        _requestStateTotalSize = response.totalSize;
        _requestStateReceivedChunks = 0;
    }
}

void NetworkClient::receiveRequestStateResponseChunkPacket(const RequestStateResponseChunk& responseChunk)
{
    if (responseChunk.cookie == _requestStateCookie)
    {
        if (_requestStateChunksReceived.size() <= responseChunk.index)
        {
            _requestStateChunksReceived.resize(responseChunk.index + 1);
        }

        auto& rchunk = _requestStateChunksReceived[responseChunk.index];
        if (rchunk.data.size() == 0)
        {
            rchunk.offset = responseChunk.offset;
            rchunk.data.assign(responseChunk.data, responseChunk.data + responseChunk.dataSize);
            _requestStateReceivedChunks++;

            _requestStateReceivedBytes += responseChunk.dataSize;
            setStatus("Receiving state: " + std::to_string(_requestStateReceivedBytes) + " / " + std::to_string(_requestStateTotalSize));
        }

        if (_requestStateReceivedChunks >= _requestStateNumChunks)
        {
            // Construct full data
            std::vector<uint8_t> fullData;
            for (size_t i = 0; i < _requestStateChunksReceived.size(); i++)
            {
                fullData.insert(fullData.end(), _requestStateChunksReceived[i].data.begin(), _requestStateChunksReceived[i].data.end());
            }

            clearStatus();
            _status = NetworkClientStatus::connected;

            processFullState(fullData);
        }
    }
}

void NetworkClient::processFullState(stdx::span<uint8_t const> fullData)
{
    auto* extra = reinterpret_cast<const ExtraState*>(fullData.data() + fullData.size() - sizeof(ExtraState));
    _localGameCommandIndex = extra->gameCommandIndex;
    _localTick = extra->tick;
    updateLocalTick();

    BinaryStream bs(fullData.data(), fullData.size() - sizeof(ExtraState));
    S5::load(bs, 0);
}

void NetworkClient::receiveChatMessagePacket(const ReceiveChatMessage& packet)
{
    Network::receiveChatMessage(packet.sender, packet.getText());
}

void NetworkClient::receivePingPacket(const PingPacket& packet)
{
    if (_status != NetworkClientStatus::connected)
        return;

    // Update the latest knowledge of server state
    _serverTick = std::max(_serverTick, packet.tick);
    _serverGameCommandIndex = std::max(_serverGameCommandIndex, packet.gameCommandIndex);

    if (_localGameCommandIndex == _serverGameCommandIndex)
    {
        // No pending game commands, we can update to this tick
        _localTick = packet.tick;
    }
}

void NetworkClient::receiveGameCommandPacket(const GameCommandPacket& packet)
{
    // Update the latest knowledge of server state
    _serverTick = std::max(_serverTick, packet.tick);
    _serverGameCommandIndex = std::max(_serverGameCommandIndex, packet.index);

    // Catch old or repeated game command index
    assert(packet.index > _localGameCommandIndex);

    // Insert into ordered game command queue
    for (auto it = _receivedGameCommands.begin(); it != _receivedGameCommands.end(); it++)
    {
        auto& p = *it;

        // Catch duplicate game command index
        assert(packet.index != p.index);

        if (packet.index <= p.index)
        {
            _receivedGameCommands.insert(it, packet);
            return;
        }
    }
    _receivedGameCommands.push_back(packet);

    updateLocalTick();
}

void NetworkClient::sendChatMessage(std::string_view message)
{
    if (_serverConnection != nullptr)
    {
        SendChatMessage packet;
        packet.length = static_cast<uint16_t>(message.size() + 1);
        std::memcpy(packet.text, message.data(), message.size());
        _serverConnection->sendPacket(packet);
    }
}

void NetworkClient::sendGameCommand(CompanyId company, const OpenLoco::Interop::registers& regs)
{
    if (_serverConnection != nullptr && _status == NetworkClientStatus::connected)
    {
        GameCommandPacket packet;
        packet.company = company;
        packet.regs = regs;
        _serverConnection->sendPacket(packet);
    }
}

void NetworkClient::updateLocalTick()
{
    // If we have the next game command, we can set local tick to the tick for that command
    if (!_receivedGameCommands.empty())
    {
        auto& nextGameCommand = _receivedGameCommands.front();
        if (nextGameCommand.index == _localGameCommandIndex + 1)
        {
            // We already have the next game command, so we can update to the tick for that command
            _localTick = nextGameCommand.tick;
        }
    }
}

bool NetworkClient::shouldProcessTick(uint32_t tick) const
{
    if (_status != NetworkClientStatus::connected)
        return true;

    return _localTick >= tick;
}

void NetworkClient::runGameCommandsForTick(uint32_t tick)
{
    if (_status != NetworkClientStatus::connected)
        return;

    // Execute all following commands if previously received
    while (!_receivedGameCommands.empty())
    {
        auto& nextPacket = _receivedGameCommands.front();
        if (nextPacket.index == _localGameCommandIndex + 1 && nextPacket.tick == tick)
        {
            _localGameCommandIndex++;
            GameCommands::doCommandForReal(static_cast<GameCommands::GameCommand>(nextPacket.regs.esi), nextPacket.company, nextPacket.regs);
            _receivedGameCommands.pop_front();
        }
        else
        {
            break;
        }
    }

    updateLocalTick();
}

void NetworkClient::initStatus(std::string_view text)
{
    Ui::Windows::NetworkStatus::open(text, [this]() { onCancel(); });
}

void NetworkClient::setStatus(std::string_view text)
{
    Ui::Windows::NetworkStatus::setText(text);
}

void NetworkClient::endStatus(std::string_view text)
{
    Ui::Windows::NetworkStatus::setText(text, nullptr);
}

void NetworkClient::clearStatus()
{
    Ui::Windows::NetworkStatus::close();
}
