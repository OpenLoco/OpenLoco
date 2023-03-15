#include "Network.h"
#include "CommandLine.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/Gfx.h"
#include "NetworkClient.h"
#include "NetworkServer.h"
#include "ScenarioManager.h"
#include "SceneManager.h"
#include "Socket.h"
#include <OpenLoco/Console/Console.h>
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
            const auto& cmdlineOptions = getCommandLineOptions();
            auto& bind = cmdlineOptions.bind;
            auto port = cmdlineOptions.port.value_or(kDefaultPort);

            _server = std::make_unique<NetworkServer>();
            _server->listen(bind, port);

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
        joinServer(host, kDefaultPort);
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
        Diagnostics::logDeprecated("Player #%d: %s", static_cast<int>(client), szMessage.c_str());
    }

    void queueGameCommand(CompanyId company, const OpenLoco::Interop::registers& regs)
    {
        // TEMP debug code
        if (regs.esi == 73)
            return;

        if (_mode == NetworkMode::server)
        {
            _server->queueGameCommand(company, regs);
        }
        else
        {
            _client->sendGameCommand(company, regs);
        }
    }

    bool shouldProcessTick(uint32_t tick)
    {
        if (_mode == NetworkMode::client)
        {
            return _client->shouldProcessTick(tick);
        }
        else
        {
            return true;
        }
    }

    void processGameCommands(uint32_t tick)
    {
        switch (_mode)
        {
            case NetworkMode::none:
                break;
            case NetworkMode::server:
                _server->runGameCommands();
                break;
            case NetworkMode::client:
                _client->runGameCommandsForTick(tick);
                break;
        }
    }

    bool isConnected()
    {
        switch (_mode)
        {
            default:
            case NetworkMode::none:
                return false;
            case NetworkMode::server:
                return true;
            case NetworkMode::client:
                return _client->getStatus() == NetworkClientStatus::connected;
        }
    }

    uint32_t getServerTick()
    {
        if (_mode == NetworkMode::client)
        {
            return _client->getLocalTick();
        }
        return ScenarioManager::getScenarioTicks();
    }
}
