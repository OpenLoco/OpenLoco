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
    using client_id_t = uint32_t;
    using port_t = uint16_t;

    constexpr port_t kDefaultPort = 11754;
    constexpr uint16_t kMaxPacketSize = 4096;
    constexpr uint16_t kNetworkVersion = 1;

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
