#include <nall/tcptext/tcp-socket.hpp>
#include <memory>
#include <thread>

#if defined(PLATFORM_WINDOWS)
  #include <ws2tcpip.h>
#endif

struct sockaddr_in;
struct sockaddr_in6;

namespace {
  constexpr u32 SLEEP_MILLIS = 2;

  struct Handle {
    s32 fd{-1};
    u64 addrStorage[16]{0};
    sockaddr_in6& addrInV6{(sockaddr_in6&)addrStorage};
    addrinfo* info{nullptr};
  };

  struct Client {
    s32 handle{-1};
    sockaddr_in address{0};
  };
}

namespace nall::TCP {

NALL_HEADER_INLINE auto Socket::open(u16 port) -> bool {
  stopServer = false;

  auto conf = settings;
  auto t = std::thread([this, port, conf]() {
    serverRunning = true;

    Handle handle{};
    handle.fd = socket(AF_INET6, SOCK_STREAM, 0);  
    if(!handle.fd) {
      serverRunning = false;
      return;
    }

    {
    #if defined(SO_RCVTIMEO)
    if(conf.timeoutReceive) {
      struct timeval rcvtimeo;
      rcvtimeo.tv_sec  = conf.timeoutReceive / 1000;
      rcvtimeo.tv_usec = conf.timeoutReceive % 1000 * 1000;
      setsockopt(handle.fd, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo, sizeof(struct timeval));
    }
    #endif

    #if defined(SO_SNDTIMEO)
    if(conf.timeoutSend) {
      struct timeval sndtimeo;
      sndtimeo.tv_sec  = conf.timeoutSend / 1000;
      sndtimeo.tv_usec = conf.timeoutSend % 1000 * 1000;
      setsockopt(handle.fd, SOL_SOCKET, SO_SNDTIMEO, &sndtimeo, sizeof(struct timeval));
    }
    #endif

    #if defined(SO_NOSIGPIPE)  //BSD, OSX
    s32 nosigpipe = 1;
    setsockopt(handle.fd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(s32));
    #endif

    #if defined(SO_REUSEADDR)  //BSD, Linux, OSX
    s32 reuseaddr = 1;
    setsockopt(handle.fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(s32));
    #endif

    #if defined(SO_REUSEPORT)  //BSD, OSX
    s32 reuseport = 1;
    setsockopt(handle.fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(s32));
    #endif
    }

    handle.addrInV6.sin6_family = AF_INET6;
    handle.addrInV6.sin6_addr = in6addr_any;
    handle.addrInV6.sin6_port = htons(port);

    if(bind(handle.fd, (struct sockaddr*)&handle.addrInV6, sizeof(handle.addrInV6)) < 0
      || listen(handle.fd, 1) < 0
    ) {
      printf("error binding socket on port %d! (%s)\n", port, strerror(errno));
      stopServer = true;
    }

    vector<u8> packet{};
    packet.resize(conf.chunkSize);
    Client client{};

    while(!stopServer) 
    {
      // scan for new connections
      if(client.handle < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        socklen_t socketSize = sizeof(sockaddr_in);
        client.handle = accept(handle.fd, (sockaddr *)&client.address, &socketSize);
        if(client.handle >= 0) {
          printf("Client connected!\n");
        }
      }
      
      // handle receiving & sending data
      if(client.handle >= 0) {
        // copy send-data to minimize lock time
        vector<u8> localSendBuffer{};
        {
          std::lock_guard{sendBufferMutex};
          if(sendBuffer.size() > 0) {
            localSendBuffer = sendBuffer;
            sendBuffer.reset();
          }
        }

        // send data
        if(localSendBuffer.size() > 0) {
          if(send(client.handle, localSendBuffer.data(), localSendBuffer.size(), 0) < 0) {
            printf("error sending data! (%s)\n", strerror(errno));
          }
        }

        // receive data from connected clients
        s32 length = recv(client.handle, packet.data(), conf.chunkSize, MSG_NOSIGNAL);
        if(length > 0) {
          std::lock_guard guard{receiveBufferMutex};
          auto oldSize = receiveBuffer.size();
          receiveBuffer.resize(receiveBuffer.size() + length);
          memcpy(receiveBuffer.data() + oldSize, packet.data(), length);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MILLIS));
      }

      // Kick client if we need to (must be done send)
      if(client.handle >= 0 && wantKickClient) {
        ::close(client.handle);
        client.handle = -1;
        wantKickClient = false;
      }
    }
    
    // Stop
    printf("Stopping server...\n");
    if(handle.fd) {
      ::close(handle.fd);
      handle.fd = -1;
    }

    if(handle.info) {
      freeaddrinfo(handle.info);
      handle.info = nullptr;
    }

    serverRunning = false;
  });
  t.detach();

  return true;
}

NALL_HEADER_INLINE auto Socket::close() -> void {
  stopServer = true;

  while(serverRunning) {
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MILLIS));
  }
}

NALL_HEADER_INLINE auto Socket::update() -> void {
  vector<u8> data{};
  
  {
    std::lock_guard guard{receiveBufferMutex};
    if(receiveBuffer.size() > 0) {
      data = receiveBuffer;
      receiveBuffer.reset();
    }
  }

  if(data.size() > 0) {
    onData(data);
  }
}

NALL_HEADER_INLINE auto Socket::disconnectClient() -> void {
  wantKickClient = true;
}

NALL_HEADER_INLINE auto Socket::sendData(const u8* data, u32 size) -> void {
  std::lock_guard{sendBufferMutex};
  u32 oldSize = sendBuffer.size();
  sendBuffer.resize(oldSize + size);
  memcpy(sendBuffer.data() + oldSize, data, size);
}

}
