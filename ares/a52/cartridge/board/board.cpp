namespace Board {

struct ProgramROM {
  auto load(VFS::Pak pak, u32 size) -> bool {
    memory.reset();
    auto fp = pak->read("program.rom");
    if(!fp || fp->size() != size) return false;
    memory.allocate(size);
    memory.load(fp);
    return true;
  }

  auto reset() -> void {
    memory.reset();
  }

  auto read(u32 address) const -> n8 {
    return memory.read(address);
  }

  explicit operator bool() const {
    return (bool)memory;
  }

private:
  Memory::Readable<n8> memory;
};

#include "linear.cpp"
#include "dual-window.cpp"

}
