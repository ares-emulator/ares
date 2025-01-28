//TODO: Implement the VDP used by tetris on the E90 board
//Verified to be available on cartridges for E92 board by Zoinkitty's E90->E92 conversion patch for mtetrisc
//Hardware is mostly unknown, seems to use an (encrypted?) character rom (tet-01m.u5)

auto Aleck64::VDP::readWord(u32 address) -> u32 {
  n32 value = 0xffffffff;

  if(address == 0x00) {
    value = !io.enable; //TODO: Verify on Hardware; mtetrisc expects this behavior
  }

  debug(unusual, "[Aleck64::VDP::readWord] ", hex(address, 8L));
  return value;
}

auto Aleck64::VDP::writeWord(u32 address, n32 data) -> void {
  if(address == 0x1e) {
    io.enable = data.bit(0);
  }

  debug(unusual, "[Aleck64::VDP::writeWord] ", hex(address, 8L), " = ", hex(data, 8L));
}

auto Aleck64::VDP::render(Node::Video::Screen screen) -> void {
  if(!io.enable) return;
}
