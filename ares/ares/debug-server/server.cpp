#include <ares/debug-server/server.hpp>

// @TODO: why does nall need this?
#include <cstdlib>
#include <utility>

//namespace N64 = ares::Nintendo64;

using string = ::nall::string;
using string_view = ::nall::string_view;

namespace {
  constexpr u32 MAX_REQUESTS_PER_UPDATE = 10;

  auto commandRead(u32 address, u32 unitCount, u32 unitSize = 1) -> string {
    /*ares::Nintendo64::Thread fakeThread{};
    string res{""};
    for(u32 i=0; i<unitCount; ++i) {
      switch(unitSize) {
        case N64::Byte: res.append(hex(static_cast< u8>(N64::bus.read<N64::Byte>(address, fakeThread)), unitSize*2, '0')); break;
        case N64::Half: res.append(hex(static_cast<u16>(N64::bus.read<N64::Half>(address, fakeThread)), unitSize*2, '0')); break;
        case N64::Word: res.append(hex(static_cast<u32>(N64::bus.read<N64::Word>(address, fakeThread)), unitSize*2, '0')); break;
        case N64::Dual: res.append(hex(static_cast<u64>(N64::bus.read<N64::Dual>(address, fakeThread)), unitSize*2, '0')); break;
      }
      res.append("");
      address += unitSize;
    }
    return res;
    */
   return "";
  }

  auto commandWrite(u32 address, u32 unitSize, u64 value) -> void {
    /*ares::Nintendo64::Thread fakeThread{};
    switch(unitSize) {
      case N64::Byte: N64::bus.write<N64::Byte>(address, value, fakeThread); break;
      case N64::Half: N64::bus.write<N64::Half>(address, value, fakeThread); break;
      case N64::Word: N64::bus.write<N64::Word>(address, value, fakeThread); break;
      case N64::Dual: N64::bus.write<N64::Dual>(address, value, fakeThread); break;
    }*/
  }

  bool insideCommand{false};
  string cmdBuffer{""};

  auto calcGdbChecksum(const string &payload) {
    u8 checksum = 0;
    for(char c : payload) {
      checksum += c;
    }
    return hex(checksum, 2, '0');
  }

  auto encodeGdb(const string &payload, bool success = true) {
    string res{"+$"};
    res.append(payload);
    res.append("#");
    res.append(calcGdbChecksum(payload));
    return res;
  }

  auto processCommand(const string& cmd) -> string 
  {
    auto cmdParts = cmd.split(":");
    auto cmdName = cmdParts[0];
    char cmdPrefix = cmdName.size() > 0 ? cmdName[0] : ' ';

    printf("Command: %s\n", cmdBuffer.data());

    switch(cmdPrefix)
    {
      case 'q':
        // handshake-commands
        if(cmdName == "qSupported")return "PacketSize=4000";
        if(cmdName == "qTStatus")return "";
        if(cmdName == "qAttached")return "0";
        if(cmdName == "qfThreadInfo")return "m0";
        if(cmdName == "qC")return "QC0";
        if(cmdName == "qOffsets")return "Text=0;Data=0;Bss=0;";
        printf("Command: %s\n", cmdBuffer.data());
        break;

      case 'v':
        if(cmdName == "vMustReplyEmpty") { // handshake-command
          return "";
        }
        printf("Command: %s\n", cmdBuffer.data());
        break;

      case '?': // why did we halt? (if we halted)
        return "T00";
      break;

      case 'g': // dump registers
        // @TODO
        return "000000000000000000000000000000000000000000000000";
      break;

      case 'p': // read specific register
        return "00\0";
      break;

      case 'm': // read memory
        {
          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 count = cmdName.slice(sepIdx+1, cmdName.size()-sepIdx).hex();
          return commandRead(address, count);
        }
      break;

      case 'M': // write memory (e.g.: M801ef90a,4:01000000)
        {
          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 unitSize = cmdName.slice(sepIdx+1, 1).hex();
          u64 value = cmdParts.size() > 1 ? cmdParts[1].hex() : 0;

          commandWrite(address, unitSize, value);
          return "";
        }

      break;

      case 'H': return "OK";
    }

    printf("Command: %s\n", cmdBuffer.data());
    return "";
  }

  auto processCommands() -> string 
  {
    auto commands = cmdBuffer.split(";");
    string res{};
    for(u32 i=0; i<commands.size(); ++i) {
      auto part = processCommand(commands[i]);

      if(part.size()) {
        res.append(processCommand(commands[i]));
        if(i != (commands.size()-1)) {
          res.append(";");
        }
      }
    }
    return encodeGdb(res);
  }
}

namespace ares::DebugServer {

  Server server{};

  auto Server::onText(string_view text) -> void {
    for(u32 i=0; i<text.size(); ++i) 
    {
      char c = text[i];
      switch(c) 
      {
        case '$':
          insideCommand = true;
          break;

        case '#': // end of message + 2-char checksum after that
          insideCommand = false;
          i+=2;
          sendText(processCommands());
          cmdBuffer = "";
          break;

        case '+': break; // "OK" response, i don't care

        default:
          if(insideCommand) {
            cmdBuffer.append(c);
          }
      }
    }  
  }

};