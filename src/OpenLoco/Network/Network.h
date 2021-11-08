#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace OpenLoco::Network
{
    typedef uint16_t port_t;
    typedef uint16_t sequence_t;

    constexpr port_t defaultPort = 11754;
    constexpr uint16_t maxPacketSize = 4096;
    constexpr uint16_t networkVersion = 1;

#pragma pack(push, 1)
    enum class PacketKind : uint16_t
    {
        unknown,
        ack,
        ping,
        connect,
        connectResponse,
        requestState,
        requestStateResponse,
        requestStateResponseChunk,
    };

    struct PacketHeader
    {
        PacketKind kind{};
        sequence_t sequence{};
        uint16_t dataSize{};
    };

    constexpr uint16_t maxPacketDataSize = maxPacketSize - sizeof(PacketHeader);

    struct Packet
    {
        PacketHeader header;
        uint8_t data[maxPacketDataSize]{};

        template<typename T>
        const T* Cast() const
        {
            return reinterpret_cast<const T*>(data);
        }

        template<PacketKind TKind, typename T>
        const T* As() const
        {
            if (header.kind == TKind && header.dataSize >= sizeof(T))
            {
                return Cast<T>();
            }
            return nullptr;
        }
    };

    struct PingPacket
    {
        static constexpr PacketKind kind = PacketKind::ping;
    };

    struct ConnectPacket
    {
        static constexpr PacketKind kind = PacketKind::connect;

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
        static constexpr PacketKind kind = PacketKind::connectResponse;

        ConnectionResult result;
        char message[256]{};
    };

    struct RequestStatePacket
    {
        static constexpr PacketKind kind = PacketKind::requestState;

        uint32_t cookie{};
    };

    struct RequestStateResponse
    {
        static constexpr PacketKind kind = PacketKind::requestStateResponse;

        uint32_t cookie{};
        uint32_t totalSize{};
        uint16_t numChunks{};
    };

    struct RequestStateResponseChunk
    {
        static constexpr PacketKind kind = PacketKind::requestStateResponseChunk;

        uint32_t cookie{};
        uint16_t index{};
        uint32_t offset{};
        uint32_t size{};
        uint8_t data[maxPacketDataSize - 14]{};
    };
    static_assert(sizeof(RequestStateResponseChunk) == maxPacketDataSize);
#pragma pack(pop)

    void openServer();
    void joinServer(std::string_view host);
    void joinServer(std::string_view host, port_t port);
    void close();
    void update();
}
