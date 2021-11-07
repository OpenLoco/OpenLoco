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
            sendPacket<PacketKind::connectResponse>(*newClientPtr.endpoint, response);

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
}
