#pragma once

#include "Network.h"
#include "NetworkBase.h"
#include "NetworkConnection.h"
#include "Socket.h"
#include <mutex>

namespace OpenLoco::Network
{
    class NetworkConnection;

    typedef uint32_t client_id_t;

    struct Client
    {
        client_id_t id{};
        std::unique_ptr<NetworkConnection> connection;
        std::string name;
    };

    class NetworkServer : public NetworkBase
    {
    private:
        std::mutex _incomingConnectionsSync;
        std::vector<std::unique_ptr<NetworkConnection>> _incomingConnections;
        std::vector<std::unique_ptr<Client>> _clients;
        client_id_t _nextClientId = 1;
        uint32_t _lastPing{};

        Client* findClient(const INetworkEndpoint& endpoint);
        void createNewClient(std::unique_ptr<NetworkConnection> conn, const ConnectPacket& packet);
        void onReceivePacketFromClient(Client& client, const Packet& packet);
        void onReceiveStateRequestPacket(Client& client, const RequestStatePacket& packet);
        void removedTimedOutClients();
        void sendPings();
        void processIncomingConnections();
        void processPackets();
        void updateClients();

    protected:
        void onClose();
        void onReceivePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);
        void onUpdate();

    public:
        void listen(port_t port);
    };
}
