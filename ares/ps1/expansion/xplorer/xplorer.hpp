struct Xplorer : ExpansionDevice {
  Xplorer(Node::Port parent, ExpansionPort::Device subtype);
  auto type() const -> u32 override { return subtype; }
  auto save() -> void override;
  auto power(bool reset) -> void override;
  auto step(u32 clocks) -> void override;
  auto serialize(serializer&) -> void override;
  auto readByte(u32 address) -> u32 override;
  auto writeByte(u32 address, u32 data) -> void override;

private:
  ExpansionPort::Device subtype;
  Node::Setting::Boolean switchEnabled;
  ParallelFlash flash;
  std::vector<u8> ram;
  u8 bank = 0;
  u64 microsecondClock = 0;
};
