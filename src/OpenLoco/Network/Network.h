#pragma once

#include "../Interop/Interop.hpp"
#include <cstdint>
#include <memory>
#include <string_view>

namespace OpenLoco::Network
{
    typedef uint32_t client_id_t;
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
        sendChatMessage,
        receiveChatMessage,
        gameCommand,
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
        size_t size() const { return sizeof(PingPacket); }

        uint32_t gameCommandIndex{};
        uint32_t tick{};
        uint32_t srand0{};
        uint32_t srand1{};
    };

    struct ConnectPacket
    {
        static constexpr PacketKind kind = PacketKind::connect;
        size_t size() const { return sizeof(ConnectPacket); }

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
        size_t size() const { return sizeof(ConnectResponsePacket); }

        ConnectionResult result;
        char message[256]{};
    };

    struct RequestStatePacket
    {
        static constexpr PacketKind kind = PacketKind::requestState;
        size_t size() const { return sizeof(RequestStatePacket); }

        uint32_t cookie{};
    };

    struct RequestStateResponse
    {
        static constexpr PacketKind kind = PacketKind::requestStateResponse;
        size_t size() const { return sizeof(RequestStateResponse); }

        uint32_t cookie{};
        uint32_t totalSize{};
        uint16_t numChunks{};
    };

    struct RequestStateResponseChunk
    {
        static constexpr PacketKind kind = PacketKind::requestStateResponseChunk;
        size_t size() const { return reinterpret_cast<size_t>(this->data + dataSize) - reinterpret_cast<size_t>(this); }

        uint32_t cookie{};
        uint16_t index{};
        uint32_t offset{};
        uint32_t dataSize{};
        uint8_t data[maxPacketDataSize - 14]{};
    };
    static_assert(sizeof(RequestStateResponseChunk) == maxPacketDataSize);

    /**
     * Extra state on top of S5 that we want to send over network
     */
    struct ExtraState
    {
        uint32_t gameCommandIndex{};
        uint32_t tick;
    };

    struct SendChatMessage
    {
        static constexpr PacketKind kind = PacketKind::sendChatMessage;
        size_t size() const { return reinterpret_cast<size_t>(this->text + length) - reinterpret_cast<size_t>(this); }

        uint16_t length{};
        char text[2048]{};

        std::string_view getText() const
        {
            return std::string_view(text, length);
        }
    };
    static_assert(sizeof(SendChatMessage) <= maxPacketDataSize);

    struct ReceiveChatMessage
    {
        static constexpr PacketKind kind = PacketKind::receiveChatMessage;
        size_t size() const { return reinterpret_cast<size_t>(this->text + length) - reinterpret_cast<size_t>(this); }

        client_id_t sender{};
        uint16_t length{};
        char text[2048]{};

        std::string_view getText() const
        {
            return std::string_view(text, length);
        }
    };
    static_assert(sizeof(SendChatMessage) <= maxPacketDataSize);

    struct GameCommandPacket
    {
        static constexpr PacketKind kind = PacketKind::gameCommand;
        size_t size() const { return sizeof(GameCommandPacket); }

        uint32_t index{};
        uint32_t tick{};
        OpenLoco::Interop::registers regs;
    };
#pragma pack(pop)

    void openServer();
    void joinServer(std::string_view host);
    void joinServer(std::string_view host, port_t port);
    void close();
    void update();

    void sendChatMessage(std::string_view message);
    void receiveChatMessage(client_id_t client, std::string_view message);

    void queueGameCommand(OpenLoco::Interop::registers regs);
    bool shouldProcessTick(uint32_t tick);
    void processGameCommands(uint32_t tick);

    /**
     * Whether the game state is networked.
     * This will return false if the client is still receiving the map from the server.
     */
    bool isConnected();
}
