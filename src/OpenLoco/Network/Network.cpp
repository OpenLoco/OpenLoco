#include "Network.h"
#include "../Console.h"
#include "../OpenLoco.h"
#include "../Utility/String.hpp"
#include "Socket.h"
#include <cassert>
#include <stdexcept>
#include <thread>

namespace OpenLoco::Network
{
    constexpr uint16_t defaultPort = 11754;
    constexpr uint16_t maxPacketSize = 4096;
    constexpr uint16_t networkVersion = 1;

    typedef uint32_t client_id_t;

    enum class NetworkMode
    {
        none,
        server,
        client
    };

    struct Client
    {
        client_id_t id{};
        std::unique_ptr<INetworkEndpoint> endpoint;
        std::string name;
    };

    enum class PacketKind : uint16_t
    {
        connect = 1,
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
        const T* As()
        {
            if (header.kind == TKind && header.dataSize >= sizeof(T))
            {
                return reinterpret_cast<T*>(data);
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

    static std::unique_ptr<IUdpSocket> _socket;
    static std::vector<std::unique_ptr<Client>> _clients;
    static std::unique_ptr<INetworkEndpoint> _serverEndpoint;
    static std::thread _recievePacketThread;
    static bool _endRecievePacketLoop;
    static NetworkMode _mode;
    static client_id_t _nextClientId = 1;

    static Client* findClient(const INetworkEndpoint& endpoint)
    {
        for (auto& client : _clients)
        {
            if (client->endpoint->equals(endpoint))
            {
                return client.get();
            }
        }
        return nullptr;
    }

    template<PacketKind TKind, typename T>
    static void sendPacket(const INetworkEndpoint& endpoint, const T& packetData)
    {
        static_assert(sizeof(packetData) <= maxPacketDataSize, "Packet too large.");

        Packet packet;
        packet.header.kind = TKind;
        packet.header.sequence = 0;
        packet.header.dataSize = sizeof(packetData);
        std::memcpy(packet.data, &packetData, packet.header.dataSize);

        size_t packetSize = sizeof(PacketHeader) + packet.header.dataSize;
        _socket->SendData(endpoint, &packet, packetSize);
    }

    template<PacketKind TKind, typename T>
    static void sendPacket(const T& packetData)
    {
        sendPacket<TKind, T>(*_serverEndpoint, packetData);
    }

    static void onRecievePacketFromClient(Client& client, const Packet& packet)
    {
    }

    static void onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, Packet& packet)
    {
        auto client = findClient(*endpoint);
        if (client == nullptr)
        {
            auto connectPacket = packet.As<PacketKind::connect, ConnectPacket>();
            if (connectPacket != nullptr)
            {
                auto newClient = std::make_unique<Client>();
                newClient->id = _nextClientId++;
                newClient->endpoint = std::move(endpoint);
                newClient->name = Utility::nullTerminatedView(connectPacket->name);
                _clients.push_back(std::move(newClient));

                ConnectResponsePacket response;
                response.result = ConnectionResult::success;
                sendPacket<PacketKind::connect>(*newClient->endpoint, response);
            }
        }
        else
        {
            onRecievePacketFromClient(*client, packet);
        }
    }

    static void recievePacketLoop()
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
                    onRecievePacket(std::move(endpoint), packet);
                }
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    static void beginRecievePacketLoop()
    {
        _endRecievePacketLoop = false;
        _recievePacketThread = std::thread(recievePacketLoop);
    }

    static void endRecievePacketLoop()
    {
        _endRecievePacketLoop = true;
        if (_recievePacketThread.joinable())
        {
            _recievePacketThread.join();
        }
        _recievePacketThread = {};
    }

    void openServer()
    {
        assert(_mode == NetworkMode::none);

        Console::log("Server opened");
        Console::log("Listening for incoming connections...");
        setScreenFlag(ScreenFlags::networked);
        setScreenFlag(ScreenFlags::networkHost);

        _socket = Socket::createUdp();
    }

    void closeServer()
    {
        assert(_mode == NetworkMode::server);

        endRecievePacketLoop();
        _socket = nullptr;

        clearScreenFlag(ScreenFlags::networked);
        clearScreenFlag(ScreenFlags::networkHost);

        Console::log("Server closed");
    }

    void connect(std::string_view host)
    {
        connect(host, defaultPort);
    }

    void connect(std::string_view host, port_t port)
    {
        assert(_mode == NetworkMode::none);

        try
        {
            auto szHost = std::string(host);
            _socket = Socket::createUdp();
            _serverEndpoint = Socket::resolve(szHost, port);

            setScreenFlag(ScreenFlags::networked);

            Console::log("Resolved endpoint for %s:%d", szHost.c_str(), defaultPort);

            beginRecievePacketLoop();

            ConnectPacket packet;
            std::strncpy(packet.name, "Ted", sizeof(packet.name));
            packet.version = networkVersion;
            sendPacket<PacketKind::connect>(packet);
        }
        catch (...)
        {
            _socket = nullptr;
            throw;
        }
    }

    void disconnect()
    {
        assert(_mode == NetworkMode::client);

        endRecievePacketLoop();
        _socket = nullptr;
        _serverEndpoint = nullptr;

        clearScreenFlag(ScreenFlags::networked);
        clearScreenFlag(ScreenFlags::networkHost);

        Console::log("Disconnected from server");
    }
}
