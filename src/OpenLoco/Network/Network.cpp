#include "Network.h"
#include "../Console.h"
#include "../OpenLoco.h"
#include "../Platform/Platform.h"
#include "../Ui/WindowManager.h"
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

    enum class NetworkStatus
    {
        none,
        connecting,
        connectedSuccessfully,
        hosting,
    };

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

    static std::unique_ptr<IUdpSocket> _socket;
    static std::vector<std::unique_ptr<Client>> _clients;
    static std::unique_ptr<INetworkEndpoint> _serverEndpoint;
    static std::thread _recievePacketThread;
    static bool _endRecievePacketLoop;
    static NetworkMode _mode;
    static NetworkStatus _status;
    static uint32_t _timeout;
    static client_id_t _nextClientId = 1;

    static void setStatus(std::string_view text);

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

    static void onRecievePacketFromServer(const Packet& packet)
    {
        auto connectResponsePacket = packet.As<PacketKind::connectResponse, ConnectResponsePacket>();
        if (connectResponsePacket->result == ConnectionResult::success)
        {
            _status = NetworkStatus::connectedSuccessfully;
            setStatus("Connected to server successfully");
        }
        else
        {
            disconnect();
        }
    }

    static void onRecievePacketFromClient(Client& client, const Packet& packet)
    {
    }

    static void onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
    {
        if (_mode == NetworkMode::server)
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

                    auto& newClientPtr = *_clients.back();

                    ConnectResponsePacket response;
                    response.result = ConnectionResult::success;
                    sendPacket<PacketKind::connectResponse>(*newClientPtr.endpoint, response);

                    Console::log("Accepted new client: %s", newClientPtr.name.c_str());
                }
            }
            else
            {
                onRecievePacketFromClient(*client, packet);
            }
        }
        else if (_mode == NetworkMode::client)
        {
            // TODO do we really need the check, it is possible but unlikely
            //      for something else to hijack the UDP client port
            if (endpoint->equals(*_serverEndpoint))
            {
                onRecievePacketFromServer(packet);
            }
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

        _socket = Socket::createUdp();
        _socket->Listen(defaultPort);

        _mode = NetworkMode::server;

        setScreenFlag(ScreenFlags::networked);
        setScreenFlag(ScreenFlags::networkHost);

        Console::log("Server opened");
        Console::log("Listening for incoming connections...");

        beginRecievePacketLoop();
    }

    void close()
    {
        _status = NetworkStatus::none;
        endRecievePacketLoop();
        _socket = nullptr;
        _serverEndpoint = nullptr;

        clearScreenFlag(ScreenFlags::networked);
        clearScreenFlag(ScreenFlags::networkHost);
    }

    void closeServer()
    {
        assert(_mode == NetworkMode::server);
        close();
        Console::log("Server closed");
    }

    static void onCancel()
    {
        switch (_status)
        {
            case NetworkStatus::connecting:
                close();
                break;
        }
    }

    static void initStatus(std::string_view text)
    {
        Ui::Windows::NetworkStatus::open(text, onCancel);
    }

    static void setStatus(std::string_view text)
    {
        Ui::Windows::NetworkStatus::setText(text);
    }

    static void sendConnectPacket()
    {
        ConnectPacket packet;
        std::strncpy(packet.name, "Ted", sizeof(packet.name));
        packet.version = networkVersion;
        sendPacket<PacketKind::connect>(packet);
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

            Console::log("Resolved endpoint for %s:%d", szHost.c_str(), defaultPort);

            beginRecievePacketLoop();

            _mode = NetworkMode::client;
            _status = NetworkStatus::connecting;
            _timeout = Platform::getTime() + 5000;

            sendConnectPacket();

            initStatus("Connecting to " + szHost);
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
        close();
        Console::log("Disconnected from server");
    }

    void update()
    {
        switch (_status)
        {
            case NetworkStatus::connecting:
                if (Platform::getTime() >= _timeout)
                {
                    close();
                    Console::log("Failed to connect to server");
                    setStatus("Failed to connect to server");
                }
                break;
            case NetworkStatus::connectedSuccessfully:
                break;
        }
    }
}
