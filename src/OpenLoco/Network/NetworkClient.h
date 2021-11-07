#pragma once

#include "Network.h"
#include "NetworkBase.h"
#include "Socket.h"
#include <cstdint>

namespace OpenLoco::Network
{
    enum class NetworkClientStatus
    {
        none,
        connecting,
        connectedSuccessfully,
        closed,
    };

    class NetworkClient : public NetworkBase
    {
    private:
        std::unique_ptr<IUdpSocket> _socket;
        std::unique_ptr<INetworkEndpoint> _serverEndpoint;
        NetworkClientStatus _status{};
        uint32_t _timeout{};

        void onCancel();
        void onRecievePacketFromServer(const Packet& packet);

        void initStatus(std::string_view text);
        void setStatus(std::string_view text);
        void clearStatus();
        void endStatus(std::string_view text);
        void sendConnectPacket();

        template<PacketKind TKind, typename T>
        void sendPacket(const T& packetData)
        {
            NetworkBase::sendPacket<TKind, T>(*_serverEndpoint, packetData);
        }

    protected:
        void onClose() override;
        void onUpdate() override;
        void onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet) override;

    public:
        void connect(std::string_view host, port_t port);
    };
}
