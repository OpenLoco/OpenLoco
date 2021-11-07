#pragma once

#include "Network.h"
#include "Socket.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace OpenLoco::Network
{
    class NetworkBase
    {
    private:
        struct SentPacket
        {
            uint32_t timestamp;
            std::unique_ptr<INetworkEndpoint> endpoint;
            Packet packet;
        };

        struct ReceivedPacket
        {
            std::unique_ptr<INetworkEndpoint> endpoint;
            Packet packet;
        };

        std::mutex _sentPacketsSync;
        std::mutex _receivedPacketsSync;
        std::vector<SentPacket> _sentPackets;
        std::queue<ReceivedPacket> _receivedPackets;
        uint16_t _sendSequence{};
        std::thread _recievePacketThread;
        bool _endRecievePacketLoop{};
        bool _isClosed{};

        void acknowledgePacket(uint16_t sequence);
        void resendUndeliveredPackets();
        void recievePacketLoop();
        void recievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);
        void processReceivePackets();
        void sendAcknowledgePacket(const INetworkEndpoint& endpoint, uint16_t sequence);
        void logPacket(const Packet& packet, bool sent, bool resend);

    protected:
        std::unique_ptr<IUdpSocket> _socket;

        template<PacketKind TKind, typename T>
        void sendPacket(const INetworkEndpoint& endpoint, const T& packetData)
        {
            static_assert(sizeof(packetData) <= maxPacketDataSize, "Packet too large.");

            Packet packet;
            packet.header.kind = TKind;
            packet.header.sequence = _sendSequence++;
            packet.header.dataSize = sizeof(packetData);
            std::memcpy(packet.data, &packetData, packet.header.dataSize);

            sendPacket(endpoint, packet);
        }

        void sendPacket(const INetworkEndpoint& endpoint, const Packet& packet);
        void beginRecievePacketLoop();
        void endRecievePacketLoop();

        virtual void onClose();
        virtual void onUpdate();
        virtual void onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);

    public:
        NetworkBase();
        virtual ~NetworkBase();

        bool isClosed() const;
        void close();
        void update();
    };
}
