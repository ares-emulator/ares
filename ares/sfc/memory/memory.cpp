#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

Bus bus;

Bus::~Bus() {
  if(lookup) delete[] lookup;
  if(target) delete[] target;
}

auto Bus::reset() -> void {
  for(auto id : range(256)) {
    reader[id] = nullptr;
    writer[id] = nullptr;
    counter[id] = 0;
  }

  if(lookup) delete[] lookup;
  if(target) delete[] target;

  lookup = new n8 [16_MiB]();
  target = new n32[16_MiB]();

  reader[0] = [](n24, n8 data) -> n8 { return data; };
  writer[0] = [](n24, n8) -> void {};

  cpu.map();
  ppu.map();
}

auto Bus::map(
  const std::function<n8   (n24, n8)>& read,
  const std::function<void (n24, n8)>& write,
  const string& addr, u32 size, u32 base, u32 mask
) -> u32 {
  u32 id = 1;
  while(counter[id]) {
    if(++id >= 256) return print("SFC error: bus map exhausted\n"), 0;
  }

  reader[id] = read;
  writer[id] = write;

  auto p = nall::split(addr, ":", 1L);
  p.resize(2);
  auto banks = nall::split(p[0], ",");
  auto addrs = nall::split(p[1], ",");
  for(auto& bank : banks) {
    for(auto& addr : addrs) {
      auto bankRange = nall::split(bank, "-", 1L);
      auto addrRange = nall::split(addr, "-", 1L);
      u32 bankLo = bankRange[0].hex();
      u32 bankHi = bankRange.size() > 1 ? bankRange[1].hex() : bankRange[0].hex();
      u32 addrLo = addrRange[0].hex();
      u32 addrHi = addrRange.size() > 1 ? addrRange[1].hex() : addrRange[0].hex();

      for(u32 bank = bankLo; bank <= bankHi; bank++) {
        for(u32 addr = addrLo; addr <= addrHi; addr++) {
          u32 pid = lookup[bank << 16 | addr];
          if(pid && --counter[pid] == 0) {
            reader[pid] = nullptr;
            writer[pid] = nullptr;
          }

          u32 offset = reduce(bank << 16 | addr, mask);
          if(size) base = mirror(base, size);
          if(size) offset = base + mirror(offset, size - base);
          lookup[bank << 16 | addr] = id;
          target[bank << 16 | addr] = offset;
          counter[id]++;
        }
      }
    }
  }

  return id;
}

auto Bus::unmap(const string& addr) -> void {
  auto p = nall::split(addr, ":", 1L);
  p.resize(2);
  auto banks = nall::split(p[0], ",");
  auto addrs = nall::split(p[1], ",");
  for(auto& bank : banks) {
    for(auto& addr : addrs) {
      auto bankRange = nall::split(bank, "-", 1L);
      auto addrRange = nall::split(addr, "-", 1L);
      u32 bankLo = bankRange[0].hex();
      u32 bankHi = bankRange.size() > 1 ? bankRange[1].hex() : bankRange[0].hex();
      u32 addrLo = addrRange[0].hex();
      u32 addrHi = addrRange.size() > 1 ? addrRange[1].hex() : addrRange[0].hex();

      for(u32 bank = bankLo; bank <= bankHi; bank++) {
        for(u32 addr = addrLo; addr <= addrHi; addr++) {
          u32 pid = lookup[bank << 16 | addr];
          if(pid && --counter[pid] == 0) {
            reader[pid] = nullptr;
            writer[pid] = nullptr;
          }

          lookup[bank << 16 | addr] = 0;
          target[bank << 16 | addr] = 0;
        }
      }
    }
  }
}

}
