#include "NetworkServer.h"
#include "../Console.h"
#include "../OpenLoco.h"
#include "../Utility/String.hpp"

using namespace OpenLoco::Network;

void NetworkServer::listen(port_t port)
{
    _socket->Listen(defaultPort);
    beginRecievePacketLoop();

    setScreenFlag(ScreenFlags::networked);
    setScreenFlag(ScreenFlags::networkHost);

    Console::log("Server opened");
    Console::log("Listening for incoming connections...");
}

void NetworkServer::onClose()
{
    clearScreenFlag(ScreenFlags::networked);
    clearScreenFlag(ScreenFlags::networkHost);
}

Client* NetworkServer::findClient(const INetworkEndpoint& endpoint)
{
    for (auto& client : _clients)
    {
        if (client->endpoint->equals(endpoint))
        {
            return client.get();
        }
    }
    return nullptr;
}

void NetworkServer::onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
    auto client = findClient(*endpoint);
    if (client == nullptr)
    {
        auto connectPacket = packet.As<PacketKind::connect, ConnectPacket>();
        if (connectPacket != nullptr)
        {
            auto newClient = std::make_unique<Client>();
            newClient->id = _nextClientId++;
            newClient->endpoint = std::move(endpoint);
            newClient->name = Utility::nullTerminatedView(connectPacket->name);
            _clients.push_back(std::move(newClient));

            auto& newClientPtr = *_clients.back();

            ConnectResponsePacket response;
            response.result = ConnectionResult::success;
            sendPacket<PacketKind::connectResponse>(newClientPtr, response);

            Console::log("Accepted new client: %s", newClientPtr.name.c_str());
        }
    }
    else
    {
        onRecievePacketFromClient(*client, packet);
    }
}

void NetworkServer::onRecievePacketFromClient(Client& client, const Packet& packet)
{
    switch (packet.header.kind)
    {
        case PacketKind::requestState:
            onReceiveStateRequestPacket(client, *packet.Cast<RequestStatePacket>());
            break;
    }
}

void NetworkServer::onReceiveStateRequestPacket(Client& client, const RequestStatePacket& request)
{
    constexpr uint16_t chunkSize = 4000;

    RequestStateResponse response;
    response.cookie = request.cookie;
    response.totalSize = 41302;
    response.numChunks = (41302 + (chunkSize - 1)) / chunkSize;
    sendPacket<PacketKind::requestStateResponse>(client, response);

    uint32_t offset = 0;
    uint32_t remaining = response.totalSize;
    uint16_t index = 0;
    while (index < response.numChunks)
    {
        RequestStateResponseChunk chunk;
        chunk.cookie = request.cookie;
        chunk.index = index;
        chunk.offset = offset;
        chunk.size = std::min<uint32_t>(chunkSize, remaining - offset);

        sendPacket<PacketKind::requestStateResponseChunk>(client, chunk);

        offset += chunk.size;
        index++;
    }
}
