//Toshiba  0x98
//Sharp    0xb0
//Samsung  0xec

// 4mbit   0xab
// 8mbit   0x2c
//16mbit   0x2f

struct Flash {
  natural ID;  //todo: can this be made const, even though it's declared as Cartridge::Flash[2] ?

  boolean modified;
  Memory::Writable<n8> rom;
  n8 vendorID;
  n8 deviceID;

  explicit operator bool() const { return (bool)rom; }

  //flash.cpp
  auto reset(natural ID) -> void;
  auto allocate(natural size) -> bool;
  auto load(shared_pointer<vfs::file> fp) -> void;
  auto save(shared_pointer<vfs::file> fp) -> void;

  auto power() -> void;
  auto read(n21 address) -> n8;
  auto write(n21 address, n8 data) -> void;

  auto status(u32) -> void;
  auto block(n21 address) -> maybe<n6>;
  auto program(n21 address, n8 data) -> void;
  auto protect(n21 blockID) -> void;
  auto erase(n21 blockID) -> void;
  auto eraseAll() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum : u32 { Read, Index, ReadID, Write, Acknowledge };
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
