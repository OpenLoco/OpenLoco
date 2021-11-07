#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace OpenLoco::Network
{
    typedef uint16_t port_t;

    constexpr port_t defaultPort = 11754;
    constexpr uint16_t maxPacketSize = 4096;
    constexpr uint16_t networkVersion = 1;

    enum class PacketKind : uint16_t
    {
        connect = 1,
        connectResponse = 2,
    };

    struct PacketHeader
    {
        PacketKind kind{};
        uint16_t sequence{};
        uint16_t dataSize{};
    };

    constexpr uint16_t maxPacketDataSize = maxPacketSize - sizeof(PacketHeader);

    struct Packet
    {
        PacketHeader header;
        uint8_t data[maxPacketDataSize]{};

        template<PacketKind TKind, typename T>
        const T* As() const
        {
            if (header.kind == TKind && header.dataSize >= sizeof(T))
            {
                return reinterpret_cast<const T*>(data);
            }
            return nullptr;
        }
    };

    struct ConnectPacket
    {
        uint16_t version{};
        char name[32]{};
    };

    enum class ConnectionResult
    {
        success,
        error,
    };

    struct ConnectResponsePacket
    {
        ConnectionResult result;
        char message[256]{};
    };

    void openServer();
    void joinServer(std::string_view host);
    void joinServer(std::string_view host, port_t port);
    void close();
    void update();
}
