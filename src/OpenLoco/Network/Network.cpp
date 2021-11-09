#include "Network.h"
#include "../Console.h"
#include "../Graphics/Gfx.h"
#include "../OpenLoco.h"
#include "NetworkClient.h"
#include "NetworkServer.h"
#include "Socket.h"
#include <cassert>
#include <stdexcept>

namespace OpenLoco::Network
{
    enum class NetworkMode
    {
        none,
        server,
        client
    };

    static NetworkMode _mode;
    static std::unique_ptr<NetworkServer> _server;
    static std::unique_ptr<NetworkClient> _client;

    static NetworkBase* getServerOrClient()
    {
        switch (_mode)
        {
            case NetworkMode::server:
                return _server.get();
            case NetworkMode::client:
                return _client.get();
            default:
                return nullptr;
        }
    }

    void openServer()
    {
        assert(_mode == NetworkMode::none);

        try
        {
            _server = std::make_unique<NetworkServer>();
            _server->listen(defaultPort);

            _mode = NetworkMode::server;
            setScreenFlag(ScreenFlags::networked);
            setScreenFlag(ScreenFlags::networkHost);
            Gfx::invalidateScreen();
        }
        catch (...)
        {
            _server = nullptr;
            throw;
        }
    }

    void joinServer(std::string_view host)
    {
        joinServer(host, defaultPort);
    }

    void joinServer(std::string_view host, port_t port)
    {
        assert(_mode == NetworkMode::none);

        try
        {
            _client = std::make_unique<NetworkClient>();
            _client->connect(host, port);
            _mode = NetworkMode::client;
        }
        catch (...)
        {
            _client = nullptr;
            throw;
        }
    }

    void close()
    {
        _server = nullptr;
        _client = nullptr;
        _mode = NetworkMode::none;
    }

    void update()
    {
        auto serverOrClient = getServerOrClient();
        if (serverOrClient != nullptr)
        {
            serverOrClient->update();
            if (serverOrClient->isClosed())
            {
                close();
            }
        }
    }

    void sendChatMessage(std::string_view message)
    {
        auto serverOrClient = getServerOrClient();
        if (serverOrClient != nullptr)
        {
            serverOrClient->sendChatMessage(message);
        }
    }

    void receiveChatMessage(client_id_t client, std::string_view message)
    {
        std::string szMessage(message);
        Console::log("Player #%d: %s", static_cast<int>(client), szMessage.c_str());
    }
}
