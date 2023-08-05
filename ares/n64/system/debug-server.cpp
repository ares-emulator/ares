#include <n64/n64.hpp>

#define NALL_HEADER_ONLY
#include <nall/http/server.hpp>
#undef NALL_HEADER_ONLY

// @TODO: why does nall need this?
#include <cstdlib>
#include <utility>

namespace HTTP = ::nall::HTTP;
namespace N64 = ares::Nintendo64;

using string = ::nall::string;
using string_view = ::nall::string_view;

namespace {
  constexpr u32 MAX_REQUESTS_PER_UPDATE = 10;

  HTTP::Server server{};

  auto commandRead(string &res, const vector<string> &args) -> bool {
    if(args.size() < 4) {
      return false;
    }

    u32 fakeCycles = 0;
    u32 unitCount = static_cast<u32>(args[1].hex());
    u32 unitSize = static_cast<u32>(args[2].hex());
    u32 address = static_cast<u32>(args[3].hex());

    for(u32 i=0; i<unitCount; ++i) {
      switch(unitSize) {
        case N64::Byte: res.append(hex(static_cast< u8>(N64::bus.read<N64::Byte>(address, fakeCycles)))); break;
        case N64::Half: res.append(hex(static_cast<u16>(N64::bus.read<N64::Half>(address, fakeCycles)))); break;
        case N64::Word: res.append(hex(static_cast<u32>(N64::bus.read<N64::Word>(address, fakeCycles)))); break;
        case N64::Dual: res.append(hex(static_cast<u64>(N64::bus.read<N64::Dual>(address, fakeCycles)))); break;
      }
      res.append(" ");
      address += unitSize;
    }
    return true;
  }

  auto commandWrite(string &res, const vector<string> &args) -> bool {
    if(args.size() < 3) {
      return false;
    }

    u32 fakeCycles = 0;
    u32 unitCount = static_cast<u32>(args[1].hex());
    u32 unitSize = static_cast<u32>(args[2].hex());
    u32 address = static_cast<u32>(args[3].hex());

    if(args.size() < (4+unitCount)) {
      return false;
    }

    for(u32 i=0; i<unitCount; ++i) {
      u64 value = (args[4+i].hex());
      switch(unitSize) {
        case N64::Byte: N64::bus.write<N64::Byte>(address, value, fakeCycles); break;
        case N64::Half: N64::bus.write<N64::Half>(address, value, fakeCycles); break;
        case N64::Word: N64::bus.write<N64::Word>(address, value, fakeCycles); break;
        case N64::Dual: N64::bus.write<N64::Dual>(address, value, fakeCycles); break;
      }
      address += unitSize;
    }

    res.append("OK");
    return true;
  }

  auto processCommands(const string &payload) -> string 
  {
    string res{""};
    for(auto &command : payload.split("\n")) 
    {
      auto cmdParts = command.split(" ");
      auto cmdName = cmdParts[0];
      bool success = false;

      if(0);
      else if(cmdName ==  "read")success = commandRead(res, cmdParts);
      else if(cmdName == "write")success = commandWrite(res, cmdParts);

      res.append(success ? "\n" : "ERR\n");
    }
    return res;
  }
}

namespace ares::Nintendo64 {

auto DebugServer::start(u32 port) -> bool {
  printf("Starting Debug-server on port: %d\n", port);
  if(!server.open(port)) {
    printf("Failed to open server!\n");
    return false;
  }

  server.main([this](HTTP::Request& req)  {
    //printf("Data[%d]: %s\n\n", req._body.size(), req._body.data());
    HTTP::Response res{req};
    res.setResponseType(200); // OK
    res.setAllowCORS(true); // @TODO: add security checks (e.g.: random token)
    res._body = processCommands(req._body);
    return res;
  });

  isOpen = true;
  return true;
}

auto DebugServer::update() -> void {
  if(!isOpen)return;

  for(u32 i=0; i<MAX_REQUESTS_PER_UPDATE; ++i) {
    if(server.scan() == "idle")return;
  }
}

auto DebugServer::stop() -> bool {
  printf("Stopping Debug-server\n");
  if(isOpen)server.close();
  isOpen = false;
  return true;
}

} // end namespace