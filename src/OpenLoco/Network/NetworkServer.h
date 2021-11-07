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

        void onRecievePacketFromClient(Client& client, const Packet& packet);

    protected:
        void onClose();
        void onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);

    private:
        Client* findClient(const INetworkEndpoint& endpoint);

    public:
        void listen(port_t port);
    };
}
