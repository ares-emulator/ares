#pragma once

#include <nall/tcptext/tcptext-server.hpp>

namespace ares::GDB {

/**
 * GDB based debugging server.
 * This allows for remote debugging over TCP.
 */
class Server : public nall::TCPText::Server {
  public:
    auto reset() -> void;

    struct {
      // Memory
      function<string(u32 address, u32 unitCount, u32 unitSize)> cmdRead{nullptr};
      function<void(u32 address, u32 unitSize, u64 value)> cmdWrite{nullptr};

      // Registers
      function<string()> cmdRegReadGeneral{nullptr};
      function<string(u32 regIdx)> cmdRegRead{nullptr};

    } hooks{};

  protected:
    auto onText(string_view text) -> void override;

  private:
    auto processCommand(const string& cmd) -> string;
};

extern Server server;

}