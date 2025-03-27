namespace Board {

struct Interface {
  VFS::Pak pak;

  Interface(Cartridge& cartridge) : cartridge(cartridge) {}
  virtual ~Interface() = default;
  virtual auto load(Node::Object) -> void {}
  virtual auto unload(Node::Object) -> void {}
  virtual auto save() -> void {}
  virtual auto readBus(u32 address) -> u16 { return (address & 0xFFFF); };
  virtual auto writeBus(u32 address, u16 data) -> void {};
  virtual auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 { return 0; };
  virtual auto tickRTC() -> void {}
  virtual auto clock() -> void {}
  virtual auto title() const -> string;
  virtual auto cic() const -> string;
  virtual auto power(bool reset) -> void {}
  virtual auto serialize(serializer&) -> void {}

  template<typename T>
  auto load(T& rom, string name) -> bool;
  template<typename T>
  auto save(T& rom, string name) -> bool;

  //there isn't really a better place to put this
  auto joybusEeprom(Memory::Writable16& eeprom, n8 send, n8 recv, n8 input[], n8 output[]) -> n1;

  Cartridge& cartridge;
};

}
