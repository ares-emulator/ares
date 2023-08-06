#pragma once

#include <nall/wstcp/tcp-socket.hpp>

/**
 * Provides text-based WebSocket or raw TCP server on top of the Socket.
 * This handles incoming messages and can send data back to the client.
 */
namespace nall::WsTCP {

struct Server : public TCP::Socket {
  bool hadHandshake{false};
  bool isWS{false};

  auto onData(const vector<u8> &data) -> void override;

  auto sendText(const string &text) -> void;
  virtual auto onText(string_view text) -> void = 0;
};

}

#if defined(NALL_HEADER_ONLY)
  #include <nall/wstcp/wstcp-server.cpp>
#endif
