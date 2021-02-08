#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

Bus bus;

Bus::~Bus() {
  if(lookup) delete[] lookup;
  if(target) delete[] target;
}

auto Bus::reset() -> void {
  for(auto id : range(256)) {
    reader[id].reset();
    writer[id].reset();
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
  const function<n8   (n24, n8)>& read,
  const function<void (n24, n8)>& write,
  const string& addr, u32 size, u32 base, u32 mask
) -> u32 {
  u32 id = 1;
  while(counter[id]) {
    if(++id >= 256) return print("SFC error: bus map exhausted\n"), 0;
  }

  reader[id] = read;
  writer[id] = write;

  auto p = addr.split(":", 1L);
  auto banks = p(0).split(",");
  auto addrs = p(1).split(",");
  for(auto& bank : banks) {
    for(auto& addr : addrs) {
      auto bankRange = bank.split("-", 1L);
      auto addrRange = addr.split("-", 1L);
      u32 bankLo = bankRange(0).hex();
      u32 bankHi = bankRange(1, bankRange(0)).hex();
      u32 addrLo = addrRange(0).hex();
      u32 addrHi = addrRange(1, addrRange(0)).hex();

      for(u32 bank = bankLo; bank <= bankHi; bank++) {
        for(u32 addr = addrLo; addr <= addrHi; addr++) {
          u32 pid = lookup[bank << 16 | addr];
          if(pid && --counter[pid] == 0) {
            reader[pid].reset();
            writer[pid].reset();
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
  auto p = addr.split(":", 1L);
  auto banks = p(0).split(",");
  auto addrs = p(1).split(",");
  for(auto& bank : banks) {
    for(auto& addr : addrs) {
      auto bankRange = bank.split("-", 1L);
      auto addrRange = addr.split("-", 1L);
      u32 bankLo = bankRange(0).hex();
      u32 bankHi = bankRange(1, bankRange(0)).hex();
      u32 addrLo = addrRange(0).hex();
      u32 addrHi = addrRange(1, addrRange(1)).hex();

      for(u32 bank = bankLo; bank <= bankHi; bank++) {
        for(u32 addr = addrLo; addr <= addrHi; addr++) {
          u32 pid = lookup[bank << 16 | addr];
          if(pid && --counter[pid] == 0) {
            reader[pid].reset();
            writer[pid].reset();
          }

          lookup[bank << 16 | addr] = 0;
          target[bank << 16 | addr] = 0;
        }
      }
    }
  }
}

}
