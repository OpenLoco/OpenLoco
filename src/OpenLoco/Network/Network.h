#pragma once

#include <cstdint>
#include <string_view>

namespace OpenLoco::Network
{
    typedef uint16_t port_t;

    void openServer();
    void closeServer();
    void connect(std::string_view host);
    void connect(std::string_view host, port_t port);
    void disconnect();
}
