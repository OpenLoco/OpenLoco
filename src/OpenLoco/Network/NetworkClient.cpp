#include "NetworkClient.h"
#include "../Console.h"
#include "../Platform/Platform.h"
#include "../Ui/WindowManager.h"
#include "NetworkConnection.h"

using namespace OpenLoco::Network;

void NetworkClient::connect(std::string_view host, port_t port)
{
    auto szHost = std::string(host);

    _serverEndpoint = Socket::resolve(szHost, port);
    _serverConnection = std::make_unique<NetworkConnection>(_socket.get(), _serverEndpoint->clone());

    Console::log("Resolved endpoint for %s:%d", szHost.c_str(), defaultPort);

    beginReceivePacketLoop();

    _status = NetworkClientStatus::connecting;
    _timeout = Platform::getTime() + 5000;

    sendConnectPacket();

    initStatus("Connecting to " + szHost + "...");
}

void NetworkClient::onClose()
{
    _serverConnection = nullptr;
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
    processReceivedPackets();

    if (hasTimedOut())
    {
        Console::log("Connection with server timed out");
        close();
        return;
    }

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
            sendRequestStatePacket();
            _status = NetworkClientStatus::waitingForState;
            break;
        case NetworkClientStatus::waitingForState:
            break;
    }
}

void NetworkClient::processReceivedPackets()
{
    if (_serverConnection != nullptr)
    {
        while (auto packet = _serverConnection->takeNextPacket())
        {
            onReceivePacketFromServer(*packet);
        }
        _serverConnection->update();
    }
}

bool NetworkClient::hasTimedOut() const
{
    if (_serverConnection != nullptr)
    {
        return _serverConnection->hasTimedOut();
    }
    return false;
}

void NetworkClient::onReceivePacket(std::unique_ptr<INetworkEndpoint> endpoint, const Packet& packet)
{
    // TODO do we really need the check, it is possible but unlikely
    //      for something else to hijack the UDP client port
    if (_serverEndpoint != nullptr && endpoint->equals(*_serverEndpoint))
    {
        _serverConnection->receivePacket(packet);
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

void NetworkClient::onReceivePacketFromServer(const Packet& packet)
{
    switch (packet.header.kind)
    {
        case PacketKind::connectResponse:
            receiveConnectionResponsePacket(*reinterpret_cast<const ConnectResponsePacket*>(packet.data));
            break;
        case PacketKind::requestStateResponse:
            receiveRequestStateResponsePacket(*reinterpret_cast<const RequestStateResponse*>(packet.data));
            break;
        case PacketKind::requestStateResponseChunk:
            receiveRequestStateResponseChunkPacket(*reinterpret_cast<const RequestStateResponseChunk*>(packet.data));
            break;
    }
}

void NetworkClient::sendConnectPacket()
{
    ConnectPacket packet;
    std::strncpy(packet.name, "Ted", sizeof(packet.name));
    packet.version = networkVersion;
    _serverConnection->sendPacket(packet);
}

void NetworkClient::sendRequestStatePacket()
{
    _requestStateCookie = (std::rand() << 16) | std::rand();
    _requestStateChunksReceived.clear();

    RequestStatePacket packet;
    packet.cookie = _requestStateCookie;
    _serverConnection->sendPacket(packet);
}

void NetworkClient::receiveConnectionResponsePacket(const ConnectResponsePacket& response)
{
    if (response.result == ConnectionResult::success)
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

void NetworkClient::receiveRequestStateResponsePacket(const RequestStateResponse& response)
{
    if (response.cookie == _requestStateCookie)
    {
        _requestStateNumChunks = response.numChunks;
        _requestStateTotalSize = response.totalSize;
        _requestStateReceivedChunks = 0;
    }
}

void NetworkClient::receiveRequestStateResponseChunkPacket(const RequestStateResponseChunk& responseChunk)
{
    if (responseChunk.cookie == _requestStateCookie)
    {
        if (_requestStateChunksReceived.size() <= responseChunk.index)
        {
            _requestStateChunksReceived.resize(responseChunk.index + 1);
        }

        auto& rchunk = _requestStateChunksReceived[responseChunk.index];
        if (rchunk.data.size() == 0)
        {
            rchunk.offset = responseChunk.offset;
            rchunk.data.assign(responseChunk.data, responseChunk.data + responseChunk.size);
            _requestStateReceivedChunks++;

            _requestStateReceivedBytes += responseChunk.size;
            setStatus("Receiving state: " + std::to_string(_requestStateReceivedBytes) + " / " + std::to_string(_requestStateTotalSize));
        }

        if (_requestStateReceivedChunks >= _requestStateNumChunks)
        {
            // Construct full data
            std::vector<uint8_t> fullData;
            for (size_t i = 0; i < _requestStateChunksReceived.size(); i++)
            {
                fullData.insert(fullData.end(), _requestStateChunksReceived[i].data.begin(), _requestStateChunksReceived[i].data.end());
            }

            setStatus("Receiving state complete");
        }
    }
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
