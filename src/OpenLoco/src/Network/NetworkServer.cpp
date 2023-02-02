#include "NetworkServer.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "NetworkConnection.h"
#include "S5/S5.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include <OpenLoco/Console/Console.h>
#include <OpenLoco/Core/Span.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <OpenLoco/Utility/Stream.hpp>
#include <OpenLoco/Utility/String.hpp>

using namespace OpenLoco;
using namespace OpenLoco::Network;

constexpr uint32_t kPingInterval = 30;

NetworkServer::~NetworkServer()
{
    close();
}

void NetworkServer::listen(const std::string& bind, port_t port)
{
    // IPv4
    try
    {
        auto socket4 = Socket::createUdp();
        socket4->listen(Protocol::ipv4, bind, port);
        _sockets.push_back(std::move(socket4));
    }
    catch (...)
    {
    }

    // IPv6
    try
    {
        auto socket6 = Socket::createUdp();
        socket6->listen(Protocol::ipv6, bind, port);
        _sockets.push_back(std::move(socket6));
    }
    catch (...)
    {
    }

    if (_sockets.empty())
    {
        throw std::runtime_error("Unable to listen on " + bind + ", port " + std::to_string(port));
    }

    beginReceivePacketLoop();

    setScreenFlag(ScreenFlags::networked);
    setScreenFlag(ScreenFlags::networkHost);

    Console::log("Server opened");
    for (const auto& socket : _sockets)
    {
        auto ipAddress = socket->getIpAddress();
        if (socket->getProtocol() == Protocol::ipv6)
        {
            ipAddress = '[' + ipAddress + ']';
        }
        Console::log("Listening for incoming connections on %s:%d...", ipAddress.c_str(), port);
    }
}

void NetworkServer::onClose()
{
    clearScreenFlag(ScreenFlags::networked);
    clearScreenFlag(ScreenFlags::networkHost);
    Console::log("Server closed");
}

Client* NetworkServer::findClient(const INetworkEndpoint& endpoint)
{
    for (auto& client : _clients)
    {
        if (client->connection->getEndpoint().equals(endpoint))
        {
            return client.get();
        }
    }
    return nullptr;
}

void NetworkServer::createNewClient(std::unique_ptr<NetworkConnection> conn, const ConnectPacket& packet)
{
    auto newClient = std::make_unique<Client>();
    newClient->id = _nextClientId++;
    newClient->connection = std::move(conn);
    newClient->name = Utility::nullTerminatedView(packet.name);
    _clients.push_back(std::move(newClient));

    auto& newClientPtr = *_clients.back();

    ConnectResponsePacket response;
    response.result = ConnectionResult::success;
    newClientPtr.connection->sendPacket(response);

    Console::log("Accepted new client: %s", newClientPtr.name.c_str());
}

void NetworkServer::onReceivePacket(IUdpSocket& socket, std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
    auto client = findClient(*endpoint);
    if (client == nullptr)
    {
        auto connectPacket = packet.as<PacketKind::connect, ConnectPacket>();
        if (connectPacket != nullptr)
        {
            auto conn = std::make_unique<NetworkConnection>(&socket, std::move(endpoint));
            conn->receivePacket(packet);

            std::unique_lock<std::mutex> lk(_incomingConnectionsSync);
            _incomingConnections.push_back(std::move(conn));
        }
    }
    else
    {
        client->connection->receivePacket(packet);
    }
}

void NetworkServer::onReceivePacketFromClient(Client& client, const Packet& packet)
{
    switch (packet.header.kind)
    {
        case PacketKind::requestState:
            onReceiveStateRequestPacket(client, *packet.cast<RequestStatePacket>());
            break;
        case PacketKind::sendChatMessage:
            onReceiveSendChatMessagePacket(client, *packet.cast<SendChatMessage>());
            break;
        case PacketKind::gameCommand:
            onReceiveGameCommandPacket(client, *packet.cast<GameCommandPacket>());
            break;
        default:
            break;
    }
}

void NetworkServer::onReceiveStateRequestPacket(Client& client, const RequestStatePacket& request)
{
    constexpr uint16_t kChunkSize = 4000;

    // Dump S5 data to stream
    MemoryStream ms;
    S5::exportGameStateToFile(ms, S5::SaveFlags::noWindowClose);

    // Append extra state
    ExtraState extra;
    extra.gameCommandIndex = _gameCommandIndex;
    extra.tick = ScenarioManager::getScenarioTicks();
    ms.write(&extra, sizeof(extra));

    RequestStateResponse response;
    response.cookie = request.cookie;
    response.totalSize = ms.getLength();
    response.numChunks = static_cast<uint16_t>((ms.getLength() + (kChunkSize - 1)) / kChunkSize);
    client.connection->sendPacket(response);

    uint32_t offset = 0;
    uint32_t remaining = response.totalSize;
    uint16_t index = 0;
    while (index < response.numChunks)
    {
        RequestStateResponseChunk chunk;
        chunk.cookie = request.cookie;
        chunk.index = index;
        chunk.offset = offset;
        chunk.dataSize = std::min<uint32_t>(kChunkSize, remaining - offset);
        std::memcpy(chunk.data, reinterpret_cast<const uint8_t*>(ms.data()) + offset, chunk.dataSize);

        client.connection->sendPacket(chunk);

        offset += chunk.dataSize;
        index++;
    }
}

void NetworkServer::onReceiveSendChatMessagePacket(Client& client, const SendChatMessage& packet)
{
    std::unique_lock<std::mutex> lk(_chatMessageQueueSync);
    _chatMessageQueue.push({ client.id, std::string(packet.getText()) });
}

void NetworkServer::onReceiveGameCommandPacket(Client& client, const GameCommandPacket& packet)
{
    queueGameCommand(packet.company, packet.regs);
}

void NetworkServer::removedTimedOutClients()
{
    for (auto it = _clients.begin(); it != _clients.end();)
    {
        auto& client = *it;
        if (client->connection->hasTimedOut())
        {
            Console::log("Client timed out: %s", client->name.c_str());
            it = _clients.erase(it);
        }
        else
        {
            it++;
        }
    }

    _clients.erase(
        std::remove_if(_clients.begin(), _clients.end(), [](const std::unique_ptr<Client>& client) { return client->connection->hasTimedOut(); }), _clients.end());
}

void NetworkServer::sendPings()
{
    auto now = Platform::getTime();
    if (now - _lastPing > kPingInterval)
    {
        _lastPing = now;

        auto& gameState = getGameState();

        PingPacket packet;
        packet.gameCommandIndex = _gameCommandIndex;
        packet.tick = gameState.scenarioTicks;
        packet.srand0 = gameState.rng.srand_0();
        packet.srand1 = gameState.rng.srand_1();
        for (auto& client : _clients)
        {
            client->connection->sendPacket(packet);
        }
    }
}

void NetworkServer::sendChatMessages()
{
    std::unique_lock<std::mutex> lk(_chatMessageQueueSync);
    while (!_chatMessageQueue.empty())
    {
        const auto& message = _chatMessageQueue.front();

        Network::receiveChatMessage(message.sender, message.message);

        ReceiveChatMessage packet;
        packet.sender = message.sender;
        packet.length = static_cast<uint16_t>(message.message.size() + 1);
        std::memcpy(packet.text, message.message.data(), message.message.size());
        sendPacketToAll(packet);

        _chatMessageQueue.pop();
    }
}

void NetworkServer::processIncomingConnections()
{
    std::unique_lock<std::mutex> lk(_incomingConnectionsSync);
    for (auto& conn : _incomingConnections)
    {
        // The connect packet should be the first one
        while (auto packet = conn->takeNextPacket())
        {
            if (auto connectPacket = packet->as<PacketKind::connect, ConnectPacket>())
            {
                createNewClient(std::move(conn), *connectPacket);
                break;
            }
        }
    }

    _incomingConnections.clear();
}

void NetworkServer::processPackets()
{
    for (auto& client : _clients)
    {
        while (auto packet = client->connection->takeNextPacket())
        {
            onReceivePacketFromClient(*client, *packet);
        }
    }
}

void NetworkServer::onUpdate()
{
    processIncomingConnections();
    processPackets();
    updateClients();
    sendChatMessages();
    sendPings();
    removedTimedOutClients();
}

void NetworkServer::updateClients()
{
    for (auto& client : _clients)
    {
        client->connection->update();
    }
}

void NetworkServer::sendChatMessage(std::string_view message)
{
    std::unique_lock<std::mutex> lk(_chatMessageQueueSync);
    _chatMessageQueue.push({ 0, std::string(message) });
}

void NetworkServer::sendGameCommand(uint32_t index, uint32_t tick, CompanyId company, const OpenLoco::Interop::registers& regs)
{
    GameCommandPacket packet;
    packet.index = index;
    packet.tick = tick;
    packet.company = company;
    packet.regs = regs;
    sendPacketToAll(packet);
}

void NetworkServer::queueGameCommand(CompanyId company, const OpenLoco::Interop::registers& regs)
{
    GameCommandPacket newPacket;
    newPacket.index = ++_gameCommandIndex;
    newPacket.tick = 0;
    newPacket.company = company;
    newPacket.regs = regs;
    _gameCommands.push(newPacket);
}

void NetworkServer::runGameCommands()
{
    auto& gameState = getGameState();
    auto tick = gameState.scenarioTicks;

    // Execute all following commands if previously received
    while (!_gameCommands.empty())
    {
        auto& gc = _gameCommands.front();

        [[maybe_unused]] auto result = GameCommands::doCommandForReal(static_cast<GameCommands::GameCommand>(gc.regs.esi), gc.company, gc.regs);

        // TODO We can't do this, we have to send a dummy command to the clients
        //      otherwise we skip a game command index
        // if (result != 0x80000000)
        // {
        sendGameCommand(gc.index, tick, gc.company, gc.regs);
        // }

        _gameCommands.pop();
    }
}
