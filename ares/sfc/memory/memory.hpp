struct AbstractMemory {
  virtual ~AbstractMemory() { reset(); }
  explicit operator bool() const { return size() > 0; }

  virtual auto reset() -> void {}
  virtual auto allocate(u32, n8 = 0xff) -> void {}

  virtual auto load(shared_pointer<vfs::file> fp) -> void {}
  virtual auto save(shared_pointer<vfs::file> fp) -> void {}

  virtual auto data() -> n8* = 0;
  virtual auto size() const -> u32 = 0;

  virtual auto read(n24 address, n8 data = 0) -> n8 = 0;
  virtual auto write(n24 address, n8 data) -> void = 0;

  u32 id = 0;
};

#include "readable.hpp"
#include "writable.hpp"
#include "protectable.hpp"

struct Bus {
  //inline.hpp
  static auto mirror(u32 address, u32 size) -> u32;
  static auto reduce(u32 address, u32 mask) -> u32;

  //memory.cpp
  ~Bus();

  //inline.hpp
  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  //memory.cpp
  auto reset() -> void;
  auto map(
    const function<n8   (n24, n8)>& read,
    const function<void (n24, n8)>& write,
    const string& address, u32 size = 0, u32 base = 0, u32 mask = 0
  ) -> u32;
  auto unmap(const string& address) -> void;

private:
  n8*  lookup = nullptr;
  n32* target = nullptr;

  function<n8   (n24, n8)> reader[256];
  function<void (n24, n8)> writer[256];
  n24 counter[256];
};

extern Bus bus;
