#pragma once

#include <nall/udp/udp-server.hpp>

struct NetworkControlInterface {
  auto open() -> void;
  auto close() -> void;
  auto updateLoop() -> void;

private:
  auto processCommand(string_view command, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto reply(const string& text, struct sockaddr* dest, socklen_t destLen) -> void;

  auto commandVersion(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandGetStatus(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandGetConfigParam(string_view param, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandSetConfigParam(string_view args, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandShowMsg(string_view text, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandSetShader(string_view path, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandReadCoreMemory(string_view args, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandWriteCoreMemory(string_view args, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandLoadStateSlot(string_view args, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandPauseToggle(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandFrameAdvance(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandFastForward(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandRewind(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandMute(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandVolumeUp(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandVolumeDown(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandSaveState(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandLoadState(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandStateSlotPlus(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandStateSlotMinus(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandScreenshot(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandFullscreenToggle(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandReset(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandQuit(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandCloseContent(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandLoadContent(string_view args, struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandDiskEjectToggle(struct sockaddr* sender, socklen_t senderLen) -> void;
  auto commandDiskLoad(string_view args, struct sockaddr* sender, socklen_t senderLen) -> void;

  nall::UDP::Server server;
  std::vector<nall::UDP::Datagram> datagrams;

  // fast forward state tracking
  bool ffVideoBlocking = false;
  bool ffAudioBlocking = false;
  bool ffAudioDynamic = false;
};

extern NetworkControlInterface nci;
