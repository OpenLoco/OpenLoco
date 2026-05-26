#pragma once

#include "Network.h"
#include "Packet.h"
#include "Socket.h"
#include <cassert>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace OpenLoco::Network
{
    class NetworkConnection
    {
    private:
        struct SentPacket
        {
            uint32_t timestamp;
            Packet packet;
        };

        IUdpSocket* _socket;
        std::unique_ptr<INetworkEndpoint> _endpoint;
        std::mutex _sentPacketsSync;
        std::mutex _receivedPacketsSync;
        std::vector<SentPacket> _sentPackets;
        std::queue<Packet> _receivedPackets;
        std::deque<sequence_t> _receivedSequences;
        uint16_t _sendSequence{};
        uint32_t _timeOfLastReceivedPacket{};

        static uint32_t getTime();
        bool checkOrRecordReceivedSequence(sequence_t sequence);
        void receiveAcknowledgePacket(sequence_t sequence);
        void sendAcknowledgePacket(sequence_t sequence);
        void resendUndeliveredPackets();
        void sendPacket(PacketKind kind, size_t dataSize, const void* packetData);
        void logPacket(const Packet& packet, bool sent, bool resend);

    public:
        NetworkConnection(IUdpSocket* socket, std::unique_ptr<INetworkEndpoint> endpoint);

        const INetworkEndpoint& getEndpoint() const;
        bool hasTimedOut() const;
        void update();
        void receivePacket(const Packet& packet);
        void sendPacket(const Packet& packet);
        std::optional<Packet> takeNextPacket();

        template<typename T>
        void sendPacket(const T& packetData)
        {
            sendPacket(T::kind, packetData.size(), &packetData);
        }
    };
}
