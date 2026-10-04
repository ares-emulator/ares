namespace Vs {

#include "board.hpp"
#include "standard.cpp"
#include "vrc1.cpp"
#include "unrom.cpp"
#include "mmc1.cpp"
#include "namco-108.cpp"
#include "sunsoft-3.cpp"
#include "protection.hpp"

auto Interface::create(string id, u32 programSize, u32 characterSize) -> Interface* {
  if(id == "standard" && (programSize == 0x8000 || programSize == 0xa000)) {
    return new Standard(programSize, characterSize);
  }
  if(id == "vrc1")      return new VRC1(programSize, characterSize);
  if(id == "unrom")     return new UNROM(programSize, characterSize);
  if(id == "mmc1")      return new MMC1(programSize, characterSize);
  if(id == "namco-108") return new Namco108(programSize, characterSize);
  if(id == "sunsoft-3") return new Sunsoft3(programSize, characterSize);
  return new Interface(programSize, characterSize);
}

auto Protection::create(string id) -> Protection* {
  if(id == "rbi-baseball")  return new RBIBaseball;
  if(id == "tko-boxing")    return new TKOBoxing;
  if(id == "super-xevious") return new SuperXevious;
  return new Protection;
}

}
