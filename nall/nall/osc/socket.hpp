#pragma once

// Minimal sender for OSC (Open Sound Control) messages over UDP.
// Only sending is supported; there is no need to receive or parse OSC.

#include <nall/stdint.hpp>
#include <nall/string.hpp>
#include <cstring>
#include <vector>

#if defined(PLATFORM_WINDOWS)
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

namespace nall::OSC {

struct Socket {
  auto open(const string& host, u16 port) -> bool;
  auto close() -> void;
  auto isOpen() const -> bool { return fd >= 0; }

  //sends an OSC message with no arguments (e.g. "/ares/alive")
  auto send(const string& address) -> bool;

  //sends an OSC message with a single numeric argument
  auto sendInt32(const string& address, s32 value) -> bool;
  auto sendInt64(const string& address, s64 value) -> bool;
  auto sendFloat32(const string& address, f32 value) -> bool;
  auto sendFloat64(const string& address, f64 value) -> bool;

  ~Socket() { close(); }

private:
  auto appendPaddedString(std::vector<u8>& buffer, const string& value) -> void;
  auto sendRaw(const string& address, const char* typeTag, const u8* data, u32 size) -> bool;

  s32 fd = -1;
};

inline auto Socket::appendPaddedString(std::vector<u8>& buffer, const string& value) -> void {
  auto data = value.data();
  auto length = value.size();
  for(u32 n = 0; n < length; n++) buffer.push_back((u8)data[n]);
  //OSC strings are null-terminated and zero-padded to a 4-byte boundary
  do {
    buffer.push_back(0);
  } while(buffer.size() % 4);
}

inline auto Socket::open(const string& host, u16 port) -> bool {
  close();

  #if defined(PLATFORM_WINDOWS)
  static bool winsockInitialized = false;
  if(!winsockInitialized) {
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
    winsockInitialized = true;
  }
  #endif

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  addrinfo* result = nullptr;
  if(getaddrinfo(host, string(port), &hints, &result) != 0 || !result) return false;

  fd = (s32)socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if(fd < 0) { freeaddrinfo(result); return false; }

  //connect() on a UDP socket just latches the default destination for send(), no handshake occurs
  if(connect(fd, result->ai_addr, (int)result->ai_addrlen) != 0) {
    freeaddrinfo(result);
    close();
    return false;
  }

  freeaddrinfo(result);
  return true;
}

inline auto Socket::close() -> void {
  if(fd < 0) return;
  #if defined(PLATFORM_WINDOWS)
    closesocket(fd);
  #else
    ::close(fd);
  #endif
  fd = -1;
}

inline auto Socket::sendRaw(const string& address, const char* typeTag, const u8* data, u32 size) -> bool {
  if(fd < 0) return false;

  std::vector<u8> packet;
  appendPaddedString(packet, address);
  appendPaddedString(packet, {",", typeTag});
  packet.insert(packet.end(), data, data + size);

  auto result = ::send(fd, (const char*)packet.data(), (int)packet.size(), 0);
  return result == (decltype(result))packet.size();
}

inline auto Socket::send(const string& address) -> bool {
  return sendRaw(address, "", nullptr, 0);
}

inline auto Socket::sendInt32(const string& address, s32 value) -> bool {
  u32 bits = (u32)value;
  u8 data[4] = {u8(bits >> 24), u8(bits >> 16), u8(bits >> 8), u8(bits >> 0)};
  return sendRaw(address, "i", data, 4);
}

inline auto Socket::sendInt64(const string& address, s64 value) -> bool {
  u64 bits = (u64)value;
  u8 data[8];
  for(u32 n = 0; n < 8; n++) data[n] = u8(bits >> (56 - n * 8));
  return sendRaw(address, "h", data, 8);
}

inline auto Socket::sendFloat32(const string& address, f32 value) -> bool {
  u32 bits;
  memcpy(&bits, &value, 4);
  u8 data[4] = {u8(bits >> 24), u8(bits >> 16), u8(bits >> 8), u8(bits >> 0)};
  return sendRaw(address, "f", data, 4);
}

inline auto Socket::sendFloat64(const string& address, f64 value) -> bool {
  u64 bits;
  memcpy(&bits, &value, 8);
  u8 data[8];
  for(u32 n = 0; n < 8; n++) data[n] = u8(bits >> (56 - n * 8));
  return sendRaw(address, "d", data, 8);
}

}
