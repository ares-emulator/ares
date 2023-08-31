#pragma once

/**
 * Provides a basic TCP connection/socket.
 * This only handles connecting clients, and receiving/sending data.
 */
namespace nall::TCP {

class Socket {
  public:
    auto open(u32 port, bool useIPv4) -> bool;
    auto close(bool notifyHandler = true) -> void;

    auto disconnectClient() -> void;

    auto isStarted() const -> bool { return fdServer >= 0; }
    auto hasClient() const -> bool { return fdClient >= 0; }

    auto getURL(u32 port, bool useIPv4) const -> string;

    ~Socket() { close(false); }

  protected:
    auto update() -> void;

    auto sendData(const u8* data, u32 size) -> void;
    virtual auto onData(const vector<u8> &data) -> void = 0;

    virtual auto onConnect() -> void = 0;
    virtual auto onDisonnect() -> void = 0;

  private:
    std::atomic<bool> stopServer{false}; // set to true to let the server-thread know to stop.
    std::atomic<bool> serverRunning{false}; // signals the current state of the server-thread
    std::atomic<bool> wantKickClient{false}; // set to true to let server know to disconnect the current client (if conn.)

    std::atomic<s32> fdServer{-1};
    std::atomic<s32> fdClient{-1};

    vector<u8> receiveBuffer{};
    std::mutex receiveBufferMutex{};

    vector<u8> sendBuffer{};
    std::mutex sendBufferMutex{};
};

}

#if defined(NALL_HEADER_ONLY)
  #include <nall/tcptext/tcp-socket.cpp>
#endif
