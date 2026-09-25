#pragma once

#include <nall/string.hpp>

#if defined(PLATFORM_WINDOWS)
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <netdb.h>
#endif

namespace nall::UDP {

struct Datagram {
  string data;
  struct sockaddr_storage sender;
  socklen_t senderLen = sizeof(sockaddr_storage);
};

class Server {
public:
  auto open(u32 port, bool useIPv4) -> bool {
    close();

    fd = ::socket(useIPv4 ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if(fd < 0) return false;

    s32 valueOn = 1;
    #if defined(SO_REUSEADDR)
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&valueOn, sizeof(s32));
    #endif

    // set non-blocking
    #if defined(PLATFORM_WINDOWS)
      u_long nonBlocking = 1;
      ioctlsocket(fd, FIONBIO, &nonBlocking);
    #else
      auto flags = fcntl(fd, F_GETFL, 0);
      fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    #endif

    s32 bindRes;
    if(useIPv4) {
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = htons(port);
      bindRes = ::bind(fd, (sockaddr*)&addr, sizeof(addr));
    } else {
      sockaddr_in6 addr{};
      addr.sin6_family = AF_INET6;
      addr.sin6_addr = in6addr_loopback;
      addr.sin6_port = htons(port);
      bindRes = ::bind(fd, (sockaddr*)&addr, sizeof(addr));
    }

    if(bindRes < 0) {
      printf("NCI: error binding UDP socket on port %d (%s)\n", port, strerror(errno));
      close();
      return false;
    }

    _open = true;
    printf("NCI: UDP server listening on port %d\n", port);
    return true;
  }

  auto close() -> void {
    if(fd >= 0) {
      #if defined(PLATFORM_WINDOWS)
        ::closesocket(fd);
      #else
        ::close(fd);
      #endif
      fd = -1;
    }
    _open = false;
  }

  auto isOpen() const -> bool { return _open; }

  auto poll(std::vector<Datagram>& out) -> void {
    if(fd < 0) return;

    char buffer[4096];
    while(true) {
      Datagram dg;
      dg.senderLen = sizeof(dg.sender);
      auto len = ::recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                            (sockaddr*)&dg.sender, &dg.senderLen);
      if(len <= 0) break;
      buffer[len] = '\0';
      dg.data = buffer;
      out.push_back(std::move(dg));
    }
  }

  auto sendTo(const string& text, const sockaddr* dest, socklen_t destLen) -> void {
    if(fd < 0) return;
    ::sendto(fd, (const char*)text.data(), text.size(), 0, dest, destLen);
  }

  ~Server() { close(); }

private:
  s32 fd = -1;
  bool _open = false;
};

}
