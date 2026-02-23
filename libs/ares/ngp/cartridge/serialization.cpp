auto Flash::serialize(serializer& s) -> void {
//s(rom);
  s(modified);
  s(vendorID);
  s(deviceID);
  s(mode);
  s(index);
}

auto Cartridge::serialize(serializer& s) -> void {
  s(flash[0]);
  s(flash[1]);
}
