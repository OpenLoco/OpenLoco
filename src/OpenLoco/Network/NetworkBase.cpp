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

void NetworkBase::recievePacketLoop()
{
    while (!_endRecievePacketLoop)
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
                recievePacket(std::move(endpoint), packet);
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void NetworkBase::beginRecievePacketLoop()
{
    _endRecievePacketLoop = false;
    _recievePacketThread = std::thread([this] { recievePacketLoop(); });
}

void NetworkBase::endRecievePacketLoop()
{
    _endRecievePacketLoop = true;
    if (_recievePacketThread.joinable())
    {
        _recievePacketThread.join();
    }
    _recievePacketThread = {};
}

void NetworkBase::sendPacket(const INetworkEndpoint& endpoint, const Packet& packet)
{
    if (packet.header.kind != PacketKind::ack)
    {
        std::unique_lock<std::mutex> lk(_sentPacketsSync);
        auto timestamp = Platform::getTime();
        _sentPackets.push_back({ timestamp, endpoint.clone(), packet });
    }

    size_t packetSize = sizeof(PacketHeader) + packet.header.dataSize;
    _socket->SendData(endpoint, &packet, packetSize);
    logPacket(packet, true, false);
}

void NetworkBase::acknowledgePacket(uint16_t sequence)
{
    std::unique_lock<std::mutex> lk(_sentPacketsSync);
    for (size_t i = 0; i < _sentPackets.size(); i++)
    {
        if (_sentPackets[i].packet.header.sequence == sequence)
        {
            _sentPackets.erase(_sentPackets.begin() + i);
            break;
        }
    }
}

void NetworkBase::sendAcknowledgePacket(const INetworkEndpoint& endpoint, uint16_t sequence)
{
    Packet packet;
    packet.header.kind = PacketKind::ack;
    packet.header.sequence = sequence;
    packet.header.dataSize = 0;
    sendPacket(endpoint, packet);
}

void NetworkBase::resendUndeliveredPackets()
{
    std::unique_lock<std::mutex> lk(_sentPacketsSync);
    auto now = Platform::getTime();
    auto timestamp = now - 3500;

    for (size_t i = 0; i < _sentPackets.size(); i++)
    {
        auto& sentPacket = _sentPackets[i];
        if (sentPacket.timestamp < timestamp)
        {
            size_t packetSize = sizeof(PacketHeader) + sentPacket.packet.header.dataSize;
            _socket->SendData(*sentPacket.endpoint, &sentPacket.packet, packetSize);
            logPacket(sentPacket.packet, true, true);

            sentPacket.timestamp = now;
        }
    }
}

void NetworkBase::recievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
    logPacket(packet, false, false);
    if (packet.header.kind == PacketKind::ack)
    {
        acknowledgePacket(packet.header.sequence);
    }
    else
    {
        sendAcknowledgePacket(*endpoint, packet.header.sequence);
        std::unique_lock<std::mutex> lk(_receivedPacketsSync);
        _receivedPackets.push({ std::move(endpoint),
                                packet });
    }
}

void NetworkBase::processReceivePackets()
{
    std::unique_lock<std::mutex> lk(_receivedPacketsSync);
    while (!_receivedPackets.empty())
    {
        auto& p = _receivedPackets.front();
        onRecievePacket(std::move(p.endpoint), p.packet);
        _receivedPackets.pop();
    }
}

void NetworkBase::update()
{
    processReceivePackets();
    resendUndeliveredPackets();
    onUpdate();
}

void NetworkBase::onClose()
{
}

void NetworkBase::onUpdate()
{
}

void NetworkBase::onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
}

void NetworkBase::close()
{
    if (_isClosed)
        return;

    endRecievePacketLoop();

    onClose();

    _socket = nullptr;
    _isClosed = true;
}

static const char* getPacketKindString(PacketKind kind)
{
    switch (kind)
    {
        case PacketKind::ack: return "ACK";
        case PacketKind::connect: return "CONNECT";
        case PacketKind::connectResponse: return "CONNECT RESPONSE";
        case PacketKind::requestState: return "REQUEST STATE";
        case PacketKind::requestStateResponse: return "REQUEST STATE RESPONSE";
        case PacketKind::requestStateResponseChunk: return "REQUEST STATE RESPONSE CHUNK";
        default: return "UNKNOWN";
    }
}

void NetworkBase::logPacket(const Packet& packet, bool sent, bool resend)
{
#if defined(DEBUG)
    auto szDirection = sent ? "SENT" : "RECV";
    auto seq = static_cast<int32_t>(packet.header.sequence);
    auto bytes = static_cast<int32_t>(packet.header.dataSize);
    if (packet.header.kind == PacketKind::ack)
    {
#ifdef LOG_ACK_PACKETS
        Console::log("[%s] #%4d | ACK", szDirection, seq);
#endif
    }
    else if (resend)
    {
        Console::log("[%s] #%4d | RESEND", szDirection, seq);
    }
    else
    {
        auto kind = getPacketKindString(packet.header.kind);
        Console::log("[%s] #%4d | %s (%d bytes)", szDirection, seq, kind, bytes);
    }
#endif
}
