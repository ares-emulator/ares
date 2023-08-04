#include <n64/n64.hpp>

#define NALL_HEADER_ONLY
#include <nall/http/server.hpp>
#undef NALL_HEADER_ONLY

// @TODO: why does nall need this?
#include <cstdlib>
#include <utility>

namespace HTTP = ::nall::HTTP;
using string = ::nall::string;
using string_view = ::nall::string_view;

namespace {
  HTTP::Server server{};
}

namespace ares::Nintendo64 {

auto DebugServer::start(u32 port) -> bool {
  printf("Starting Debug-server on port: %d\n", port);
  if(!server.open(port)) {
    printf("Failed to open server!\n");
    return false;
  }

  server.main([this](HTTP::Request& req)  {
    printf("Data[%d]: %s\n\n", req._body.size(), req._body.data());

    HTTP::Response res{req};
    res.setResponseType(200); // OK
    res.setAllowCORS(true); // @TODO: add security checks (random token?)
    res._body = processCommand(req._body);
    return res;
  });

  return true;
}

auto DebugServer::update() -> void {
  auto scanRes = server.scan();
}

auto DebugServer::stop() -> bool {
  printf("Stopping Debug-server\n");
  server.close();
  return true;
}

auto DebugServer::processCommand(const string &command) -> string {
  auto cmdParts = command.split(" ");
  auto cmdName = cmdParts[0];
  u32 cyclesDummy; // some function modify the cycle count, the debugger should not do that

  if(cmdName == "read")  {
    if(cmdParts.size() < 4) {
      return "ERR";
    }
    u32 unitCount = static_cast<u32>(cmdParts[1].hex());
    u32 unitSize = static_cast<u32>(cmdParts[2].hex());
    u32 address = static_cast<u32>(cmdParts[3].hex());

    string res{""};
    for(u32 i=0; i<unitCount; ++i) {
      switch(unitSize) {
        case Byte: res.append(hex(( u8)bus.read<Byte>(address, cyclesDummy))); break;
        case Half: res.append(hex((u16)bus.read<Half>(address, cyclesDummy))); break;
        case Word: res.append(hex((u32)bus.read<Word>(address, cyclesDummy))); break;
        case Dual: res.append(hex((u64)bus.read<Dual>(address, cyclesDummy))); break;
      }
      res.append(" ");
      address += unitSize;
    }
    return res;
  }
  
  if(cmdName == "write") {
    if(cmdParts.size() < 4) {
      return "ERR";
    }
    u32 unitCount = static_cast<u32>(cmdParts[1].hex());
    u32 unitSize = static_cast<u32>(cmdParts[2].hex());
    u32 address = static_cast<u32>(cmdParts[3].hex());
    u64 value = (cmdParts[4].hex());

    for(u32 i=0; i<unitCount; ++i) {
      switch(unitSize) {
        case Byte: bus.write<Byte>(address, value, cyclesDummy); break;
        case Half: bus.write<Half>(address, value, cyclesDummy); break;
        case Word: bus.write<Word>(address, value, cyclesDummy); break;
        case Dual: bus.write<Dual>(address, value, cyclesDummy); break;
      }
      address += unitSize;
    }
    return "OK";
  }

  return "ERR";
}

} // end namespace