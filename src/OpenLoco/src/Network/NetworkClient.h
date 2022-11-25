#pragma once

#include "Core/Span.hpp"
#include "Network.h"
#include "NetworkBase.h"
#include "Socket.h"
#include <cstdint>
#include <list>
#include <vector>

namespace OpenLoco::Network
{
    class NetworkConnection;

    enum class NetworkClientStatus
    {
        none,
        connecting,
        connectedSuccessfully,
        waitingForState,
        connected,
        closed,
    };

    class NetworkClient : public NetworkBase
    {
    private:
        std::unique_ptr<INetworkEndpoint> _serverEndpoint;
        std::unique_ptr<NetworkConnection> _serverConnection;
        NetworkClientStatus _status{};
        uint32_t _timeout{};
        uint32_t _localGameCommandIndex;
        uint32_t _serverGameCommandIndex;
        uint32_t _localTick;
        uint32_t _serverTick;
        std::list<GameCommandPacket> _receivedGameCommands;

        struct ReceivedChunk
        {
            uint32_t offset{};
            std::vector<uint8_t> data;
        };

        uint32_t _requestStateCookie{};
        uint32_t _requestStateTotalSize{};
        uint16_t _requestStateNumChunks{};
        std::vector<ReceivedChunk> _requestStateChunksReceived;
        uint32_t _requestStateReceivedBytes{};
        uint32_t _requestStateReceivedChunks{};

        void onCancel();
        void processReceivedPackets();
        bool hasTimedOut() const;
        void onReceivePacketFromServer(const Packet& packet);
        void processFullState(stdx::span<uint8_t const> data);
        void updateLocalTick();

        void initStatus(std::string_view text);
        void setStatus(std::string_view text);
        void clearStatus();
        void endStatus(std::string_view text);

        void sendConnectPacket();
        void sendRequestStatePacket();

        void receiveConnectionResponsePacket(const ConnectResponsePacket& response);
        void receiveRequestStateResponsePacket(const RequestStateResponse& response);
        void receiveRequestStateResponseChunkPacket(const RequestStateResponseChunk& responseChunk);
        void receiveChatMessagePacket(const ReceiveChatMessage& packet);
        void receivePingPacket(const PingPacket& packet);
        void receiveGameCommandPacket(const GameCommandPacket& packet);

    protected:
        void onClose() override;
        void onUpdate() override;
        void onReceivePacket(IUdpSocket& socket, std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet) override;

    public:
        ~NetworkClient() override;

        NetworkClientStatus getStatus() const;
        uint32_t getLocalTick() const;

        void connect(std::string_view host, port_t port);
        void sendChatMessage(std::string_view message) override;
        void sendGameCommand(CompanyId company, const OpenLoco::Interop::registers& regs);

        bool shouldProcessTick(uint32_t tick) const;
        void runGameCommandsForTick(uint32_t tick);
    };
}
