#include <span>

namespace nall::vfs {

struct file : node {
  virtual auto readable() const -> bool { return true; }
  virtual auto writable() const -> bool { return false; }

  virtual auto data() const -> const u8* = 0;
  virtual auto data() -> u8* = 0;
  virtual auto size() const -> u64 = 0;
  virtual auto offset() const -> u64 = 0;
  virtual auto resize(u64 size) -> bool = 0;

  virtual auto seek(s64 offset, index = index::absolute) -> void = 0;
  virtual auto read() -> u8 = 0;
  virtual auto write(u8 data) -> void = 0;
  virtual auto flush() -> void {}

  auto end() const -> bool {
    return offset() >= size();
  }

  auto read(std::span<u8> span) -> void {
    for(auto& byte : span) byte = read();
  }

  template<typename T> auto read(T* ptr, size_t size) -> void {
    read({(u8*)ptr, size});
  }

  auto readl(u32 bytes) -> u64 {
    u64 data = 0;
    for(auto n : range(bytes)) data |= (u64)read() << n * 8;
    return data;
  }

  auto readm(u32 bytes) -> u64 {
    u64 data = 0;
    for(auto n : range(bytes)) data = data << 8 | read();
    return data;
  }

  auto reads() -> string {
    seek(0);
    string s;
    s.resize(size());
    read(s);
    return s;
  }

  auto write(std::span<const u8> view) -> void {
    for(auto byte : view) write(byte);
  }

  template<typename T> auto write(const T* ptr, size_t size) -> void {
    write({(const u8*)ptr, size});
  }

  auto writel(u64 data, u32 bytes) -> void {
    for(auto n : range(bytes)) write(data), data >>= 8;
  }

  auto writem(u64 data, u32 bytes) -> void {
    for(auto n : reverse(range(bytes))) write(data >> n * 8);
  }

  auto writes(const string& s) -> void {
    write(s);
  }
};

}
