#pragma once

#include "Network.h"
#include "NetworkBase.h"
#include "NetworkConnection.h"
#include "Socket.h"
#include <mutex>

namespace OpenLoco::Network
{
    class NetworkConnection;

    struct Client
    {
        client_id_t id{};
        std::unique_ptr<NetworkConnection> connection;
        std::string name;
    };

    struct ChatMessage
    {
        client_id_t sender;
        std::string message;
    };

    class NetworkServer : public NetworkBase
    {
    private:
        std::mutex _incomingConnectionsSync;
        std::mutex _chatMessageQueueSync;

        std::vector<std::unique_ptr<NetworkConnection>> _incomingConnections;
        std::vector<std::unique_ptr<Client>> _clients;
        std::queue<ChatMessage> _chatMessageQueue;
        client_id_t _nextClientId = 1;
        uint32_t _lastPing{};

        Client* findClient(const INetworkEndpoint& endpoint);
        void createNewClient(std::unique_ptr<NetworkConnection> conn, const ConnectPacket& packet);
        void onReceivePacketFromClient(Client& client, const Packet& packet);
        void onReceiveStateRequestPacket(Client& client, const RequestStatePacket& packet);
        void onReceiveSendChatMessagePacket(Client& client, const SendChatMessage& packet);
        void onReceiveGameCommandPacket(Client& client, const GameCommandPacket& packet);
        void removedTimedOutClients();
        void sendPings();
        void sendChatMessages();
        void processIncomingConnections();
        void processPackets();
        void updateClients();

        template<typename T>
        void sendPacketToAll(const T& packet)
        {
            for (auto& client : _clients)
            {
                client->connection->sendPacket(packet);
            }
        }

    protected:
        void onClose();
        void onReceivePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);
        void onUpdate();

    public:
        void listen(port_t port);
        void sendChatMessage(std::string_view message) override;
        void sendGameCommand(uint32_t tick, OpenLoco::Interop::registers regs);
    };
}
