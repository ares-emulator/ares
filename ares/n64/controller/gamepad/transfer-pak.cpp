// Reference: Bizhawk (waterbox/ares64/ares/ares/n64/controller/gamepad/transfer-pak.hpp)
// MIT Licensed

auto Gamepad::TransferPak::load(Node::Object parent) -> void {
#if defined(CORE_GB)
  ares::GameBoy::cartridgeSlot.load(parent);
#endif
}

auto Gamepad::TransferPak::unload() -> void {
#if defined(CORE_GB)
  ares::GameBoy::cartridgeSlot.unload();
#endif
}

auto Gamepad::TransferPak::save() -> void {
#if defined(CORE_GB)
  ares::GameBoy::cartridge.save();
#endif
}

auto Gamepad::TransferPak::read(u16 address) -> u8 {
#if defined(CORE_GB)
  address &= 0x7fff;

  if(!pakEnable) return 0;
  if(address <= 0x1fff) return 0x84;
  if(address <= 0x2fff) return addressBank;
  if(address <= 0x3fff) {
    n8 status;
    status.bit(0)   = cartEnable;
    status.bit(1)   = 0;
    status.bit(2,3) = resetState;
    status.bit(4,5) = 0;
    status.bit(6)   = !(bool)ares::GameBoy::cartridge.board;
    status.bit(7)   = pakEnable;
    if(cartEnable && resetState == 3) resetState = 2;
    else if(!cartEnable && resetState == 2) resetState = 1;
    else if(!cartEnable && resetState == 1) resetState = 0;
    return status;
  }
  if(!cartEnable) return 0;
  n8 data = 0;
  return ares::GameBoy::cartridge.read(2, 0x4000 * addressBank + address - 0x4000, data);
#endif
  return 0;
}

auto Gamepad::TransferPak::write(u16 address, u8 data) -> void {
#if defined(CORE_GB)
  address &= 0x7fff;
  if(address <= 0x1fff) {
    auto enabled = pakEnable;
    if(data == 0x84) pakEnable = 1;
    else if(data == 0xfe) pakEnable = 0;
    if(!enabled && pakEnable) {
      addressBank = 3;
      cartEnable = 0;
      resetState = 0;
    }
    return;
  }

  if(!pakEnable) return;

  if(address <= 0x2fff) {
    addressBank = data > 3 ? 0 : data;
    return;
  }

  if(address <= 0x3fff) {
    auto enabled = cartEnable;
    cartEnable = data & 1;
    if(!enabled && cartEnable) {
      resetState = 3;
      ares::GameBoy::cartridge.power();
    }
    return;
  }

  if(!cartEnable) return;
  ares::GameBoy::cartridge.write(2, 0x4000 * addressBank + address - 0x4000, data);
#endif
};
