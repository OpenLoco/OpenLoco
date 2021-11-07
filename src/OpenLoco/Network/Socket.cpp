#include <atomic>
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
    #include "../common.h"
    using SOCKET = int32_t;
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
    #define LAST_SOCKET_ERROR() errno
    #define closesocket close
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
        bool IsInitialised() const
        {
            return _isInitialised;
        }

        bool Initialise()
        {
            if (!_isInitialised)
            {
                Console::logVerbose("WSAStartup()");
                WSADATA wsa_data;
                if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
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
        return wsa.Initialise();
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
        sockaddr _address{};
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

        const sockaddr& GetAddress() const
        {
            return _address;
        }

        socklen_t GetAddressLen() const
        {
            return _addressLen;
        }

        int32_t GetPort() const
        {
            if (_address.sa_family == AF_INET)
            {
                return reinterpret_cast<const sockaddr_in*>(&_address)->sin_port;
            }

            return reinterpret_cast<const sockaddr_in6*>(&_address)->sin6_port;
        }

        std::string GetHostname() const override
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
    public:
        static bool ResolveAddress(const std::string& address, uint16_t port, sockaddr_storage* ss, socklen_t* ss_len)
        {
            return ResolveAddress(AF_UNSPEC, address, port, ss, ss_len);
        }

        static bool ResolveAddressIPv4(const std::string& address, uint16_t port, sockaddr_storage* ss, socklen_t* ss_len)
        {
            return ResolveAddress(AF_INET, address, port, ss, ss_len);
        }

    protected:
        static bool SetNonBlocking(SOCKET socket, bool on)
        {
#ifdef _WIN32
            u_long nonBlocking = on;
            return ioctlsocket(socket, FIONBIO, &nonBlocking) == 0;
#else
            int32_t flags = fcntl(socket, F_GETFL, 0);
            return fcntl(socket, F_SETFL, on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)) == 0;
#endif
        }

        static bool SetOption(SOCKET socket, int32_t a, int32_t b, bool value)
        {
            int32_t ivalue = value ? 1 : 0;
            return setsockopt(socket, a, b, reinterpret_cast<const char*>(&ivalue), sizeof(ivalue)) == 0;
        }

    private:
        static bool ResolveAddress(
            int32_t family, const std::string& address, uint16_t port, sockaddr_storage* ss, socklen_t* ss_len)
        {
            std::string serviceName = std::to_string(port);

            addrinfo hints = {};
            hints.ai_family = family;
            if (address.empty())
            {
                hints.ai_flags = AI_PASSIVE;
            }

            addrinfo* result = nullptr;
            int errorcode = getaddrinfo(address.empty() ? nullptr : address.c_str(), serviceName.c_str(), &hints, &result);
            if (errorcode != 0)
            {
                Console::error("Resolving address failed: Code %d.", errorcode);
                Console::error("Resolution error message: %s.", gai_strerror(errorcode));
                return false;
            }

            if (result == nullptr)
            {
                return false;
            }

            std::memcpy(ss, result->ai_addr, result->ai_addrlen);
            *ss_len = static_cast<socklen_t>(result->ai_addrlen);
            freeaddrinfo(result);
            return true;
        }
    };

    class UdpSocket final : public IUdpSocket, protected BaseSocket
    {
    private:
        SocketStatus _status = SocketStatus::Closed;
        uint16_t _listeningPort = 0;
        SOCKET _socket = INVALID_SOCKET;
        NetworkEndpoint _endpoint;

        std::string _hostName;
        std::string _error;

    public:
        UdpSocket() = default;

        ~UdpSocket() override
        {
            CloseSocket();
        }

        SocketStatus GetStatus() const override
        {
            return _status;
        }

        const char* GetError() const override
        {
            return _error.empty() ? nullptr : _error.c_str();
        }

        void Listen(uint16_t port) override
        {
            Listen("", port);
        }

        void Listen(const std::string& address, uint16_t port) override
        {
            if (_status != SocketStatus::Closed)
            {
                throw std::runtime_error("Socket not closed.");
            }

            sockaddr_storage ss{};
            socklen_t ss_len;
            if (!ResolveAddressIPv4(address, port, &ss, &ss_len))
            {
                throw SocketException("Unable to resolve address.");
            }

            // Create the listening socket
            _socket = CreateSocket();
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
                CloseSocket();
                throw;
            }

            _listeningPort = port;
            _status = SocketStatus::Listening;
        }

        size_t SendData(const std::string& address, uint16_t port, const void* buffer, size_t size) override
        {
            sockaddr_storage ss{};
            socklen_t ss_len;
            if (!ResolveAddressIPv4(address, port, &ss, &ss_len))
            {
                throw SocketException("Unable to resolve address.");
            }
            NetworkEndpoint endpoint(reinterpret_cast<const sockaddr*>(&ss), ss_len);
            return SendData(endpoint, buffer, size);
        }

        size_t SendData(const INetworkEndpoint& destination, const void* buffer, size_t size) override
        {
            if (_socket == INVALID_SOCKET)
            {
                _socket = CreateSocket();
            }

            const auto& dest = dynamic_cast<const NetworkEndpoint*>(&destination);
            if (dest == nullptr)
            {
                throw std::invalid_argument("destination is not compatible.");
            }
            auto ss = &dest->GetAddress();
            auto ss_len = dest->GetAddressLen();

            if (_status != SocketStatus::Listening)
            {
                _endpoint = *dest;
            }

            size_t totalSent = 0;
            do
            {
                const char* bufferStart = static_cast<const char*>(buffer) + totalSent;
                size_t remainingSize = size - totalSent;
                int32_t sentBytes = sendto(
                    _socket, bufferStart, static_cast<int32_t>(remainingSize), FLAG_NO_PIPE, static_cast<const sockaddr*>(ss), ss_len);
                if (sentBytes == SOCKET_ERROR)
                {
                    return totalSent;
                }
                totalSent += sentBytes;
            } while (totalSent < size);
            return totalSent;
        }

        NetworkReadPacket ReceiveData(
            void* buffer, size_t size, size_t* sizeReceived, std::unique_ptr<INetworkEndpoint>* sender) override
        {
            sockaddr_in senderAddr{};
            socklen_t senderAddrLen = sizeof(sockaddr_in);
            if (_status != SocketStatus::Listening)
            {
                senderAddrLen = _endpoint.GetAddressLen();
                std::memcpy(&senderAddr, &_endpoint.GetAddress(), senderAddrLen);
            }
            auto readBytes = recvfrom(
                _socket, static_cast<char*>(buffer), static_cast<int32_t>(size), 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);
            if (readBytes <= 0)
            {
                *sizeReceived = 0;
                return NetworkReadPacket::NoData;
            }

            *sizeReceived = readBytes;
            if (sender != nullptr)
            {
                *sender = std::make_unique<NetworkEndpoint>(reinterpret_cast<sockaddr*>(&senderAddr), senderAddrLen);
            }
            return NetworkReadPacket::Success;
        }

        void Close() override
        {
            CloseSocket();
        }

        const char* GetHostName() const override
        {
            return _hostName.empty() ? nullptr : _hostName.c_str();
        }

    private:
        explicit UdpSocket(SOCKET socket, const std::string& hostName)
            : _status(SocketStatus::Connected)
            , _socket(socket)
            , _hostName(hostName)
        {
        }

        SOCKET CreateSocket()
        {
            auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET)
            {
                throw SocketException("Unable to create socket.");
            }

            // Enable send and receiving of broadcast messages
            if (!SetOption(sock, SOL_SOCKET, SO_BROADCAST, true))
            {
                Console::logVerbose("setsockopt(socket, SO_BROADCAST) failed: %d", LAST_SOCKET_ERROR());
            }

            // Turn off IPV6_V6ONLY so we can accept both v4 and v6 connections
            if (!SetOption(sock, IPPROTO_IPV6, IPV6_V6ONLY, false))
            {
                Console::logVerbose("setsockopt(socket, IPV6_V6ONLY) failed: %d", LAST_SOCKET_ERROR());
            }

            if (!SetOption(sock, SOL_SOCKET, SO_REUSEADDR, true))
            {
                Console::logVerbose("setsockopt(socket, SO_REUSEADDR) failed: %d", LAST_SOCKET_ERROR());
            }

            if (!SetNonBlocking(sock, true))
            {
                throw SocketException("Failed to set non-blocking mode.");
            }

            return sock;
        }

        void CloseSocket()
        {
            if (_socket != INVALID_SOCKET)
            {
                closesocket(_socket);
                _socket = INVALID_SOCKET;
            }
            _status = SocketStatus::Closed;
        }
    };

    namespace Socket
    {
        std::unique_ptr<IUdpSocket> createUdp()
        {
            InitialiseWSA();
            return std::make_unique<UdpSocket>();
        }

        std::unique_ptr<INetworkEndpoint> resolve(const std::string& address, uint16_t port)
        {
            sockaddr_storage ss{};
            socklen_t ss_len;
            if (!BaseSocket::ResolveAddressIPv4(address, port, &ss, &ss_len))
            {
                throw SocketException("Unable to resolve address.");
            }
            return std::make_unique<NetworkEndpoint>(reinterpret_cast<const sockaddr*>(&ss), ss_len);
        }
    }
}
