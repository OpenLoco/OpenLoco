#pragma once

#include "Network.h"
#include "NetworkBase.h"
#include "Socket.h"

namespace OpenLoco::Network
{
    typedef uint32_t client_id_t;

    struct Client
    {
        client_id_t id{};
        std::unique_ptr<INetworkEndpoint> endpoint;
        std::string name;
    };

    class NetworkServer : public NetworkBase
    {
    private:
        std::vector<std::unique_ptr<Client>> _clients;
        client_id_t _nextClientId = 1;

        template<PacketKind TKind, typename T>
        void sendPacket(Client& client, const T& packet)
        {
            NetworkBase::sendPacket<TKind, T>(*client.endpoint, packet);
        }

        void onRecievePacketFromClient(Client& client, const Packet& packet);
        void onReceiveStateRequestPacket(Client& client, const RequestStatePacket& packet);

    protected:
        void onClose();
        void onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);

    private:
        Client* findClient(const INetworkEndpoint& endpoint);

    public:
        void listen(port_t port);
    };
}
