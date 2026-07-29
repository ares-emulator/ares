NetworkControlInterface nci;

auto NetworkControlInterface::open() -> void {
  if(!settings.nci.enabled) return;
  server.open(settings.nci.port, settings.nci.useIPv4);
}

auto NetworkControlInterface::close() -> void {
  server.close();
}

auto NetworkControlInterface::updateLoop() -> void {
  if(!server.isOpen()) return;

  datagrams.clear();
  server.poll(datagrams);

  for(auto& dg : datagrams) {
    processCommand(dg.data, (sockaddr*)&dg.sender, dg.senderLen);
  }
}

auto NetworkControlInterface::reply(const string& text, sockaddr* dest, socklen_t destLen) -> void {
  server.sendTo(text, dest, destLen);
}

auto NetworkControlInterface::processCommand(string_view command, sockaddr* sender, socklen_t senderLen) -> void {
  // trim trailing whitespace/newlines
  string cmd = command;
  cmd.strip();

  // split into command name and arguments
  string name;
  string args;
  if(auto pos = cmd.find(" ")) {
    name = slice(cmd, 0, pos.get());
    args = slice(cmd, pos.get() + 1);
  } else {
    name = cmd;
  }

  name = name.upcase();

  if(name == "VERSION") return commandVersion(sender, senderLen);
  if(name == "GET_STATUS") return commandGetStatus(sender, senderLen);
  if(name == "SET_CONFIG_PARAM") return commandSetConfigParam(args, sender, senderLen);
  if(name == "GET_CONFIG_PARAM") return commandGetConfigParam(args, sender, senderLen);
  if(name == "SHOW_MSG") return commandShowMsg(args, sender, senderLen);
  if(name == "SET_SHADER") return commandSetShader(args, sender, senderLen);
  if(name == "READ_CORE_MEMORY") return commandReadCoreMemory(args, sender, senderLen);
  if(name == "WRITE_CORE_MEMORY") return commandWriteCoreMemory(args, sender, senderLen);
  if(name == "LOAD_STATE_SLOT") return commandLoadStateSlot(args, sender, senderLen);
  if(name == "PAUSE_TOGGLE") return commandPauseToggle(sender, senderLen);
  if(name == "FRAMEADVANCE") return commandFrameAdvance(sender, senderLen);
  if(name == "FAST_FORWARD") return commandFastForward(sender, senderLen);
  if(name == "REWIND") return commandRewind(sender, senderLen);
  if(name == "MUTE") return commandMute(sender, senderLen);
  if(name == "VOLUME_UP") return commandVolumeUp(sender, senderLen);
  if(name == "VOLUME_DOWN") return commandVolumeDown(sender, senderLen);
  if(name == "SAVE_STATE") return commandSaveState(sender, senderLen);
  if(name == "LOAD_STATE") return commandLoadState(sender, senderLen);
  if(name == "STATE_SLOT_PLUS") return commandStateSlotPlus(sender, senderLen);
  if(name == "STATE_SLOT_MINUS") return commandStateSlotMinus(sender, senderLen);
  if(name == "SCREENSHOT") return commandScreenshot(sender, senderLen);
  if(name == "FULLSCREEN_TOGGLE") return commandFullscreenToggle(sender, senderLen);
  if(name == "RESET") return commandReset(sender, senderLen);
  if(name == "QUIT") return commandQuit(sender, senderLen);
  if(name == "CLOSE_CONTENT") return commandCloseContent(sender, senderLen);
  if(name == "LOAD_CONTENT") return commandLoadContent(args, sender, senderLen);
  if(name == "DISK_EJECT_TOGGLE") return commandDiskEjectToggle(sender, senderLen);
  if(name == "DISK_LOAD") return commandDiskLoad(args, sender, senderLen);

  reply({"UNKNOWN_COMMAND ", name}, sender, senderLen);
}

auto NetworkControlInterface::commandVersion(sockaddr* sender, socklen_t senderLen) -> void {
  reply({ares::Version}, sender, senderLen);
}

auto NetworkControlInterface::commandGetStatus(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("GET_STATUS CONTENTLESS", sender, senderLen);
    return;
  }

  string status = program.paused ? "PAUSED" : "PLAYING";
  string gameName;
  if(emulator->game) {
    gameName = Location::base(emulator->game->location);
    gameName = Location::prefix(gameName);
  }

  reply({"GET_STATUS ", status, " ", emulator->name, ",", gameName}, sender, senderLen);
}

auto NetworkControlInterface::commandGetConfigParam(string_view param, sockaddr* sender, socklen_t senderLen) -> void {
  string path = param;
  path.strip();

  if(auto node = settings[path]) {
    reply({"GET_CONFIG_PARAM ", path, " ", node.value()}, sender, senderLen);
  } else {
    reply({"UNSUPPORTED ", path}, sender, senderLen);
  }
}

auto NetworkControlInterface::commandSetConfigParam(string_view args, sockaddr* sender, socklen_t senderLen) -> void {
  string argsStr = args;
  argsStr.strip();

  // split on first space: <path> <value>
  string path;
  string value;
  if(auto pos = argsStr.find(" ")) {
    path = slice(argsStr, 0, pos.get());
    value = slice(argsStr, pos.get() + 1);
  } else {
    reply({"UNSUPPORTED ", argsStr}, sender, senderLen);
    return;
  }

  if(!settings[path]) {
    reply({"UNSUPPORTED ", path}, sender, senderLen);
    return;
  }

  settings(path).setValue(value);
  settings.process(true);
  settings.save();

  reply({"SET_CONFIG_PARAM ", path, " ", value}, sender, senderLen);
}

auto NetworkControlInterface::commandShowMsg(string_view text, sockaddr* sender, socklen_t senderLen) -> void {
  program.showMessage(text);
  reply("SHOW_MSG OK", sender, senderLen);
}

auto NetworkControlInterface::commandSetShader(string_view path, sockaddr* sender, socklen_t senderLen) -> void {
  string shaderPath = path;
  shaderPath.strip();
  ruby::video.setShader(shaderPath);
  reply({"SET_SHADER ", shaderPath}, sender, senderLen);
}

struct NCIMemoryTarget {
  ares::Node::Debugger::Memory node;
  u32 offset = 0;
};

// node names are matched case-insensitively, ignoring spaces and underscores,
// so "Cartridge_ROM", "cartridgerom" and "Cartridge ROM" are equivalent
static auto nciNormalizeMemoryNodeName(string name) -> string {
  name.downcase();
  name.replace(" ", "");
  name.replace("_", "");
  return name;
}

template<typename T>
static auto nciFindMemoryNode(T& memories, const string& name) -> ares::Node::Debugger::Memory {
  auto normalized = nciNormalizeMemoryNodeName(name);
  for(auto& node : memories) {
    if(nciNormalizeMemoryNodeName(node->name()) == normalized) return node;
  }
  return {};
}

// Resolves a READ_CORE_MEMORY/WRITE_CORE_MEMORY address token to a debugger
// memory node and an offset within it. A "<node>:<hex offset>" token targets
// any node explicitly by name. A plain hex address is interpreted through the
// emulated system's memory map when one is known; systems without a map fall
// back to treating the address as a raw offset into the first node.
static auto nciResolveMemoryTarget(const string& token) -> maybe<NCIMemoryTarget> {
  auto memories = ares::Node::enumerate<ares::Node::Debugger::Memory>(emulator->root);
  if(memories.empty()) return nothing;

  if(auto pos = token.find(":")) {
    string name = slice(token, 0, pos.get());
    string offset = slice(token, pos.get() + 1);
    if(auto node = nciFindMemoryNode(memories, name)) {
      return NCIMemoryTarget{node, (u32)offset.hex()};
    }
    return nothing;
  }

  u32 address = token.hex();

  if(emulator->name.beginsWith("Nintendo 64")) {
    u32 physical = address;
    if(address >= 0x80000000u && address <= 0xBFFFFFFFu) physical = address & 0x1FFFFFFFu;  //KSEG0/KSEG1
    string name;
    u32 base = 0;
    if(physical <= 0x03EFFFFFu) { name = "RDRAM"; base = 0; }
    else if(physical >= 0x04000000u && physical <= 0x04000FFFu) { name = "RSP DMEM"; base = 0x04000000u; }
    else if(physical >= 0x04001000u && physical <= 0x04001FFFu) { name = "RSP IMEM"; base = 0x04001000u; }
    else if(physical >= 0x08000000u && physical <= 0x0FFFFFFFu) { name = "Cartridge SRAM"; base = 0x08000000u; }
    else if(physical >= 0x10000000u && physical <= 0x1FBFFFFFu) { name = "Cartridge ROM"; base = 0x10000000u; }
    else if(physical >= 0x1FC007C0u && physical <= 0x1FC007FFu) { name = "PIF RAM"; base = 0x1FC007C0u; }
    else return nothing;
    if(auto node = nciFindMemoryNode(memories, name)) {
      return NCIMemoryTarget{node, physical - base};
    }
    return nothing;
  }

  return NCIMemoryTarget{memories.front(), address};
}

auto NetworkControlInterface::commandReadCoreMemory(string_view args, sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("READ_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  string argsStr = args;
  auto parts = nall::split(argsStr, " ");
  if(parts.size() < 2) {
    reply("READ_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  u32 length = parts[1].natural();
  if(length == 0 || length > 256) {
    reply("READ_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  auto target = nciResolveMemoryTarget(parts[0]);
  if(!target) {
    reply("READ_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  auto memory = target->node;
  string result = {"READ_CORE_MEMORY ", parts[0]};
  for(u32 i = 0; i < length; i++) {
    u32 addr = target->offset + i;
    if(addr >= memory->size()) {
      result.append(" -1");
      break;
    }
    result.append(" ", hex(memory->read(addr), 2L));
  }
  reply(result, sender, senderLen);
}

auto NetworkControlInterface::commandWriteCoreMemory(string_view args, sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("WRITE_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  string argsStr = args;
  auto parts = nall::split(argsStr, " ");
  if(parts.size() < 2) {
    reply("WRITE_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  auto target = nciResolveMemoryTarget(parts[0]);
  if(!target) {
    reply("WRITE_CORE_MEMORY -1", sender, senderLen);
    return;
  }

  auto memory = target->node;
  for(u32 i = 1; i < parts.size(); i++) {
    u32 addr = target->offset + (i - 1);
    if(addr >= memory->size()) {
      reply("WRITE_CORE_MEMORY -1", sender, senderLen);
      return;
    }
    memory->write(addr, parts[i].hex());
  }

  reply({"WRITE_CORE_MEMORY ", parts[0], " ", parts.size() - 1}, sender, senderLen);
}

auto NetworkControlInterface::commandLoadStateSlot(string_view args, sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("LOAD_STATE_SLOT -1", sender, senderLen);
    return;
  }

  string slotStr = args;
  slotStr.strip();
  u32 slot = slotStr.natural();
  if(slot < 1 || slot > 9) {
    reply("LOAD_STATE_SLOT -1", sender, senderLen);
    return;
  }

  if(program.stateLoad(slot)) {
    reply({"LOAD_STATE_SLOT ", slot}, sender, senderLen);
  } else {
    reply("LOAD_STATE_SLOT -1", sender, senderLen);
  }
}

auto NetworkControlInterface::commandPauseToggle(sockaddr* sender, socklen_t senderLen) -> void {
  program.pause(!program.paused);
  reply({"PAUSE_TOGGLE ", program.paused ? "PAUSED" : "UNPAUSED"}, sender, senderLen);
}

auto NetworkControlInterface::commandFrameAdvance(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("FRAMEADVANCE -1", sender, senderLen);
    return;
  }
  if(!program.paused) program.pause(true);
  program.requestFrameAdvance = true;
  reply("FRAMEADVANCE OK", sender, senderLen);
}

auto NetworkControlInterface::commandFastForward(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("FAST_FORWARD -1", sender, senderLen);
    return;
  }

  program.fastForwarding = !program.fastForwarding;
  if(program.fastForwarding) {
    ffVideoBlocking = ruby::video.blocking();
    ffAudioBlocking = ruby::audio.blocking();
    ffAudioDynamic = ruby::audio.dynamic();
    ruby::video.setBlocking(false);
    ruby::audio.setBlocking(false);
    ruby::audio.setDynamic(false);
  } else {
    ruby::video.setBlocking(ffVideoBlocking);
    ruby::audio.setBlocking(ffAudioBlocking);
    ruby::audio.setDynamic(ffAudioDynamic);
  }
  reply({"FAST_FORWARD ", program.fastForwarding ? "ON" : "OFF"}, sender, senderLen);
}

auto NetworkControlInterface::commandRewind(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("REWIND -1", sender, senderLen);
    return;
  }
  if(program.rewind.frequency == 0) {
    reply("REWIND -1 rewind not enabled", sender, senderLen);
    return;
  }

  program.rewinding = !program.rewinding;
  if(program.rewinding) {
    program.rewindSetMode(Program::Rewind::Mode::Rewinding);
  } else {
    program.rewindSetMode(Program::Rewind::Mode::Playing);
  }
  reply({"REWIND ", program.rewinding ? "ON" : "OFF"}, sender, senderLen);
}

auto NetworkControlInterface::commandMute(sockaddr* sender, socklen_t senderLen) -> void {
  program.mute();
  reply({"MUTE ", settings.audio.mute ? "ON" : "OFF"}, sender, senderLen);
}

auto NetworkControlInterface::commandVolumeUp(sockaddr* sender, socklen_t senderLen) -> void {
  if(settings.audio.volume <= 1.9) settings.audio.volume += 0.1;
  reply({"VOLUME_UP ", settings.audio.volume}, sender, senderLen);
}

auto NetworkControlInterface::commandVolumeDown(sockaddr* sender, socklen_t senderLen) -> void {
  if(settings.audio.volume >= 0.1) settings.audio.volume -= 0.1;
  reply({"VOLUME_DOWN ", settings.audio.volume}, sender, senderLen);
}

auto NetworkControlInterface::commandSaveState(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("SAVE_STATE -1", sender, senderLen);
    return;
  }
  if(program.stateSave(program.state.slot)) {
    reply({"SAVE_STATE ", program.state.slot}, sender, senderLen);
  } else {
    reply("SAVE_STATE -1", sender, senderLen);
  }
}

auto NetworkControlInterface::commandLoadState(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("LOAD_STATE -1", sender, senderLen);
    return;
  }
  if(program.stateLoad(program.state.slot)) {
    reply({"LOAD_STATE ", program.state.slot}, sender, senderLen);
  } else {
    reply("LOAD_STATE -1", sender, senderLen);
  }
}

auto NetworkControlInterface::commandStateSlotPlus(sockaddr* sender, socklen_t senderLen) -> void {
  if(program.state.slot == 9) program.state.slot = 1;
  else program.state.slot++;
  program.showMessage({"Selected state slot ", program.state.slot});
  reply({"STATE_SLOT_PLUS ", program.state.slot}, sender, senderLen);
}

auto NetworkControlInterface::commandStateSlotMinus(sockaddr* sender, socklen_t senderLen) -> void {
  if(program.state.slot == 1) program.state.slot = 9;
  else program.state.slot--;
  program.showMessage({"Selected state slot ", program.state.slot});
  reply({"STATE_SLOT_MINUS ", program.state.slot}, sender, senderLen);
}

auto NetworkControlInterface::commandScreenshot(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("SCREENSHOT -1", sender, senderLen);
    return;
  }
  program.requestScreenshot = true;
  reply("SCREENSHOT OK", sender, senderLen);
}

auto NetworkControlInterface::commandFullscreenToggle(sockaddr* sender, socklen_t senderLen) -> void {
  program.requestFullscreenToggle = true;
  reply("FULLSCREEN_TOGGLE OK", sender, senderLen);
}

auto NetworkControlInterface::commandReset(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("RESET -1", sender, senderLen);
    return;
  }
  emulator->root->power(true);
  program.showMessage("System reset");
  reply("RESET OK", sender, senderLen);
}

auto NetworkControlInterface::commandQuit(sockaddr* sender, socklen_t senderLen) -> void {
  reply("QUIT OK", sender, senderLen);
  program.requestQuit = true;
}

auto NetworkControlInterface::commandCloseContent(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("CLOSE_CONTENT -1", sender, senderLen);
    return;
  }
  reply("CLOSE_CONTENT OK", sender, senderLen);
  program.requestUnload = true;
}

auto NetworkControlInterface::commandLoadContent(string_view args, sockaddr* sender, socklen_t senderLen) -> void {
  string argsStr = args;
  argsStr.strip();

  if(!argsStr) {
    reply("LOAD_CONTENT -1 no path specified", sender, senderLen);
    return;
  }

  if(program.requestLoad) {
    reply("LOAD_CONTENT -1 busy", sender, senderLen);
    return;
  }

  string path;
  string system;

  if(file::exists(argsStr)) {
    path = argsStr;
  } else if(auto pos = argsStr.find(" ")) {
    system = slice(argsStr, 0, pos.get());
    path = slice(argsStr, pos.get() + 1);
    path.strip();
    if(!file::exists(path)) {
      reply("LOAD_CONTENT -1 file not found", sender, senderLen);
      return;
    }
  } else {
    reply("LOAD_CONTENT -1 file not found", sender, senderLen);
    return;
  }

  program.requestLoadPath = path;
  program.requestLoadSystem = system;
  program.requestLoad = true;
  reply("LOAD_CONTENT OK", sender, senderLen);
}

auto NetworkControlInterface::commandDiskEjectToggle(sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("DISK_EJECT_TOGGLE -1", sender, senderLen);
    return;
  }

  for(auto port : ares::Node::enumerate<ares::Node::Port>(emulator->root)) {
    if(!port->hotSwappable()) continue;
    if(port->type() == "Controller" || port->type() == "Expansion") continue;

    if(port->connected()) {
      port->disconnect();
      reply("DISK_EJECT_TOGGLE EJECTED", sender, senderLen);
    } else {
      port->allocate();
      port->connect();
      reply("DISK_EJECT_TOGGLE CLOSED", sender, senderLen);
    }
    return;
  }

  reply("DISK_EJECT_TOGGLE -1 no disc tray found", sender, senderLen);
}

auto NetworkControlInterface::commandDiskLoad(string_view args, sockaddr* sender, socklen_t senderLen) -> void {
  if(!emulator) {
    reply("DISK_LOAD -1", sender, senderLen);
    return;
  }

  string path = args;
  path.strip();

  if(!file::exists(path)) {
    reply({"DISK_LOAD -1 file not found: ", path}, sender, senderLen);
    return;
  }

  for(auto port : ares::Node::enumerate<ares::Node::Port>(emulator->root)) {
    if(!port->hotSwappable()) continue;
    if(port->type() == "Controller" || port->type() == "Expansion") continue;

    // disconnect (eject)
    port->disconnect();

    // load new disc via the game pak
    if(emulator->game) {
      emulator->game->load(path);
    }

    // reconnect after brief delay to simulate tray close
    // (The emulator core needs time to detect the empty drive state)
    port->allocate();
    port->connect();

    reply({"DISK_LOAD OK ", path}, sender, senderLen);
    return;
  }

  reply("DISK_LOAD -1 no disc tray found", sender, senderLen);
}
