#include <nall/instruction-set.hpp>
#include <nall/array.hpp>
#include <nall/memory.hpp>
#include <nall/vector.hpp>

#if defined(ARCHITECTURE_X86) || defined(ARCHITECTURE_AMD64)
  #if defined(PLATFORM_WINDOWS)
    #include <intrin.h>
    namespace nall {
      inline auto cpuidex(int info[4], int leaf, int subleaf) -> void {
        __cpuidex(info, leaf, subleaf);
      }
    }
  #else
    #include <cpuid.h>
    namespace nall {
      inline auto cpuidex(int info[4], int leaf, int subleaf) -> void {
        __cpuid_count(leaf, subleaf, info[0], info[1], info[2], info[3]);
      }
    }
  #endif
#endif

namespace nall {

NALL_HEADER_INLINE instruction_set::information::information() {
#if defined(ARCHITECTURE_X86) || defined(ARCHITECTURE_AMD64)
  array<int[4]> cpui;

  //get the number of the highest valid function ID
  cpuidex(cpui.data(), 0, 0);
  int maxId = cpui[0];

  vector<array<int[4]>> data;
  for(int i = 0; i <= maxId; i++) {
    cpuidex(cpui.data(), i, 0);
    data.append(cpui);
  }

  //capture vendor string
  char vendor[0x20] = {0};
  memory::copy(vendor, &data[0][1], sizeof(data[0][1]));
  memory::copy(vendor + 4, &data[0][3], sizeof(data[0][3]));
  memory::copy(vendor + 8, &data[0][2], sizeof(data[0][2]));
  this->vendor = vendor;
  if(this->vendor == "GenuineIntel") {
    isIntel = true;
  } else if(this->vendor == "AuthenticAMD") {
    isAMD = true;
  }

  //load flags for function 0x00000001
  if(maxId >= 1) {
    f_1_ecx = data[1][2];
    f_1_edx = data[1][3];
  }

  //load flags for function 0x00000007
  if(maxId >= 7) {
    f_7_ebx = data[7][1];
    f_7_ecx = data[7][2];
  }

  //get the number of the highest valid extended ID
  cpuidex(cpui.data(), 0x80000000, 0);
  int maxExId = cpui[0];

  vector<array<int[4]>> extdata;
  for(int i = 0x80000000; i <= maxExId; i++) {
    cpuidex(cpui.data(), i, 0);
    extdata.append(cpui);
  }

  //load flags for function 0x80000001
  if(maxExId >= 0x80000001) {
    f_81_ecx = extdata[1][2];
    f_81_edx = extdata[1][3];
  }

  //interpret CPU brand string if reported
  char brand[0x40] = {0};
  if(maxExId >= 0x80000004) {
    memory::copy(brand, extdata[2].data(), sizeof(cpui));
    memory::copy(brand + 16, extdata[3].data(), sizeof(cpui));
    memory::copy(brand + 32, extdata[4].data(), sizeof(cpui));
    this->brand = brand;
  }
#endif
}

}
