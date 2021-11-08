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
        std::thread _receivePacketThread;
        bool _endReceivePacketLoop{};
        bool _isClosed{};

        void receivePacketLoop();

    protected:
        std::unique_ptr<IUdpSocket> _socket;

        void beginReceivePacketLoop();
        void endReceivePacketLoop();

        virtual void onClose();
        virtual void onUpdate();
        virtual void onReceivePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet);

    public:
        NetworkBase();
        virtual ~NetworkBase();

        bool isClosed() const;
        void close();
        void update();
    };
}
