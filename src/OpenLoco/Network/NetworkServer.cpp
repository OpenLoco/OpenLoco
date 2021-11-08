#include "NetworkServer.h"
#include "../Console.h"
#include "../Core/Span.hpp"
#include "../OpenLoco.h"
#include "../S5/S5.h"
#include "../Utility/String.hpp"
#include "NetworkConnection.h"
#include <sstream>

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
            newClient->endpoint = endpoint->clone();
            newClient->connection = std::make_unique<NetworkConnection>(_socket.get(), std::move(endpoint));
            newClient->name = Utility::nullTerminatedView(connectPacket->name);
            _clients.push_back(std::move(newClient));

            auto& newClientPtr = *_clients.back();

            ConnectResponsePacket response;
            response.result = ConnectionResult::success;
            newClientPtr.connection->sendPacket(response);

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

    std::stringstream ss;
    S5::save(ss, S5::SaveFlags::noWindowClose);
    auto final = ss.str();
    auto saveData = stdx::span(reinterpret_cast<const uint8_t*>(final.data()), final.size());

    RequestStateResponse response;
    response.cookie = request.cookie;
    response.totalSize = saveData.size();
    response.numChunks = static_cast<uint16_t>((saveData.size() + (chunkSize - 1)) / chunkSize);
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
        chunk.size = std::min<uint32_t>(chunkSize, remaining - offset);

        client.connection->sendPacket(chunk);

        offset += chunk.size;
        index++;
    }
}
