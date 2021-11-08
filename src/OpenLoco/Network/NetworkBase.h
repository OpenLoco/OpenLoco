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
