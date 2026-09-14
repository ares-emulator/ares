#include <nall/nall.hpp>
using namespace nall;

#include <nall/main.hpp>
#include <nall/udp/udp-server.hpp>

static u32 passCount = 0;
static u32 failCount = 0;
static u32 skipCount = 0;

static auto check(bool condition, const char* name) -> void {
  if(condition) {
    print("  PASS: ", name, "\n");
    passCount++;
  } else {
    print("  FAIL: ", name, "\n");
    failCount++;
  }
}

// helper: create a client socket that sends to localhost on the given port (IPv4)
static auto createClientSocket(u32 port, sockaddr_in& dest) -> s32 {
  s32 fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(fd < 0) return -1;

  dest = {};
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  dest.sin_port = htons(port);

  return fd;
}

static auto testOpenClose() -> void {
  print("Test 1: Server open/close lifecycle\n");

  nall::UDP::Server server;
  check(!server.isOpen(), "server initially closed");

  bool opened = server.open(0, true);  // port 0 = OS assigns random port
  // port 0 may not be supported on all systems; use a high port instead
  if(!opened) {
    server.close();
    opened = server.open(49200, true);
  }
  check(opened, "server opens successfully");
  check(server.isOpen(), "server reports open");

  server.close();
  check(!server.isOpen(), "server reports closed after close");
}

static auto testSendReceive() -> void {
  print("Test 2: Send datagram and receive via poll\n");

  u32 port = 49201;
  nall::UDP::Server server;
  bool opened = server.open(port, true);
  check(opened, "server opens on port");

  sockaddr_in dest;
  s32 clientFd = createClientSocket(port, dest);
  check(clientFd >= 0, "client socket created");

  const char* msg = "HELLO_TEST";
  ::sendto(clientFd, msg, strlen(msg), 0, (sockaddr*)&dest, sizeof(dest));

  // brief pause to let the datagram arrive
  usleep(10000);

  std::vector<nall::UDP::Datagram> datagrams;
  server.poll(datagrams);
  check(datagrams.size() == 1, "received exactly one datagram");
  if(datagrams.size() == 1) {
    check(datagrams[0].data == "HELLO_TEST", "datagram content matches");
  } else {
    failCount++;  // content check failed
  }

  ::close(clientFd);
  server.close();
}

static auto testMultipleDatagrams() -> void {
  print("Test 3: Multiple datagrams in sequence\n");

  u32 port = 49202;
  nall::UDP::Server server;
  server.open(port, true);

  sockaddr_in dest;
  s32 clientFd = createClientSocket(port, dest);

  ::sendto(clientFd, "MSG1", 4, 0, (sockaddr*)&dest, sizeof(dest));
  ::sendto(clientFd, "MSG2", 4, 0, (sockaddr*)&dest, sizeof(dest));
  ::sendto(clientFd, "MSG3", 4, 0, (sockaddr*)&dest, sizeof(dest));

  usleep(10000);

  std::vector<nall::UDP::Datagram> datagrams;
  server.poll(datagrams);
  check(datagrams.size() == 3, "received three datagrams");
  if(datagrams.size() == 3) {
    check(datagrams[0].data == "MSG1", "first datagram correct");
    check(datagrams[1].data == "MSG2", "second datagram correct");
    check(datagrams[2].data == "MSG3", "third datagram correct");
  } else {
    failCount += 3;
  }

  ::close(clientFd);
  server.close();
}

static auto testSendToReply() -> void {
  print("Test 4: sendTo reply to sender\n");

  u32 serverPort = 49203;
  u32 clientPort = 49204;

  nall::UDP::Server server;
  server.open(serverPort, true);

  // bind client socket to a known port so we can receive the reply
  s32 clientFd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  sockaddr_in clientAddr{};
  clientAddr.sin_family = AF_INET;
  clientAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  clientAddr.sin_port = htons(clientPort);
  ::bind(clientFd, (sockaddr*)&clientAddr, sizeof(clientAddr));

  // set non-blocking on client
  auto flags = fcntl(clientFd, F_GETFL, 0);
  fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

  // send to server
  sockaddr_in dest{};
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  dest.sin_port = htons(serverPort);
  ::sendto(clientFd, "PING", 4, 0, (sockaddr*)&dest, sizeof(dest));

  usleep(10000);

  // server receives and replies
  std::vector<nall::UDP::Datagram> datagrams;
  server.poll(datagrams);
  check(datagrams.size() == 1, "server received datagram");

  if(datagrams.size() == 1) {
    server.sendTo("PONG", (sockaddr*)&datagrams[0].sender, datagrams[0].senderLen);

    usleep(10000);

    char buf[64];
    auto len = ::recvfrom(clientFd, buf, sizeof(buf) - 1, 0, nullptr, nullptr);
    check(len > 0, "client received reply");
    if(len > 0) {
      buf[len] = '\0';
      check(string(buf) == "PONG", "reply content matches");
    } else {
      failCount++;
    }
  } else {
    failCount += 2;
  }

  ::close(clientFd);
  server.close();
}

static auto testPollEmpty() -> void {
  print("Test 5: Poll returns nothing when no data\n");

  u32 port = 49205;
  nall::UDP::Server server;
  server.open(port, true);

  std::vector<nall::UDP::Datagram> datagrams;
  server.poll(datagrams);
  check(datagrams.empty(), "no datagrams when nothing sent");

  server.close();
}

static auto testReopenAfterClose() -> void {
  print("Test 6: Server reopen after close\n");

  u32 port = 49206;
  nall::UDP::Server server;

  server.open(port, true);
  check(server.isOpen(), "first open succeeds");
  server.close();
  check(!server.isOpen(), "close succeeds");

  bool reopened = server.open(port, true);
  check(reopened, "reopen succeeds on same port");
  check(server.isOpen(), "server reports open after reopen");

  // verify it still works
  sockaddr_in dest;
  s32 clientFd = createClientSocket(port, dest);
  ::sendto(clientFd, "REOPEN", 6, 0, (sockaddr*)&dest, sizeof(dest));

  usleep(10000);

  std::vector<nall::UDP::Datagram> datagrams;
  server.poll(datagrams);
  check(datagrams.size() == 1, "receives data after reopen");
  if(datagrams.size() == 1) {
    check(datagrams[0].data == "REOPEN", "data correct after reopen");
  } else {
    failCount++;
  }

  ::close(clientFd);
  server.close();
}

auto nall::main(Arguments arguments) -> void {
  print("NCI UDP Server Tests\n");
  print("====================\n\n");

  testOpenClose();
  testSendReceive();
  testMultipleDatagrams();
  testSendToReply();
  testPollEmpty();
  testReopenAfterClose();

  print("\npass ", passCount, " / fail ", failCount, " / skip ", skipCount, "\n");

  if(failCount > 0) exit(EXIT_FAILURE);
}
