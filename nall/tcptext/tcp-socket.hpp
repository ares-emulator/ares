#pragma once

/**
 * Provides a basic TCP connection/socket.
 * This only handles connecting clients, and receiving/sending data.
 */
namespace nall::TCP {

struct Socket {
  struct Settings {
    s32 connectionLimit =     1 * 1024;  //server
    s32 headSizeLimit   =    16 * 1024;  //client, server
    s32 bodySizeLimit   = 65536 * 1024;  //client, server
    s32 chunkSize       =    32 * 1024;  //client, server
    s32 threadStackSize =   128 * 1024;  //server
    s32 timeoutReceive  =           10;  //server
    s32 timeoutSend     =    15 * 1000;  //server
  } settings;

  auto open(u16 port) -> bool;
  auto close() -> void;
  auto update() -> void;

  auto disconnectClient() -> void;

  auto sendData(const u8* data, u32 size) -> void;
  virtual auto onData(const vector<u8> &data) -> void = 0;

  ~Socket() { close(); }

  private:
    std::atomic<bool> stopServer{false}; // set to true to let the server-thread know to stop.
    std::atomic<bool> serverRunning{false}; // signals the current state of the server-thread
    std::atomic<bool> wantKickClient{false}; // set to true to let server know to disconnect the current client (if conn.)

    vector<u8> receiveBuffer{};
    std::mutex receiveBufferMutex{};

    vector<u8> sendBuffer{};
    std::mutex sendBufferMutex{};
};

}

#if defined(NALL_HEADER_ONLY)
  #include <nall/tcptext/tcp-socket.cpp>
#endif
