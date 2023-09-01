#include <nall/tcptext/tcptext-server.hpp>

namespace {
  nall::string buffer{};
}

namespace nall::TCPText {
  NALL_HEADER_INLINE auto Server::sendText(const string &text) -> void {
    if(isHTTP) {
      string res{"HTTP/1.1 200 OK\r\n"
          "Access-Control-Allow-Origin: *\r\n"
          "Content-Type: text/plain\r\n"
          "Connection: Keep-Alive\r\n"
          "Content-Length: "};
      res.append(text.size(), "\r\n\r\n", text);
      sendData((const u8*)res.data(), res.size());
    } else {
      sendData((const u8*)text.data(), text.size());
    }
  }

  NALL_HEADER_INLINE auto Server::onData(const vector<u8> &data) -> void {
    string_view dataStr((const char*)data.data(), (u32)data.size());

    if(!hadHandshake) {
      hadHandshake = true;

      // This is a security check and a feature at the same time.
      // Any website can request localhost via JS, while it can't see the result, 
      // GDB will receive the data and commands could be injected (true for all GDB-servers).
      // Allow connections knowing the secret key, and block all others.
      isHTTP = dataStr[0] != '+'; // assume anything non-GDB is http

      // @TODO: check for token
      printf("Is HTTP: %d\n", isHTTP);

      onConnect();
    }

    onText(dataStr);
  }
}
