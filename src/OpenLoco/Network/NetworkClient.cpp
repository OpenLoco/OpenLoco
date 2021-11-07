#include "NetworkClient.h"
#include "../Console.h"
#include "../Platform/Platform.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Network;

void NetworkClient::connect(std::string_view host, port_t port)
{
    auto szHost = std::string(host);

    _serverEndpoint = Socket::resolve(szHost, port);

    Console::log("Resolved endpoint for %s:%d", szHost.c_str(), defaultPort);

    beginRecievePacketLoop();

    _status = NetworkClientStatus::connecting;
    _timeout = Platform::getTime() + 5000;

    sendConnectPacket();

    initStatus("Connecting to " + szHost + "...");
}

void NetworkClient::onClose()
{
    if (_status != NetworkClientStatus::none && _status != NetworkClientStatus::connecting)
    {
        _status = NetworkClientStatus::closed;
        clearScreenFlag(ScreenFlags::networked);
        Console::log("Disconnected from server");
    }
    else if (_status == NetworkClientStatus::connecting)
    {
        _status = NetworkClientStatus::closed;
        Console::log("Connecting to server cancelled");
    }
}

void NetworkClient::onUpdate()
{
    switch (_status)
    {
        case NetworkClientStatus::connecting:
            if (Platform::getTime() >= _timeout)
            {
                close();
                Console::log("Failed to connect to server");
                endStatus("Failed to connect to server");
            }
            break;
        case NetworkClientStatus::connectedSuccessfully:
            break;
    }
}

void NetworkClient::onRecievePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
    // TODO do we really need the check, it is possible but unlikely
    //      for something else to hijack the UDP client port
    if (_serverEndpoint != nullptr && endpoint->equals(*_serverEndpoint))
    {
        onRecievePacketFromServer(packet);
    }
}

void NetworkClient::onCancel()
{
    switch (_status)
    {
        case NetworkClientStatus::connecting:
            close();
            break;
    }
}

void NetworkClient::onRecievePacketFromServer(const Packet& packet)
{
    auto connectResponsePacket = packet.As<PacketKind::connectResponse, ConnectResponsePacket>();
    if (connectResponsePacket->result == ConnectionResult::success)
    {
        _status = NetworkClientStatus::connectedSuccessfully;
        setStatus("Connected to server successfully");
        setScreenFlag(ScreenFlags::networked);
    }
    else
    {
        close();
    }
}

void NetworkClient::sendConnectPacket()
{
    ConnectPacket packet;
    std::strncpy(packet.name, "Ted", sizeof(packet.name));
    packet.version = networkVersion;
    sendPacket<PacketKind::connect>(packet);
}

void NetworkClient::initStatus(std::string_view text)
{
    Ui::Windows::NetworkStatus::open(text, [this]() { onCancel(); });
}

void NetworkClient::setStatus(std::string_view text)
{
    Ui::Windows::NetworkStatus::setText(text);
}

void NetworkClient::endStatus(std::string_view text)
{
    Ui::Windows::NetworkStatus::setText(text, nullptr);
}

void NetworkClient::clearStatus()
{
    Ui::Windows::NetworkStatus::close();
}
