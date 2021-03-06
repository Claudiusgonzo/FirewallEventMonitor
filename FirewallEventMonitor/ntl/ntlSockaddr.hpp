// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See License.txt in the project root for license information.

#pragma once

// cpp headers
#include <algorithm>
#include <string>
#include <vector>
// os headers
#include <Windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>
// ntl headers
#include "ntlVersionConversion.hpp"
#include "ntlException.hpp"
#include "ntlScopeGuard.hpp"

#pragma prefast(push)
// ignore prefast IPv4 warnings
#pragma prefast(disable: 24002)
// ignore IDN warnings when explicitly asking to resolve a short-string
#pragma prefast(disable: 38026)

namespace ntl {

    enum class ByteOrder
    {
        HostOrder,
        NetworkOrder
    };

    static const DWORD IP_STRING_MAX_LENGTH = 65;

    class Sockaddr {
    public:

        static
        std::vector<Sockaddr> ResolveName(_In_ LPCWSTR _name)
        {
            ADDRINFOW* addr_result = nullptr;
            ScopeGuard(freeAddrOnExit, { if (addr_result) ::FreeAddrInfoW(addr_result); });

            std::vector<Sockaddr> return_addrs;
            if (0 == ::GetAddrInfoW(_name, nullptr, nullptr, &addr_result)) {
                for (ADDRINFOW* addrinfo = addr_result; addrinfo != nullptr; addrinfo = addrinfo->ai_next) {
                    return_addrs.push_back(Sockaddr(addrinfo->ai_addr, static_cast<int>(addrinfo->ai_addrlen)));
                }
            } else {
                throw ntl::Exception(::WSAGetLastError(), L"GetAddrInfoW", L"ntl::Sockaddr::ResolveName", false);
            }

            return return_addrs;
        }

        explicit Sockaddr(short family = AF_UNSPEC) NOEXCEPT;
        explicit Sockaddr(_In_reads_bytes_(inLength) const SOCKADDR* inAddr, const int inLength) NOEXCEPT;
        explicit Sockaddr(_In_reads_bytes_(inLength) const SOCKADDR* inAddr, const size_t inLength) NOEXCEPT;
        explicit Sockaddr(_In_ const SOCKADDR_IN*) NOEXCEPT;
        explicit Sockaddr(_In_ const SOCKADDR_IN6*) NOEXCEPT;
        explicit Sockaddr(_In_ const SOCKADDR_INET*) NOEXCEPT;
        explicit Sockaddr(_In_ const SOCKADDR_STORAGE*) NOEXCEPT;
        explicit Sockaddr(_In_ const SOCKET_ADDRESS*) NOEXCEPT;

        Sockaddr(const Sockaddr&) NOEXCEPT;
        Sockaddr& operator=(const Sockaddr&) NOEXCEPT;

        Sockaddr(Sockaddr&&) NOEXCEPT;
        Sockaddr& operator=(Sockaddr&&) NOEXCEPT;

        bool operator==(const Sockaddr&) const NOEXCEPT;
        bool operator!=(const Sockaddr&) const NOEXCEPT;

        virtual ~Sockaddr() = default;

        void reset(short family = AF_UNSPEC) NOEXCEPT;

        void swap(_Inout_ Sockaddr&) NOEXCEPT;

        bool setSocketAddress(SOCKET) NOEXCEPT;

        void setSockaddr(_In_reads_bytes_(length) const SOCKADDR*, int length) NOEXCEPT;
        void setSockaddr(_In_ const SOCKADDR_IN*) NOEXCEPT;
        void setSockaddr(_In_ const SOCKADDR_IN6*) NOEXCEPT;
        void setSockaddr(_In_ const SOCKADDR_INET*) NOEXCEPT;
        void setSockaddr(_In_ const SOCKADDR_STORAGE*) NOEXCEPT;
        void setSockaddr(_In_ const SOCKET_ADDRESS*) NOEXCEPT;

        void setPort(unsigned short, ByteOrder = ByteOrder::HostOrder) NOEXCEPT;

        // for dual-mode sockets, when needing to explicitly connect to the target v4 address,
        // - one must map the v4 address to its mapped v6 address
        void mapDualMode4To6() NOEXCEPT;

        // setting by string returns a bool if was able to convert to an address
        bool setAddress(_In_ PCWSTR wszAddr) NOEXCEPT;
        bool setAddress(_In_ LPCSTR szAddr) NOEXCEPT;

        void setAddress(_In_ const IN_ADDR*) NOEXCEPT;
        void setAddress(_In_ const IN6_ADDR*) NOEXCEPT;

        void setFlowInfo(unsigned long) NOEXCEPT;
        void setScopeId(unsigned long) NOEXCEPT;

        void setAddressLoopback() NOEXCEPT;
        void setAddressAny() NOEXCEPT;

        bool isAddressLoopback() const NOEXCEPT;
        bool isAddressAny() const NOEXCEPT;

        // the odd syntax below is necessary to pass a reference to an array
        // necessary as the [] operators take precedence over the ref & operator
        // writeAddress only prints the IP address, not the scope or port
        std::wstring writeAddress() const;
        bool writeAddress(WCHAR (&address)[IP_STRING_MAX_LENGTH]) const NOEXCEPT;
        bool writeAddress(CHAR (&address)[IP_STRING_MAX_LENGTH]) const NOEXCEPT;
        // writeCompleteAddress prints the IP address, scope, and port
        std::wstring writeCompleteAddress(bool trim_scope = false) const;
        bool writeCompleteAddress(WCHAR (&address)[IP_STRING_MAX_LENGTH], bool trim_scope = false) const NOEXCEPT;
        bool writeCompleteAddress(CHAR (&address)[IP_STRING_MAX_LENGTH], bool trim_scope = false) const NOEXCEPT;

        //
        // Accessors
        //
        int               length() const NOEXCEPT;
        unsigned short    port() const NOEXCEPT;
        short             family() const NOEXCEPT;
        unsigned long     flowinfo() const NOEXCEPT;
        unsigned long     scopeId() const NOEXCEPT;
        // returning non-const from const methods, for API compatibility
        SOCKADDR*         sockaddr() const NOEXCEPT;
        SOCKADDR_IN*      sockaddr_in() const NOEXCEPT;
        SOCKADDR_IN6*     sockaddr_in6() const NOEXCEPT;
        SOCKADDR_INET*    sockaddr_inet() const NOEXCEPT;
        SOCKADDR_STORAGE* sockaddr_storage() const NOEXCEPT;
        IN_ADDR*          in_addr() const NOEXCEPT;
        IN6_ADDR*         in6_addr() const NOEXCEPT;

    private:
        static const size_t SADDR_SIZE = sizeof(SOCKADDR_STORAGE);
        SOCKADDR_STORAGE saddr;
    };

    //
    // non-member swap
    //
    inline
    void swap(_Inout_ Sockaddr& left_, _Inout_ Sockaddr& right_) NOEXCEPT
    {
        left_.swap(right_);
    }


    inline Sockaddr::Sockaddr(short family) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        saddr.ss_family = family;
    }

    inline Sockaddr::Sockaddr(_In_reads_bytes_(inLength) const SOCKADDR* inAddr, const int inLength) NOEXCEPT
    {
        size_t length = (static_cast<size_t>(inLength) <= SADDR_SIZE) ? inLength : SADDR_SIZE;

        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, length);
    }
    inline Sockaddr::Sockaddr(_In_reads_bytes_(inLength) const SOCKADDR* inAddr, const size_t inLength) NOEXCEPT
    {
        size_t length = (inLength <= SADDR_SIZE) ? inLength : SADDR_SIZE;

        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, length);
    }

    inline Sockaddr::Sockaddr(const SOCKADDR_IN* inAddr) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN));
    }
    inline Sockaddr::Sockaddr(const SOCKADDR_IN6* inAddr) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN6));
    }
    inline Sockaddr::Sockaddr(const SOCKADDR_INET* inAddr) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        if (AF_INET == inAddr->si_family) {
            ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN));
        } else {
            ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN6));
        }
    }
    inline Sockaddr::Sockaddr(const SOCKADDR_STORAGE* inAddr) NOEXCEPT
    {
        ::CopyMemory(&saddr, inAddr, SADDR_SIZE);
    }
    inline Sockaddr::Sockaddr(const SOCKET_ADDRESS* inAddr) NOEXCEPT
    {
        size_t length = (inAddr->iSockaddrLength <= SADDR_SIZE) ?
            inAddr->iSockaddrLength :
            SADDR_SIZE;

        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr->lpSockaddr, length);
    }

    inline Sockaddr::Sockaddr(const Sockaddr& inAddr) NOEXCEPT
    {
        ::CopyMemory(&saddr, &inAddr.saddr, SADDR_SIZE);
    }
    inline Sockaddr& Sockaddr::operator=(const Sockaddr& inAddr) NOEXCEPT
    {
        // copy and swap
        Sockaddr temp(inAddr);
        this->swap(temp);
        return *this;
    }

    inline Sockaddr::Sockaddr(Sockaddr&& inAddr) NOEXCEPT
    {
        ::CopyMemory(&saddr, &inAddr.saddr, SADDR_SIZE);
    }
    inline Sockaddr& Sockaddr::operator=(Sockaddr&& inAddr) NOEXCEPT
    {
        ::CopyMemory(&saddr, &inAddr.saddr, SADDR_SIZE);
        ::ZeroMemory(&inAddr.saddr, SADDR_SIZE);
        return *this;
    }

    inline bool Sockaddr::operator==(const Sockaddr& _inAddr) const NOEXCEPT
    {
        return (0 == ::memcmp(&this->saddr, &_inAddr.saddr, SADDR_SIZE));
    }
    inline bool Sockaddr::operator!=(const Sockaddr& _inAddr) const NOEXCEPT
    {
        return !(*this == _inAddr);
    }

    inline void Sockaddr::reset(short family) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        saddr.ss_family = family;
    }

    inline void Sockaddr::swap(_Inout_ Sockaddr& inAddr) NOEXCEPT
    {
        using std::swap;
        swap(saddr, inAddr.saddr);
    }

    inline bool Sockaddr::setSocketAddress(SOCKET s) NOEXCEPT
    {
        int namelen = this->length();
        return (0 == ::getsockname(s, this->sockaddr(), &namelen));
    }

    inline void Sockaddr::setSockaddr(_In_reads_bytes_(inLength) const SOCKADDR* inAddr, int inLength) NOEXCEPT
    {
        size_t length = (inLength <= SADDR_SIZE) ? inLength : SADDR_SIZE;

        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, length);
    }
    inline void Sockaddr::setSockaddr(const SOCKADDR_IN* inAddr) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN));
    }
    inline void Sockaddr::setSockaddr(const SOCKADDR_IN6* inAddr) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN6));
    }
    inline void Sockaddr::setSockaddr(const SOCKADDR_INET* inAddr) NOEXCEPT
    {
        ::ZeroMemory(&saddr, SADDR_SIZE);
        if (AF_INET == inAddr->si_family) {
            ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN));
        } else {
            ::CopyMemory(&saddr, inAddr, sizeof(SOCKADDR_IN6));
        }
    }
    inline void Sockaddr::setSockaddr(const SOCKADDR_STORAGE* inAddr) NOEXCEPT
    {
        ::CopyMemory(&saddr, inAddr, SADDR_SIZE);
    }
    inline void Sockaddr::setSockaddr(const SOCKET_ADDRESS* inAddr) NOEXCEPT
    {
        size_t length = (static_cast<size_t>(inAddr->iSockaddrLength) <= SADDR_SIZE) ?
            static_cast<size_t>(inAddr->iSockaddrLength) :
            SADDR_SIZE;

        ::ZeroMemory(&saddr, SADDR_SIZE);
        ::CopyMemory(&saddr, inAddr->lpSockaddr, length);
    }

    inline void Sockaddr::setAddressLoopback() NOEXCEPT
    {
        if (AF_INET == saddr.ss_family) {
            PSOCKADDR_IN in4 = reinterpret_cast<PSOCKADDR_IN>(&saddr);
            unsigned short in4_port = in4->sin_port;
            ::ZeroMemory(&saddr, SADDR_SIZE);

            in4->sin_family = AF_INET;
            in4->sin_port = in4_port;
            in4->sin_addr.s_addr = 0x0100007f; // htons(INADDR_LOOPBACK);
        } else if (AF_INET6 == saddr.ss_family) {
            PSOCKADDR_IN6 in6 = reinterpret_cast<PSOCKADDR_IN6>(&saddr);
            unsigned short in6_port = in6->sin6_port;
            ::ZeroMemory(&saddr, SADDR_SIZE);

            in6->sin6_family = AF_INET6;
            in6->sin6_port = in6_port;
            in6->sin6_addr.s6_bytes[15] = 1; // IN6ADDR_LOOPBACK_INIT;
        } else {
            ntl::AlwaysFatalCondition(
                L"Sockaddr: unknown family in the SOCKADDR_STORAGE (this %p)", this);
        }
    }
    inline void Sockaddr::setAddressAny() NOEXCEPT
    {
        if (AF_INET == saddr.ss_family) {
            PSOCKADDR_IN in4 = reinterpret_cast<PSOCKADDR_IN>(&saddr);
            unsigned short in4_port = in4->sin_port;
            ::ZeroMemory(&saddr, SADDR_SIZE);

            in4->sin_family = AF_INET;
            in4->sin_port = in4_port;
        } else if (AF_INET6 == saddr.ss_family) {
            PSOCKADDR_IN6 in6 = reinterpret_cast<PSOCKADDR_IN6>(&saddr);
            unsigned short in6_port = in6->sin6_port;
            ::ZeroMemory(&saddr, SADDR_SIZE);

            in6->sin6_family = AF_INET6;
            in6->sin6_port = in6_port;
        }
    }
    inline bool Sockaddr::isAddressLoopback() const NOEXCEPT
    {
        Sockaddr loopback(*this);
        loopback.setAddressLoopback();
        return (0 == ::memcmp(&(loopback.saddr), &(this->saddr), sizeof(SOCKADDR_STORAGE)));
    }
    inline bool Sockaddr::isAddressAny() const NOEXCEPT
    {
        Sockaddr any_addr(*this);
        any_addr.setAddressAny();
        return (0 == ::memcmp(&(any_addr.saddr), &(this->saddr), sizeof(SOCKADDR_STORAGE)));
    }

    inline void Sockaddr::setPort(unsigned short port, ByteOrder order) NOEXCEPT
    {
        PSOCKADDR_IN addr_in = reinterpret_cast<PSOCKADDR_IN>(&saddr);
        addr_in->sin_port = (order == ByteOrder::HostOrder) ? ::htons(port) : port;
    }

    inline void Sockaddr::mapDualMode4To6() NOEXCEPT
    {
        static const IN6_ADDR v4MappedPrefix = IN6ADDR_V4MAPPEDPREFIX_INIT;

        Sockaddr tempV6(AF_INET6);
        IN6_ADDR* a6 = tempV6.in6_addr();
        const IN_ADDR* a4 = this->in_addr();

        *a6 = v4MappedPrefix;
        a6->u.Byte[12] = a4->S_un.S_un_b.s_b1;
        a6->u.Byte[13] = a4->S_un.S_un_b.s_b2;
        a6->u.Byte[14] = a4->S_un.S_un_b.s_b3;
        a6->u.Byte[15] = a4->S_un.S_un_b.s_b4;

        tempV6.setPort(this->port());
        this->swap(tempV6);
    }

    inline bool Sockaddr::setAddress(_In_ LPCWSTR wszAddr) NOEXCEPT
    {
        ADDRINFOW addr_hints;
        ::ZeroMemory(&addr_hints, sizeof(addr_hints));
        addr_hints.ai_flags = AI_NUMERICHOST;

        ADDRINFOW* addr_result = nullptr;
        if (0 == ::GetAddrInfoW(wszAddr, nullptr, &addr_hints, &addr_result)) {
            this->setSockaddr(addr_result->ai_addr, static_cast<int>(addr_result->ai_addrlen));
            ::FreeAddrInfoW(addr_result);
            return true;
        } else {
            return false;
        }
    }

    inline bool Sockaddr::setAddress(_In_ LPCSTR szAddr) NOEXCEPT
    {
        ADDRINFOA addr_hints;
        ::ZeroMemory(&addr_hints, sizeof(addr_hints));
        addr_hints.ai_flags = AI_NUMERICHOST;

        ADDRINFOA* addr_result = nullptr;
#pragma prefast(suppress:38026, "The explicit use of AI_NUMERICHOST makes GetAddrInfoA's lack of IDN support irrelevant here - they wouldn't be accepted even if we used GetAddrInfoW")
        if (0 == ::GetAddrInfoA(szAddr, nullptr, &addr_hints, &addr_result)) {
            this->setSockaddr(addr_result->ai_addr, static_cast<int>(addr_result->ai_addrlen));
            FreeAddrInfoA(addr_result);
            return true;
        } else {
            return false;
        }
    }

    inline void Sockaddr::setAddress(const IN_ADDR* inAddr) NOEXCEPT
    {
        saddr.ss_family = AF_INET;
        PSOCKADDR_IN addr_in = reinterpret_cast<PSOCKADDR_IN>(&saddr);
        addr_in->sin_addr = *inAddr;
    }
    inline void Sockaddr::setAddress(const IN6_ADDR* inAddr) NOEXCEPT
    {
        saddr.ss_family = AF_INET6;
        PSOCKADDR_IN6 addr_in6 = reinterpret_cast<PSOCKADDR_IN6>(&saddr);
        addr_in6->sin6_addr = *inAddr;
    }

    inline void Sockaddr::setFlowInfo(unsigned long flowinfo) NOEXCEPT
    {
        if (AF_INET6 == saddr.ss_family) {
            PSOCKADDR_IN6 addr_in6 = reinterpret_cast<PSOCKADDR_IN6>(&saddr);
            addr_in6->sin6_flowinfo = flowinfo;
        }
    }
    inline void Sockaddr::setScopeId(unsigned long scopeid) NOEXCEPT
    {
        if (AF_INET6 == saddr.ss_family) {
            PSOCKADDR_IN6 addr_in6 = reinterpret_cast<PSOCKADDR_IN6>(&saddr);
            addr_in6->sin6_scope_id = scopeid;
        }
    }

    inline std::wstring Sockaddr::writeAddress() const
    {
        WCHAR return_string[IP_STRING_MAX_LENGTH];
        this->writeAddress(return_string);
        return_string[IP_STRING_MAX_LENGTH - 1] = L'\0';
        return return_string;
    }
    inline bool Sockaddr::writeAddress(WCHAR (&address)[IP_STRING_MAX_LENGTH]) const NOEXCEPT
    {
        ::ZeroMemory(address, IP_STRING_MAX_LENGTH * sizeof(WCHAR));

        const PVOID pAddr = (AF_INET == saddr.ss_family) ? reinterpret_cast<PVOID>(this->in_addr()) : reinterpret_cast<PVOID>(this->in6_addr());
        if (nullptr != ::InetNtopW(saddr.ss_family, pAddr, address, IP_STRING_MAX_LENGTH)) {
            return true;
        }
        return false;
    }

    inline bool Sockaddr::writeAddress(CHAR (&address)[IP_STRING_MAX_LENGTH]) const NOEXCEPT
    {
        ::ZeroMemory(address, IP_STRING_MAX_LENGTH * sizeof(CHAR));
        
        const PVOID pAddr = (AF_INET == saddr.ss_family) ? reinterpret_cast<PVOID>(this->in_addr()) : reinterpret_cast<PVOID>(this->in6_addr());
        if (NULL != ::InetNtopA(saddr.ss_family, pAddr, address, IP_STRING_MAX_LENGTH)) {
            return true;
        }
        return false;
    }

    inline std::wstring Sockaddr::writeCompleteAddress(bool trim_scope) const
    {
        WCHAR return_string[IP_STRING_MAX_LENGTH];
        this->writeCompleteAddress(return_string, trim_scope);
        return_string[IP_STRING_MAX_LENGTH - 1] = L'\0';
        return return_string;
    }
    inline bool Sockaddr::writeCompleteAddress(WCHAR (&address)[IP_STRING_MAX_LENGTH], bool trim_scope) const NOEXCEPT
    {
        ::ZeroMemory(address, IP_STRING_MAX_LENGTH * sizeof(WCHAR));

        DWORD addressLength = IP_STRING_MAX_LENGTH;
        if (0 == ::WSAAddressToStringW(this->sockaddr(), static_cast<DWORD>(SADDR_SIZE), nullptr, address, &addressLength)) {
            if ((this->family() == AF_INET6) && trim_scope) {
                WCHAR* end = address + addressLength;
                WCHAR* scope_ptr = std::find(address, end, L'%');
                if (scope_ptr != end) {
                    const WCHAR* move_ptr = std::find(address, end, L']');
                    if (move_ptr != end) {
                        while (move_ptr != end) {
                            *scope_ptr = *move_ptr;
                            ++scope_ptr;
                            ++move_ptr;
                        }
                    } else {
                        // no port was appended
                        while (scope_ptr != end) {
                            *scope_ptr = L'\0';
                            ++scope_ptr;
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }

    inline bool Sockaddr::writeCompleteAddress(CHAR (&address)[IP_STRING_MAX_LENGTH], bool trim_scope) const NOEXCEPT
    {
        ::ZeroMemory(address, IP_STRING_MAX_LENGTH * sizeof(CHAR));

        DWORD addressLength = IP_STRING_MAX_LENGTH;
#pragma warning( suppress : 4996) 
        if (0 == ::WSAAddressToStringA(this->sockaddr(), static_cast<DWORD>(SADDR_SIZE), nullptr, address, &addressLength)) {
            if ((this->family() == AF_INET6) && trim_scope) {
                CHAR* end = address + addressLength;
                CHAR* scope_ptr = std::find(address, end, '%');
                if (scope_ptr != end) {
                    const CHAR* move_ptr = std::find(address, end, ']');
                    if (move_ptr != end) {
                        while (move_ptr != end) {
                            *scope_ptr = *move_ptr;
                            ++scope_ptr;
                            ++move_ptr;
                        }
                    } else {
                        // no port was appended
                        while (scope_ptr != end) {
                            *scope_ptr = '\0';
                            ++scope_ptr;
                        }
                    }
                }
            }
            return true;
        }
        return false;
    }

    inline int Sockaddr::length() const NOEXCEPT
    {
        return static_cast<int>(SADDR_SIZE);
    }

    inline short Sockaddr::family() const NOEXCEPT
    {
        return saddr.ss_family;
    }
    inline unsigned short Sockaddr::port() const NOEXCEPT
    {
        const SOCKADDR_IN* addr_in = reinterpret_cast<const SOCKADDR_IN*>(&saddr);
        return ::ntohs(addr_in->sin_port);
    }
    inline unsigned long Sockaddr::flowinfo() const NOEXCEPT
    {
        if (AF_INET6 == saddr.ss_family) {
            const SOCKADDR_IN6* addr_in6 = reinterpret_cast<const SOCKADDR_IN6*>(&saddr);
            return addr_in6->sin6_flowinfo;
        } else {
            return 0;
        }
    }
    inline unsigned long Sockaddr::scopeId() const NOEXCEPT
    {
        if (AF_INET6 == saddr.ss_family) {
            const SOCKADDR_IN6* addr_in6 = reinterpret_cast<const SOCKADDR_IN6*>(&saddr);
            return addr_in6->sin6_scope_id;
        } else {
            return 0;
        }
    }

    inline SOCKADDR* Sockaddr::sockaddr() const NOEXCEPT
    {
        return const_cast<SOCKADDR*>(
            reinterpret_cast<const SOCKADDR*>(&saddr));
    }
    inline SOCKADDR_IN* Sockaddr::sockaddr_in() const NOEXCEPT
    {
        return const_cast<SOCKADDR_IN*>(
            reinterpret_cast<const SOCKADDR_IN*>(&saddr));
    }
    inline SOCKADDR_IN6* Sockaddr::sockaddr_in6() const NOEXCEPT
    {
        return const_cast<SOCKADDR_IN6*>(
            reinterpret_cast<const SOCKADDR_IN6*>(&saddr));
    }
    inline SOCKADDR_INET* Sockaddr::sockaddr_inet() const NOEXCEPT
    {
        return const_cast<SOCKADDR_INET*>(
            reinterpret_cast<const SOCKADDR_INET*>(&saddr));
    }
    inline SOCKADDR_STORAGE* Sockaddr::sockaddr_storage() const NOEXCEPT
    {
        return const_cast<SOCKADDR_STORAGE*>(&saddr);
    }
    inline IN_ADDR* Sockaddr::in_addr() const NOEXCEPT
    {
        const SOCKADDR_IN* addr_in = reinterpret_cast<const SOCKADDR_IN*>(&saddr);
        return const_cast<IN_ADDR*>(&(addr_in->sin_addr));
    }
    inline IN6_ADDR* Sockaddr::in6_addr() const NOEXCEPT
    {
        const SOCKADDR_IN6* addr_in6 = reinterpret_cast<const SOCKADDR_IN6*>(&saddr);
        return const_cast<IN6_ADDR*>(&(addr_in6->sin6_addr));
    }

}; // namespace ntl

#pragma prefast(pop)
