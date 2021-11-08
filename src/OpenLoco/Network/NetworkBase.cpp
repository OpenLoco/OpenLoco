#include "NetworkBase.h"
#include "../Console.h"
#include "../Platform/Platform.h"

using namespace OpenLoco;
using namespace OpenLoco::Network;

NetworkBase::NetworkBase()
{
    _socket = Socket::createUdp();
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
        Packet packet;
        size_t packetSize{};

        std::unique_ptr<INetworkEndpoint> endpoint;
        auto result = _socket->ReceiveData(&packet, sizeof(Packet), &packetSize, &endpoint);
        if (result == NetworkReadPacket::Success)
        {
            // Validate packet
            if (packet.header.dataSize <= packetSize - sizeof(PacketHeader))
            {
                onReceivePacket(std::move(endpoint), packet);
            }
        }
        else
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

void NetworkBase::onReceivePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
}

void NetworkBase::close()
{
    if (_isClosed)
        return;

    endReceivePacketLoop();

    onClose();

    _socket = nullptr;
    _isClosed = true;
}
