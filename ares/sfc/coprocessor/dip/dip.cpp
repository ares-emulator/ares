//DIP switch
//used for Nintendo Super System emulation

DIP dip;
#include "serialization.cpp"

auto DIP::power() -> void {
}

auto DIP::read(uint24, uint8) -> uint8 {
  return value;
}

auto DIP::write(uint24, uint8) -> void {
}
