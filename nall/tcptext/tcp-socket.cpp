#include <nall/tcptext/tcp-socket.hpp>
#include <memory>
#include <thread>

#if defined(PLATFORM_WINDOWS)
  #include <ws2tcpip.h>
#else
  #include <netinet/tcp.h>
#endif

struct sockaddr_in;
struct sockaddr_in6;

namespace {
  constexpr bool TCP_LOG_MESSAGES = false;

  constexpr u32 TCP_BUFFER_SIZE = 1024 * 16;
  constexpr u32 CLIENT_SLEEP_MS = 10; // ms to sleep while checking for new clients
  constexpr u32 CYCLES_BEFORE_SLEEP = 100; // how often to do a send/receive check before a sleep

  std::atomic<s32> fdServer{-1};
  std::atomic<s32> fdClient{-1};
}

namespace nall::TCP {

/**
 * Opens a TCP server with callbacks to send and receive data.
 * 
 * This spawns 3 new threads:
 * threadServer:  listens for new connections, kicks connections
 * threadSend:    sends data to the client
 * threadReceive: receives data from the client
 * 
 * Each contains it's own loop including sleeps to not use too much CPU.
 * The exception is threadReceive which relies on the blocking recv() call (kernel wakes it up again).
 * 
 * Incoming and outgoing data is synchronized using mutexes,
 * and put into buffers that are shared with the main thread.
 * Meaning, the thread that calls 'update()' with also be the one that gets 'onData()' calls.
 * No additional synchronization is needed.
 * 
 * NOTE: if you work on the loop/sleeps, make sure to test CPU usage and package-latency.
 */

NALL_HEADER_INLINE auto Socket::open(u32 port) -> bool {
  stopServer = false;

  printf("Opening TCP-server on [::1]:%d\n", port);

  auto threadServer = std::thread([this, port]() {
    serverRunning = true;

    fdServer = socket(AF_INET6, SOCK_STREAM, 0);  
    if(fdServer < 0) {
      serverRunning = false;
      return;
    }

    {
      s32 valueOn = 1;
      #if defined(SO_NOSIGPIPE)  //BSD, OSX
        setsockopt(fdServer, SOL_SOCKET, SO_NOSIGPIPE, &valueOn, sizeof(s32));
      #endif

      #if defined(SO_REUSEADDR)  //BSD, Linux, OSX
        setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, &valueOn, sizeof(s32));
      #endif

      #if defined(SO_REUSEPORT)  //BSD, OSX
        setsockopt(fdServer, SOL_SOCKET, SO_REUSEPORT, &valueOn, sizeof(s32));
      #endif

      #if defined(TCP_NODELAY)
        setsockopt(fdServer, IPPROTO_TCP, TCP_NODELAY, &valueOn, sizeof(s32));
      #endif

      #if defined(SO_RCVTIMEO)
        struct timeval rcvtimeo;
        rcvtimeo.tv_sec  = 1;
        rcvtimeo.tv_usec = 0;
        setsockopt(fdServer, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeo, sizeof(rcvtimeo));
      #endif
    }

    sockaddr_in6 serverAddrV6{};
    serverAddrV6.sin6_family = AF_INET6;
    serverAddrV6.sin6_addr = in6addr_loopback;
    serverAddrV6.sin6_port = htons(port);

    if(::bind(fdServer, (sockaddr*)&serverAddrV6, sizeof(serverAddrV6)) < 0 || listen(fdServer, 1) < 0) {
      printf("error binding socket on port %d! (%s)\n", port, strerror(errno));
      stopServer = true;
    }

    while(!stopServer) 
    {
      // scan for new connections
      if(fdClient < 0) {
        fdClient = accept(fdServer, nullptr, nullptr);
        if(fdClient >= 0) {
          printf("Client connected!\n");
        }
      }
      
      // Kick client if we need to
      if(fdClient >= 0 && wantKickClient) {
        ::close(fdClient);
        fdClient = -1;
        wantKickClient = false;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
    }
    
    printf("Stopping TCP-server...\n");
    if(fdServer >= 0) {
      ::close(fdServer);
      fdServer = -1;
    }

    printf("TCP-server stopped\n");
    serverRunning = false;
  });

  auto threadSend = std::thread([this]() 
  {
    vector<u8> localSendBuffer{};
    u32 cycles = 0;

    while(!stopServer) 
    {
      if(fdClient < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
        continue;
      }

      { // copy send-data to minimize lock time
        std::lock_guard guard{sendBufferMutex};
        if(sendBuffer.size() > 0) {
          localSendBuffer = sendBuffer;
          sendBuffer.resize(0);
        }
      }

      // send data
      if(localSendBuffer.size() > 0) {
        auto bytesWritten = send(fdClient, localSendBuffer.data(), localSendBuffer.size(), 0);
        if(bytesWritten < localSendBuffer.size()) {
          printf("Error sending data! (%s)\n", strerror(errno));
        }

        if constexpr(TCP_LOG_MESSAGES) {
          printf("%.4f | TCP >: [%ld]: %.*s\n", (f64)chrono::millisecond() / 1000.0, localSendBuffer.size(), localSendBuffer.size() > 100 ? 100 : (int)localSendBuffer.size(), (char*)localSendBuffer.data());
        }

        localSendBuffer.resize(0);
        cycles = 0; // sending once has a good chance of sending more -> reset sleep timer
      }

      if(cycles++ >= CYCLES_BEFORE_SLEEP) {
        usleep(1);
        cycles = 0;
      } 
    }
  });

  auto threadReceive = std::thread([this]() 
  {
    u8 packet[TCP_BUFFER_SIZE]{0};

    while(!stopServer) 
    {
      if(fdClient < 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(CLIENT_SLEEP_MS));
        continue;
      }

      // receive data from connected clients
      s32 length = recv(fdClient, packet, TCP_BUFFER_SIZE, MSG_NOSIGNAL);
      if(length > 0) {
        std::lock_guard guard{receiveBufferMutex};
        auto oldSize = receiveBuffer.size();
        receiveBuffer.resize(oldSize + length);
        memcpy(receiveBuffer.data() + oldSize, packet, length);

        if constexpr(TCP_LOG_MESSAGES) {
          printf("%.4f | TCP <: [%d]: %.*s ([%d]: %.*s)\n", (f64)chrono::millisecond() / 1000.0, length, length, (char*)receiveBuffer.data(), length, length, (char*)packet);
        }
      }
    }
  });

  threadServer.detach();
  threadSend.detach();
  threadReceive.detach();

  return true;
}

NALL_HEADER_INLINE auto Socket::close() -> void {
  stopServer = true;

  while(serverRunning) {
    usleep(1000); // wait for other threads to stop
  }
}

NALL_HEADER_INLINE auto Socket::update() -> void {
  vector<u8> data{};
  
  { // local copy, minimize lock time
    std::lock_guard guard{receiveBufferMutex};
    if(receiveBuffer.size() > 0) {
      data = receiveBuffer;
      receiveBuffer.resize(0);
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
  std::lock_guard guard{sendBufferMutex};
  u32 oldSize = sendBuffer.size();
  sendBuffer.resize(oldSize + size);
  memcpy(sendBuffer.data() + oldSize, data, size);
}

}
