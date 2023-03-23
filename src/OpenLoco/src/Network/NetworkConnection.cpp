#include "NetworkConnection.h"
#include "Logging.h"
#include <OpenLoco/Platform/Platform.h>
#include <cstring>

using namespace OpenLoco::Network;

constexpr uint32_t kRedeliverTimeout = 1000;
constexpr uint32_t kConnectionTimeout = 15000;

NetworkConnection::NetworkConnection(IUdpSocket* socket, std::unique_ptr<INetworkEndpoint> endpoint)
    : _socket(socket)
    , _endpoint(std::move(endpoint))
{
}

const INetworkEndpoint& NetworkConnection::getEndpoint() const
{
    return *_endpoint;
}

uint32_t NetworkConnection::getTime()
{
    return Platform::getTime();
}

bool NetworkConnection::hasTimedOut() const
{
    auto durationSinceLastPacket = getTime() - _timeOfLastReceivedPacket;
    if (durationSinceLastPacket > kConnectionTimeout)
    {
        return true;
    }
    return false;
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

    while (_receivedSequences.size() >= 1024)
    {
        _receivedSequences.pop_back();
    }

    _receivedSequences.push_back(sequence);
    return false;
}

void NetworkConnection::receivePacket(const Packet& packet)
{
    _timeOfLastReceivedPacket = Platform::getTime();

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
        if (!checkOrRecordReceivedSequence(packet.header.sequence))
        {
            std::unique_lock<std::mutex> lk(_receivedPacketsSync);
            _receivedPackets.push(packet);
        }
    }
}

void NetworkConnection::sendPacket(PacketKind kind, size_t dataSize, const void* packetData)
{
    assert(dataSize <= kMaxPacketDataSize);

    Packet packet;
    packet.header.kind = kind;
    packet.header.sequence = _sendSequence++;
    packet.header.dataSize = static_cast<uint16_t>(dataSize);
    std::memcpy(packet.data, packetData, packet.header.dataSize);

    sendPacket(packet);
}

void NetworkConnection::sendPacket(const Packet& packet)
{
    if (packet.header.kind != PacketKind::ack)
    {
        std::unique_lock<std::mutex> lk(_sentPacketsSync);
        auto timestamp = getTime();
        _sentPackets.push_back({ timestamp, packet });
    }

    size_t packetSize = sizeof(PacketHeader) + packet.header.dataSize;
    _socket->sendData(*_endpoint, &packet, packetSize);
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
    auto now = getTime();
    auto timestamp = now - kRedeliverTimeout;

    for (size_t i = 0; i < _sentPackets.size(); i++)
    {
        auto& sentPacket = _sentPackets[i];
        if (sentPacket.timestamp < timestamp)
        {
            size_t packetSize = sizeof(PacketHeader) + sentPacket.packet.header.dataSize;
            _socket->sendData(*_endpoint, &sentPacket.packet, packetSize);
            logPacket(sentPacket.packet, true, true);

            sentPacket.timestamp = now;
        }
    }
}

std::optional<Packet> NetworkConnection::takeNextPacket()
{
    std::unique_lock<std::mutex> lk(_receivedPacketsSync);
    if (!_receivedPackets.empty())
    {
        auto packet = _receivedPackets.front();
        _receivedPackets.pop();
        return packet;
    }
    return std::nullopt;
}

[[maybe_unused]] static const char* getPacketKindString(PacketKind kind)
{
    switch (kind)
    {
        case PacketKind::ack: return "ACK";
        case PacketKind::ping: return "PING";
        case PacketKind::connect: return "CONNECT";
        case PacketKind::connectResponse: return "CONNECT RESPONSE";
        case PacketKind::requestState: return "REQUEST STATE";
        case PacketKind::requestStateResponse: return "REQUEST STATE RESPONSE";
        case PacketKind::requestStateResponseChunk: return "REQUEST STATE RESPONSE CHUNK";
        case PacketKind::sendChatMessage: return "SEND CHAT";
        case PacketKind::receiveChatMessage: return "RECEIVE CHAT";
        case PacketKind::gameCommand: return "GAME COMMAND";
        default: return "UNKNOWN";
    }
}

void NetworkConnection::logPacket([[maybe_unused]] const Packet& packet, [[maybe_unused]] bool sent, [[maybe_unused]] bool resend)
{
#if defined(DEBUG)
#ifdef LOG_PACKETS
    auto szDirection = sent ? "SENT" : "RECV";
    auto seq = static_cast<int32_t>(packet.header.sequence);
    auto bytes = static_cast<int32_t>(packet.header.dataSize);
    if (packet.header.kind == PacketKind::ack || packet.header.kind == PacketKind::ping || resend)
    {
#ifdef LOG_ACK_PACKETS
        Logging::logDeprecated("[%s] #%4d | ACK", szDirection, seq);
#endif
    }
    else if (resend)
    {
        Logging::logDeprecated("[%s] #%4d | RESEND", szDirection, seq);
    }
    else if (packet.header.kind == PacketKind::gameCommand)
    {
        auto kind = getPacketKindString(packet.header.kind);
        const auto& gc = packet.Cast<GameCommandPacket>();
        Logging::logDeprecated("[%s] #%4d | %s (Index = %d Tick = %d Command = %d)", szDirection, seq, kind, gc->index, gc->tick, gc->regs.esi);
    }
    else
    {
        auto kind = getPacketKindString(packet.header.kind);
        Logging::logDeprecated("[%s] #%4d | %s (%d bytes)", szDirection, seq, kind, bytes);
    }
#endif
#endif
}
