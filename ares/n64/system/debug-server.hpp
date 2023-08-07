#pragma once
#include <nall/wstcp/tcptext-server.hpp>

/**
 * GDB based debugging server.
 * This allows for remote debugging over TCP.
 */
class DebugServer : public nall::TCPText::Server {
  public:
    auto onText(string_view text) -> void override;
};
