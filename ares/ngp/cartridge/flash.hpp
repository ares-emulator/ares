//Toshiba  0x98
//Sharp    0xb0
//Samsung  0xec

// 4mbit   0xab
// 8mbit   0x2c
//16mbit   0x2f

struct Flash {
  natural ID;  //todo: can this be made const, even though it's declared as Cartridge::Flash[2] ?

  boolean modified;
  Memory::Writable<uint8> rom;
  uint8 vendorID;
  uint8 deviceID;

  explicit operator bool() const { return (bool)rom; }

  //flash.cpp
  auto reset(natural ID) -> void;
  auto allocate(natural size) -> bool;
  auto load(shared_pointer<vfs::file> fp) -> void;
  auto save(shared_pointer<vfs::file> fp) -> void;

  auto power() -> void;
  auto read(uint21 address) -> uint8;
  auto write(uint21 address, uint8 data) -> void;

  auto status(uint) -> void;
  auto block(uint21 address) -> maybe<uint6>;
  auto program(uint21 address, uint8 data) -> void;
  auto protect(uint21 blockID) -> void;
  auto erase(uint21 blockID) -> void;
  auto eraseAll() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum : uint { Read, Index, ReadID, Write };
  natural mode;
  natural index;

//unserialized
  struct Block {
    boolean writable;
    natural offset;
    natural length;
  };
  vector<Block> blocks;
};
