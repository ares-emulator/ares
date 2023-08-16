#pragma once

/**
 * Provides a basic TCP connection/socket.
 * This only handles connecting clients, and receiving/sending data.
 */
namespace nall::TCP {

class Socket {
  public:
    auto open(u32 port) -> bool;
    auto close() -> void;

    auto disconnectClient() -> void;

    ~Socket() { close(); }

  protected:
    auto update() -> void;

    auto sendData(const u8* data, u32 size) -> void;
    virtual auto onData(const vector<u8> &data) -> void = 0;
    virtual auto onConnect() -> void = 0;

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
