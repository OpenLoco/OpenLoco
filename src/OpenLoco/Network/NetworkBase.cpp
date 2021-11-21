#include "NetworkBase.h"
#include "../Console.h"
#include "../Platform/Platform.h"

using namespace OpenLoco;
using namespace OpenLoco::Network;

NetworkBase::NetworkBase()
{
}

NetworkBase::~NetworkBase()
{
    close();
}

bool NetworkBase::isClosed() const
{
    return _isClosed;
};

void NetworkBase::receivePacketLoop()
{
    while (!_endReceivePacketLoop)
    {
        bool receivedPacket{};
        for (auto& socket : _sockets)
        {
            Packet packet;
            size_t packetSize{};

            std::unique_ptr<INetworkEndpoint> endpoint;
            auto result = socket->receiveData(&packet, sizeof(Packet), &packetSize, &endpoint);
            if (result == NetworkReadPacket::success)
            {
                // Validate packet
                if (packet.header.dataSize <= packetSize - sizeof(PacketHeader))
                {
                    onReceivePacket(*socket, std::move(endpoint), packet);
                }
                receivedPacket = true;
            }
        }
        if (!receivedPacket)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void NetworkBase::beginReceivePacketLoop()
{
    _endReceivePacketLoop = false;
    _receivePacketThread = std::thread([this] { receivePacketLoop(); });
}

void NetworkBase::endReceivePacketLoop()
{
    _endReceivePacketLoop = true;
    if (_receivePacketThread.joinable())
    {
        _receivePacketThread.join();
    }
    _receivePacketThread = {};
}

void NetworkBase::update()
{
    onUpdate();
}

void NetworkBase::onClose()
{
}

void NetworkBase::onUpdate()
{
}

void NetworkBase::onReceivePacket(IUdpSocket& socket, std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
}

void NetworkBase::close()
{
    if (_isClosed)
        return;

    endReceivePacketLoop();

    onClose();

    _sockets.clear();
    _isClosed = true;
}
