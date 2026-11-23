#include <ares/ares.hpp>
#include "scsi.hpp"

namespace ares {

auto SCSIDevice::commandLength(n8 opcode) -> u32 {
  switch(opcode >> 5) {
  case 0: return 6;
  case 1: case 2: return 10;
  case 5: return 12;
  }
  return 6;
}

}
