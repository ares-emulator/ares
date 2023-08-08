#pragma once

#include <nall/tcptext/tcptext-server.hpp>

namespace ares::DebugServer {

/**
 * GDB based debugging server.
 * This allows for remote debugging over TCP.
 */
class Server : public nall::TCPText::Server {
  public:
    auto onText(string_view text) -> void override;
};

extern Server server;

}