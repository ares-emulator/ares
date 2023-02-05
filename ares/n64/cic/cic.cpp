#include <n64/n64.hpp>

namespace ares::Nintendo64 {

CIC cic;

auto CIC::power(bool reset) -> void {
  string model = cartridge.node ? cartridge.cic() : dd.cic();
  if(model == "CIC-NUS-6101") region = NTSC, seed = 0x3f, checksum = 0x45cc73ee317aull, version = 1;
  if(model == "CIC-NUS-6102") region = NTSC, seed = 0x3f, checksum = 0xa536c0f1d859ull;
  if(model == "CIC-NUS-7101") region = PAL,  seed = 0x3f, checksum = 0xa536c0f1d859ull;
  if(model == "CIC-NUS-7102") region = PAL,  seed = 0x3f, checksum = 0x44160ec5d9afull, version = 1;
  if(model == "CIC-NUS-6103") region = NTSC, seed = 0x78, checksum = 0x586fd4709867ull;
  if(model == "CIC-NUS-7103") region = PAL,  seed = 0x78, checksum = 0x586fd4709867ull;
  if(model == "CIC-NUS-6105") region = NTSC, seed = 0x91, checksum = 0x8618a45bc2d3ull;
  if(model == "CIC-NUS-7105") region = PAL,  seed = 0x91, checksum = 0x8618a45bc2d3ull;
  if(model == "CIC-NUS-6106") region = NTSC, seed = 0x85, checksum = 0x2bbad4e6eb74ull;
  if(model == "CIC-NUS-7106") region = PAL,  seed = 0x85, checksum = 0x2bbad4e6eb74ull;
  if(model == "CIC-NUS-8303") region = NTSC, seed = 0xdd, type = 1;
  if(model == "CIC-NUS-8401") region = NTSC, seed = 0xdd, type = 1;
  if(model == "CIC-NUS-DDUS") region = NTSC, seed = 0xde, type = 1;
  state = BootRegion;
  fifo.resize(16);
}

auto CIC::scramble(n4 *buf, int size) -> void {
  for(int i : range(size)) buf[i] += buf[i-1] + 1;
}

auto CIC::poll() -> void {
  if(state == BootRegion) {
    n4 val;
    val.bit(0) = 1;
    val.bit(1) = 0;
    val.bit(2) = region == PAL;
    val.bit(3) = type;
    fifo.write(val);
    state = BootSeed;
    return;
  }

  if(state == BootSeed) {
    n4 buf[6];
    buf[0] = 0xA;  //true random
    buf[1] = 0x5;  //true random
    buf[2] = seed.bit(4,7);
    buf[3] = seed.bit(0,3);
    buf[4] = seed.bit(4,7);
    buf[5] = seed.bit(0,3);
    for (auto i : range(2)) scramble(buf, 6);
    for (auto i : range(6)) fifo.write(buf[i]);
    state = BootChecksum;
    return;
  }

  if(state == BootChecksum) {
    n4 buf[16];
    buf[0] = 0x4;  //true random
    buf[1] = 0x7;  //true random
    buf[2] = 0xA;  //true random
    buf[3] = 0x1;  //true random
    for (auto i : range(12)) buf[i+4] = checksum.bit(44-i*4,47-i*4);
    for (auto i : range(4))  scramble(buf, 16);
    for (auto i : range(16)) fifo.write(buf[i]);
    state = Run;
    return;
  }
}

auto CIC::read() -> n4 {
  if(fifo.empty()) cic.poll();
  return fifo.read();
}

}