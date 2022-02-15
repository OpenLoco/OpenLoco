#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <future>
#include <string>
#include <thread>

// clang-format off
// MSVC: include <math.h> here otherwise PI gets defined twice
#include <cmath>

#ifdef _WIN32
    #pragma comment(lib, "Ws2_32.lib")

    // winsock2 must be included before windows.h
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #define LAST_SOCKET_ERROR() WSAGetLastError()
    #undef EWOULDBLOCK
    #define EWOULDBLOCK WSAEWOULDBLOCK
    #ifndef SHUT_RD
        #define SHUT_RD SD_RECEIVE
    #endif
    #ifndef SHUT_WR
        #define SHUT_WR SD_SEND
    #endif
    #ifndef SHUT_RDWR
        #define SHUT_RDWR SD_BOTH
    #endif
    #define FLAG_NO_PIPE 0
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <net/if.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <unistd.h>
    using SOCKET = int32_t;
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
    #define LAST_SOCKET_ERROR() errno
    #define closesocket ::close
    #define ioctlsocket ioctl
    #if defined(__linux__)
        #define FLAG_NO_PIPE MSG_NOSIGNAL
    #else
        #define FLAG_NO_PIPE 0
    #endif // defined(__linux__)

#endif // _WIN32
// clang-format on

#include "../Console.h"
#include "Socket.h"

namespace OpenLoco::Network
{
    constexpr auto CONNECT_TIMEOUT = std::chrono::milliseconds(3000);

// RAII WSA initialisation needed for Windows
#ifdef _WIN32
    class WSA
    {
    private:
        bool _isInitialised{};

    public:
        bool isInitialised() const
        {
            return _isInitialised;
        }

        bool initialise()
        {
            if (!_isInitialised)
            {
                Console::logVerbose("WSAStartup()");
                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
                {
                    Console::error("Unable to initialise winsock.");
                    return false;
                }
                _isInitialised = true;
            }
            return true;
        }

        ~WSA()
        {
            if (_isInitialised)
            {
                Console::logVerbose("WSACleanup()");
                WSACleanup();
                _isInitialised = false;
            }
        }
    };

    static bool InitialiseWSA()
    {
        static WSA wsa;
        return wsa.initialise();
    }
#else
    static bool InitialiseWSA()
    {
        return true;
    }
#endif

    class SocketException : public std::runtime_error
    {
    public:
        explicit SocketException(const std::string& message)
            : std::runtime_error(message)
        {
        }
    };

    class NetworkEndpoint final : public INetworkEndpoint
    {
    private:
        union
        {
            sockaddr _address;
            sockaddr_in _address4;
            sockaddr_in6 _address6;
        };
        socklen_t _addressLen{};

    public:
        NetworkEndpoint()
        {
        }

        NetworkEndpoint(const sockaddr* address, socklen_t addressLen)
        {
            std::memcpy(&_address, address, addressLen);
            _addressLen = addressLen;
        }

        NetworkEndpoint(const sockaddr_storage* address, socklen_t addressLen)
            : NetworkEndpoint(reinterpret_cast<const sockaddr*>(address), addressLen)
        {
        }

        const sockaddr& getAddress() const
        {
            return _address;
        }

        socklen_t getAddressLen() const
        {
            return _addressLen;
        }

        int32_t getPort() const
        {
            return _address.sa_family == AF_INET6 ? _address6.sin6_port : _address4.sin_port;
        }

        Protocol getProtocol() const override
        {
            switch (_address.sa_family)
            {
                case AF_INET:
                    return Protocol::ipv4;
                case AF_INET6:
                    return Protocol::ipv6;
                default:
                    throw std::invalid_argument("Unknown protocol");
            }
        }

        std::string getIpAddress() const override
        {
            char buffer[256]{};
            switch (_address.sa_family)
            {
                case AF_INET:
                    inet_ntop(AF_INET, &_address4.sin_addr, buffer, sizeof(buffer));
                    break;
                case AF_INET6:
                    inet_ntop(AF_INET6, &_address6.sin6_addr, buffer, sizeof(buffer));
                    break;
            }
            return buffer;
        }

        std::string getHostname() const override
        {
            char hostname[256]{};
            int res = getnameinfo(&_address, _addressLen, hostname, sizeof(hostname), nullptr, 0, NI_NUMERICHOST);
            if (res == 0)
            {
                return hostname;
            }
            return {};
        }

        std::unique_ptr<INetworkEndpoint> clone() const override
        {
            return std::make_unique<NetworkEndpoint>(*this);
        }

        bool equals(const INetworkEndpoint& other) const override
        {
            auto& other2 = *static_cast<const NetworkEndpoint*>(&other);
            return compareSockets(&_address, &other2._address) == 0;
        }

        static int compareSockets(const sockaddr* x, const sockaddr* y)
        {
#define CMP(a, b) \
    if (a != b)   \
    return a < b ? -1 : 1

            CMP(x->sa_family, y->sa_family);

            if (x->sa_family == AF_INET)
            {
                const sockaddr_in *xin = (const sockaddr_in*)x, *yin = (const sockaddr_in*)y;
                CMP(ntohl(xin->sin_addr.s_addr), ntohl(yin->sin_addr.s_addr));
                CMP(ntohs(xin->sin_port), ntohs(yin->sin_port));
            }
            else if (x->sa_family == AF_INET6)
            {
                const sockaddr_in6 *xin6 = (const sockaddr_in6*)x, *yin6 = (const sockaddr_in6*)y;
                int r = memcmp(xin6->sin6_addr.s6_addr, yin6->sin6_addr.s6_addr, sizeof(xin6->sin6_addr.s6_addr));
                if (r != 0)
                    return r;
                CMP(ntohs(xin6->sin6_port), ntohs(yin6->sin6_port));
                CMP(xin6->sin6_flowinfo, yin6->sin6_flowinfo);
                CMP(xin6->sin6_scope_id, yin6->sin6_scope_id);
            }
            else
            {
                return -1;
            }

#undef CMP
            return 0;
        }
    };

    class BaseSocket
    {
    protected:
        static bool setNonBlocking(SOCKET socket, bool on)
        {
#ifdef _WIN32
            u_long nonBlocking = on;
            return ioctlsocket(socket, FIONBIO, &nonBlocking) == 0;
#else
            int32_t flags = fcntl(socket, F_GETFL, 0);
            return fcntl(socket, F_SETFL, on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == 0;
#endif
        }

        static bool setOption(SOCKET socket, int32_t a, int32_t b, bool value)
        {
            int32_t ivalue = value ? 1 : 0;
            return setsockopt(socket, a, b, reinterpret_cast<const char*>(&ivalue), sizeof(ivalue)) == 0;
        }

        static int protocolToFamily(Protocol protocol)
        {
            switch (protocol)
            {
                case Protocol::ipv4:
                    return AF_INET;
                case Protocol::ipv6:
                    return AF_INET6;
                case Protocol::any:
                    return AF_UNSPEC;
                default:
                    throw std::invalid_argument("Invalid protocol");
            }
        }

    public:
        static void resolveAddress(
            Protocol protocol, const std::string& address, uint16_t port, sockaddr_storage* ss, socklen_t* ss_len)
        {
            std::string serviceName = std::to_string(port);

            addrinfo hints = {};
            hints.ai_family = protocolToFamily(protocol);
            if (address.empty())
            {
                hints.ai_flags = AI_PASSIVE;
            }

            addrinfo* result = nullptr;
            int errorcode = getaddrinfo(address.empty() ? nullptr : address.c_str(), serviceName.c_str(), &hints, &result);
            if (errorcode != 0)
            {
                auto msg = gai_strerror(errorcode);
                throw SocketException("Unable to resolve address: " + std::to_string(errorcode) + "  " + msg);
            }

            if (result == nullptr)
            {
                throw SocketException("Unable to resolve address");
            }

            std::memcpy(ss, result->ai_addr, result->ai_addrlen);
            *ss_len = static_cast<socklen_t>(result->ai_addrlen);
            freeaddrinfo(result);
        }
    };

    class UdpSocket final : public IUdpSocket, protected BaseSocket
    {
    private:
        SocketStatus _status = SocketStatus::closed;
        sockaddr_storage _listeningAddress{};
        socklen_t _listeningAddressLen;
        uint16_t _listeningPort = 0;
        SOCKET _socket = INVALID_SOCKET;
        NetworkEndpoint _endpoint;

        std::string _hostName;
        std::string _error;

    public:
        UdpSocket() = default;

        ~UdpSocket() override
        {
            closeSocket();
        }

        SocketStatus getStatus() const override
        {
            return _status;
        }

        const char* getError() const override
        {
            return _error.empty() ? nullptr : _error.c_str();
        }

        void listen(Protocol protocol, uint16_t port) override
        {
            listen(protocol, "", port);
        }

        void listen(Protocol protocol, const std::string& address, uint16_t port)
        {
            if (_status != SocketStatus::closed)
            {
                throw std::runtime_error("Socket not closed.");
            }

            sockaddr_storage ss{};
            socklen_t ss_len;
            resolveAddress(protocol, address, port, &ss, &ss_len);

            // Create the listening socket
            _socket = createSocket(protocol);
            try
            {
                // Bind to address:port and listen
                if (bind(_socket, reinterpret_cast<sockaddr*>(&ss), ss_len) != 0)
                {
                    throw SocketException("Unable to bind to socket.");
                }
            }
            catch (const std::exception&)
            {
                closeSocket();
                throw;
            }

            _listeningAddress = ss;
            _listeningAddressLen = ss_len;
            _listeningPort = port;
            _status = SocketStatus::listening;
        }

        size_t sendData(Protocol protocol, const std::string& address, uint16_t port, const void* buffer, size_t size) override
        {
            sockaddr_storage ss{};
            socklen_t ss_len;
            resolveAddress(protocol, address, port, &ss, &ss_len);
            NetworkEndpoint endpoint(reinterpret_cast<const sockaddr*>(&ss), ss_len);
            return sendData(endpoint, buffer, size);
        }

        size_t sendData(const INetworkEndpoint& destination, const void* buffer, size_t size) override
        {
            if (_socket == INVALID_SOCKET)
            {
                _socket = createSocket(destination.getProtocol());
            }

            const auto& dest = dynamic_cast<const NetworkEndpoint*>(&destination);
            if (dest == nullptr)
            {
                throw std::invalid_argument("destination is not compatible.");
            }
            auto ss = &dest->getAddress();
            auto ss_len = dest->getAddressLen();

            if (_status != SocketStatus::listening)
            {
                _endpoint = *dest;
            }

            size_t totalSent = 0;
            do
            {
                const char* bufferStart = static_cast<const char*>(buffer) + totalSent;
                size_t remainingSize = size - totalSent;
                int32_t sentBytes = sendto(
                    _socket, bufferStart, static_cast<int32_t>(remainingSize), FLAG_NO_PIPE, ss, ss_len);
                if (sentBytes == SOCKET_ERROR)
                {
                    return totalSent;
                }
                totalSent += sentBytes;
            } while (totalSent < size);
            return totalSent;
        }

        NetworkReadPacket receiveData(
            void* buffer, size_t size, size_t* sizeReceived, std::unique_ptr<INetworkEndpoint>* sender) override
        {
            sockaddr_in6 senderAddr{};
            socklen_t senderAddrLen = sizeof(sockaddr_in);
            if (_status != SocketStatus::listening)
            {
                senderAddrLen = _endpoint.getAddressLen();
                std::memcpy(&senderAddr, &_endpoint.getAddress(), senderAddrLen);
            }
            auto readBytes = recvfrom(
                _socket, static_cast<char*>(buffer), static_cast<int32_t>(size), 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
            if (readBytes <= 0)
            {
                *sizeReceived = 0;
                return NetworkReadPacket::noData;
            }

            *sizeReceived = readBytes;
            if (sender != nullptr)
            {
                *sender = std::make_unique<NetworkEndpoint>(reinterpret_cast<sockaddr*>(&senderAddr), senderAddrLen);
            }
            return NetworkReadPacket::success;
        }

        void close() override
        {
            closeSocket();
        }

        const char* getHostName() const override
        {
            return _hostName.empty() ? nullptr : _hostName.c_str();
        }

        Protocol getProtocol() const override
        {
            NetworkEndpoint endpoint(&_listeningAddress, _listeningAddressLen);
            return endpoint.getProtocol();
        }

        std::string getIpAddress() const override
        {
            NetworkEndpoint endpoint(&_listeningAddress, _listeningAddressLen);
            return endpoint.getIpAddress();
        }

    private:
        explicit UdpSocket(SOCKET socket, const std::string& hostName)
            : _status(SocketStatus::connected)
            , _socket(socket)
            , _hostName(hostName)
        {
        }

        SOCKET createSocket(Protocol protocol)
        {
            auto family = protocolToFamily(protocol);
            if (protocol == Protocol::any)
            {
                family = AF_INET6;
            }

            auto sock = socket(family, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET)
            {
                throw SocketException("Unable to create socket.");
            }

            // Enable send and receiving of broadcast messages
            // if (!setOption(sock, SOL_SOCKET, SO_BROADCAST, true))
            // {
            //     Console::logVerbose("setsockopt(socket, SO_BROADCAST) failed: %d", LAST_SOCKET_ERROR());
            // }

            if (protocol == Protocol::any)
            {
                // Turn off IPV6_V6ONLY so we can accept both v4 and v6 connections
                // Incomming IPv4 addresses will be mapped to IPv6 addresses
                if (!setOption(sock, IPPROTO_IPV6, IPV6_V6ONLY, false))
                {
                    Console::logVerbose("setsockopt(socket, IPV6_V6ONLY) failed: %d", LAST_SOCKET_ERROR());
                }
            }
            else if (protocol == Protocol::ipv6)
            {
                // Turn on IPV6_V6ONLY so we only accept both v6 connections
                if (!setOption(sock, IPPROTO_IPV6, IPV6_V6ONLY, true))
                {
                    Console::logVerbose("setsockopt(socket, IPV6_V6ONLY) failed: %d", LAST_SOCKET_ERROR());
                }
            }

            if (!setOption(sock, SOL_SOCKET, SO_REUSEADDR, true))
            {
                Console::logVerbose("setsockopt(socket, SO_REUSEADDR) failed: %d", LAST_SOCKET_ERROR());
            }

            if (!setNonBlocking(sock, true))
            {
                throw SocketException("Failed to set non-blocking mode.");
            }

            return sock;
        }

        void closeSocket()
        {
            if (_socket != INVALID_SOCKET)
            {
                closesocket(_socket);
                _socket = INVALID_SOCKET;
            }
            _status = SocketStatus::closed;
        }
    };

    namespace Socket
    {
        std::unique_ptr<IUdpSocket> createUdp()
        {
            InitialiseWSA();
            return std::make_unique<UdpSocket>();
        }

        std::unique_ptr<INetworkEndpoint> resolve(Protocol protocol, const std::string& address, uint16_t port)
        {
            InitialiseWSA();
            sockaddr_storage ss{};
            socklen_t ss_len;
            BaseSocket::resolveAddress(protocol, address, port, &ss, &ss_len);
            return std::make_unique<NetworkEndpoint>(reinterpret_cast<const sockaddr*>(&ss), ss_len);
        }
    }
}
