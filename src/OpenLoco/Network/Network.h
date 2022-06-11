#pragma once

#include "../Types.hpp"
#include <cstdint>
#include <string_view>

namespace OpenLoco::Interop
{
    struct registers;
}

namespace OpenLoco::Network
{
    typedef uint32_t client_id_t;
    typedef uint16_t port_t;

    constexpr port_t defaultPort = 11754;
    constexpr uint16_t maxPacketSize = 4096;
    constexpr uint16_t networkVersion = 1;

    void openServer();
    void joinServer(std::string_view host);
    void joinServer(std::string_view host, port_t port);
    void close();
    void update();

    void sendChatMessage(std::string_view message);
    void receiveChatMessage(client_id_t client, std::string_view message);

    void queueGameCommand(CompanyId company, const OpenLoco::Interop::registers& regs);
    bool shouldProcessTick(uint32_t tick);
    void processGameCommands(uint32_t tick);

    /**
     * Whether the game state is networked.
     * This will return false if the client is still receiving the map from the server.
     */
    bool isConnected();

    /**
     * Gets the current tick the server is on.
     */
    uint32_t getServerTick();
}
