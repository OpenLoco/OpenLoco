#pragma once

#include <memory>
#include <string>
#include <vector>

namespace OpenLoco::Network
{
    enum class SocketStatus
    {
        closed,
        waiting,
        resolving,
        connecting,
        connected,
        listening,
    };

    enum class NetworkReadPacket : int32_t
    {
        success,
        noData,
        moreData,
        disconnected
    };

    /**
     * Represents an address and port.
     */
    struct INetworkEndpoint
    {
        virtual ~INetworkEndpoint()
        {
        }

        virtual std::string getHostname() const = 0;
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

        virtual SocketStatus getStatus() const = 0;
        virtual const char* getError() const = 0;
        virtual const char* getHostName() const = 0;

        virtual void listen(uint16_t port) = 0;
        virtual void listen(const std::string& address, uint16_t port) = 0;

        virtual size_t sendData(const std::string& address, uint16_t port, const void* buffer, size_t size) = 0;
        virtual size_t sendData(const INetworkEndpoint& destination, const void* buffer, size_t size) = 0;
        virtual NetworkReadPacket receiveData(
            void* buffer, size_t size, size_t* sizeReceived, std::unique_ptr<INetworkEndpoint>* sender)
            = 0;

        virtual void close() = 0;
    };

    namespace Socket
    {
        [[nodiscard]] std::unique_ptr<IUdpSocket> createUdp();
        std::unique_ptr<INetworkEndpoint> resolve(const std::string& address, uint16_t port);
    }
}
