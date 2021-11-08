#include "NetworkConnection.h"
#include "../Console.h"
#include "../Platform/Platform.h"

using namespace OpenLoco::Network;

constexpr uint32_t redeliverTimeout = 3500;

NetworkConnection::NetworkConnection(IUdpSocket* socket, std::unique_ptr<INetworkEndpoint> endpoint)
    : _socket(socket)
    , _endpoint(std::move(endpoint))
{
}

void NetworkConnection::update()
{
    resendUndeliveredPackets();
}

bool NetworkConnection::checkOrRecordReceivedSequence(sequence_t sequence)
{
    for (auto s : _receivedSequences)
    {
        if (s == sequence)
        {
            return true;
        }
    }
    _receivedSequences.push_back(sequence);
    return false;
}

void NetworkConnection::recievePacket(const Packet& packet)
{
    logPacket(packet, false, false);
    if (packet.header.kind == PacketKind::ack)
    {
        receiveAcknowledgePacket(packet.header.sequence);
    }
    else
    {
        // Send ACK back, even if we have already received this packet before
        // the ACK we sent before, may not have been delivered successfully
        sendAcknowledgePacket(packet.header.sequence);

        // Only store the packet, if this is the first time we received it
        if (checkOrRecordReceivedSequence(packet.header.sequence))
        {
            std::unique_lock<std::mutex> lk(_receivedPacketsSync);
            _receivedPackets.push(packet);
        }
    }
}

void NetworkConnection::sendPacket(const Packet& packet)
{
    if (packet.header.kind != PacketKind::ack)
    {
        std::unique_lock<std::mutex> lk(_sentPacketsSync);
        auto timestamp = Platform::getTime();
        _sentPackets.push_back({ timestamp, packet });
    }

    size_t packetSize = sizeof(PacketHeader) + packet.header.dataSize;
    _socket->SendData(*_endpoint, &packet, packetSize);
    logPacket(packet, true, false);
}

void NetworkConnection::receiveAcknowledgePacket(sequence_t sequence)
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

void NetworkConnection::sendAcknowledgePacket(sequence_t sequence)
{
    Packet packet;
    packet.header.kind = PacketKind::ack;
    packet.header.sequence = sequence;
    packet.header.dataSize = 0;
    sendPacket(packet);
}

void NetworkConnection::resendUndeliveredPackets()
{
    std::unique_lock<std::mutex> lk(_sentPacketsSync);
    auto now = Platform::getTime();
    auto timestamp = now - redeliverTimeout;

    for (size_t i = 0; i < _sentPackets.size(); i++)
    {
        auto& sentPacket = _sentPackets[i];
        if (sentPacket.timestamp < timestamp)
        {
            size_t packetSize = sizeof(PacketHeader) + sentPacket.packet.header.dataSize;
            _socket->SendData(*_endpoint, &sentPacket.packet, packetSize);
            logPacket(sentPacket.packet, true, true);

            sentPacket.timestamp = now;
        }
    }
}

Packet NetworkConnection::takeNextPacket()
{
    Packet packet;
    std::unique_lock<std::mutex> lk(_receivedPacketsSync);
    if (!_receivedPackets.empty())
    {
        packet = _receivedPackets.front();
        _receivedPackets.pop();
    }
    return packet;
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

void NetworkConnection::logPacket(const Packet& packet, bool sent, bool resend)
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
