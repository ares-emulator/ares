auto GTIA::read(n8 address) -> n8 {
  address &= 0x1f;
  if(address >= 0x10 && address <= 0x13) return console.readTrigger(address - 0x10, graphicsControl);
  return peek(address);
}

auto GTIA::peek(n8 address) const -> n8 {
  address &= 0x1f;
  if(address <= 0x0f) return collision.read(address);
  if(address <= 0x13) return console.triggerValue(address - 0x10);
  if(address == 0x14) return 0x0f; // NTSC 0x0f, PAL 0x01
  if(address == 0x1f) return console.pins();
  return 0x0f;
}

auto GTIA::writeControl(n8 address, n8 data) -> void {
  address &= 0x1f;
  if(address >= 0x08 && address <= 0x0b) playerMissile.writePlayerSize(address - 0x08, data);
  else if(address == 0x0c) playerMissile.writeMissileSize(data);
  else if(address == 0x1c) playerMissile.writeVerticalDelay(data);
  else if(address == 0x1d) {
    graphicsControl = data;
    if(!graphicsControl.bit(2)) console.releaseTriggers();
  }
  else if(address == 0x1e) collision.clear();
  else if(address == 0x1f) console.write(data);
}

auto GTIA::write(n8 address, n8 data) -> void {
  address &= 0x1f;
  data &= address >= 0x12 && address <= 0x1a ? 0xfe : 0xff;
  if(address <= 0x07) return playerMissile.writePosition(address, data);
  if(address >= 0x0d && address <= 0x11) return playerMissile.writeGraphics(address - 0x0d, data);
  if(address >= 0x12 && address <= 0x1a) return colors.write(address - 0x12, data);
  if(address == 0x1b) return priority.write(data);
  writeControl(address, data);
}
