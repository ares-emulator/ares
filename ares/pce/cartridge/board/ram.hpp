struct RAM : Interface {
  auto load(Markup::Node) -> void override;
  auto save(Markup::Node) -> void override;
  auto read(uint8 bank, uint13 address) -> uint8 override;
  auto write(uint8 bank, uint13 address, uint8 data) -> void override;
  auto power() -> void override;
  auto serialize(serializer&) -> void;

  Memory::Readable<uint8> rom;
  Memory::Writable<uint8> ram;
};
