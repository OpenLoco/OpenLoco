#pragma once

#include "Network.h"
#include "Socket.h"
#include <cstdint>
#include <memory>
#include <thread>

namespace OpenLoco::Network
{
    class NetworkBase
    {
    private:
        std::thread _recievePacketThread;
        bool _endRecievePacketLoop{};
        bool _isClosed{};

        void recievePacketLoop();

    protected:
        std::unique_ptr<IUdpSocket> _socket;

        template<PacketKind TKind, typename T>
        void sendPacket(const INetworkEndpoint& endpoint, const T& packetData)
        {
            static_assert(sizeof(packetData) <= maxPacketDataSize, "Packet too large.");

            Packet packet;
            packet.header.kind = TKind;
            packet.header.sequence = 0;
            packet.header.dataSize = sizeof(packetData);
            std::memcpy(packet.data, &packetData, packet.header.dataSize);

            size_t packetSize = sizeof(PacketHeader) + packet.header.dataSize;
            _socket->SendData(endpoint, &packet, packetSize);
        }

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
