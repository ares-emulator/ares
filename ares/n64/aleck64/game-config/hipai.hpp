struct hipai : Aleck64::GameConfig {
  u8 mahjongRow = 0;

  auto readExpansionPort() -> u32 {
    n32 value = 0xffff'ffff;
    value.bit(16, 23) = aleck64.mahjong.read(mahjongRow);
    return value;
  }

  auto writeExpansionPort(n32 data) -> void {
    mahjongRow = data.bit(8, 15);
  };
};
