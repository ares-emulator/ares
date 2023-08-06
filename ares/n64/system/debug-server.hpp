#pragma once
#include <nall/wstcp/wstcp-server.hpp>

/**
 * GDB based debugging server.
 * This allows for remote debugging over TCP.
 */
class DebugServer : public nall::WsTCP::Server {
  public:
    auto onText(string_view text) -> void override;
};
