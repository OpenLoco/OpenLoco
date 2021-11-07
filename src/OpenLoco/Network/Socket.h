#pragma once

#include <memory>
#include <string>
#include <vector>

namespace OpenLoco::Network
{
    enum class SocketStatus
    {
        Closed,
        Waiting,
        Resolving,
        Connecting,
        Connected,
        Listening,
    };

    enum class NetworkReadPacket : int32_t
    {
        Success,
        NoData,
        MoreData,
        Disconnected
    };

    /**
     * Represents an address and port.
     */
    struct INetworkEndpoint
    {
        virtual ~INetworkEndpoint()
        {
        }

        virtual std::string GetHostname() const = 0;
        virtual std::unique_ptr<INetworkEndpoint> clone() const = 0;
        virtual bool equals(const INetworkEndpoint& other) const = 0;
    };

    /**
     * Represents a UDP socket / listener.
     */
    struct IUdpSocket
    {
    public:
        virtual ~IUdpSocket() = default;

        virtual SocketStatus GetStatus() const = 0;
        virtual const char* GetError() const = 0;
        virtual const char* GetHostName() const = 0;

        virtual void Listen(uint16_t port) = 0;
        virtual void Listen(const std::string& address, uint16_t port) = 0;

        virtual size_t SendData(const std::string& address, uint16_t port, const void* buffer, size_t size) = 0;
        virtual size_t SendData(const INetworkEndpoint& destination, const void* buffer, size_t size) = 0;
        virtual NetworkReadPacket ReceiveData(
            void* buffer, size_t size, size_t* sizeReceived, std::unique_ptr<INetworkEndpoint>* sender)
            = 0;

        virtual void Close() = 0;
    };

    namespace Socket
    {
        [[nodiscard]] std::unique_ptr<IUdpSocket> createUdp();
        std::unique_ptr<INetworkEndpoint> resolve(const std::string& address, uint16_t port);
    }
}
