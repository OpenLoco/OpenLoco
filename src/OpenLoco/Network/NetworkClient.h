#pragma once

#include "Network.h"
#include "NetworkBase.h"
#include "Socket.h"
#include <cstdint>
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
        closed,
    };

    class NetworkClient : public NetworkBase
    {
    private:
        std::unique_ptr<INetworkEndpoint> _serverEndpoint;
        std::unique_ptr<NetworkConnection> _serverConnection;
        NetworkClientStatus _status{};
        uint32_t _timeout{};

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

        void initStatus(std::string_view text);
        void setStatus(std::string_view text);
        void clearStatus();
        void endStatus(std::string_view text);

        void sendConnectPacket();
        void sendRequestStatePacket();

        void receiveConnectionResponsePacket(const ConnectResponsePacket& response);
        void receiveRequestStateResponsePacket(const RequestStateResponse& response);
        void receiveRequestStateResponseChunkPacket(const RequestStateResponseChunk &responseChunk);

        template<PacketKind TKind, typename T>
        void sendPacket(const T& packetData)
        {
            NetworkBase::sendPacket<TKind, T>(*_serverEndpoint, packetData);
        }

    protected:
        void onClose() override;
        void onUpdate() override;
        void onReceivePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet) override;

    public:
        void connect(std::string_view host, port_t port);
    };
}
