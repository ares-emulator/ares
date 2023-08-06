#include <nall/wstcp/wstcp-server.hpp>

namespace {

}

namespace nall::WsTCP {
  NALL_HEADER_INLINE auto Server::sendText(const string &text) -> void {
    sendData((const u8*)text.data(), text.size());
  }

  NALL_HEADER_INLINE auto Server::onData(const vector<u8> &data) -> void {
    string_view dataStr((const char*)data.data(), (u32)data.size());

    if(!hadHandshake) {
      hadHandshake = true;
      if(dataStr[0] == 'G' && dataStr[1] == 'E' && dataStr[2] == 'T') {
        isWS = true;
      }
    }
    onText(dataStr);
  }
}
